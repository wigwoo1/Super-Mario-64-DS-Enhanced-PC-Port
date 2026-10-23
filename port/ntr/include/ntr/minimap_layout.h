#pragma once
#include "ntr/video.h"
#include <algorithm>
#include <cstdint>
#include <cstddef>

namespace ntr {
// Bounds of visible map pixels, in thousandths of the source texture.
struct MinimapHudBounds {unsigned left=0,top=0,right=1000,bottom=1000;};

inline MinimapHudBounds minimap_visible_bounds(const uint8_t *transparent,unsigned width,unsigned height) {
    if(!transparent || !width || !height)return {};
    unsigned left=width,top=height,right=0,bottom=0;
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x)
        if(!transparent[size_t(y)*width+x]) {
            left=std::min(left,x);top=std::min(top,y);
            right=std::max(right,x+1);bottom=std::max(bottom,y+1);
        }
    if(right<=left || bottom<=top)return {};
    return {left*1000/width,top*1000/height,
            (right*1000+width-1)/width,(bottom*1000+height-1)/height};
}

inline uint16_t minimap_position_limit(uint16_t scale,unsigned visible_edge) {
    // Only transparent padding may extend beyond the output. Round inward so
    // the last visible source pixel remains inside the screen at every size.
    return uint16_t(1000-(std::min<unsigned>(scale,1000)*std::clamp(visible_edge,1u,1000u)+999)/1000);
}

inline VideoSettings minimap_visible_placement(VideoSettings s,MinimapHudBounds bounds) {
    s.minimap_x=std::min(s.minimap_x,minimap_position_limit(s.minimap_scale,bounds.right));
    s.minimap_y=std::min(s.minimap_y,minimap_position_limit(s.minimap_scale,bounds.bottom));
    return s;
}
}
