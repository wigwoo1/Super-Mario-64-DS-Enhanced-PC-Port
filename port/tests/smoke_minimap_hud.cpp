#include "ntr/minimap_hud.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

int main() {
    using namespace ntr;
    constexpr unsigned w=256,h=192;
    std::vector<uint32_t> pixels(w*h);
    VideoSettings s;
    s.minimap=true;s.minimap_x=710;s.minimap_y=35;s.minimap_scale=260;
    minimap_hud_build(pixels.data(),w,h,s,true,21,47,6);
    const auto count=[](const std::vector<uint32_t>&p){return std::count_if(p.begin(),p.end(),[](uint32_t x){return (x>>24)!=0;});};
    const auto normal=count(pixels);
    if(normal<300||normal>1500)return 1;
    // The default right-corner minimap places its counters immediately to its left.
    if(pixels[24*w+165]==0)return 2;
    std::vector<uint32_t> hidden(w*h,0xffffffffu);
    minimap_hud_build(hidden.data(),w,h,s,false,21,47,6);
    if(std::any_of(hidden.begin(),hidden.end(),[](uint32_t x){return x!=0;}))return 3;
    s.minimap=false;
    minimap_hud_build(pixels.data(),w,h,s,true,21,47,6);
    if(count(pixels)!=normal)return 7;
    s.minimap=true;
    s.minimap_scale=500;s.minimap_x=0;s.minimap_y=0;
    minimap_hud_build(pixels.data(),w,h,s,true,150,999,99);
    const auto large=count(pixels);
    if(large<=normal)return 4;
    if(pixels[60*w+150]==0)return 5;
    std::vector<uint32_t> dst(w*h,0xff000000u),overlay(w*h,0u);
    overlay[0]=0x80ff0000u;minimap_hud_blend(dst.data(),overlay.data(),w,h);
    if(dst[0]!=0xff800000u)return 6;
    std::puts("PASS minimap counters follow map size/position, hide outside gameplay, and blend safely");
    return 0;
}
