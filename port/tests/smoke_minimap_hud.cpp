Warning: fs was declared with const; use let for reassignable variables.
#include "ntr/minimap_hud.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

int main() {
    using namespace ntr;
    // Synthetic tiles distinguish DS 2D row stride from linear tile packing.
    std::vector<uint8_t> tiles(0x1000),palette(512);
    std::fill(tiles.begin(),tiles.begin()+32,0x11);
    std::fill(tiles.begin()+32*32,tiles.begin()+33*32,0x22);
    palette[34]=31;palette[36]=0xe0;palette[37]=3;
    std::array<uint8_t,8> attributes{{248,0,248,0x41,0,0x10,255,255}};
    HudSprite sprite;
    if(!minimap_hud_decode(sprite,attributes.data(),8,tiles.data(),tiles.size(),palette.data(),palette.size()))return 1;
    if(sprite.width!=16 || sprite.height!=16 || sprite.pixels[0]!=0xffff0000 || sprite.pixels[8*16]!=0xff00ff00 || sprite.pixels[8]!=0)return 2;
    const auto original=sprite;
    if(minimap_hud_decode(sprite,attributes.data(),7,tiles.data(),tiles.size(),palette.data(),palette.size()) || sprite.pixels!=original.pixels)return 3;
    if(minimap_hud_decode(sprite,attributes.data(),8,tiles.data(),16,palette.data(),palette.size()) || sprite.pixels!=original.pixels)return 4;
    attributes[3]|=0x30; // horizontal and vertical flip
    if(!minimap_hud_decode(sprite,attributes.data(),8,tiles.data(),tiles.size(),palette.data(),palette.size()) || sprite.pixels[15]!=0xff00ff00 || sprite.pixels[15*16+15]!=0xffff0000)return 5;
    attributes[3]&=~0x30;
    std::array<uint8_t,16> overlap{};
    std::copy(attributes.begin(),attributes.end(),overlap.begin());overlap[6]=overlap[7]=0;
    std::copy(attributes.begin(),attributes.end(),overlap.begin()+8);overlap[12]=32;
    if(!minimap_hud_decode(sprite,overlap.data(),overlap.size(),tiles.data(),tiles.size(),palette.data(),palette.size()) || sprite.pixels[0]!=0xffff0000)return 6;
    MinimapHudArt art;art.star=art.coin=original;
    for(auto &s:art.lives)s=original;
    for(auto &s:art.health)s=original;
    minimap_hud_set_art(art);
    constexpr unsigned w=256,h=192;std::vector<uint32_t> pixels(w*h),other(w*h);
    VideoSettings s;s.minimap=true;s.minimap_x=710;s.minimap_y=35;s.minimap_scale=260;
    minimap_hud_build(pixels.data(),w,h,s,true,21,47,6,8,3);
    const auto count=[](const auto &p){return std::count_if(p.begin(),p.end(),[](uint32_t x){return (x>>24)!=0;});};
    const auto normal=count(pixels);if(normal<500 || normal>2500)return 7;
    for(unsigned y=0;y<h;++y)for(unsigned x=182;x<w;++x)if(pixels[y*w+x])return 8;
    minimap_hud_build(other.data(),w,h,s,false,21,47,6,8,3);
    if(count(other))return 9;
    s.minimap=false;minimap_hud_build(other.data(),w,h,s,true,21,47,6,8,3);
    if(other!=pixels)return 10;
    minimap_hud_build(other.data(),w,h,s,true,21,47,6,4,3);if(other==pixels)return 11;
    art.lives[0].pixels.assign(256,0xff123456);minimap_hud_set_art(art);
    minimap_hud_build(other.data(),w,h,s,true,21,47,6,8,0);if(other==pixels)return 12;
    s.minimap=true;s.minimap_scale=500;s.minimap_x=0;s.minimap_y=0;
    minimap_hud_build(pixels.data(),w,h,s,true,150,999,99,0,1);if(count(pixels)<=normal)return 13;
    std::vector<uint32_t> dst(w*h,0xff000000u),overlay(w*h,0u);overlay[0]=0x80ff0000u;
    minimap_hud_blend(dst.data(),overlay.data(),w,h);if(dst[0]!=0xff800000u)return 14;
    s.minimap_scale=260;s.minimap_x=710;s.minimap_y=35;
    minimap_hud_build(pixels.data(),w,h,s,true,21,47,6,8,3);
    auto bounds=[](const std::vector<uint32_t> &p,unsigned width,unsigned height){
        unsigned left=width,top=height,right=0,bottom=0;
        for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x)if(p[size_t(y)*width+x]>>24){
            left=std::min(left,x);top=std::min(top,y);right=std::max(right,x+1);bottom=std::max(bottom,y+1);
        }
        return std::array<unsigned,4>{left,top,right,bottom};
    };
    const auto native=bounds(pixels,w,h);
    constexpr unsigned rw=1280,rh=720;std::vector<uint32_t> high(size_t(rw)*rh);
    minimap_hud_build(high.data(),rw,rh,s,true,21,47,6,8,3);
    const auto scaled=bounds(high,rw,rh);
    if(scaled[2]-scaled[0]<5*(native[2]-native[0])-4 ||
       scaled[3]-scaled[1]<3*(native[3]-native[1]) ||
       scaled[3]-scaled[1]>5*(native[3]-native[1]))return 15;
    const auto antialiased=std::count_if(high.begin(),high.end(),[](uint32_t p){const unsigned a=p>>24;return a>0xd9&&a<255;});
    if(antialiased<20)return 16;
    std::puts("PASS original OBJ decode bounds/stride/palette/flips/priority and four minimap counters with health/character selection");
}
