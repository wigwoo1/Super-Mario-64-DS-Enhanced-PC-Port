#include "fsr_bridge.h"
#include "fsr_transport.h"
#include <memory>

namespace ntr {
namespace {
using namespace fsr_transport;

constexpr DXGI_FORMAT formats[]={DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R32_FLOAT,
                                 DXGI_FORMAT_R32G32_FLOAT,DXGI_FORMAT_R8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM};
struct Session {
    std::array<ComPtr<ID3D11Resource>,5> wrapped;
    ComPtr<ID3D11ShaderResourceView> output;
    ComPtr<ID3D12Fence> fence;
    Handle child,mapping;
    FsrControl *parameters=nullptr;
    unsigned frames=0;
    unsigned limit=0;
    bool primed=false;
    bool output_acquired=false;

    ~Session() {
        if(parameters)UnmapViewOfFile(parameters);
        if(child.value && WaitForSingleObject(child.value,0)==WAIT_TIMEOUT) {
            TerminateProcess(child.value,1);
        }
    }
};

std::unique_ptr<Gpu> gpu;
ComPtr<ID3D11On12Device> on12;
ComPtr<ID3D11Device> device11;
ComPtr<ID3D11DeviceContext> immediate11;
ComPtr<ID3D12Fence> present_idle;
uint64_t present_idle_value=0;
std::unique_ptr<Session> session;

std::wstring helper_path() {
    wchar_t buffer[32768];
    const auto *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(automation && *automation=='1') {
        const DWORD size=GetEnvironmentVariableW(L"SM64DS_PC_FSR_HELPER",buffer,std::size(buffer));
        if(size>0 && size<std::size(buffer))return {buffer,size};
    }
    const DWORD size=GetModuleFileNameW(nullptr,buffer,std::size(buffer));
    if(!size || size>=std::size(buffer))return {};
    std::wstring path(buffer,size);
    const auto separator=path.find_last_of(L"\\/");
    return path.substr(0,separator+1)+L"sm64ds_fsr3_helper.exe";
}

// The helper owns all waits for the warm-up frame. The game queues the
// GPU copies and continues presenting with its normal renderer.
void queue_inputs(Session &state,ID3D11Resource *const (&sources)[4],const FsrFrame &frame,UINT64 ready) {
    ID3D11Resource *inputs[]={state.wrapped[0].Get(),state.wrapped[1].Get(),
                              state.wrapped[2].Get(),state.wrapped[3].Get()};
    state.parameters->frame=frame;
    MemoryBarrier();
    on12->AcquireWrappedResources(inputs,4);
    for(unsigned i=0;i<4;++i)immediate11->CopyResource(inputs[i],sources[i]);
    on12->ReleaseWrappedResources(inputs,4);
    immediate11->Flush();
    check(gpu->queue->Signal(state.fence.Get(),ready),"Signal FSR inputs");
    fsr_trace_job("producer queued input signal",ready);
}
}

bool fsr_device_open(ID3D11Device **device,ID3D11DeviceContext **immediate) {
    try {
        gpu=std::make_unique<Gpu>();
        gpu->open();
        IUnknown *queues[]={gpu->queue.Get()};
        check(D3D11On12CreateDevice(gpu->device.Get(),D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,
                                    queues,1,0,&device11,&immediate11,nullptr),"Open FSR D3D11On12 renderer");
        check(device11.As(&on12),"Query FSR renderer interop");
        check(device11.CopyTo(device),"Return FSR device");
        check(immediate11.CopyTo(immediate),"Return FSR context");
        return true;
    } catch(const std::exception &error) {
        std::fprintf(stderr,"[FSR bridge] renderer unavailable: %s\n",error.what());
        fsr_device_close();
        return false;
    }
}

bool fsr_open_swap_chain(IDXGIFactory *factory,void *window,IDXGISwapChain3 **swap) {
    try {
        ComPtr<IDXGIFactory2> factory2;
        check(factory->QueryInterface(IID_PPV_ARGS(&factory2)),"Query FSR swap-chain factory");
        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count=1;
        desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount=2;
        desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.AlphaMode=DXGI_ALPHA_MODE_IGNORE;
        ComPtr<IDXGISwapChain1> chain;
        check(factory2->CreateSwapChainForHwnd(gpu->queue.Get(),static_cast<HWND>(window),
                                                &desc,nullptr,nullptr,&chain),"Create FSR flip swap chain");
        check(chain->QueryInterface(IID_PPV_ARGS(swap)),"Query FSR flip swap chain");
        return true;
    } catch(const std::exception &error) {
        std::fprintf(stderr,"[FSR bridge] swap chain unavailable: %s\n",error.what());
        return false;
    }
}

bool fsr_wrap_back_buffer(IDXGISwapChain3 *swap,unsigned index,ID3D11Resource **buffer) {
    try {
        ComPtr<ID3D12Resource> surface;
        check(swap->GetBuffer(index,IID_PPV_ARGS(&surface)),"Read D3D12 back buffer");
        D3D11_RESOURCE_FLAGS flags{};
        flags.BindFlags=D3D11_BIND_RENDER_TARGET;
        // The 11On12 renderer owns the draws; the queue hands the buffer over
        // in PRESENT state and expects it back in that state.
        check(on12->CreateWrappedResource(surface.Get(),&flags,D3D12_RESOURCE_STATE_PRESENT,
                                          D3D12_RESOURCE_STATE_PRESENT,IID_PPV_ARGS(buffer)),"Wrap D3D12 back buffer");
        return true;
    } catch(const std::exception &error) {
        std::fprintf(stderr,"[FSR bridge] back buffer unavailable: %s\n",error.what());
        return false;
    }
}

void fsr_acquire_back_buffer(ID3D11Resource *buffer) {on12->AcquireWrappedResources(&buffer,1);}
void fsr_release_back_buffer(ID3D11Resource *buffer) {
    on12->ReleaseWrappedResources(&buffer,1);
    immediate11->Flush();
}

bool fsr_drain_present_queue() {
    if(fsr_device_lost())return false;
    try {
        if(!present_idle)
            check(gpu->device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&present_idle)),"Create present idle fence");
        immediate11->Flush();
        check(gpu->queue->Signal(present_idle.Get(),++present_idle_value),"Signal present queue");
        gpu->wait(present_idle.Get(),present_idle_value,nullptr,1500);
        return true;
    } catch(const std::exception &error) {
        fsr_report_device_status("drain presentation queue");
        std::fprintf(stderr,"[FSR bridge] present queue unavailable: %s\n",error.what());
        return false;
    }
}

