#pragma once
#include "ntr/video.h"
#include <cstdint>
#include <cstddef>

namespace pc_pause {
enum class Page { Options, Display, Visual, Graphics, Map, Position, Save, Screens, Gameplay };
enum class Action { None, Changed, GameOptions, BackPause, SaveProgress };
uint16_t events(uint16_t keys, uint16_t &previous, uint16_t &repeat, unsigned &ticks);
struct Menu {
    Page page = Page::Options;
    unsigned row = 0;
    uint16_t previous = 0, repeat = 0;
    unsigned ticks = 0;
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
