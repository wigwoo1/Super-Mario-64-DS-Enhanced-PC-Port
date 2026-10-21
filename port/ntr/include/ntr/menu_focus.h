#pragma once
#include "ntr/ppu.h"
#include <algorithm>
#include <cstdlib>

namespace ntr {
// DS-sized pixel artwork, scaled with the menu on every renderer/layout.
inline void draw_menu_focus(Framebuffer &frame,int left,int top,int right,int bottom) {
    if(left<0 || right<=left || bottom<=top)return;
    const int l=std::clamp(left-2,0,255),r=std::clamp(right+2,0,255);
    const int t=std::clamp(top-2,0,191),b=std::clamp(bottom+2,0,191);
    constexpr uint32_t dark=0xff704016,gold=0xffffbd32,light=0xffffef9c;
    auto pixel=[&](int x,int y,uint32_t color) {
        if(x>=0 && x<SCREEN_W && y>=0 && y<SCREEN_H)frame.px[y][x]=color;
    };
    for(int x=l+2;x<=r-2;++x) {
        pixel(x,t,dark);pixel(x,t+1,light);pixel(x,b-1,gold);pixel(x,b,dark);
    }
    for(int y=t+2;y<=b-2;++y) {
        pixel(l,y,dark);pixel(l+1,y,light);pixel(r-1,y,gold);pixel(r,y,dark);
    }
    // Small diamond studs at the four corners.
    for(int cx:{l+2,r-2})for(int cy:{t+2,b-2})
        for(int dy=-2;dy<=2;++dy)for(int dx=-2;dx<=2;++dx)
            if(std::abs(dx)+std::abs(dy)<=2)
                pixel(cx+dx,cy+dy,dx==0 && dy==0?light:gold);
    // Outlined gold pointer. Move it inside when the option touches the edge.
    const int ax=l>=11?l-10:l+4,ay=(t+b)/2;
    for(int y=-5;y<=5;++y)for(int x=0;x<=5-std::abs(y);++x)
        pixel(ax+x,ay+y,x==0 || x==5-std::abs(y)?dark:gold);
    pixel(ax+1,ay-2,light);pixel(ax+2,ay-1,light);
}
}
