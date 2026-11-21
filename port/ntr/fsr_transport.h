#pragma once
// GPU resource and process transport shared by the x86 port and x64 FSR helper.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>
#include <d3d11on12.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <stdexcept>
#include <string>
#include <vector>

namespace ntr::fsr_transport {
using Microsoft::WRL::ComPtr;
struct Handle {
    HANDLE value = nullptr;
    explicit Handle(HANDLE owned = nullptr) : value(owned) {}
    Handle(const Handle &) = delete;
    Handle &operator=(const Handle &) = delete;
    ~Handle() { if (value) CloseHandle(value); }
};

inline void check(HRESULT result, const char *what) {
    if (FAILED(result)) {
        std::fprintf(stderr, "%s HRESULT 0x%08lx\n", what, static_cast<unsigned long>(result));
        throw std::runtime_error(what);
    }
}

inline void require(bool result, const char *what) {
    if (!result) throw std::runtime_error(what);
}

inline bool fsr_trace_enabled(const wchar_t *name) {
    wchar_t flag[2]{};
    return GetEnvironmentVariableW(L"SM64DS_PC_AUTOMATION_ONLY",flag,2)==1 && flag[0]==L'1' &&
           GetEnvironmentVariableW(name,flag,2)==1 && flag[0]==L'1';
}

inline bool fsr_device_trace() {
    static const bool enabled=fsr_trace_enabled(L"SM64DS_PC_FSR_DEVICE_TRACE");
    return enabled;
}

inline void fsr_trace_job(const char *stage, UINT64 fence_value) {
    static const bool enabled=fsr_trace_enabled(L"SM64DS_PC_FSR_QUEUE_TRACE");
    if(enabled)
        std::fprintf(stderr,"[FSR queue] %s fence=%llu ms=%llu\n",stage,
                     static_cast<unsigned long long>(fence_value),
                     static_cast<unsigned long long>(GetTickCount64()));
}

struct Gpu {
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> commands;
    Handle event{CreateEventW(nullptr, FALSE, FALSE, nullptr)};
    bool reported_removal=false;
    const char *role="producer";

    ~Gpu() {
        if(device && fsr_device_trace())device_status("GPU teardown");
    }

