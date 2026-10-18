#include "hal/wanted_settings_surface.h"
#include "ntr/mmio.h"
#include "ntr/ppu.h"
#include <array>
#include <cstdio>
#include <cstring>
static pc_pause::WantedSurface surface;
static ntr::Framebuffer frame;
int main() {
    if(!ntr::io_init()) return 1;
    std::array<unsigned char,0x20000> old_vram;
    std::array<unsigned char,0x200> old_palette;
    std::array<unsigned char,0x70> old_regs;
    for(unsigned i=0;i<old_vram.size();++i)old_vram[i]=static_cast<unsigned char>(i*17+3);
    old_palette.fill(0xa5);old_regs.fill(0x7c);
    std::memcpy(reinterpret_cast<void *>(0x06200000),old_vram.data(),old_vram.size());
    std::memcpy(reinterpret_cast<void *>(0x05000400),old_palette.data(),old_palette.size());
    std::memcpy(reinterpret_cast<void *>(0x04001000),old_regs.data(),old_regs.size());
    std::array<unsigned char,0x4000> font; font.fill(0x11);
    surface.open(font.data());
    pc_pause::Menu menu; ntr::VideoSettings settings;
    surface.draw(menu,settings);
    ntr::ppu_begin_window_frame();ntr::ppu_scanout(ntr::ENGINE_B,frame);
    // Text is visible over its selected panel; the backdrop stays black.
    if(frame.px[40][80]!=0xffffffff || frame.px[0][0]!=0xff000000) return 2;
    if(frame.px[32][0]==0xff000000) return 3;
    menu.page=pc_pause::Page::Screens;surface.draw(menu,settings);
    surface.close();
    if(std::memcmp(reinterpret_cast<void *>(0x06200000),old_vram.data(),old_vram.size()) ||
       std::memcmp(reinterpret_cast<void *>(0x05000400),old_palette.data(),old_palette.size()) ||
       std::memcmp(reinterpret_cast<void *>(0x04001000),old_regs.data(),old_regs.size())) return 4;
    surface.close(); // repeated close must preserve the restored state
    std::puts("Wanted settings renders text/panels and restores VRAM, palette, registers");
    return 0;
}
