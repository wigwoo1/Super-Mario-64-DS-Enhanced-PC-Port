#pragma once
#include "ntr/video.h"
#include <cstdint>
#include <cstddef>

namespace pc_pause {
enum class Page { Options, Display, Visual, Graphics, Map, Position, Save, Screens, Gameplay, Camera, Controls, Keyboard, Gamepad };
enum class Action { None, Changed, GameOptions, BackPause, SaveProgress };
// Shared pixel geometry for drawing and both stage/minigame touch handling.
constexpr unsigned row_top = 24, row_height = 24;
inline bool controller_modes_hit(unsigned state,unsigned x,unsigned y) {
    if(state!=1 && state!=3 && state!=13)return false;
    const unsigned top=(state==1?72:state==3?64:80);
    return x>=8 && x<248 && y>=top && y<top+32;
}
inline int hit_row(unsigned y, unsigned count) {
    return y >= row_top && y < row_top + count * row_height && (y-row_top)%row_height<16
        ? static_cast<int>((y-row_top)/row_height) : -1;
}
inline bool cache_button_panel(const uint16_t *map,unsigned offset,uint16_t *panel) {
    if(offset%32 || offset>1024-128)return false;
    for(unsigned i=0;i<128;++i)panel[i]=map[offset+i];
    return true;
}
uint16_t events(uint16_t keys, uint16_t &previous, uint16_t &repeat, unsigned &ticks);
// Copy while the title owns its font resource; never retain its heap pointer.
bool cache_options_font(const unsigned char *font, unsigned base_tile, const unsigned char *widths);
void clear_options_font();
bool options_font_ready();
struct Menu {
    Page page = Page::Options;
    unsigned row = 0;
    uint16_t previous = 0, repeat = 0;
    unsigned ticks = 0;
    unsigned binding_bank=0, capture=0, capture_ticks=0;
    bool reset_confirm=false;
    bool binding_page() const {return page==Page::Keyboard || page==Page::Gamepad;}
    Action binding_sample(uint32_t value,bool cancel,ntr::VideoSettings &settings);
    int save_status = 0; // 0 idle, 1 saved, -1 failed; never persisted as settings.
    void reset(uint16_t held);
    unsigned rows() const;
    Action update(uint16_t keys, ntr::VideoSettings &settings);
    void line(unsigned index, const ntr::VideoSettings &, char *out, size_t size) const;
};
// Draw with the native 8x16 font and a cached 32x4-tile game button panel.
void draw(const Menu &, const ntr::VideoSettings &, unsigned char *chars,
          uint16_t *text_map, uint16_t *panel_map, const unsigned char *font,
          const uint16_t *panel, uint16_t blank_panel, bool touch_navigation=false);
}
