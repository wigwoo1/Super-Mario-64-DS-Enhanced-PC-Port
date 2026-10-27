#pragma once
#include "ntr/ppu.h"
#include <vector>

namespace ntr {
enum class MinigameLayout : uint8_t { TopBottom, LeftRight, RightLeft };
struct ScreenRect { int x=0, y=0, width=0, height=0; };
struct DualScreenLayout { ScreenRect top, bottom; };
ScreenRect screen_fit(unsigned source_width, unsigned source_height, const ScreenRect &slot);
// Split the selected axis 75/25, then fit each original 4:3 screen in its slot.
DualScreenLayout screen_layout(MinigameLayout, unsigned width, unsigned height);
const char *screen_layout_name(MinigameLayout);
// Reject the top screen and black bars. Coordinates are in output pixels.
bool screen_touch(const ScreenRect &, int x, int y, uint16_t &ds_x, uint16_t &ds_y);
void screen_compose(const Framebuffer &top, const Framebuffer &bottom,
                    const DualScreenLayout &, unsigned width, unsigned height,
                    std::vector<uint32_t> &output);
// Original mission selector: one4:3 canvas with the lower controls inset.
DualScreenLayout star_select_layout(unsigned width,unsigned height);
bool star_select_touch(const ScreenRect &canvas,int x,int y,uint16_t &,uint16_t &);
void star_select_overlay(const Framebuffer &controls,Framebuffer &canvas);
}
