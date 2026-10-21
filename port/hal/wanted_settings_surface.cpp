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
        pal[bank*16+1]=0x0842; // dark outer rim
        pal[bank*16+2]=bank==1?0x0259:0x5bff; // orange / selected cream
        pal[bank*16+3]=0x7bff; // bevel highlight
        pal[bank*16+4]=0x0150; // bevel shadow
        pal[bank*16+5]=0x02df; // gold inner edge
        pal[bank*16+6]=bank==1?pal[bank*16+2]:0x0150; // selected arrow
    }
    auto *tiles=chars+0xc000;
    std::memset(tiles,0,10*32);
    for(unsigned tile=0;tile<9;++tile) for(unsigned y=0;y<8;++y) for(unsigned x=0;x<8;++x) {
        const bool left=tile%3==0,right=tile%3==2,top=tile/3==0,bottom=tile/3==2;
        unsigned color=2;
        if((left&&x<3)||(right&&x>4)||(top&&y<3)||(bottom&&y>4))color=5;
        if((left&&x==1)||(top&&y==1))color=3;
        if((right&&x==6)||(bottom&&y==6))color=4;
        if((left&&x==0)||(right&&x==7)||(top&&y==0)||(bottom&&y==7))color=1;
        // The shared renderer keeps the top/bottom eight-pixel strips.
        const unsigned py=top?y:bottom?y+8:16;
        if(left && x>=3 && x<=6 && py<16 && unsigned(std::abs(int(py)-7))<=int(6-x))color=6;
        tiles[tile*32+y*4+x/2]|=color<<((x&1)*4);
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
