#ifndef NTR_GPU_H
#define NTR_GPU_H
#include "ntr/ppu.h"
#include "ntr/video.h"
#include <vector>
namespace ntr {
// A null window enables offscreen verification. WARP is only requested by tests.
bool gpu_open(void *window, bool warp = false, VideoRenderer renderer = VideoRenderer::D3D11);
void gpu_close();
void gpu_set_anisotropic_filtering(uint8_t level);
uint8_t gpu_anisotropic_filtering();
bool gpu_render(const Framebuffer &base, const uint32_t *minimap,
                const VideoSettings &settings, bool geometry, const uint32_t *foreground = nullptr,
                const DualScreenLayout *screens = nullptr, const Framebuffer *sub = nullptr,
                bool engine_a_on_top = true);
// Screen-authored menus use their original full-frame projection.
void gpu_set_menu_scene(bool active);
bool gpu_present(unsigned width, unsigned height, bool preserve_aspect = false);
bool gpu_readback(std::vector<uint32_t> &pixels, unsigned &width, unsigned &height);
const char *gpu_adapter();
unsigned gpu_triangle_count();
struct GpuFrameStats {
    uint64_t hash_texels=0,upload_bytes=0,uploads=0,identity_uses=0,cache_hits=0,cpu_us=0;
    uint64_t cache_bytes=0,cache_budget=0,evictions=0,replacement_attempts=0,replacement_uploads=0,staging_peak_bytes=0;
};
GpuFrameStats gpu_frame_stats();
// Diagnostic A/B comparison; normal rendering uses immutable decoded identities.
void gpu_force_texture_scan(bool force);
// Diagnostic override; at least one 4096x4096 RGBA texture must fit.
void gpu_set_texture_budget_bytes(uint64_t bytes);
}
#endif
