#include "ntr/screen_layout.h"
#include <algorithm>

namespace ntr {
ScreenRect screen_fit(unsigned source_width,unsigned source_height,const ScreenRect &slot) {
    if(slot.width<=0 || slot.height<=0 || !source_width || !source_height) return {};
    int w=slot.width,h=int(int64_t(w)*source_height/source_width);
    if(h>slot.height) { h=slot.height;w=int(int64_t(h)*source_width/source_height); }
    return {slot.x+(slot.width-w)/2,slot.y+(slot.height-h)/2,w,h};
}
namespace {
ScreenRect fit(int x,int y,int width,int height) { return screen_fit(4,3,{x,y,width,height}); }
}
DualScreenLayout screen_layout(MinigameLayout mode, unsigned width, unsigned height) {
    const int w=static_cast<int>(width),h=static_cast<int>(height);
    if(mode==MinigameLayout::LeftRight) {
        const int split=w*3/4;
        return {fit(0,0,split,h),fit(split,0,w-split,h)};
    }
    if(mode==MinigameLayout::RightLeft) {
        const int split=w-w*3/4;
        return {fit(split,0,w-split,h),fit(0,0,split,h)};
    }
    const int split=h*3/4;
    return {fit(0,0,w,split),fit(0,split,w,h-split)};
}
const char *screen_layout_name(MinigameLayout mode) {
    return mode==MinigameLayout::LeftRight?"LEFT/RIGHT":
        mode==MinigameLayout::RightLeft?"RIGHT/LEFT":"TOP/BOTTOM";
}
bool screen_touch(const ScreenRect &r,int x,int y,uint16_t &ds_x,uint16_t &ds_y) {
    ds_x=ds_y=0;
    if(r.width<=0 || r.height<=0 || x<r.x || y<r.y || x>=r.x+r.width || y>=r.y+r.height) return false;
    ds_x=static_cast<uint16_t>((x-r.x)*SCREEN_W/r.width);
    ds_y=static_cast<uint16_t>((y-r.y)*SCREEN_H/r.height);
    return true;
}
void screen_compose(const Framebuffer &top,const Framebuffer &bottom,
                    const DualScreenLayout &layout,unsigned width,unsigned height,
                    std::vector<uint32_t> &output) {
    output.assign(static_cast<size_t>(width)*height,0xff000000u);
    auto draw=[&](const Framebuffer &source,const ScreenRect &r) {
        for(int y=std::max(0,r.y);y<std::min<int>(height,r.y+r.height);++y)
            for(int x=std::max(0,r.x);x<std::min<int>(width,r.x+r.width);++x)
                output[static_cast<size_t>(y)*width+x]=source.px[(y-r.y)*SCREEN_H/r.height][(x-r.x)*SCREEN_W/r.width]|0xff000000u;
    };
    draw(top,layout.top);draw(bottom,layout.bottom);
}
}
