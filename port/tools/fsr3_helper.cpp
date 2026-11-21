// Build this companion as x64 against the FidelityFX API 3.1.4 headers.
#include "../ntr/fsr_bridge.h"
#include "../ntr/fsr_transport.h"
#include <ffx_api/dx12/ffx_api_dx12.h>
#include <ffx_api/ffx_upscale.h>
#include <cmath>
#include <cstring>
#include <limits>

using namespace ntr;
using namespace ntr::fsr_transport;

template<class Object>
ComPtr<Object> open_shared(Gpu &gpu,const wchar_t *encoded) {
    Handle handle(reinterpret_cast<HANDLE>(uintptr_t(read_unsigned(encoded))));
    ComPtr<Object> object;
    check(gpu.device->OpenSharedHandle(handle.value,IID_PPV_ARGS(&object)),"Open shared FSR object");
    return object;
}

void sdk_check(ffxReturnCode_t result,const char *step) {
    if(result==FFX_API_RETURN_OK)return;
    std::fprintf(stderr,"%s error 0x%x\n",step,unsigned(result));
    throw std::runtime_error(step);
}

int wmain(int argc,wchar_t **argv) {
    try {
        require(sizeof(void *)==8 && argc==12 && wcscmp(argv[1],L"--live-fsr")==0,"Invalid FSR invocation");
        wchar_t path[32768];
        const DWORD length=GetEnvironmentVariableW(L"SM64DS_PC_FSR_HELPER_LOG",path,std::size(path));
        if(length>0 && length<std::size(path)) {
            FILE *log=nullptr;
            require(_wfreopen_s(&log,path,L"a",stderr)==0,"Open live FSR log");
            std::setvbuf(log,nullptr,_IONBF,0);
        }
        const LUID adapter{DWORD(read_unsigned(argv[2])),LONG(DWORD(read_unsigned(argv[3])))};
        Gpu gpu;
        gpu.open(&adapter);
        std::array<ComPtr<ID3D12Resource>,5> resources;
        for(unsigned i=0;i<resources.size();++i)resources[i]=open_shared<ID3D12Resource>(gpu,argv[4+i]);
        const auto fence=open_shared<ID3D12Fence>(gpu,argv[9]);
        Handle mapping(reinterpret_cast<HANDLE>(uintptr_t(read_unsigned(argv[10]))));
        auto *control=static_cast<FsrControl *>(MapViewOfFile(mapping.value,FILE_MAP_WRITE,0,0,sizeof(FsrControl)));
        require(control!=nullptr,"Read FSR frame control");
        Handle parent(reinterpret_cast<HANDLE>(uintptr_t(read_unsigned(argv[11]))));
        const auto runtime=LoadLibraryExW(L"amd_fidelityfx_dx12.dll",nullptr,
            LOAD_LIBRARY_SEARCH_APPLICATION_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
        require(runtime!=nullptr,"Load FidelityFX runtime");
        const auto create=reinterpret_cast<PfnFfxCreateContext>(GetProcAddress(runtime,"ffxCreateContext"));
        const auto destroy=reinterpret_cast<PfnFfxDestroyContext>(GetProcAddress(runtime,"ffxDestroyContext"));
        const auto dispatch=reinterpret_cast<PfnFfxDispatch>(GetProcAddress(runtime,"ffxDispatch"));
        const auto query=reinterpret_cast<PfnFfxQuery>(GetProcAddress(runtime,"ffxQuery"));
        require(create && destroy && dispatch && query,"Resolve FidelityFX API");
        uint64_t count=1;
        const char *provider=nullptr;
        ffxQueryDescGetVersions version{{FFX_API_QUERY_DESC_TYPE_GET_VERSIONS,nullptr},
            FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE,gpu.device.Get(),&count,nullptr,&provider};
        sdk_check(query(nullptr,&version.header),"Query FidelityFX version");
        require(count && provider && strcmp(provider,"3.1.4")==0,"Unexpected FidelityFX version");
        const auto description=resources[0]->GetDesc();
        const uint32_t width=uint32_t(description.Width),height=description.Height;
        const auto output=resources.back()->GetDesc();
        const uint32_t output_width=uint32_t(output.Width),output_height=output.Height;
        for(unsigned i=0;i<4;++i)
            require(resources[i]->GetDesc().Width==width && resources[i]->GetDesc().Height==height,"Shared extent mismatch");
        const struct { uint32_t mode; const char *name; } presets[]={
            {FFX_UPSCALE_QUALITY_MODE_NATIVEAA,"Native AA"},
            {FFX_UPSCALE_QUALITY_MODE_QUALITY,"Quality"},
            {FFX_UPSCALE_QUALITY_MODE_BALANCED,"Balanced"},
            {FFX_UPSCALE_QUALITY_MODE_PERFORMANCE,"Performance"},
        };
        // The two extents identify the preset without changing the shared frame protocol.
        const char *preset_name=nullptr;
        for(const auto &preset:presets) {
            uint32_t expected_width=0,expected_height=0;
            ffxQueryDescUpscaleGetRenderResolutionFromQualityMode resolution{
                {FFX_API_QUERY_DESC_TYPE_UPSCALE_GETRENDERRESOLUTIONFROMQUALITYMODE,nullptr},
                output_width,output_height,preset.mode,&expected_width,&expected_height};
            sdk_check(query(nullptr,&resolution.header),"Query FidelityFX render resolution");
            if(expected_width==width && expected_height==height) {
                preset_name=preset.name;
                break;
            }
        }
        require(preset_name!=nullptr,"FidelityFX render resolution mismatch");
        int32_t phases=0;
        ffxQueryDescUpscaleGetJitterPhaseCount phase{{FFX_API_QUERY_DESC_TYPE_UPSCALE_GETJITTERPHASECOUNT,nullptr},
            width,output_width,&phases};
        sdk_check(query(nullptr,&phase.header),"Query FidelityFX phases");
        require(phases>0,"Invalid jitter period");
        gpu.wait(fence.Get(),1,parent.value,INFINITE);
        MemoryBarrier();
        if(control->stop)return 0;
        const unsigned limit=control->limit;
        const bool infinite=std::isinf(control->frame.far_plane);
        const uint32_t flags=FFX_UPSCALE_ENABLE_AUTO_EXPOSURE |
            (infinite?FFX_UPSCALE_ENABLE_DEPTH_INFINITE:0u);
        ffxCreateBackendDX12Desc backend{{FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12,nullptr},gpu.device.Get()};
        ffxCreateContextDescUpscale setup{{FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE,&backend.header},
            flags,{width,height},{output_width,output_height},nullptr};
        ffxContext context{};
        sdk_check(create(&context,&setup.header,nullptr),"Create live FSR context");
        unsigned frames=0;
        for(UINT64 i=0;!limit || i<=limit;++i) {
            const UINT64 ready=2*i+1;
            gpu.wait(fence.Get(),ready,parent.value,INFINITE);
            fsr_trace_job("companion received inputs",ready);
            MemoryBarrier();
            if(control->stop)break;
            const FsrFrame values=control->frame;
            require(values.near_plane>0 && values.far_plane>values.near_plane &&
                    std::isinf(values.far_plane)==infinite &&
                    values.vertical_fov>0 && values.vertical_fov<3.14f,"Invalid camera planes or FOV");
            gpu.begin();
            for(unsigned input=0;input<4;++input)
                gpu.transition(resources[input].Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            gpu.transition(resources[4].Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            ffxDispatchDescUpscale data{};
            data.header.type=FFX_API_DISPATCH_DESC_TYPE_UPSCALE;
            data.commandList=gpu.commands.Get();
            data.color=ffxApiGetResourceDX12(resources[0].Get());
            data.depth=ffxApiGetResourceDX12(resources[1].Get());
            data.motionVectors=ffxApiGetResourceDX12(resources[2].Get());
            data.reactive=ffxApiGetResourceDX12(resources[3].Get());
            data.output=ffxApiGetResourceDX12(resources[4].Get(),FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
            data.jitterOffset={values.jitter_x,values.jitter_y};
            FfxApiFloatCoords2D expected{};
            if(i) {
                ffxQueryDescUpscaleGetJitterOffset jitter{{FFX_API_QUERY_DESC_TYPE_UPSCALE_GETJITTEROFFSET,nullptr},
                    int32_t((i-1)%uint64_t(phases)),phases,&expected.x,&expected.y};
                sdk_check(query(nullptr,&jitter.header),"Query FidelityFX jitter");
            }
            require(std::abs(expected.x-values.jitter_x)<0.00001f &&
                    std::abs(expected.y-values.jitter_y)<0.00001f,"Live jitter disagreement");
            data.motionVectorScale={1,1};
            data.renderSize={width,height};
            data.upscaleSize={output_width,output_height};
            data.frameTimeDelta=values.elapsed_ms;
            data.preExposure=data.viewSpaceToMetersFactor=1;
            data.cameraNear=values.near_plane;
            data.cameraFar=infinite?std::numeric_limits<float>::max():values.far_plane;
            data.cameraFovAngleVertical=values.vertical_fov;
            data.reset=values.reset!=0;
            sdk_check(dispatch(&context,&data.header),"Dispatch live FSR");
            for(unsigned input=0;input<4;++input)
                gpu.transition(resources[input].Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COMMON);
            gpu.transition(resources[4].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COMMON);
            gpu.submit();
            if(!i) {
                control->jitter_phases=uint32_t(phases);
                MemoryBarrier();
            }
            check(gpu.queue->Signal(fence.Get(),ready+1),"Signal live FSR result");
            fsr_trace_job("companion queued output signal",ready+1);
            gpu.wait(fence.Get(),ready+1);
            fsr_trace_job("companion completed output",ready+1);
            if(!i)std::fprintf(stderr,"[FSR companion] warm-up complete\n");
            else ++frames;
        }
        sdk_check(destroy(&context,nullptr),"Destroy live FSR context");
        UnmapViewOfFile(control);
        if(width!=output_width || height!=output_height)
            std::fprintf(stderr,"FSR %s %s %ux%u -> %ux%u, phases=%d: %u live frames\n",
                         provider,preset_name,width,height,output_width,output_height,phases,frames);
        else std::fprintf(stderr,"FSR %s %s %ux%u: %u live frames\n",provider,preset_name,width,height,frames);
        FreeLibrary(runtime);
        return 0;
    } catch(const std::exception &error) {
        std::fprintf(stderr,"[FSR companion] %s\n",error.what());
        return 1;
    }
}
