#include "ntr/mmio.h"
#include "ntr/ppu.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
int main() {
    if (!ntr::io_init()) return 2;
    ntr::Framebuffer frame;
    auto scan = [&] { ntr::ppu_scanout(ntr::ENGINE_B, frame); };
    auto scan_transparent = [&] { ntr::ppu_scanout(ntr::ENGINE_B, frame, true); };
    auto word = [](unsigned address, unsigned short value) { NTR_MMIO(uint16_t,address)=value; };
    // Real MAP mode: sub BG3, extended affine tiles, char base 4,
    // screen block 31. The 128-pixel map must be sampled linearly.
    NTR_MMIO(uint32_t,0x04001000)=0x00010803;
    word(0x0400100e,0x1f10);
    word(0x04001030,256);word(0x04001032,0);
    word(0x04001034,0);word(0x04001036,256);
    NTR_MMIO(uint32_t,0x04001038)=0;
    NTR_MMIO(uint32_t,0x0400103c)=0;
    word(0x05000400,0);word(0x05000402,31);word(0x05000404,31<<5);
    std::memset((void*)0x06210040,1,64);
    std::memset((void*)0x06210080,2,64);
    std::memset((void*)0x0620f800,0,512);
    word(0x0620f800,1);word(0x0620f802,2);
    scan();
    if(frame.px[0][0]!=0xffff0000 || frame.px[0][8]!=0xff00ff00 || frame.px[0][128]!=0xff000000) return 3;
    scan_transparent();
    if(frame.px[0][0]!=0xffff0000 || frame.px[0][8]!=0xff00ff00 || frame.px[0][128]!=0x00000000) return 10;
    // Filtering the touch-target BG2 must preserve the independent BG3 map;
    // filtering BG3 must leave only the transparent backdrop, without MMIO edits.
    ntr::ppu_scanout(ntr::ENGINE_B, frame, true, 0x0b);
    if(frame.px[0][0]!=0xffff0000 || frame.px[0][8]!=0xff00ff00) return 11;
    ntr::ppu_scanout(ntr::ENGINE_B, frame, true, 0x07);
    if(frame.px[0][0]!=0x00000000 || frame.px[0][8]!=0x00000000 ||
       NTR_MMIO(uint32_t,0x04001000)!=0x00010803) return 12;
    // An actual visible BG2 (blue) overlays BG3 (red/green). Excluding only
    // BG2 reveals the map below and leaves the source display state intact.
    word(0x0400100c, 0x1e80);
    word(0x05000406, 31<<10);
    std::memset((void*)0x06200040,3,64);
    for (unsigned i=0;i<1024;++i) word(0x0620f000+i*2,1);
    NTR_MMIO(uint32_t,0x04001000)=0x00010c03;
    scan_transparent();
    if(frame.px[0][0]!=0xff0000ff || frame.px[0][8]!=0xff0000ff) return 13;
    ntr::ppu_scanout(ntr::ENGINE_B, frame, true, 0x0b);
    if(frame.px[0][0]!=0xffff0000 || frame.px[0][8]!=0xff00ff00 ||
       NTR_MMIO(uint32_t,0x04001000)!=0x00010c03) return 14;
    NTR_MMIO(uint32_t,0x04001000)=0x00010803;
    NTR_MMIO(uint32_t,0x04001038)=8*256;
    scan();if(frame.px[0][0]!=0xff00ff00) return 4;
    NTR_MMIO(uint32_t,0x04001038)=0x0ffff800; // -8 pixels in signed 28-bit 20.8
    scan();if(frame.px[0][0]!=0xff000000 || frame.px[0][8]!=0xffff0000) return 5;
    word(0x0620f81e,2);word(0x0400100e,0x3f10); // wrap to last tile
    scan();if(frame.px[0][0]!=0xff00ff00) return 6;
    NTR_MMIO(uint32_t,0x04001038)=0;
    word(0x04001030,0);word(0x04001032,256);
    word(0x04001034,256);word(0x04001036,0); // exchange axes
    scan();if(frame.px[8][0]!=0xff00ff00) return 7;
    // Palette bank 1 in sub BG3's extended slot, distinct from standard red.
    NTR_MMIO(uint32_t,0x04001000)=0x40010803;
    word(0x0620f800,0x1001);
    word(0x0689e202,31<<10);
    scan();if(frame.px[0][0]!=0xff0000ff) return 9;
    // Legacy affine mode uses one-byte tile indices and 8-bit graphics.
    NTR_MMIO(uint32_t,0x04001000)=0x00010801;
    word(0x0400100e,0x1f10);
    *((volatile unsigned char*)0x0620f800)=1;
    scan();if(frame.px[0][0]!=0xffff0000) return 8;
    // Main BG0 is either tiled 2D or the separately rendered 3D layer.
    // Sub BG0 stays tiled even if the main-only selector bit is present.
    for (int engine=0;engine<2;++engine) {
        const unsigned reg=0x04000000+engine*0x1000;
        const unsigned vram=engine?0x06200000:0x06000000;
        const unsigned palette=0x05000000+engine*0x400;
        std::memset((void*)vram,0,0x10000);
        std::memset((void*)(reg+8),0,0x40);
        word(reg+8,0x1f80);
        word(palette,0);word(palette+2,31);
        std::memset((void*)(vram+64),1,64);
        for(unsigned i=0;i<1024;++i)word(vram+0xf800+i*2,1);
        NTR_MMIO(uint32_t,reg)=0x10100;
        ntr::ppu_scanout(static_cast<ntr::Engine>(engine),frame);
        if(frame.px[0][0]!=0xffff0000)return 15;
        NTR_MMIO(uint32_t,reg)=0x10108;
        ntr::ppu_scanout(static_cast<ntr::Engine>(engine),frame);
        if(frame.px[0][0]!=(engine?0xffff0000:0xff000000))return 16;
        ntr::ppu_scanout(static_cast<ntr::Engine>(engine),frame,true);
        if(frame.px[0][0]!=(engine?0xffff0000:0))return 17;
        NTR_MMIO(uint32_t,reg)=0x10100;
        ntr::ppu_scanout(static_cast<ntr::Engine>(engine),frame);
        if(frame.px[0][0]!=0xffff0000)return 18;
    }
    // Secret Slide requests an opaque black 3D clear over a green palette
    // backdrop. Apply it at BG0's priority, without changing tiled BG0,
    // transparent clears, foreground-only captures, windows or the sub engine.
    std::memset((void*)0x04000008,0,0x54);
    word(0x05000000,31<<5);word(0x05000002,31);
    word(0x04000008,2);word(0x0400000a,0x1f83);
    NTR_MMIO(uint32_t,0x04000000)=0x10308;
    NTR_MMIO(uint32_t,0x04000350)=0x3f1f0000;
    ntr::ppu_scanout(ntr::ENGINE_A,frame);
    if(frame.px[0][0]!=0xff000000)return 22;
    // A genuinely higher-priority BG1 still covers the rear-plane.
    word(0x0400000a,0x1f81);
    ntr::ppu_scanout(ntr::ENGINE_A,frame);
    if(frame.px[0][0]!=0xffff0000)return 23;
    // Foreground-only capture must not paint black over rendered geometry.
    word(0x0400000a,0x1f83);
    ntr::ppu_scanout(ntr::ENGINE_A,frame,true,0x0e);
    if(frame.px[0][0]!=0xffff0000)return 24;
    NTR_MMIO(uint32_t,0x04000000)=0x10108;
    ntr::ppu_scanout(ntr::ENGINE_A,frame,true,0x0e);
    if(frame.px[0][0]!=0x0000ff00)return 25;
    NTR_MMIO(uint32_t,0x04000350)=0x3f000000;
    ntr::ppu_scanout(ntr::ENGINE_A,frame);
    if(frame.px[0][0]!=0xff00ff00)return 26;
    // The clear color is a register, not a hard-coded black fallback.
    NTR_MMIO(uint32_t,0x04000350)=0x3f1f7c00;
    ntr::ppu_scanout(ntr::ENGINE_A,frame);
    if(frame.px[0][0]!=0xff0000ff)return 27;
    ntr::ppu_scanout(ntr::ENGINE_B,frame);
    if(frame.px[0][0]!=0xffff0000)return 31;
    NTR_MMIO(uint32_t,0x04000000)=0x10008; // BG0 disabled
    ntr::ppu_scanout(ntr::ENGINE_A,frame);
    if(frame.px[0][0]!=0xff00ff00)return 32;
    NTR_MMIO(uint32_t,0x04000000)=0x10100; // tiled BG0
    word(0x04000008,0x1f80);
    ntr::ppu_scanout(ntr::ENGINE_A,frame);
    if(frame.px[0][0]!=0xffff0000)return 33;
    NTR_MMIO(uint32_t,0x04000000)=0x10108;
    word(0x04000008,2);
    word(0x04000060,1u<<14); // a bitmap rear-plane is not this constant
    ntr::ppu_scanout(ntr::ENGINE_A,frame);
    if(frame.px[0][0]!=0xff00ff00)return 34;
    word(0x04000060,0);
    word(0x04000010,256);
    ntr::ppu_scanout(ntr::ENGINE_A,frame);
    if(frame.px[0][0]!=0xff00ff00)return 28;
    word(0x04000010,0);
    NTR_MMIO(uint32_t,0x04000000)=0x12108;
    word(0x04000040,0x1020);word(0x04000044,0x1020);
    word(0x04000048,1);word(0x0400004a,0);
    ntr::ppu_begin_window_frame();
    ntr::ppu_scanout(ntr::ENGINE_A,frame);
    if(frame.px[16][16]!=0xff0000ff || frame.px[0][0]!=0xff00ff00)return 29;
    NTR_MMIO(uint32_t,0x04000000)=0x10188;
    ntr::ppu_scanout(ntr::ENGINE_A,frame);
    if(frame.px[0][0]!=0xffffffff)return 30;
    NTR_MMIO(uint32_t,0x04000350)=0; // restore for subsequent 2D tests
    std::memset((void*)0x04000040,0,0xc);
    ntr::ppu_begin_window_frame();
    // Wanted uses BG2/BG3 with BG0 disabled. Diagnostics must never read a
    // zero-initialized disabled layer's screen/character addresses.
#if defined(_WIN32)
    _putenv_s("SM64DS_PC_TRACE","1");
#else
    setenv("SM64DS_PC_TRACE","1",1);
#endif
    NTR_MMIO(uint32_t,0x04000000)=0x10400;
    word(0x0400000c,0x1f80);
    ntr::ppu_scanout(ntr::ENGINE_A,frame);
    if(frame.px[0][0]!=0xffff0000) return 19;
    if(ntr::ppu_geometry_active()) return 20;
    const unsigned controls[]={0x10108u,0x10008u,0x10100u,0x10188u,0x00108u,0x20108u,0x30108u};
    for(unsigned control : controls) {
        NTR_MMIO(uint32_t,0x04000000)=control;
        if(ntr::ppu_geometry_active() != (control==0x10108u)) return 21;
    }
    std::puts("PASS: affine tiles, BG0 2D/3D selection and disabled-BG0 trace");
}