    void open(const LUID *required = nullptr) {
        require(event.value != nullptr, "Create event");
        role=required?"companion":"producer";
        if(fsr_device_trace()) {
            ComPtr<ID3D12DeviceRemovedExtendedDataSettings> settings;
            const auto status=D3D12GetDebugInterface(IID_PPV_ARGS(&settings));
            if(SUCCEEDED(status)) {
                settings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
                settings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            }
            std::fprintf(stderr,"[FSR DRED] %s enable HRESULT 0x%08lx\n",role,
                         static_cast<unsigned long>(status));
        }
        ComPtr<IDXGIFactory4> factory;
        check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)), "Create DXGI factory");
        ComPtr<IDXGIAdapter1> adapter;
        if (required) {
            check(factory->EnumAdapterByLuid(*required, IID_PPV_ARGS(&adapter)), "Match producer adapter");
            check(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)),
                  "Open helper device");
        } else {
            for (UINT index = 0; !device; ++index) {
                adapter.Reset();
                check(factory->EnumAdapters1(index, &adapter), "Find hardware adapter");
                DXGI_ADAPTER_DESC1 description{};
                check(adapter->GetDesc1(&description), "Read descriptor");
                if (!(description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
                    D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
            }
        }
        DXGI_ADAPTER_DESC1 description{};
        check(adapter->GetDesc1(&description), "Read selected adapter");
        std::fwprintf(stderr, L"%u-bit adapter: %ls\n", unsigned(sizeof(void *) * 8), description.Description);
        D3D12_COMMAND_QUEUE_DESC desc{};
        check(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)), "Create command queue");
        check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)),
              "Create allocator");
        check(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr,
                                       IID_PPV_ARGS(&commands)), "Create list");
        if(fsr_device_trace()) {
            queue->SetName(required?L"FSR companion queue":L"FSR producer queue");
            commands->SetName(required?L"FSR companion dispatch":L"FSR producer commands");
        }
        check(commands->Close(), "Close empty list");
    }

    HRESULT device_status(const char *step) {
        const auto reason=device->GetDeviceRemovedReason();
        if(SUCCEEDED(reason) || !fsr_device_trace() || reported_removal)return reason;
        reported_removal=true;
        std::fprintf(stderr,"[FSR DRED] %s device removed at %s HRESULT 0x%08lx\n",role,step,
                     static_cast<unsigned long>(reason));
        ComPtr<ID3D12DeviceRemovedExtendedData> dred;
        if(FAILED(device.As(&dred)))return reason;
        D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT breadcrumbs{};
        const auto crumb_status=dred->GetAutoBreadcrumbsOutput(&breadcrumbs);
        unsigned logged=0;
        if(SUCCEEDED(crumb_status))
            for(auto node=breadcrumbs.pHeadAutoBreadcrumbNode; node && logged<16; node=node->pNext) {
                if(!node->pLastBreadcrumbValue || !node->pCommandHistory)continue;
                const UINT finished=*node->pLastBreadcrumbValue;
                if(finished>=node->BreadcrumbCount)continue;
                const int last=finished?int(node->pCommandHistory[(finished-1)%65536u]):-1;
                const int next=int(node->pCommandHistory[finished%65536u]);
                std::fprintf(stderr,"[FSR DRED] %s %s / %s completed=%u total=%u last=%d next=%d\n",role,
                             node->pCommandQueueDebugNameA?node->pCommandQueueDebugNameA:"unnamed queue",
                             node->pCommandListDebugNameA?node->pCommandListDebugNameA:"unnamed list",
                             finished,node->BreadcrumbCount,last,next);
                ++logged;
            }
        D3D12_DRED_PAGE_FAULT_OUTPUT fault{};
        const auto fault_status=dred->GetPageFaultAllocationOutput(&fault);
        std::fprintf(stderr,"[FSR DRED] %s breadcrumbs HRESULT 0x%08lx incomplete=%u page fault HRESULT 0x%08lx address=0x%llx\n",
                     role,static_cast<unsigned long>(crumb_status),logged,static_cast<unsigned long>(fault_status),
                     static_cast<unsigned long long>(fault.PageFaultVA));
        return reason;
    }

    ComPtr<ID3D12Resource> resource(const D3D12_RESOURCE_DESC &desc, D3D12_HEAP_TYPE type,
                                    D3D12_HEAP_FLAGS flags, D3D12_RESOURCE_STATES state) {
        D3D12_HEAP_PROPERTIES props{};
        props.Type = type;
        props.CreationNodeMask = props.VisibleNodeMask = 1;
        ComPtr<ID3D12Resource> result;
        check(device->CreateCommittedResource(&props, flags, &desc, state, nullptr, IID_PPV_ARGS(&result)),
              "Create committed resource");
        return result;
    }

    void begin() {
        check(allocator->Reset(), "Reset allocator");
        check(commands->Reset(allocator.Get(), nullptr), "Reset list");
    }

    void submit() {
        check(commands->Close(), "Close list");
        ID3D12CommandList *lists[] = {commands.Get()};
        queue->ExecuteCommandLists(1, lists);
    }

    void wait(ID3D12Fence *fence, UINT64 value, HANDLE child = nullptr, DWORD timeout=10000) {
        const UINT64 initial=fence->GetCompletedValue();
        if(initial >= value) {
            check(device_status("completed fence"), "Check GPU device");
            require(initial!=UINT64_MAX, "Shared GPU fence invalidated by device removal");
            return;
        }
        check(fence->SetEventOnCompletion(value, event.value), "Register fence wait");
        HANDLE handles[] = {event.value, child};
        const ULONGLONG deadline = timeout == INFINITE ? 0 : GetTickCount64() + timeout;
        DWORD remaining = timeout;
        for (;;) {
            const DWORD result = WaitForMultipleObjects(child ? 2 : 1, handles, FALSE, remaining);
            const DWORD wait_error = result == WAIT_FAILED ? GetLastError() : ERROR_SUCCESS;
            check(device_status("fence wait"), "Check GPU device");
            const UINT64 completed = fence->GetCompletedValue();
            require(completed!=UINT64_MAX, "Shared GPU fence invalidated by device removal");
            if(completed >= value)return;
            if(result == WAIT_OBJECT_0 + 1)
                throw std::runtime_error("Peer process exited before the GPU fence completed");
            if(result == WAIT_FAILED)
                check(HRESULT_FROM_WIN32(wait_error), "Wait for GPU fence event");

            // A fence from an earlier timed-out wait can still signal this event.
            // Keep waiting for the requested fence without extending the deadline.
            const ULONGLONG now = GetTickCount64();
            if(result == WAIT_TIMEOUT || (timeout != INFINITE && now >= deadline)) {
                std::fprintf(stderr, "GPU fence timeout: expected=%llu completed=%llu\n",
                             static_cast<unsigned long long>(value), static_cast<unsigned long long>(completed));
                throw std::runtime_error("GPU fence did not complete before the deadline");
            }
            if(timeout != INFINITE)remaining=static_cast<DWORD>(deadline - now);
        }
    }

    void transition(ID3D12Resource *texture, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Transition = {texture, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, before, after};
        commands->ResourceBarrier(1, &barrier);
    }

    HANDLE share(ID3D12DeviceChild *object) {
        SECURITY_ATTRIBUTES attributes{sizeof(attributes), nullptr, TRUE};
        HANDLE handle{};
        check(device->CreateSharedHandle(object, &attributes, GENERIC_ALL, nullptr, &handle), "Share GPU object");
        return handle;
    }
};

inline UINT64 read_unsigned(const wchar_t *argument) {
    wchar_t *end = nullptr;
    const auto value = std::wcstoull(argument, &end, 10);
    require(end != argument && *end == L'\0', "Invalid numeric argument");
    return value;
}

inline HANDLE start_helper(const wchar_t *exe, LUID adapter, std::vector<HANDLE> objects,
                    const std::wstring &request = L"--helper") {
    SIZE_T bytes = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &bytes);
    require(bytes != 0, "Get attribute list size");
    std::vector<char> storage(bytes);
    auto *attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
    require(InitializeProcThreadAttributeList(attributes, 1, 0, &bytes) != FALSE, "Create attribute list");
    const bool listed = UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
        objects.data(), objects.size() * sizeof(HANDLE), nullptr, nullptr) != FALSE;
    if (!listed) {
        DeleteProcThreadAttributeList(attributes);
        throw std::runtime_error("Restrict inherited handles");
    }
    std::wstring command = L"\"" + std::wstring(exe) + L"\" " + request + L" " +
        std::to_wstring(adapter.LowPart) + L" " + std::to_wstring(DWORD(adapter.HighPart));
    for (HANDLE object : objects) command += L" " + std::to_wstring(reinterpret_cast<uintptr_t>(object));
    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.lpAttributeList = attributes;
    PROCESS_INFORMATION process{};
    const BOOL started = CreateProcessW(exe, command.data(), nullptr, nullptr, TRUE,
        EXTENDED_STARTUPINFO_PRESENT | CREATE_NO_WINDOW, nullptr, nullptr, &startup.StartupInfo, &process);
    DeleteProcThreadAttributeList(attributes);
    require(started != FALSE, "Start x64 helper");
    CloseHandle(process.hThread);
    return process.hProcess;
}


}
