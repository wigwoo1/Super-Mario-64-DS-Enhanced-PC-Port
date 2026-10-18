#include "ntr/input.h"
#include "ntr/mmio.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Xinput.h>
#endif

namespace ntr {
namespace {

InputState g_state;
uint16_t g_keyinput = 0x0FFF;
bool g_touch = false;
uint16_t g_touch_x = 0, g_touch_y = 0;
unsigned long long g_input_poll_count = 0;
bool g_auto_confirm_logged = false;
bool g_menu_navigation = true;
bool g_test_controls=false;uint16_t g_test_buttons=0;int16_t g_test_x=0,g_test_y=0;

uint16_t scripted_buttons(unsigned long long poll, const char *schedule) {
    uint16_t result = 0;
    // Opt-in replay pulses: decimal poll : hexadecimal DS key mask, comma
    // separated. Each lasts five polls, matching the existing startup pulses.
    while (schedule && *schedule) {
        char *time_end = nullptr, *mask_end = nullptr;
        const auto start = std::strtoull(schedule, &time_end, 10);
        if (time_end != schedule && *time_end == ':') {
            const auto mask = std::strtoul(time_end + 1, &mask_end, 16);
            if (mask_end != time_end + 1 && (*mask_end == ',' || !*mask_end) &&
                start != 0 && mask <= 0x0fff && poll >= start && poll - start < 5)
                result |= static_cast<uint16_t>(mask);
        }
        const char *next = std::strchr(schedule, ',');
        schedule = next ? next + 1 : nullptr;
    }
    return result;
}

#if defined(_WIN32)
using XInputGetStateFn = DWORD(WINAPI *)(DWORD, XINPUT_STATE *);
HMODULE g_xinput = nullptr;
XInputGetStateFn g_xinput_get_state = nullptr;

void load_xinput() {
    if (g_xinput_get_state) return;
    const char *dlls[] = {"xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll"};
    for (const char *dll : dlls) {
        g_xinput = LoadLibraryA(dll);
        if (g_xinput) {
            g_xinput_get_state = reinterpret_cast<XInputGetStateFn>(
                GetProcAddress(g_xinput, "XInputGetState"));
            if (g_xinput_get_state) break;
            FreeLibrary(g_xinput);
            g_xinput = nullptr;
        }
    }
}

bool key_down(int key) { return (GetAsyncKeyState(key) & 0x8000) != 0; }

void write_touch_latch(bool pressed, uint16_t x, uint16_t y) {
    // ARM9 touch callback reads a 32-bit packed sample from DTCM 0x027FFFAA:
    // 12-bit raw X, 12-bit raw Y, pen-valid, and pressure state.
    const uint32_t raw_x = std::min<uint32_t>(4095, x * 4095u / 255u);
    const uint32_t raw_y = std::min<uint32_t>(4095, y * 4095u / 191u);
    const uint32_t packed = raw_x | (raw_y << 12) |
        (pressed ? (1u << 24) : 0u);
    io_write(0x027FFFAAu, static_cast<uint16_t>(packed), 2);
    io_write(0x027FFFACu, static_cast<uint16_t>(packed >> 16), 2);
}
#endif

}  // namespace

void input_poll() {
#if defined(_WIN32)
    ++g_input_poll_count;
    load_xinput();
    XINPUT_STATE pad{};
    if (g_xinput_get_state && g_xinput_get_state(0, &pad) == ERROR_SUCCESS) {
        g_state.buttons = pad.Gamepad.wButtons;
        g_state.left_x = pad.Gamepad.sThumbLX;
        g_state.left_y = pad.Gamepad.sThumbLY;
        g_state.right_x = pad.Gamepad.sThumbRX;
        g_state.right_y = pad.Gamepad.sThumbRY;
        g_state.left_trigger = pad.Gamepad.bLeftTrigger;
        g_state.right_trigger = pad.Gamepad.bRightTrigger;
    } else {
        g_state.buttons = 0;
        g_state.left_x = g_state.left_y = 0;
        g_state.right_x = g_state.right_y = 0;
        g_state.left_trigger = g_state.right_trigger = 0;
    }

    // Keep the left stick exclusively on the game's virtual joystick path.
    // The Circle Pad patch injects its magnitude and angle into that record;
    // mapping it to D-pad bits as well would mix digital movement with the
    // continuous direction and speed. Keyboard and physical D-pad remain
    // digital controls.
    uint16_t pressed = 0;
    const uint16_t buttons = g_state.buttons;
    if ((buttons & XINPUT_GAMEPAD_A) || key_down(VK_SPACE)) pressed |= 1u << 0;
    if ((buttons & XINPUT_GAMEPAD_B) || key_down(VK_LSHIFT)) pressed |= 1u << 1;
    if ((buttons & XINPUT_GAMEPAD_BACK) || key_down(VK_BACK)) pressed |= 1u << 2;
    if ((buttons & XINPUT_GAMEPAD_START) || key_down(VK_RETURN)) pressed |= 1u << 3;
    if ((buttons & XINPUT_GAMEPAD_DPAD_RIGHT) || key_down('D')) pressed |= 1u << 4;
    if ((buttons & XINPUT_GAMEPAD_DPAD_LEFT) || key_down('A')) pressed |= 1u << 5;
    if ((buttons & XINPUT_GAMEPAD_DPAD_UP) || key_down('W')) pressed |= 1u << 6;
    if ((buttons & XINPUT_GAMEPAD_DPAD_DOWN) || key_down('S')) pressed |= 1u << 7;
    if ((buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) || key_down('E')) pressed |= 1u << 8;
    if ((buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) || key_down('Q')) pressed |= 1u << 9;
    if ((buttons & XINPUT_GAMEPAD_X) || key_down('X')) pressed |= 1u << 10;
    if ((buttons & XINPUT_GAMEPAD_Y) || key_down('C')) pressed |= 1u << 11;

    // Optional host-side automation for unattended startup/gameplay tracing.
    // These variables are unset during normal play and do not alter the UI.
    const char *automation_only = std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if (automation_only && automation_only[0] == '1') {
        // Desktop typing, a connected pad, and mouse clicks must not change
        // the route taken by a scripted replay. Synthetic inputs below remain.
        g_state = {};
        pressed = 0;
        g_touch = false;
        g_touch_x = g_touch_y = 0;
        write_touch_latch(false, 0, 0);
    }
    // Replay taps use the same hardware latch as mouse input. Restrict this
    // test interface to isolated automation; each valid tap lasts five polls.
    if (automation_only && automation_only[0]=='1') {
        const char *tap=std::getenv("SM64DS_PC_TEST_TOUCH");
        while(tap && *tap) {
            char *time_end=nullptr,*x_end=nullptr,*y_end=nullptr;
            const auto start=std::strtoull(tap,&time_end,10);
            if(time_end!=tap && *time_end==':') {
                const auto x=std::strtoul(time_end+1,&x_end,10);
                if(x_end!=time_end+1 && *x_end==':') {
                    const auto y=std::strtoul(x_end+1,&y_end,10);
                    if(y_end!=x_end+1 && (!*y_end || *y_end==',') && start &&
                       x<=255 && y<=191 && g_input_poll_count>=start && g_input_poll_count-start<5)
                        input_set_touch(true,static_cast<uint16_t>(x),static_cast<uint16_t>(y));
                }
            }
            const char *next=std::strchr(tap,',');tap=next?next+1:nullptr;
        }
    }
    const char *when = std::getenv("SM64DS_PC_AUTOCONFIRM_POLL");
    if (when) {
        char *end = nullptr;
        const unsigned long long target = std::strtoull(when, &end, 10);
        const char *repeat_text = std::getenv("SM64DS_PC_AUTOCONFIRM_REPEAT");
        char *repeat_end = nullptr;
        const unsigned long long repeat = repeat_text
            ? std::strtoull(repeat_text, &repeat_end, 10) : 0;
        const unsigned long long elapsed = g_input_poll_count >= target
            ? g_input_poll_count - target : ~0ull;
        const bool valid_repeat = repeat_text && repeat_end != repeat_text && repeat >= 5;
        if (end != when && target != 0 && elapsed != ~0ull &&
            (valid_repeat ? elapsed % repeat < 5 : elapsed < 5)) {
            pressed |= 1u << 0;  // A / confirm
            const char *trace = std::getenv("SM64DS_PC_TRACE");
            if (!g_auto_confirm_logged && trace && trace[0] && trace[0] != '0') {
                std::fprintf(stderr, "[sm64ds] input automation: A held on polls %llu-%llu\n",
                    target, target + 4);
                std::fflush(stderr);
                g_auto_confirm_logged = true;
            }
        }
    }
    const char *down_when = std::getenv("SM64DS_PC_AUTODOWN_POLL");
    if (down_when) {
        char *down_end = nullptr;
        const unsigned long long target = std::strtoull(down_when, &down_end, 10);
        const unsigned long long elapsed = g_input_poll_count >= target
            ? g_input_poll_count - target : ~0ull;
        if (down_end != down_when && target != 0 && elapsed < 5) {
            pressed |= 1u << 7;  // D-pad down
            const char *trace = std::getenv("SM64DS_PC_TRACE");
            if (elapsed == 0 && trace && trace[0] && trace[0] != '0') {
                std::fprintf(stderr, "[sm64ds] input automation: Down held on polls %llu-%llu\n",
                    target, target + 4);
                std::fflush(stderr);
            }
        }
    }
    const char *test_x = std::getenv("SM64DS_PC_TEST_STICK_X");
    const char *test_y = std::getenv("SM64DS_PC_TEST_STICK_Y");
    const char *stick_start_text = std::getenv("SM64DS_PC_TEST_STICK_START_POLL");
    const char *stick_end_text = std::getenv("SM64DS_PC_TEST_STICK_END_POLL");
    const unsigned long long stick_start = stick_start_text
        ? std::strtoull(stick_start_text, nullptr, 10) : 0;
    const unsigned long long stick_end = stick_end_text
        ? std::strtoull(stick_end_text, nullptr, 10) : ~0ull;
    if ((test_x || test_y) && g_input_poll_count >= stick_start &&
        g_input_poll_count < stick_end) {
        char *end_x = nullptr, *end_y = nullptr;
        long x = test_x ? std::strtol(test_x, &end_x, 10) : 0;
        long y = test_y ? std::strtol(test_y, &end_y, 10) : 0;
        if ((!test_x || end_x != test_x) && (!test_y || end_y != test_y)) {
            x = std::max(-32768L, std::min(32767L, x));
            y = std::max(-32768L, std::min(32767L, y));
            g_state.left_x = static_cast<int16_t>(x);
            g_state.left_y = static_cast<int16_t>(y);
        }
    }
    // Optional multi-segment real-stick replay, restricted to isolated tests.
    // Decimal start:end:x:y records; last matching segment wins.
    if (automation_only && automation_only[0]=='1') {
        const char *segment=std::getenv("SM64DS_PC_TEST_STICK_SCHEDULE");
        while(segment && *segment) {
            unsigned long long begin=0,end=0;
            long x=0,y=0;int consumed=0;
            if(std::sscanf(segment,"%llu:%llu:%ld:%ld%n",&begin,&end,&x,&y,&consumed)==4 &&
               consumed>0 && (!segment[consumed] || segment[consumed]==',') &&
               begin>0 && end>begin && x>=-32768 && x<=32767 && y>=-32768 && y<=32767 &&
               g_input_poll_count>=begin && g_input_poll_count<end) {
                g_state.left_x=static_cast<int16_t>(x);
                g_state.left_y=static_cast<int16_t>(y);
            }
            const char *next=std::strchr(segment,',');segment=next?next+1:nullptr;
        }
    }
    pressed |= scripted_buttons(g_input_poll_count, std::getenv("SM64DS_PC_TEST_BUTTONS"));
    const char *bot_controls=std::getenv("SM64DS_PC_TEST_BOSS_BOT");
    const char *route_controls=std::getenv("SM64DS_PC_TEST_COURSE_ROUTE");
    if(g_test_controls && automation_only && std::strcmp(automation_only,"1")==0 &&
       ((bot_controls && std::strcmp(bot_controls,"1")==0) || (route_controls && *route_controls))) {
        pressed |= g_test_buttons;g_state.left_x=g_test_x;g_state.left_y=g_test_y;
    }
    g_keyinput = static_cast<uint16_t>((~pressed) & 0x0FFFu);

    g_state.touch = g_touch;
    g_state.touch_x = g_touch_x;
    g_state.touch_y = g_touch_y;
#endif
}

void input_test_controls(bool enabled,uint16_t buttons,int16_t x,int16_t y) {
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    const char *boss_bot=std::getenv("SM64DS_PC_TEST_BOSS_BOT");
    const char *course_route=std::getenv("SM64DS_PC_TEST_COURSE_ROUTE");
    g_test_controls=enabled && automation && std::strcmp(automation,"1")==0 &&
        ((boss_bot && std::strcmp(boss_bot,"1")==0) || (course_route && *course_route));
    g_test_buttons=buttons&0xfff;g_test_x=x;g_test_y=y;
}
InputState input_state() { return g_state; }

void input_set_touch(bool pressed, uint16_t x, uint16_t y) {
    g_touch = pressed;
    g_touch_x = std::min<uint16_t>(x, 255);
    g_touch_y = std::min<uint16_t>(y, 191);
#if defined(_WIN32)
    if (io_init()) write_touch_latch(g_touch, g_touch_x, g_touch_y);
#endif
    g_state.touch = g_touch;
    g_state.touch_x = g_touch_x;
    g_state.touch_y = g_touch_y;
}

uint16_t input_keyinput() { return g_keyinput; }

void normalize_stick(int16_t raw_x, int16_t raw_y, float *x, float *y) {
    constexpr float deadzone = 7849.0f / 32767.0f;
    float sx = static_cast<float>(raw_x) / 32767.0f;
    float sy = static_cast<float>(raw_y) / 32767.0f;
    const float radius = std::sqrt(sx * sx + sy * sy);
    if (radius <= deadzone || radius <= 0.0f) {
        *x = *y = 0.0f;
        return;
    }

    // A circular deadzone preserves the stick's direction near diagonals.
    // Rescale the remaining travel so full deflection still reaches 1.0.
    const float magnitude = std::min(1.0f, (radius - deadzone) / (1.0f - deadzone));
    const float scale = magnitude / radius;
    *x = sx * scale;
    *y = sy * scale;
}

void input_left_stick(float *x, float *y) {
    normalize_stick(g_state.left_x, g_state.left_y, x, y);
}

}  // namespace ntr