bool fsr_device_lost() {
    return gpu && FAILED(gpu->device_status("presentation device status"));
}

void fsr_report_device_status(const char *step) {
    if(gpu && fsr_device_trace())gpu->device_status(step);
}

void fsr_test_device_loss(uint64_t tick) {
    static const uint64_t target=[] {
        const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
        const char *value=std::getenv("SM64DS_PC_TEST_FSR_DEVICE_LOSS_TICK");
        return automation && std::strcmp(automation,"1")==0 && value?std::strtoull(value,nullptr,10):0ull;
    }();
    static bool attempted=false;
    if(!target || tick!=target || attempted)return;
    attempted=true;
    ComPtr<ID3D12Device5> removable;
    if(!gpu || FAILED(gpu->device.As(&removable))) {
        std::fprintf(stderr,"[FSR bridge] device removal test unavailable\n");
        return;
    }
    removable->RemoveDevice();
    std::fprintf(stderr,"[FSR bridge] test removed D3D12 device tick=%llu\n",
                 static_cast<unsigned long long>(tick));
}

bool fsr_helper_available() {
    const auto path=helper_path();
    if(path.empty())return false;
    const DWORD attributes=GetFileAttributesW(path.c_str());
    return attributes!=INVALID_FILE_ATTRIBUTES && !(attributes&FILE_ATTRIBUTE_DIRECTORY);
}

