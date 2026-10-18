#include "ntr/mmio.h"
#include "ntr/ppu.h"
#include "ntr/rt.h"
#include <cstdio>
#include <cstring>
#include <initializer_list>

namespace {
constexpr uint32_t red=0xffff0000, green=0xff00ff00, blue=0xff0000ff, black=0xff000000;
static ntr::Framebuffer frame;
void wr16(uint32_t a,uint16_t v) { NTR_MMIO(uint16_t,a)=v; }
void wr32(uint32_t a,uint32_t v) { NTR_MMIO(uint32_t,a)=v; }
void setup(ntr::Engine engine) {
    const uint32_t reg=engine==ntr::ENGINE_A?0x04000000:0x04001000;
    const uint32_t vram=engine==ntr::ENGINE_A?0x06000000:0x06200000;
    const uint32_t pal=engine==ntr::ENGINE_A?0x05000000:0x05000400;
    const uint32_t obj=engine==ntr::ENGINE_A?0x06400000:0x06600000;
    const uint32_t oam=engine==ntr::ENGINE_A?0x07000000:0x07000400;
    std::memset(reinterpret_cast<void*>(reg),0,0x60);
    std::memset(reinterpret_cast<void*>(vram),0,0x10000);
    wr16(pal,0);wr16(pal+2,31);wr16(pal+4,31<<5);wr16(pal+0x202,31<<10);
    std::memset(reinterpret_cast<void*>(vram+32),0x11,32);
    std::memset(reinterpret_cast<void*>(vram+0x4000+32),0x22,32);
    for(int i=0;i<1024;++i) {wr16(vram+0x8000+i*2,1);wr16(vram+0x8800+i*2,1);}
    wr16(reg+8,0x1000);wr16(reg+10,0x1105); // BG0 red priority0; BG1 green priority1
    wr32(reg,0x11310); // display on, BG0/1, OBJ, 1D
    for(int i=0;i<128;++i) wr16(oam+i*8,0x200);
    std::memset(reinterpret_cast<void*>(obj),0x11,32);
    ntr::ppu_begin_window_frame();
}
bool pixel(int x,int y,uint32_t expected,const char* label) {
    if(frame.px[y][x]==expected) return true;
    std::fprintf(stderr,"%s at %d,%d: %08x expected %08x\n",label,x,y,frame.px[y][x],expected);
    return false;
}
unsigned irq_calls=0;bool irq_state_ok=true;
void hblank() {
    const unsigned line=NTR_MMIO(uint16_t,0x04000006);
    irq_state_ok &= (NTR_MMIO(uint16_t,0x04000004)&2)!=0;
    ++irq_calls;
    // Alternate red/green windows; writes belong to the NEXT line.
    wr16(0x04000048,line%2?1:2);
}
void game() {
    wr16(0x04000004,0x10);
    wr32(0x04000210,ntr::IRQ_VBLANK|ntr::IRQ_HBLANK);
    ntr::rt_irq_set_handler(ntr::IRQ_HBLANK,hblank);
    ntr::rt_vblank_wait();
}
}
int main() {
    if(!ntr::io_init())return 2;
    for(auto engine:{ntr::ENGINE_A,ntr::ENGINE_B}) {
        setup(engine);
        const uint32_t reg=engine==ntr::ENGINE_A?0x04000000:0x04001000;
        const uint32_t oam=engine==ntr::ENGINE_A?0x07000000:0x07000400;
        const uint32_t obj=engine==ntr::ENGINE_A?0x06400000:0x06600000;
        ntr::ppu_scanout(engine,frame);
        if(!pixel(30,30,red,"no windows"))return 3;
        wr32(reg,0x17310); // WIN0 + WIN1
        wr16(reg+0x40,0x1020);wr16(reg+0x44,0x1020);
        wr16(reg+0x42,0x0828);wr16(reg+0x46,0x0828);
        wr16(reg+0x48,0x0102);wr16(reg+0x4a,0);
        ntr::ppu_scanout(engine,frame);
        if(!pixel(16,16,green,"WIN0 beats WIN1") || !pixel(8,8,red,"WIN1") ||
           !pixel(32,16,red,"exclusive WIN0 end") || !pixel(40,40,black,"outside"))return 4;
        wr32(reg,0x13310);wr16(reg+0x40,0xf008);wr16(reg+0x44,0xb008);
        ntr::ppu_scanout(engine,frame);
        if(!pixel(0,0,green,"wrapped window") || !pixel(250,180,green,"wrapped far corner") ||
           !pixel(8,0,black,"wrapped exclusive end"))return 5;
        wr16(reg+0x40,0x1010);
        ntr::ppu_scanout(engine,frame);
        if(!pixel(16,0,black,"equal bounds empty"))return 6;
        // A mode2 sprite contributes coverage from nonzero texels, never color.
        wr32(reg,0x19310);wr16(reg+0x4a,0x0201);
        wr16(oam,0x808);wr16(oam+2,8);wr16(oam+4,0);
        for(int y=0;y<8;++y) std::memset(reinterpret_cast<void*>(obj+y*4+2),0,2);
        ntr::ppu_scanout(engine,frame);ntr::ppu_scanout_obj(engine,frame);
        if(!pixel(8,8,green,"OBJ window") || !pixel(12,8,red,"transparent OBJ hole"))return 7;
        // Rectangular windows have priority over the OBJ mask.
        wr32(reg,0x1b310);wr16(reg+0x40,0x080c);wr16(reg+0x44,0x0810);wr16(reg+0x48,0);
        ntr::ppu_scanout(engine,frame);
        if(!pixel(8,8,black,"WIN0 beats OBJ"))return 8;
        // Normal OBJ visibility and BG/OBJ priority are independently gated.
        wr32(reg,0x13310);wr16(reg+0x4a,0x13);wr16(reg+0x48,0x13);
        wr16(oam,8);wr16(oam+4,0x400);
        ntr::ppu_scanout(engine,frame);ntr::ppu_scanout_obj(engine,frame);
        if(!pixel(8,8,red,"higher BG priority"))return 9;
        wr16(oam+4,0);
        ntr::ppu_scanout(engine,frame);ntr::ppu_scanout_obj(engine,frame);
        if(!pixel(8,8,blue,"OBJ wins priority tie"))return 10;
        wr16(reg+0x48,3);
        ntr::ppu_scanout(engine,frame);ntr::ppu_scanout_obj(engine,frame);
        if(!pixel(8,8,red,"window hides normal OBJ"))return 11;
        // A captured horizontal boundary must not be replaced by the last line.
        wr16(reg+0x48,2);wr16(reg+0x4a,1);
        ntr::ppu_begin_window_frame();
        wr16(reg+0x40,0x0810);ntr::ppu_capture_window_line(8);
        wr16(reg+0x40,0x1820);ntr::ppu_capture_window_line(9);
        ntr::ppu_scanout(engine,frame);
        if(!pixel(8,8,green,"line snapshot") || !pixel(8,9,red,"next snapshot"))return 12;
    }
    setup(ntr::ENGINE_A);wr32(0x04000000,0x12310);
    wr16(0x04000040,0x00ff);wr16(0x04000044,0x00c0);wr16(0x04000048,1);
    ntr::rt_run(game,nullptr,1);
    ntr::ppu_scanout(ntr::ENGINE_A,frame);
    if(!irq_state_ok || irq_calls!=263 || !pixel(20,0,red,"HBlank starts after line0") ||
       !pixel(20,1,green,"HBlank updates next line") || !pixel(20,2,red,"HBlank alternating"))return 13;
    std::puts("PASS: both window engines, priority, transparent OBJ masks, scanline capture and HBlank delivery");
    return 0;
}
