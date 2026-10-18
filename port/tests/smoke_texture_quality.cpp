#include "ntr/texture.h"
#include "ntr/video.h"
#include "ntr/gx.h"
#include "ntr/mmio.h"
#include "ntr/gpu.h"
#include <cmath>
#include <cstdio>
#include <vector>

namespace {
int failures;
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
void triangle(uint32_t attributes=0x001f10c0) {
    ntr::gx_write_port(0x040004a4,attributes);
    ntr::gx_write_port(0x04000500,0);
    for(int i=0;i<3;++i) {
        ntr::gx_write_port(0x04000488,64u|(64u<<16)); // original texel coordinate 4,4
        ntr::gx_write_port(0x0400048c,static_cast<uint16_t>(i==1?2048:-2048)|
            (static_cast<uint32_t>(static_cast<uint16_t>(i==2?2048:-2048))<<16));
        ntr::gx_write_port(0x0400048c,0);
    }
    ntr::gx_write_port(0x04000504,0);
}
}
int main() {
    using namespace ntr;
    unsigned char bytes[128],palette[64]={};
    for(unsigned i=0;i<64;++i) {
        const unsigned value=(i%4?0x8000:0)|i|(i<<5);
        bytes[2*i]=static_cast<unsigned char>(value);bytes[2*i+1]=static_cast<unsigned char>(value>>8);
    }
    TextureDesc d;d.width=d.height=8;d.format=7;
    d.data=bytes;d.data_len=sizeof(bytes);d.pal=palette;d.pal_len=sizeof(palette);
    std::vector<uint32_t> original,decoded;int width=0,height=0;
    texture_set_quality(TextureQuality::Original);
    CHECK(texture_decode(d,original,&width,&height));
    const int sizes[]={8,6,4,2,8,8};
    for(unsigned mode=0;mode<6;++mode) {
        const auto quality=static_cast<TextureQuality>(mode);
        texture_set_quality(quality);
        CHECK(texture_decode(d,decoded,&width,&height));
        CHECK(width==sizes[mode] && height==sizes[mode] && decoded.size()==static_cast<size_t>(width*height));
        if(mode>=1 && mode<=3) {
            for(int y=0;y<height;++y) for(int x=0;x<width;++x) {
                const int sx=(2*x+1)*8/(2*width),sy=(2*y+1)*8/(2*height);
                CHECK(decoded[y*width+x]==original[sy*8+sx]);
            }
        } else if(quality==TextureQuality::None) {
            for(unsigned i=0;i<64;++i) CHECK(decoded[i]==((original[i]&0xff000000u)|0x00ffffffu));
        } else CHECK(decoded==original); // Unclassified Upscaled must remain original.
        CHECK(texture_cycle_quality(texture_cycle_quality(quality),-1)==quality);
    }
    // Original alpha levels, including partial effects, survive No Textures.
    d.format=1;d.width=d.height=8;d.data_len=64;
    for(unsigned i=0;i<64;++i) bytes[i]=static_cast<unsigned char>((i%8)<<5);
    texture_set_quality(TextureQuality::Original);CHECK(texture_decode(d,original));
    texture_set_quality(TextureQuality::None);CHECK(texture_decode(d,decoded));
    for(unsigned i=0;i<64;++i) CHECK((original[i]>>24)==(decoded[i]>>24) && (decoded[i]&0xffffff)==0xffffff);
    // Non-square and minimum dimensions are deterministic, including rounding.
    d.format=7;d.width=1;d.height=8;d.data_len=16;
    texture_set_quality(TextureQuality::Percent75);CHECK(texture_decode(d,decoded,&width,&height));
    CHECK(width==1 && height==6);
    texture_set_quality(static_cast<TextureQuality>(255));CHECK(texture_quality()==TextureQuality::Original);

    // Exercise the actual game VRAM/cache path without clearing between modes.
    CHECK(io_init());gx_reset();
    auto *vram=reinterpret_cast<unsigned short *>(0x06800000u);
    for(unsigned i=0;i<64;++i) vram[i]=static_cast<unsigned short>(0x8000|i);
    const uint32_t param=7u<<26;
    for(unsigned mode=0;mode<6;++mode) {
        texture_set_quality(static_cast<TextureQuality>(mode));
        gx_teximage_param(param);gx_pltt_base(0);triangle();
        size_t count=0;const auto *polys=gx_polygons(count);
        CHECK(count==mode+1);
        const auto &t=polys[count-1];
        CHECK(t.tw==sizes[mode] && t.th==sizes[mode]);
        CHECK(std::abs(t.v[0].u/t.tw-.5f)<1e-6f && std::abs(t.v[0].v/t.th-.5f)<1e-6f);
    }
    texture_set_quality(TextureQuality::Original);gx_teximage_param(param);triangle();
    size_t count=0;const auto *polys=gx_polygons(count);
    CHECK(polys[0].tex==polys[count-1].tex); // Reusing a cached mode does not hold stale pixels.
    CHECK(polys[4].tex[0]==0xffffffff && polys[0].tex[0]==0xff000000);
    texture_set_quality(TextureQuality::None);triangle(0x001f10d0);
    polys=gx_polygons(count);CHECK(polys[count-1].attributes==0x001f10c0);
    triangle(0x000f10f0);polys=gx_polygons(count);CHECK(polys[count-1].attributes==0x000f10f0);
    // Real GPU/software agreement: removing color retains material lighting,
    // transparent geometry holes and the texture's normalized placement.
    CHECK(gpu_open(nullptr,true));
    VideoSettings settings;settings.width=256;settings.height=192;
    settings.minimap=false;settings.upscale_2d=true;
    Framebuffer base;for(auto &row:base.px)for(auto &p:row)p=0xff102030;
    for(unsigned mode=0;mode<6;++mode) {
        gx_reset();texture_set_quality(static_cast<TextureQuality>(mode));
        auto *encoded=reinterpret_cast<unsigned char *>(0x06800000u);
        for(unsigned i=0;i<64;++i)encoded[i]=static_cast<unsigned char>((i%8<4?0:7)<<5|1);
        auto *pal=reinterpret_cast<unsigned short *>(0x06880000u);pal[1]=0x7c00;
        gx_teximage_param(1u<<26);gx_pltt_base(0);
        gx_write_port(0x04000480,0x03e0);gx_write_port(0x040004a4,0x001f10c0);
        gx_write_port(0x04000500,1);
        const short xy[4][2]={{-4096,-4096},{4096,-4096},{4096,4096},{-4096,4096}};
        const unsigned uv[4][2]={{0,128},{128,128},{128,0},{0,0}};
        for(unsigned i=0;i<4;++i) {
            gx_write_port(0x04000488,uv[i][0]|(uv[i][1]<<16));
            gx_write_port(0x0400048c,static_cast<unsigned short>(xy[i][0])|
                (static_cast<uint32_t>(static_cast<unsigned short>(xy[i][1]))<<16));
            gx_write_port(0x0400048c,0);
        }
        gx_write_port(0x04000504,0);
        std::vector<uint32_t> pixels;unsigned w=0,h=0;
        CHECK(gpu_render(base,nullptr,settings,true));CHECK(gpu_readback(pixels,w,h));
        CHECK(w==256 && h==192);
        auto reference=base;gx_render(reference);
        for(unsigned y:{32u,96u,160u}) {
            CHECK(pixels[y*256+64]==0xff102030); // Original alpha hole, all modes.
            CHECK(pixels[y*256+192]==(mode==4?0xff00ff00:0xff000000));
            CHECK(pixels[y*256+64]==reference.px[y][64]);
            CHECK(pixels[y*256+192]==reference.px[y][192]);
        }
    }
    gpu_close();gx_reset();texture_set_quality(TextureQuality::Original);
    if(failures)return 1;
    std::puts("PASS: six texture qualities, dimensions, exact center samples, alpha preservation, minimum sizes, live VRAM cache separation and unchanged normalized UVs");
}
