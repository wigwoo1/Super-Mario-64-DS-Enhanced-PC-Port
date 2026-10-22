#ifndef NTR_MINIMAP_HUD_H
#define NTR_MINIMAP_HUD_H

#include "ntr/video.h"
#include <cstdint>

namespace ntr {
// Build a transparent overlay around the minimap. Coordinates and icon sizes
// use the DS display's logical pixels so they scale with the map on every GPU.
void minimap_hud_build(uint32_t *pixels, unsigned width, unsigned height,
                       const VideoSettings &settings, bool visible,
                       unsigned stars, unsigned coins, unsigned lives);
void minimap_hud_blend(uint32_t *destination, const uint32_t *overlay,
                       unsigned width, unsigned height);
}

#endif
