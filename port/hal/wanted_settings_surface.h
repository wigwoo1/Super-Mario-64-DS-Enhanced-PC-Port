#pragma once
#include "hal/pc_pause_settings_state.h"
#include <array>

namespace pc_pause {
// A private tiled screen for Wanted's sprite-based pause menu. All borrowed
// engine-B memory and registers are restored before its original UI resumes.
class WantedSurface {
    std::array<unsigned char,0x20000> vram{};
    std::array<unsigned char,0x200> palette{};
    std::array<unsigned char,0x70> registers{};
    std::array<unsigned char,0x4000> font{};
    bool opened=false;
public:
    void open(const void *source_font);
    void draw(const Menu &, const ntr::VideoSettings &);
    void close();
};
}
