// DS 2D engine scan-out.
//
// DISPCNT/BGxCNT select tilemaps and tiles in mapped VRAM. Most state is
// stable for a frame, but window bounds can change in HBlank. The runtime
// snapshots those registers per visible line before rendering the frame.
//
// See notes/assessment.md section 2a and docs/mmio-inventory.md.

#ifndef NTR_PPU_H
#define NTR_PPU_H

#include <stdint.h>

namespace ntr {

constexpr int SCREEN_W = 256;
constexpr int SCREEN_H = 192;

enum Engine { ENGINE_A = 0, ENGINE_B = 1 };

struct Framebuffer {
    uint32_t px[SCREEN_H][SCREEN_W];   // 0xAARRGGBB
};

// Rasterise text/affine backgrounds and opaque constant 3D rear-planes,
// with rectangular and OBJ window masks.
// Bitmap backgrounds, mosaic and color effects are not implemented.
void ppu_scanout(Engine eng, Framebuffer &fb, bool transparent_backdrop = false,
                 uint8_t background_mask = 0x0f);

// Rasterise one engine's OBJ layer (sprites) from its OAM + OBJ VRAM over
// whatever fb already holds. Plain and affine sprites, 16- and 256-color
// tiles, 1D and 2D mapping, window masks and 2D priorities. Bitmap OBJs, mosaic
// and blending are not implemented. Transparent color-0 texels leave fb untouched.
void ppu_scanout_obj(Engine eng, Framebuffer &fb);

// Runtime captures window registers before each visible line's HBlank.
// Repeated host presentations reuse these snapshots without rerunning IRQs.
void ppu_begin_window_frame();
void ppu_capture_window_line(int y);

// Enabled 2D backgrounds strictly in front of main-engine 3D BG0.
uint8_t ppu_backgrounds_above_3d();
bool ppu_geometry_active();

// Debug output, so a frame can be inspected without a window yet.
bool ppu_write_bmp(const char *path, const Framebuffer &fb);

}  // namespace ntr

#endif  // NTR_PPU_H
