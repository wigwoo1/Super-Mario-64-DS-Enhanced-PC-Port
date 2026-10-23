#pragma once
#include <algorithm>
#include <cstdint>
#include "ntr/minimap_layout.h"

namespace ntr {
// Pointer coordinates and saved placement use the same 0..1000 space.
struct MinimapDrag {
    bool held=false,active=false;
    int grab_x=0,grab_y=0;
    bool update(bool down,int x,int y,uint16_t scale,uint16_t &left,uint16_t &top,MinimapHudBounds bounds={}) {
        const auto max_x=minimap_position_limit(scale,bounds.right);
        const auto max_y=minimap_position_limit(scale,bounds.bottom);
        const int drawn_left=std::min(left,max_x),drawn_top=std::min(top,max_y);
        const bool pressed=down&&!held;
        held=down;
        if(pressed && x>=drawn_left && y>=drawn_top && x<drawn_left+scale && y<drawn_top+scale) {
            active=true;grab_x=x-drawn_left;grab_y=y-drawn_top;
        }
        if(!down) {active=false;return false;}
        if(!active)return false;
        const auto nx=static_cast<uint16_t>(std::clamp(x-grab_x,0,int(max_x)));
        const auto ny=static_cast<uint16_t>(std::clamp(y-grab_y,0,int(max_y)));
        const bool changed=nx!=left || ny!=top;
        left=nx;top=ny;return changed;
    }
    void reset() {*this=MinimapDrag{};}
};
}
