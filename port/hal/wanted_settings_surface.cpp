#include "hal/wanted_settings_surface.h"
#include <algorithm>
#include <cstring>

namespace pc_pause {
void WantedSurface::open(const void *source_font) {
    if(opened) return;
    std::memcpy(font.data(),source_font,font.size());
    std::memcpy(vram.data(),reinterpret_cast<void *>(0x06200000),vram.size());
    std::memcpy(palette.data(),reinterpret_cast<void *>(0x05000400),palette.size());
    std::memcpy(registers.data(),reinterpret_cast<void *>(0x04001000),registers.size());
    opened=true;
}
void WantedSurface::draw(const Menu &menu,const ntr::VideoSettings &settings) {
    if(!opened) return;
    auto *chars=reinterpret_cast<unsigned char *>(0x06200000);
    auto *text=reinterpret_cast<uint16_t *>(0x06208000);
    auto *panels=reinterpret_cast<uint16_t *>(0x06208800);
    // 4-bit text: glyphs at 4000..5fff; maps at 8000/8800;
    // panel tiles at c000. These regions cannot overlap.
    *reinterpret_cast<volatile uint32_t *>(0x04001000)=0x00010300;
    *reinterpret_cast<volatile uint16_t *>(0x04001008)=0x1000;
    *reinterpret_cast<volatile uint16_t *>(0x0400100a)=0x110d;
    std::memset(reinterpret_cast<void *>(0x04001010),0,8); // scroll
    std::memset(reinterpret_cast<void *>(0x04001050),0,6); // blend
    *reinterpret_cast<volatile uint16_t *>(0x0400106c)=0;
    auto *pal=reinterpret_cast<uint16_t *>(0x05000400);
    pal[0]=0;
    std::fill(pal+1,pal+16,0x7fff); // original font ink, all nonzero indices
    for(unsigned bank=1;bank<=2;++bank) {
        pal[bank*16]=0;
        pal[bank*16+1]=0x02df; // gold border
        pal[bank*16+2]=bank==1?0x2884:0x4968;
    }
    auto *tiles=chars+0xc000;
    std::memset(tiles,0,10*32);
    for(unsigned tile=0;tile<9;++tile) for(unsigned y=0;y<8;++y) for(unsigned x=0;x<8;++x) {
        const bool border=(tile%3==0 && x==0)||(tile%3==2 && x==7)||
                          (tile/3==0 && y==0)||(tile/3==2 && y==7);
        tiles[tile*32+y*4+x/2]|=(border?1:2)<<((x&1)*4);
    }
    uint16_t panel[128];
    for(unsigned y=0;y<4;++y) for(unsigned x=0;x<32;++x)
        panel[y*32+x]=(y==0?0:y==3?6:3)+(x==0?0:x==31?2:1);
    pc_pause::draw(menu,settings,chars,text,panels,font.data(),panel,9,true);
}
void WantedSurface::close() {
    if(!opened) return;
    std::memcpy(reinterpret_cast<void *>(0x06200000),vram.data(),vram.size());
    std::memcpy(reinterpret_cast<void *>(0x05000400),palette.data(),palette.size());
    std::memcpy(reinterpret_cast<void *>(0x04001000),registers.data(),registers.size());
    opened=false;
}
}