bool fsr_bridge_start(unsigned width,unsigned height,unsigned output_width,unsigned output_height,unsigned limit) {
    if(!gpu || session)return false;
    try {
        const auto helper=helper_path();
        require(!helper.empty(),"Missing FSR helper path");
        auto pending=std::make_unique<Session>();
        std::array<ComPtr<ID3D12Resource>,std::size(formats)> shared;
        constexpr const wchar_t *names[]={L"FSR scene color",L"FSR scene depth",L"FSR scene motion",
                                          L"FSR reactive mask",L"FSR resolved output"};
        std::array<Handle,std::size(formats)+1> handles;
        std::vector<HANDLE> inherited;
        for(unsigned i=0;i<std::size(formats);++i) {
            D3D12_RESOURCE_DESC desc{};
            desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            desc.Width=i==4?output_width:width;
            desc.Height=i==4?output_height:height;
            desc.DepthOrArraySize=desc.MipLevels=desc.SampleDesc.Count=1;
            desc.Format=formats[i];
            desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
            if(i==4)desc.Flags|=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            shared[i]=gpu->resource(desc,D3D12_HEAP_TYPE_DEFAULT,D3D12_HEAP_FLAG_SHARED,D3D12_RESOURCE_STATE_COMMON);
            if(fsr_device_trace())shared[i]->SetName(names[i]);
            const D3D11_RESOURCE_FLAGS flags{D3D11_BIND_SHADER_RESOURCE};
            check(on12->CreateWrappedResource(shared[i].Get(),&flags,D3D12_RESOURCE_STATE_COMMON,
                  D3D12_RESOURCE_STATE_COMMON,IID_PPV_ARGS(&pending->wrapped[i])),"Wrap FSR resource");
            handles[i].value=gpu->share(shared[i].Get());
            inherited.push_back(handles[i].value);
        }
        check(device11->CreateShaderResourceView(pending->wrapped[4].Get(),nullptr,&pending->output),"FSR output view");
        ID3D11Resource *initial[]={pending->wrapped[0].Get(),pending->wrapped[1].Get(),pending->wrapped[2].Get(),
                                  pending->wrapped[3].Get(),pending->wrapped[4].Get()};
        on12->ReleaseWrappedResources(initial,5);
        immediate11->Flush();
        check(gpu->device->CreateFence(0,D3D12_FENCE_FLAG_SHARED,IID_PPV_ARGS(&pending->fence)),"FSR fence");
        handles.back().value=gpu->share(pending->fence.Get());
        inherited.push_back(handles.back().value);
        SECURITY_ATTRIBUTES attributes{sizeof(attributes),nullptr,TRUE};
        pending->mapping.value=CreateFileMappingW(INVALID_HANDLE_VALUE,&attributes,PAGE_READWRITE,0,sizeof(FsrControl),nullptr);
        require(pending->mapping.value!=nullptr,"FSR frame parameters mapping");
        pending->parameters=static_cast<FsrControl *>(MapViewOfFile(pending->mapping.value,FILE_MAP_WRITE,0,0,sizeof(FsrControl)));
        require(pending->parameters!=nullptr,"Map FSR frame parameters");
        *pending->parameters={};
        pending->parameters->limit=limit;
        pending->limit=limit;
        inherited.push_back(pending->mapping.value);
        Handle parent;
        require(DuplicateHandle(GetCurrentProcess(),GetCurrentProcess(),GetCurrentProcess(),&parent.value,
                                SYNCHRONIZE,TRUE,0)!=FALSE,"Share FSR parent lifetime");
        inherited.push_back(parent.value);
        pending->child.value=start_helper(helper.c_str(),gpu->device->GetAdapterLuid(),inherited,L"--live-fsr");
        session=std::move(pending);
        std::fprintf(stderr,"[FSR bridge] started scene=%ux%u output=%ux%u\n",width,height,output_width,output_height);
        return true;
    } catch(const std::exception &error) {
        std::fprintf(stderr,"[FSR bridge] start failed: %s\n",error.what());
        return false;
    }
}

