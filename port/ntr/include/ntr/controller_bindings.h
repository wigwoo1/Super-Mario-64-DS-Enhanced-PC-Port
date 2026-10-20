#pragma once
#include <cstdint>

namespace ntr {
// XInput A/B -> DS logical buttons. Menus retain A confirm / B cancel;
// ordinary gameplay uses DS B for jump and DS A for attack.
inline uint16_t controller_face_buttons(uint16_t buttons, bool menu) {
    const uint16_t a = buttons & 0x1000 ? (menu ? 1 : 2) : 0;
    const uint16_t b = buttons & 0x2000 ? (menu ? 2 : 1) : 0;
    const uint16_t x = buttons & 0x4000 ? (menu ? 0x400 : 0x800) : 0;
    const uint16_t y = buttons & 0x8000 ? (menu ? 0x800 : 0x400) : 0;
    return static_cast<uint16_t>(a | b | x | y);
}
}
