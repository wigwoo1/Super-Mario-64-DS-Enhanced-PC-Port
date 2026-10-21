// Native keyboard, Xbox-compatible controller, and mouse stylus input.
#ifndef NTR_INPUT_H
#define NTR_INPUT_H

#include <stdint.h>

namespace ntr {

struct InputState {
    uint16_t buttons = 0;  // XInput button mask
    int16_t left_x = 0;    // -32768..32767
    int16_t left_y = 0;
    int16_t right_x = 0;
    int16_t right_y = 0;
    uint8_t left_trigger = 0;
    uint8_t right_trigger = 0;
    bool camera_zoom_pressed = false; // rising edge of Xbox right-stick click
    bool camera_mode_pressed = false; // rising edge of Xbox left-stick click
    bool touch = false;
    uint16_t touch_x = 0;  // 0..255 in bottom-screen coordinates
    uint16_t touch_y = 0;  // 0..191
};

void input_poll();
// Isolated adaptive replays send the same raw DS buttons/stick as schedules.
// Accepted only with both AUTOMATION_ONLY=1 and TEST_BOSS_BOT=1.
void input_test_controls(bool enabled, uint16_t buttons, int16_t x, int16_t y);
InputState input_state();
// Retain a host click across the scene's intervening VBlank(s), consume once.
bool input_take_camera_zoom();
// Retain a gameplay camera-mode click across the scene's intervening VBlank(s).
bool input_take_camera_mode();

// Set the mouse stylus position in bottom-screen coordinates. The host also
// updates the DS touch-sample latch consumed by the original touch callback.
void input_set_touch(bool pressed, uint16_t x, uint16_t y);

// Emulated, active-low 12-button KEYINPUT register value.
uint16_t input_keyinput();

// Radially dead-zoned left stick in [-1, 1], with preserved partial travel.
// The C entry point lets the decomp's controller update consume the native
// stick without exposing XInput types to game code.
void input_left_stick(float *x, float *y);
// Pure conversion used by input_left_stick and host-side calibration checks.
void normalize_stick(int16_t raw_x, int16_t raw_y, float *x, float *y);

}  // namespace ntr

extern "C" void ntr_input_left_stick(float *x, float *y);
// Map active host travel into the retail touch joystick's usable speed range.
// Menu navigation and raw stick calibration keep their original normalization.
extern "C" void ntr_input_movement_stick(float *x, float *y, unsigned mode);
// Apply only to the local gameplay controller after the original button map.
// This never injects a Run press into touch menus or minigames.
extern "C" unsigned short ntr_input_run_buttons(unsigned short held, int analog);
// Title navigation converts stick directions into menu keys. Gameplay keeps
// those keys digital and consumes stick magnitude through the virtual stick.
extern "C" void ntr_input_get_menu_keys(unsigned short *keys);
extern "C" void ntr_input_set_menu_navigation(int enabled);
// Original six-byte sample layout: x, y, pressed. Used by the host copy of
// the game's four-player touch-history update for the local player only.
extern "C" void ntr_input_touch_sample(unsigned short *sample);

#endif
