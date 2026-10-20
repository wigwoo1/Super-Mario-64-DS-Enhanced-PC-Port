#pragma once
#include <algorithm>
#include <cstdint>

namespace ntr {
// Pointer coordinates and saved placement use the same 0..1000 space.
struct MinimapDrag {
    bool held=false,active=false;
    int grab_x=0,grab_y=0;
    bool update(bool down,int x,int y,uint16_t scale,uint16_t &left,uint16_t &top) {
        const bool pressed=down&&!held;
        held=down;
        if(pressed && x>=left && y>=top && x<int(left)+scale && y<int(top)+scale) {
            active=true;grab_x=x-left;grab_y=y-top;
        }
        if(!down) {active=false;return false;}
        if(!active)return false;
        const auto nx=static_cast<uint16_t>(std::clamp(x-grab_x,0,1000-int(scale)));
        const auto ny=static_cast<uint16_t>(std::clamp(y-grab_y,0,1000-int(scale)));
        const bool changed=nx!=left || ny!=top;
        left=nx;top=ny;return changed;
    }
    void reset() {*this=MinimapDrag{};}
};
}
