#ifndef NTR_MINIMAP_HUD_H
#define NTR_MINIMAP_HUD_H

#include "ntr/video.h"
#include <cstdint>
#include <cstddef>
#include <vector>

namespace ntr {
struct HudSprite {unsigned width=0,height=0;std::vector<uint32_t> pixels;};
struct MinimapHudArt {HudSprite star,coin,lives[4],health[9];};
// Decode original 4bpp, 2D-mapped OBJ templates into private RGBA surfaces.
bool minimap_hud_decode(HudSprite &,const uint8_t *attributes,size_t bytes,
                       const uint8_t *tiles,size_t tile_bytes,const uint8_t *palette,size_t palette_bytes);
void minimap_hud_set_art(MinimapHudArt art);
// Build a transparent overlay around the minimap. Coordinates and icon sizes
// use the DS display's logical pixels so they scale with the map on every GPU.
void minimap_hud_build(uint32_t *pixels, unsigned width, unsigned height,
                       const VideoSettings &settings, bool visible,
                       unsigned stars, unsigned coins, unsigned lives,
                       unsigned health=8,unsigned character=3);
void minimap_hud_blend(uint32_t *destination, const uint32_t *overlay,
                       unsigned width, unsigned height);
}

#endif
