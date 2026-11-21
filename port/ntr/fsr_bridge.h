#pragma once
#include <cstdint>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Resource;
struct ID3D11ShaderResourceView;
struct IDXGIFactory;
struct IDXGISwapChain3;

namespace ntr {

struct FsrFrame {
    float jitter_x, jitter_y;
    float near_plane, far_plane, vertical_fov;
    float elapsed_ms;
    uint32_t reset;
};
static_assert(sizeof(FsrFrame)==28);
struct FsrControl {
    FsrFrame frame;
    uint32_t jitter_phases;
    uint32_t limit, stop;
};
static_assert(sizeof(FsrControl)==40);
inline constexpr unsigned fsr_probe_frames=96;

bool fsr_device_open(ID3D11Device **device,ID3D11DeviceContext **immediate);
bool fsr_open_swap_chain(IDXGIFactory *factory,void *window,IDXGISwapChain3 **swap);
bool fsr_wrap_back_buffer(IDXGISwapChain3 *swap,unsigned index,ID3D11Resource **buffer);
void fsr_acquire_back_buffer(ID3D11Resource *buffer);
void fsr_release_back_buffer(ID3D11Resource *buffer);
bool fsr_drain_present_queue();
bool fsr_device_lost();
void fsr_report_device_status(const char *step);
void fsr_test_device_loss(uint64_t tick);
bool fsr_helper_available();
bool fsr_bridge_start(unsigned width,unsigned height,unsigned output_width,unsigned output_height,unsigned limit);
bool fsr_bridge_prime(ID3D11Resource *const (&sources)[4],const FsrFrame &frame);
bool fsr_bridge_ready();
bool fsr_bridge_alive();
unsigned fsr_bridge_jitter_phases();
ID3D11ShaderResourceView *fsr_bridge_dispatch(ID3D11Resource *const (&sources)[4],const FsrFrame &frame);
ID3D11ShaderResourceView *fsr_bridge_output();
void fsr_bridge_release_output();
void fsr_bridge_stop();
void fsr_device_close();

}