extern "C" void ntr_input_left_stick(float *x, float *y) {
    ntr::input_left_stick(x, y);
}

extern "C" void ntr_input_movement_stick(float *x, float *y, unsigned mode) {
    ntr::input_left_stick(x, y);
    const float travel = std::sqrt(*x * *x + *y * *y);
    if (travel <= 0.0f) return;
    // The retail touch joystick has a second stationary range, plus integer
    // speed steps. Start above its first nonzero step (1252/1451 in Q12).
    // Leave room for the original radius-105 vector quantization in CheckInput.
    const float minimum = (mode == 2 ? 1536.0f : 1344.0f) / 4096.0f;
    const float magnitude = minimum + (1.0f - minimum) * std::min(1.0f, travel);
    const float scale = magnitude / travel;
    *x *= scale;
    *y *= scale;
}

extern "C" void ntr_input_touch_sample(unsigned short *sample) {
    if(!sample) return;
    const auto input=ntr::input_state();
    sample[0]=input.touch_x;sample[1]=input.touch_y;sample[2]=input.touch?1:0;
}

extern "C" void ntr_input_get_menu_keys(unsigned short *keys) {
    if (!keys) return;
    *keys = static_cast<unsigned short>((~ntr::input_keyinput()) & 0x0FFFu);
    if (!ntr::g_menu_navigation) return;
    float x = 0.0f, y = 0.0f;
    ntr::input_left_stick(&x, &y);
    if (x >= 0.45f) *keys |= 1u << 4;
    if (x <= -0.45f) *keys |= 1u << 5;
    if (y >= 0.45f) *keys |= 1u << 6;
    if (y <= -0.45f) *keys |= 1u << 7;
}

extern "C" void ntr_input_set_menu_navigation(int enabled) {
    ntr::g_menu_navigation = enabled != 0;
}
