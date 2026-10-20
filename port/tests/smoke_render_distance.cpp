#include "hal/render_distance_policy.h"
#include <cstdio>

int main() {
    using ntr::RenderDistance;
    if(pc_render_distance::limit(RenderDistance::Minimum,100,false)!=100)return 1;
    if(pc_render_distance::limit(RenderDistance::Low,100,false)!=200)return 2;
    if(pc_render_distance::limit(RenderDistance::Medium,100,false)!=400)return 3;
    if(pc_render_distance::limit(RenderDistance::EntireMap,100,false)!=pc_render_distance::kMaximumDepth)return 4;
    if(pc_render_distance::limit(RenderDistance::Low,100,true)!=400)return 5;
    if(pc_render_distance::limit(RenderDistance::Medium,100,true)!=800)return 6;
    if(pc_render_distance::visible(99,100)==false || pc_render_distance::visible(100,100)==false)return 7;
    if(pc_render_distance::visible(101,100) || pc_render_distance::visible(-1,100) ||
       pc_render_distance::visible(pc_render_distance::kOutsideFrustum,pc_render_distance::kMaximumDepth))return 8;
    if(pc_render_distance::limit(RenderDistance::Medium,0x40000000,false)!=pc_render_distance::kMaximumDepth)return 9;
    if(pc_render_distance::projection_far(RenderDistance::Minimum,1000)!=1000)return 10;
    if(pc_render_distance::projection_far(RenderDistance::Low,1000)!=2000)return 11;
    if(pc_render_distance::projection_far(RenderDistance::Medium,1000)!=4000)return 12;
    if(pc_render_distance::projection_far(RenderDistance::EntireMap,1000)!=pc_render_distance::kMaximumDepth)return 13;
    if(pc_render_distance::projection_far(RenderDistance::Medium,0x40000000)!=pc_render_distance::kMaximumDepth)return 14;
    std::puts("render-distance policy: preset thresholds and frustum rejection pass");
    return 0;
}
