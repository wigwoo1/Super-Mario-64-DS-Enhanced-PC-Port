#pragma once
#include "ntr/input.h"

namespace ntr {
// SDL's HID drivers support Bluetooth/USB Nintendo controllers. Output uses
// the existing Xbox-shaped state, with Nintendo face buttons matched by label.
bool sdl_gamepad_poll(InputState &state);
void sdl_gamepad_shutdown();
}
