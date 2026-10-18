#include "ntr/gpu.h"
#include "ntr/gx.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>
namespace {
int failures;
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
void scene(const uint32_t *pixels,int w,int h,ntr::TexturePixelsInfo info={}) {
    ntr::gx_reset();ntr::gx_bind_texture(pixels,w,h,8,8,info);
    ntr::gx_write_port(0x040004a4,0x001f10c0);ntr::gx_write_port(0x04000500,1);
    const short xy[4][2]={{-3277,-3277},{3277,-3277},{3277,3277},{-3277,3277}};
    const unsigned uv[4][2]={{0,128},{128,128},{128,0},{0,0}};
    for(unsigned i=0;i<4;++i) {
        ntr::gx_write_port(0x04000488,uv[i][0]|(uv[i][1]<<16));
        ntr::gx_write_port(0x0400048c,static_cast<uint16_t>(xy[i][0])|
            (static_cast<uint32_t>(static_cast<uint16_t>(xy[i][1]))<<16));
        ntr::gx_write_port(0x0400048c,0);
    }
    ntr::gx_write_port(0x04000504,0);
}
}
int main() {
    using namespace ntr;
    CHECK(gpu_open(nullptr,true));if(failures)return 1;
    Framebuffer base;for(auto &row:base.px)for(auto &p:row)p=0xff102030;
    VideoSettings settings;settings.width=256;settings.height=192;settings.minimap=false;settings.upscale_2d=true;
    std::vector<uint32_t> texture(4096u*4096u);
    for(unsigned y=0;y<4096;++y)for(unsigned x=0;x<4096;++x)texture[y*4096+x]=x<2048?0xff123456:0x8088dd44;
    texture_pixel_work_reset();const auto info=texture_pixels_info(texture.data(),4096,4096);
    auto work=texture_pixel_work_reset();CHECK(work.calls==1 && work.texels==4096u*4096u && info.partial_alpha && info.immutable);
    CHECK(texture_pixels_key(texture.data(),4096,4096)==info.key);
    CHECK(!texture_pixels_info(nullptr,1,1).immutable && texture_pixels_key(nullptr,1,1)==0);
    scene(texture.data(),4096,4096,info);
    std::vector<uint32_t> baseline,optimized;unsigned width,height;
    gpu_force_texture_scan(true);CHECK(gpu_render(base,nullptr,settings,true));
    CHECK(gpu_readback(baseline,width,height));CHECK(width==256 && height==192);
    uint64_t scan[2]={},cpu[2]={},uploads[2]={};
    for(unsigned phase=0;phase<2;++phase) {
        gpu_force_texture_scan(phase==0);texture_pixel_work_reset();
        const auto start=std::chrono::steady_clock::now();
        for(unsigned frame=0;frame<20;++frame) {
            CHECK(gpu_render(base,nullptr,settings,true));const auto s=gpu_frame_stats();
            scan[phase]+=s.hash_texels;cpu[phase]+=s.cpu_us;uploads[phase]+=s.uploads;
            if(phase==1)CHECK(s.identity_uses==1 && s.hash_texels==0);
        }
        const auto elapsed=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count();
        work=texture_pixel_work_reset();
        CHECK(phase==0?work.calls==20:work.calls==0);
        std::printf("PROFILE %s: frames=20 CPU_submit_us=%llu elapsed_us=%lld hashed_texels=%llu uploads=%llu\n",
            phase==0?"forced_scan":"immutable",cpu[phase],static_cast<long long>(elapsed),scan[phase],uploads[phase]);
    }
    CHECK(scan[0]==20ull*4096*4096 && scan[1]==0 && uploads[0]==0 && uploads[1]==0);
    CHECK(gpu_readback(optimized,width,height));CHECK(baseline==optimized);
    // A new immutable decode has a new identity and uploads once.
    gx_reset();for(unsigned y=0;y<4096;++y)std::fill(texture.begin()+y*4096,texture.begin()+y*4096+2048,0xffdd0000);
    const auto changed=texture_pixels_info(texture.data(),4096,4096);CHECK(changed.key!=info.key);
    scene(texture.data(),4096,4096,changed);CHECK(gpu_render(base,nullptr,settings,true));
    CHECK(gpu_frame_stats().uploads==1 && gpu_frame_stats().hash_texels==0);
    CHECK(gpu_readback(optimized,width,height));CHECK(optimized[96*256+64]!=baseline[96*256+64]);
    // Mutable clients retain per-frame detection, even at the same address.
    uint32_t mutable_pixel=0xffff0000;scene(&mutable_pixel,1,1);
    CHECK(gpu_render(base,nullptr,settings,true));CHECK(gpu_frame_stats().hash_texels==1);
    CHECK(gpu_readback(baseline,width,height));mutable_pixel=0xff0000ff;
    CHECK(gpu_render(base,nullptr,settings,true));CHECK(gpu_frame_stats().hash_texels==1 && gpu_frame_stats().uploads==1);
    CHECK(gpu_readback(optimized,width,height));CHECK(optimized[96*256+128]!=baseline[96*256+128]);
    gpu_close();gx_reset();if(failures)return 1;
    std::puts("PASS: identical4K texture pixels with zero steady-state rescans/uploads; partial alpha, changed identities and mutable bindings retain correct behavior");
}
