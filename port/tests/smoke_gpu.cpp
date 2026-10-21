#include "ntr/gpu.h"
#include "ntr/gx.h"
#include "ntr/mmio.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <utility>
namespace {
int failures;
#define CHECK(value) do { if(!(value)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#value);++failures; } } while(0)
int fixed(float x) { return static_cast<int>(std::lround(x*4096)); }
void setup(uint32_t attrs=0x001f10c0) {
    ntr::gx_reset();ntr::gx_write_port(0x04000440,0);
    const float projection[16]={1,0,0,0,0,1,0,0,0,0,-7.0f/5,-1,0,0,-12.0f/5,0};
    for(float v:projection) ntr::gx_write_port(0x04000458,static_cast<uint32_t>(fixed(v)));
    ntr::gx_write_port(0x04000440,1);ntr::gx_write_port(0x04000454,0);
    ntr::gx_write_port(0x040004a4,attrs);
}
void v(float x,float y,float z,float u=0,float tv=0) {
    ntr::gx_write_port(0x04000488,uint16_t(std::lround(u*16))|(uint32_t(uint16_t(std::lround(tv*16)))<<16));
    ntr::gx_write_port(0x0400048c,uint16_t(fixed(x))|(uint32_t(uint16_t(fixed(y)))<<16));
    ntr::gx_write_port(0x0400048c,uint16_t(fixed(z)));
}
void triangle(float z=-2) {
    ntr::gx_write_port(0x04000500,0);v(-0.9f,-0.6f,z);v(0.8f,-0.6f,z);v(0.1f,0.9f,z);
    ntr::gx_write_port(0x04000504,0);
}
void rectangle(float left,float bottom,float right,float top,float z,uint32_t attrs,uint16_t color) {
    ntr::gx_write_port(0x040004a4,attrs);ntr::gx_write_port(0x04000480,color);
    ntr::gx_write_port(0x04000500,1);
    v(left*-z,bottom*-z,z);v(right*-z,bottom*-z,z);
    v(right*-z,top*-z,z);v(left*-z,top*-z,z);
    ntr::gx_write_port(0x04000504,0);
}
}
int main(int argc,char **argv) {
    const bool opengl=argc>1 && std::strcmp(argv[1],"--opengl")==0;
    CHECK(ntr::io_init()); // Reserve DS register ranges before D3D allocations.
    CHECK(ntr::gpu_open(nullptr,true,opengl?ntr::VideoRenderer::OpenGLSoftware:ntr::VideoRenderer::D3D11));
    if(failures) return 1;
    if(opengl)CHECK(std::strstr(ntr::gpu_adapter(),"llvmpipe")!=nullptr);
    for(uint8_t level:{uint8_t(2),uint8_t(4),uint8_t(8),uint8_t(16)}) {
        ntr::gpu_set_anisotropic_filtering(level);
        CHECK(ntr::gpu_anisotropic_filtering()==level);
    }
    ntr::gpu_set_anisotropic_filtering(0);
    CHECK(ntr::gpu_anisotropic_filtering()==0);
    std::printf("GPU smoke adapter: %s\n",ntr::gpu_adapter());
    ntr::Framebuffer base;
    for(auto &row:base.px) for(auto &p:row) p=0xff101820;
    ntr::VideoSettings s;s.width=256;s.height=192;s.upscale_2d=true;s.minimap=false;
    std::vector<uint32_t> pixels;unsigned width=0,height=0;
    {
        std::vector<uint32_t> cached;unsigned cw=0,ch=0;
        CHECK(!ntr::gpu_readback_gameplay(cached,cw,ch));
        static ntr::Framebuffer map;for(auto &row:map.px)for(auto &pixel:row)pixel=0xffff0000;
        auto preview=s;preview.width=512;preview.height=384;preview.minimap=true;
        preview.minimap_x=preview.minimap_y=250;preview.minimap_scale=500;
        CHECK(ntr::gpu_render(base,&map.px[0][0],preview,false,nullptr,nullptr,nullptr,true,true));
        CHECK(ntr::gpu_readback(pixels,width,height));
        CHECK(width==512 && height==384 && pixels[192*512+256]==0xffff0000);
        CHECK(ntr::gpu_readback_gameplay(cached,cw,ch));
        CHECK(cw==512 && ch==384 && cached[192*512+256]==0xff101820);
        static ntr::Framebuffer pause;pause=base;for(auto &row:pause.px)for(auto &pixel:row)pixel=0xff777777;
        CHECK(ntr::gpu_render(pause,nullptr,s,false));
        CHECK(ntr::gpu_readback_gameplay(cached,cw,ch));
        CHECK(cw==512 && ch==384 && cached[0]==0xff101820); // Pause did not replace gameplay cache.
        CHECK(ntr::gpu_render(base,nullptr,s,false,nullptr,nullptr,nullptr,true,true));
        CHECK(ntr::gpu_readback_gameplay(cached,cw,ch));
        CHECK(cw==256 && ch==192 && cached.back()==0xff101820); // Resize replaces cache correctly.
    }
    setup();triangle();
    CHECK(ntr::gpu_render(base,nullptr,s,true));CHECK(ntr::gpu_readback(pixels,width,height));
    CHECK(width==256 && height==192 && ntr::gpu_triangle_count()==1);
    auto reference=base;ntr::gx_render(reference);
    unsigned differences=0;
    for(unsigned i=0;i<pixels.size();++i) differences+=pixels[i]!=(&reference.px[0][0])[i];
    CHECK(differences<30); // Hardware top-left edge coverage differs at shared boundaries.
    CHECK(pixels[100*256+128]==0xffffffff && pixels[0]==0xff101820);
    // Native pixels must resolve edges inside a single logical DS pixel block.
    s.width=3840;s.height=2160;
    CHECK(ntr::gpu_render(base,nullptr,s,true));CHECK(ntr::gpu_readback(pixels,width,height));
    CHECK(width==3840 && height==2160 && pixels.size()==8294400);
    unsigned mixed=0;
    for(unsigned y=0;y<192;++y) for(unsigned x=0;x<256;++x) {
        const unsigned x0=x*width/256,x1=(x+1)*width/256;
        const unsigned y0=y*height/192,y1=(y+1)*height/192;
        bool white=false,background=false;
        for(unsigned yy=y0;yy<y1;++yy) for(unsigned xx=x0;xx<x1;++xx) {
            white|=pixels[yy*width+xx]==0xffffffff;background|=pixels[yy*width+xx]==0xff101820;
        }
        mixed+=white&&background;
    }
    CHECK(mixed>50);
    // Screen-sized menu backgrounds fill the output even though their original
    // draw uses perspective geometry. Gameplay must restore widened FOV.
    setup();rectangle(-1,-1,1,1,-2,0x001f10c0,0x7fff);
    ntr::gpu_set_menu_scene(true);
    CHECK(ntr::gpu_render(base,nullptr,s,true));CHECK(ntr::gpu_readback(pixels,width,height));
    CHECK(pixels[(height/2)*width+width/20]==0xffffffff);
    CHECK(pixels[(height/2)*width+width*19/20]==0xffffffff);
    ntr::gpu_set_menu_scene(false);
    // Peach's letter is 2D ink placed over a perspective panel. Its original
    // DS coordinates must use the same centered horizontal projection at
    // every aspect, including native DS and ultrawide, in both GPU backends.
    static ntr::Framebuffer letter;
    for(auto &row:letter.px)for(auto &p:row)p=0;
    for(int y=48;y<144;++y)for(int x=24;x<120;++x)letter.px[y][x]=0xffff0000;
    for(auto dimensions:{std::pair<unsigned,unsigned>{256,192},{1280,720},{960,400}}) {
        s.width=dimensions.first;s.height=dimensions.second;setup();
        ntr::gpu_set_letter_overlay(true);
        CHECK(ntr::gpu_render(base,nullptr,s,true,&letter.px[0][0]));
        CHECK(ntr::gpu_readback(pixels,width,height));
        unsigned left=width,right=0;
        for(unsigned x=0;x<width;++x)if(pixels[(height/2)*width+x]==0xffff0000) {
            left=std::min(left,x);right=x;
        }
        const double scale=height/192.0;
        CHECK(std::abs(double(left)-(width-256*scale)/2-24*scale)<=1);
        CHECK(std::abs(double(right+1)-(width-256*scale)/2-120*scale)<=1);
        ntr::gpu_set_letter_overlay(false);
        CHECK(ntr::gpu_render(base,nullptr,s,true,&letter.px[0][0]));
        CHECK(ntr::gpu_readback(pixels,width,height));
        CHECK(pixels[(height/2)*width+width/10]==0xffff0000);
    }
    s.width=3840;s.height=2160;setup();rectangle(-1,-1,1,1,-2,0x001f10c0,0x7fff);
    CHECK(ntr::gpu_render(base,nullptr,s,true));CHECK(ntr::gpu_readback(pixels,width,height));
    CHECK(pixels[(height/2)*width+width/20]==0xff101820);
    setup();ntr::gx_write_port(0x04000500,0);
    v(2.1f,-0.2f,-2);v(2.5f,-0.2f,-2);v(2.3f,0.2f,-2);ntr::gx_write_port(0x04000504,0);
    CHECK(ntr::gpu_render(base,nullptr,s,true));CHECK(ntr::gpu_triangle_count()>0);
    s.width=256;s.height=192;
    CHECK(ntr::gpu_render(base,nullptr,s,true));CHECK(ntr::gpu_triangle_count()==0);
    // Repeat, alpha rejection and depth must match the software perspective probe.
    s.width=256;s.height=192;setup();
    const uint32_t tex[8]={0xffff0000,0xff00ff00,0xff0000ff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff};
    ntr::gx_bind_texture(tex,8,1);ntr::gx_write_port(0x04000500,0);
    v(-0.6f,-0.6f,-1,0);v(2.4f,-2.4f,-4,4.5f);v(0,1.2f,-2,0);ntr::gx_write_port(0x04000504,0);
    CHECK(ntr::gpu_render(base,nullptr,s,true));CHECK(ntr::gpu_readback(pixels,width,height));
    CHECK(pixels[115*256+128]==0xffff0000);
    setup();ntr::gx_write_port(0x04000480,0x7c00);triangle(-3);
    const uint32_t transparent=0;ntr::gx_bind_texture(&transparent,1,1);triangle(-2);
    ntr::gx_bind_texture(nullptr,0,0);ntr::gx_write_port(0x04000480,0x001f);triangle(-4);
    CHECK(ntr::gpu_render(base,nullptr,s,true));CHECK(ntr::gpu_readback(pixels,width,height));
    CHECK(pixels[100*256+128]==0xff0000ff);
    // Cull, clip, complete-frame swap and live viewport resizing.
    setup(0x001f1000);triangle();CHECK(ntr::gpu_render(base,nullptr,s,true));CHECK(ntr::gpu_triangle_count()==0);
    setup();triangle(2);CHECK(ntr::gpu_render(base,nullptr,s,true));CHECK(ntr::gpu_triangle_count()==0);
    setup();triangle();ntr::gx_write_port(0x04000540,0);
    CHECK(ntr::gpu_render(base,nullptr,s,true));CHECK(ntr::gpu_triangle_count()==1);
    setup();ntr::gx_write_port(0x04000580,64|(48u<<8)|(191u<<16)|(143u<<24));triangle();
    CHECK(ntr::gpu_render(base,nullptr,s,true));CHECK(ntr::gpu_readback(pixels,width,height));
    CHECK(pixels[100*256+128]==0xffffffff && pixels[50*256+128]==0xff101820);
    // Shadow mask volumes must never become visible geometry. Shadows blend
    // only where depth-failing masks marked a receiver, excluding caster IDs.
    constexpr uint32_t ground=0x021f10c0,caster=0x011f10c0,mask=0x000f10f0,shade=0x010f10f0;
    auto read=[&](){
        CHECK(ntr::gpu_render(base,nullptr,s,true));CHECK(ntr::gpu_readback(pixels,width,height));
        if(s.width==256 && s.height==192) {
            auto software=base;ntr::gx_render(software);
            // Independent scalar implementation agrees away from raster edges.
            for(int x:{76,128,179}) for(int y:{70,96,120})
                for(unsigned shift:{0u,8u,16u}) {
                    const int cpu=(software.px[y][x]>>shift)&255;
                    const int gpu=(pixels[y*256+x]>>shift)&255;
                    CHECK(std::abs(cpu-gpu)<=1);
                }
        }
    };
    auto sample=[&](float x,float y=0) {
        const float aspect=(256.0f*height)/(192.0f*width);
        return pixels[unsigned((1-y)*0.5f*height)*width+unsigned((x*aspect+1)*0.5f*width)];
    };
    auto floor=[&](){rectangle(-.9f,-.9f,.9f,.9f,-3,ground,0x7fff);};
    auto player=[&](){rectangle(-.15f,-.15f,.15f,.15f,-2,caster,0x001f);};
    auto volume=[&](uint32_t id=1){
        rectangle(-.6f,-.6f,.6f,.6f,-4,mask,0);
        rectangle(-.6f,-.6f,.6f,.6f,-1.5f,(shade&0x00ffffff)|(id<<24),0);
    };
    setup();floor();rectangle(-.6f,-.6f,.6f,.6f,-4,mask,0);read();
    CHECK(sample(.4f)==0xffffffff); // Mask does not write black color.
    setup();floor();volume();player();read(); // Opaque caster submitted after the shadow.
    CHECK(sample(0)==0xffff0000);
    CHECK(sample(.4f)==0xff808080 && sample(.8f)==0xffffffff);
    setup();floor();player();volume();volume();read();
    CHECK(sample(.4f)==0xff808080 && sample(0)==0xffff0000); // Same ID is not blended twice.
    setup();floor();player();volume();volume(4);read();
    CHECK(sample(.4f)==0xff404040); // Different IDs can cast overlapping shadows.
    setup();floor();rectangle(-.6f,-.6f,.6f,.6f,-2.5f,mask,0);
    rectangle(-.6f,-.6f,.6f,.6f,-1.5f,shade,0);read();
    CHECK(sample(.4f)==0xffffffff); // Passing mask depth must NOT mark a shadow.
    setup();floor();rectangle(-.7f,-.7f,.7f,.7f,-4,mask,0);
    rectangle(-.7f,-.7f,0,.7f,-1.5f,shade,0);
    rectangle(0,-.7f,.7f,.7f,-2.5f,mask,0);
    rectangle(0,-.7f,.7f,.7f,-1.5f,(shade&0x00ffffff)|(4u<<24),0);read();
    CHECK(sample(-.4f)==0xff808080 && sample(.4f)==0xffffffff); // Clear old mask, preserve receiver IDs.
    setup();rectangle(-.9f,-.9f,0,.9f,-3,ground,0x7fff);volume();read();
    CHECK(sample(-.4f)==0xff808080 && sample(.4f)==0xff101820); // No receiver: no shadow on backdrop.
    for(unsigned update=0;update<2;++update) {
        setup();rectangle(-.9f,-.9f,.9f,.9f,-4,ground,0x7c00);
        rectangle(-.6f,-.6f,.6f,.6f,-2,0x030f10c0|(update<<11),0x001f);
        rectangle(-.6f,-.6f,.6f,.6f,-3,0x040f10c0,0x03e0);read();
        CHECK(sample(0)==(update?0xff800080:0xff408040));
    }
    // DS translucent-ID rejection is per pixel and differs from opaque IDs.
    // A fading head must hide its later crown/hair layers. IDs0 and63 also
    // need a validity bit; neither may collide with the cleared stencil.
    for(unsigned id:{0u,22u,63u}) {
        setup();rectangle(-.9f,-.9f,.9f,.9f,-4,(id<<24)|0x001f10c0,0x7c00);
        rectangle(-.6f,-.6f,.2f,.6f,-2,(id<<24)|0x000f10c0,0x001f);
        rectangle(0,-.6f,.6f,.6f,-3,(id<<24)|0x000f10c0,0x03e0);read();
        CHECK(sample(.1f)==0xff800080); // Matching opaque ID allows first blend.
        CHECK(sample(.4f)==0xff008080); // Uncovered pixels of same object draw.
        setup();floor();rectangle(-.6f,-.6f,.6f,.6f,-4,(id<<24)|0x000f10c0,0x001f);
        rectangle(-.6f,-.6f,.6f,.6f,-2,(id<<24)|0x000f10c0,0x03e0);read();
        CHECK(sample(.1f)==0xff80ff80); // Depth failure did not mark the ID.
    }
    for(unsigned alpha:{1u,15u,28u,30u}) {
        setup();floor();rectangle(-.6f,-.6f,.6f,.6f,-2,0x160010c0|(alpha<<16),0x001f);read();
        const auto once=sample(.1f);
        rectangle(-.6f,-.6f,.6f,.6f,-1.5f,0x160010c0|(alpha<<16),0x03e0);read();
        CHECK(sample(.1f)==once); // Repeat IDs are suppressed even when nearer.
    }
    // Partial texture alpha is split into opaque/translucent pixel passes.
    setup();floor();const uint32_t half_red=0x80ff0000;
    ntr::gx_bind_texture(&half_red,1,1);
    rectangle(-.6f,-.6f,.6f,.6f,-2,0x031f10c0,0x7fff);read();
    CHECK(sample(.4f)==0xffff7878);
    const auto half_once=sample(.4f);
    rectangle(-.6f,-.6f,.6f,.6f,-1.5f,0x031f10c0,0x7fff);read();
    CHECK(sample(.4f)==half_once); // Texel alpha also participates in ID rejection.
    // The same volume rules must work at actual native 4K resolution.
    s.width=3840;s.height=2160;setup();floor();volume();player();read();
    CHECK(sample(0)==0xffff0000 && sample(.4f)==0xff808080);
    s.width=256;s.height=192;
    // All hardware address flags, both axes, negative coordinates and boundaries.
    // Expected tables are independent examples for a two-texel axis.
    s.width=256;s.height=192;s.minimap=false;
    const uint32_t address_pixels[]={0xffff0000,0xff00ff00,0xff0000ff,0xffffffff};
    const float coords[]={-2.25f,-1.25f,-0.25f,0.25f,1.25f,2.25f,3.25f,4.25f};
    const int expected[3][8]={{0,0,0,0,1,1,1,1},{1,0,1,0,1,0,1,0},{1,1,0,0,1,1,0,0}};
    for(unsigned flags=0;flags<16;++flags)for(int i=0;i<8;++i) {
        setup();ntr::gx_teximage_param(flags<<16);
        ntr::gx_bind_texture(address_pixels,2,2);
        ntr::gx_write_port(0x04000500,0);
        v(-0.9f,-0.6f,-2,coords[i],coords[7-i]);
        v(0.8f,-0.6f,-2,coords[i],coords[7-i]);
        v(0.1f,0.9f,-2,coords[i],coords[7-i]);
        ntr::gx_write_port(0x04000504,0);
        const int sx=!(flags&1)?0:flags&4?2:1,sy=!(flags&2)?0:flags&8?2:1;
        const auto wanted=address_pixels[expected[sy][7-i]*2+expected[sx][i]];
        auto cpu=base;ntr::gx_render(cpu);
        CHECK(cpu.px[100][128]==wanted);
        CHECK(ntr::gpu_render(base,nullptr,s,true));CHECK(ntr::gpu_readback(pixels,width,height));
        CHECK(pixels[100*256+128]==wanted);
    }
    // Original toon uploads are raw halfword copies, not geometry commands.
    // Test both table endpoints and ensure green/blue vertex channels are ignored.
    auto *toon=reinterpret_cast<unsigned short*>(0x04000380);
    std::fill(toon,toon+32,0);toon[0]=0x001f;toon[31]=0x7c00;
    const uint32_t gray=0xff404040;
    for(unsigned hi=0;hi<2;++hi)for(unsigned red=0;red<2;++red) {
        setup(0x001f10e0);ntr::io_write(0x04000060,hi?2:0,2);
        ntr::gx_bind_texture(&gray,1,1);
        ntr::gx_write_port(0x04000480,red?0x001f:0x03e0);triangle();
        const uint32_t wanted=hi?(red?0xff4040ff:0xffff0000):(red?0xff000040:0xff400000);
        auto cpu=base;ntr::gx_render(cpu);CHECK(cpu.px[100][128]==wanted);
        CHECK(ntr::gpu_render(base,nullptr,s,true));CHECK(ntr::gpu_readback(pixels,width,height));
        CHECK(pixels[100*256+128]==wanted);
    }
    // Per-frame palette changes must affect the next render of the same geometry.
    ntr::io_write(0x04000060,0,2);toon[31]=0x03e0;
    CHECK(ntr::gpu_render(base,nullptr,s,true));CHECK(ntr::gpu_readback(pixels,width,height));
    CHECK(pixels[100*256+128]==0xff004000);
    // A red18->19 gradient expands to DS interpolation values303->319.
    // Its palette boundary is x=72, not the midpoint produced by round(red*31).
    setup(0x001f10e0);ntr::io_write(0x04000060,0,2);
    std::fill(toon,toon+32,0);toon[18]=0x001f;toon[19]=0x03e0;
    ntr::gx_write_port(0x04000500,1);
    ntr::gx_write_port(0x04000480,18);v(-1,-1,-2);
    ntr::gx_write_port(0x04000480,19);v(1,-1,-2);v(1,1,-2);
    ntr::gx_write_port(0x04000480,18);v(-1,1,-2);
    ntr::gx_write_port(0x04000504,0);
    reference=base;ntr::gx_render(reference);
    CHECK(reference.px[96][70]==0xffff0000 && reference.px[96][74]==0xff00ff00);
    CHECK(ntr::gpu_render(base,nullptr,s,true));CHECK(ntr::gpu_readback(pixels,width,height));
    CHECK(pixels[96*256+70]==0xffff0000 && pixels[96*256+74]==0xff00ff00);
    // BG0 wins equal priorities; disabled/behind layers cannot obscure 3D.
    ntr::io_write(0x04000000,0x00010f08,4);
    ntr::io_write(0x04000008,2,2);ntr::io_write(0x0400000a,1,2);
    ntr::io_write(0x0400000c,2,2);ntr::io_write(0x0400000e,3,2);
    CHECK(ntr::ppu_backgrounds_above_3d()==2);
    ntr::io_write(0x04000000,0x00010d08,4);CHECK(ntr::ppu_backgrounds_above_3d()==0);
    ntr::io_write(0x04000000,0x00010f00,4);CHECK(ntr::ppu_backgrounds_above_3d()==0);
    ntr::io_write(0x04000000,0x00010f88,4);CHECK(ntr::ppu_backgrounds_above_3d()==0);
    std::fill(&reference.px[0][0],&reference.px[0][0]+256*192,0);
    reference.px[96][70]=0xff123456;
    CHECK(ntr::gpu_render(base,nullptr,s,true,&reference.px[0][0]));
    CHECK(ntr::gpu_readback(pixels,width,height));
    CHECK(pixels[96*256+70]==0xff123456 && pixels[96*256+74]==0xff00ff00);
    CHECK(ntr::gpu_render(base,nullptr,s,false,&reference.px[0][0]));
    CHECK(ntr::gpu_readback(pixels,width,height));CHECK(pixels[96*256+70]==0xff101820);
    // Pause suppresses 3D. Map alpha/layout is independent of rendering size.
    uint32_t map[256*192];std::fill(map,map+256*192,0x80ff0000);
    s.minimap=true;s.minimap_scale=250;s.minimap_x=500;s.minimap_y=500;
    CHECK(ntr::gpu_render(base,map,s,false));CHECK(ntr::gpu_readback(pixels,width,height));
    CHECK(ntr::gpu_triangle_count()==0 && pixels[100*256+140]!=0xff101820 && pixels[10]==0xff101820);
    base.px[0][0]=0x005aef18;
    CHECK(ntr::gpu_render(base,nullptr,s,false));CHECK(ntr::gpu_readback(pixels,width,height));
    CHECK(pixels[0]==0xff5aef18); // Paused main view retains transparent-backdrop RGB.
    s.width=1920;s.height=1080;CHECK(ntr::gpu_render(base,nullptr,s,false));
    s.width=3840;s.height=2160;CHECK(ntr::gpu_render(base,nullptr,s,false));
    // Each physical screen remains 4:3 in its 75/25 slot, including when
    // POWCNT1 routes 3D onto the touch screen. Check GPU against CPU composition.
    static ntr::Framebuffer sub; // Keep this extra 192 KiB out of the x86 test stack.
    for(auto &row:base.px)for(auto &p:row)p=0xff102030;
    for(auto &row:sub.px)for(auto &p:row)p=0xff506070;
    s.width=1280;s.height=720;s.upscale_2d=true;
    for(unsigned mode=0;mode<3;++mode) for(bool main_top:{true,false}) {
        const auto screens=ntr::screen_layout(static_cast<ntr::MinigameLayout>(mode),s.width,s.height);
        CHECK(ntr::gpu_render(base,nullptr,s,false,nullptr,&screens,&sub,main_top));
        CHECK(ntr::gpu_readback(pixels,width,height));
        std::vector<uint32_t> expected;
        ntr::screen_compose(main_top?base:sub,main_top?sub:base,screens,width,height,expected);
        CHECK(pixels==expected);
        setup();triangle();
        CHECK(ntr::gpu_render(base,nullptr,s,true,nullptr,&screens,&sub,main_top));
        CHECK(ntr::gpu_readback(pixels,width,height));
        const auto &main=main_top?screens.top:screens.bottom;
        const auto &other=main_top?screens.bottom:screens.top;
        CHECK(pixels[size_t(main.y+main.height/2)*width+main.x+main.width/2]==0xffffffff);
        CHECK(pixels[size_t(other.y+other.height/2)*width+other.x+other.width/2]==0xff506070);
    }
    ntr::gpu_close();
    if(failures) return 1;
    std::printf("PASS: 4K raster pixels (mixed DS blocks=%u), software parity, UV/depth/cutouts, clipping/culling, viewport, swap, shadow mask/depth-fail/caster IDs/overlap/reset, alpha/depth writes, pause, map and resize\n",mixed);
}