bool fsr_bridge_prime(ID3D11Resource *const (&sources)[4],const FsrFrame &frame) {
    if(!session)return false;
    try {
        queue_inputs(*session,sources,frame,1);
        session->primed=true;
        return true;
    } catch(const std::exception &error) {
        std::fprintf(stderr,"[FSR bridge] warm-up submit failed: %s\n",error.what());
        fsr_bridge_stop();
        return false;
    }
}

bool fsr_bridge_ready() {
    return fsr_bridge_alive() &&
           session->fence->GetCompletedValue()>=2 &&
           SUCCEEDED(gpu->device->GetDeviceRemovedReason());
}

bool fsr_bridge_alive() {
    return session && WaitForSingleObject(session->child.value,0)==WAIT_TIMEOUT;
}

unsigned fsr_bridge_jitter_phases() {
    MemoryBarrier();
    return session?session->parameters->jitter_phases:0;
}

ID3D11ShaderResourceView *fsr_bridge_dispatch(ID3D11Resource *const (&sources)[4],const FsrFrame &frame) {
    if(!session)return nullptr;
    try {
        auto &state=*session;
        const UINT64 ready=2*state.frames+3;
        queue_inputs(state,sources,frame,ready);
        gpu->wait(state.fence.Get(),ready+1,state.child.value,1500);
        ++state.frames;
        return fsr_bridge_output();
    } catch(const std::exception &error) {
        fsr_report_device_status("FSR dispatch");
        std::fprintf(stderr,"[FSR bridge] dispatch failed: %s\n",error.what());
        fsr_bridge_stop();
        return nullptr;
    }
}

ID3D11ShaderResourceView *fsr_bridge_output() {
    if(!session || !session->frames || session->output_acquired)return nullptr;
    ID3D11Resource *output=session->wrapped[4].Get();
    on12->AcquireWrappedResources(&output,1);
    session->output_acquired=true;
    return session->output.Get();
}

void fsr_bridge_release_output() {
    if(!session || !session->output_acquired)return;
    ID3D11Resource *output=session->wrapped[4].Get();
    on12->ReleaseWrappedResources(&output,1);
    immediate11->Flush();
    session->output_acquired=false;
}

void fsr_bridge_stop() {
    if(!session)return;
    fsr_bridge_release_output();
    if(session->limit && session->frames==session->limit) {
        DWORD code=1;
        const bool verified=WaitForSingleObject(session->child.value,1000)==WAIT_OBJECT_0 &&
                            GetExitCodeProcess(session->child.value,&code) && code==0;
        std::fprintf(stderr,"[FSR bridge] frames=%u helper_ok=%u\n",session->frames,unsigned(verified));
    } else {
        session->parameters->stop=1;
        MemoryBarrier();
        if(fsr_bridge_alive() && SUCCEEDED(gpu->device->GetDeviceRemovedReason())) {
            const UINT64 ready=session->primed?2*session->frames+3:1;
            if(SUCCEEDED(gpu->queue->Signal(session->fence.Get(),ready)))
                WaitForSingleObject(session->child.value,100);
        }
        if(fsr_device_trace() && fsr_bridge_alive())
            WaitForSingleObject(session->child.value,2500);
        std::fprintf(stderr,"[FSR bridge] stopped after %u frames\n",session->frames);
    }
    session.reset();
}

void fsr_device_close() {
    fsr_bridge_stop();
    present_idle.Reset();present_idle_value=0;
    on12.Reset();immediate11.Reset();device11.Reset();gpu.reset();
}
}
