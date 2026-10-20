#include "ntr/input.h"
#include "ntr/mmio.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

static bool near(float actual, float expected, float epsilon = 0.002f) {
    return std::fabs(actual - expected) <= epsilon;
}

int main() {
    float x = 0.0f, y = 0.0f;
    ntr::normalize_stick(0, 0, &x, &y);
    if (x != 0.0f || y != 0.0f) return 1;

    // The Xbox circular deadzone is 7849/32767. Samples inside it must not
    // move Mario, including diagonal noise that would pass per-axis checks.
    ntr::normalize_stick(4000, 4000, &x, &y);
    if (x != 0.0f || y != 0.0f) return 2;

    ntr::normalize_stick(32767, 0, &x, &y);
    if (!near(x, 1.0f) || !near(y, 0.0f)) return 3;
    ntr::normalize_stick(0, -32767, &x, &y);
    if (!near(x, 0.0f) || !near(y, -1.0f)) return 4;

    // Diagonal normalization must preserve direction and cap the vector at
    // unit magnitude, matching the 0x69-radius SM64DS virtual joystick.
    ntr::normalize_stick(32767, 32767, &x, &y);
    if (!near(x, 0.7071f) || !near(y, 0.7071f) ||
        !near(std::sqrt(x * x + y * y), 1.0f)) return 5;

    // Intermediate travel remains analog instead of becoming a digital key.
    ntr::normalize_stick(20000, 0, &x, &y);
    if (!(x > 0.0f && x < 1.0f && y == 0.0f)) return 6;

#if defined(_WIN32)
    _putenv_s("SM64DS_PC_AUTOMATION_ONLY", "1");
    _putenv_s("SM64DS_PC_AUTOCONFIRM_POLL", "2");
    _putenv_s("SM64DS_PC_AUTOCONFIRM_REPEAT", "0");
    _putenv_s("SM64DS_PC_AUTODOWN_POLL", "0");
    _putenv_s("SM64DS_PC_TEST_STICK_X", "");
    _putenv_s("SM64DS_PC_TEST_STICK_Y", "");
    ntr::input_set_touch(true, 100, 100);
    ntr::input_poll();
    if (ntr::input_keyinput() != 0x0fff || ntr::input_state().touch) return 7;
    ntr::input_poll();
    if (ntr::input_keyinput() != 0x0ffe) return 8;

    // A timed replay must start neutral, supply partial analog travel only
    // inside its window, and return to neutral without adding D-pad bits.
    _putenv_s("SM64DS_PC_AUTOCONFIRM_POLL", "0");
    _putenv_s("SM64DS_PC_TEST_STICK_X", "20000");
    _putenv_s("SM64DS_PC_TEST_STICK_Y", "0");
    _putenv_s("SM64DS_PC_TEST_STICK_START_POLL", "5");
    _putenv_s("SM64DS_PC_TEST_STICK_END_POLL", "7");
    for (int poll = 3; poll <= 8; ++poll) {
        ntr::input_poll();
        const int expected_x = poll >= 5 && poll < 7 ? 20000 : 0;
        if (ntr::input_state().left_x != expected_x ||
            ntr::input_state().left_y != 0 || ntr::input_keyinput() != 0x0fff) return 9;
        unsigned short menu_keys = 0, game_keys = 0;
        ntr_input_set_menu_navigation(1);
        ntr_input_get_menu_keys(&menu_keys);
        ntr_input_set_menu_navigation(0);
        ntr_input_get_menu_keys(&game_keys);
        if (menu_keys != (expected_x ? 0x10 : 0) || game_keys != 0) return 10;
    }
    _putenv_s("SM64DS_PC_AUTODOWN_POLL", "9");
    ntr::input_poll();
    unsigned short digital_keys = 0;
    ntr_input_get_menu_keys(&digital_keys);
    if (digital_keys != 0x80) return 11;
    _putenv_s("SM64DS_PC_AUTODOWN_POLL", "0");
    _putenv_s("SM64DS_PC_TEST_BUTTONS", "10:8,bad,12:1000,18:2");
    for (int poll = 10; poll <= 24; ++poll) {
        ntr::input_poll();
        const unsigned expected = poll < 15 ? 8 : poll >= 18 && poll < 23 ? 2 : 0;
        if (ntr::input_keyinput() != (0xfff ^ expected)) return 12;
    }
    _putenv_s("SM64DS_PC_TEST_BUTTONS", "");
    _putenv_s("SM64DS_PC_TEST_TOUCH", "27:128:96,bad,25:256:96,25:128:192,25:12:,25::8,35:0:191");
    for(int poll=25;poll<=41;++poll) {
        ntr::input_poll();
        const bool active=(poll>=27 && poll<32)||(poll>=35 && poll<40);
        const auto state=ntr::input_state();
        const unsigned latch=ntr::io_read(0x027FFFAA,2)|(ntr::io_read(0x027FFFAC,2)<<16);
        if(state.touch!=active || bool(latch&(1u<<24))!=active)return 13;
        if(active && (state.touch_x!=(poll<32?128:0) || state.touch_y!=(poll<32?96:191)))return 14;
        if(ntr::input_keyinput()!=0xfff)return 15;
    }
    _putenv_s("SM64DS_PC_TEST_TOUCH", "");
    // Adaptive gameplay replays must be impossible during ordinary play.
    _putenv_s("SM64DS_PC_TEST_COURSE_ROUTE", "");
    _putenv_s("SM64DS_PC_TEST_BOSS_BOT", "0");
    ntr::input_test_controls(true,1,20000,-20000);ntr::input_poll();
    if(ntr::input_keyinput()!=0xfff || ntr::input_state().left_x)return 16;
    _putenv_s("SM64DS_PC_TEST_BOSS_BOT", "1");
    ntr::input_test_controls(true,1,20000,-20000);ntr::input_poll();
    if(ntr::input_keyinput()!=0xffe || ntr::input_state().left_x!=20000 || ntr::input_state().left_y!=-20000)return 17;
    // Revoking the second opt-in immediately suppresses an existing override.
    _putenv_s("SM64DS_PC_TEST_BOSS_BOT", "0");ntr::input_poll();
    if(ntr::input_keyinput()!=0xfff || ntr::input_state().left_x)return 18;
    _putenv_s("SM64DS_PC_AUTOMATION_ONLY", "0");
    _putenv_s("SM64DS_PC_TEST_BOSS_BOT", "1");
    ntr::input_test_controls(true,1,20000,0);
    _putenv_s("SM64DS_PC_AUTOMATION_ONLY", "1");ntr::input_poll();
    if(ntr::input_keyinput()!=0xfff || ntr::input_state().left_x)return 19;
    _putenv_s("SM64DS_PC_TEST_BOSS_BOT", "");
    _putenv_s("SM64DS_PC_TEST_COURSE_ROUTE", "1:-75:-1250:40");
    ntr::input_test_controls(true,2,-18000,21000);ntr::input_poll();
    if(ntr::input_keyinput()!=0xffd || ntr::input_state().left_x!=-18000 || ntr::input_state().left_y!=21000)return 20;
    _putenv_s("SM64DS_PC_TEST_COURSE_ROUTE", "");ntr::input_poll();
    if(ntr::input_keyinput()!=0xfff || ntr::input_state().left_x)return 21;
    _putenv_s("SM64DS_PC_AUTOMATION_ONLY", "0");
    _putenv_s("SM64DS_PC_TEST_COURSE_ROUTE", "1:0:0:40");
    ntr::input_test_controls(true,1,20000,0);
    _putenv_s("SM64DS_PC_AUTOMATION_ONLY", "1");ntr::input_poll();
    if(ntr::input_keyinput()!=0xfff || ntr::input_state().left_x)return 22;
    ntr::input_test_controls(false,0,0,0);
    _putenv_s("SM64DS_PC_TEST_COURSE_ROUTE", "");
    _putenv_s("SM64DS_PC_TEST_POWER_STAR", "1");
    ntr::input_test_controls(true,2,18000,-18000);ntr::input_poll();
    if(ntr::input_keyinput()!=0xffd || ntr::input_state().left_x!=18000 || ntr::input_state().left_y!=-18000)return 23;
    _putenv_s("SM64DS_PC_TEST_POWER_STAR", "0");ntr::input_poll();
    if(ntr::input_keyinput()!=0xfff || ntr::input_state().left_x)return 24;
    _putenv_s("SM64DS_PC_AUTOMATION_ONLY", "0");
    _putenv_s("SM64DS_PC_TEST_POWER_STAR", "1");
    ntr::input_test_controls(true,1,20000,0);
    _putenv_s("SM64DS_PC_AUTOMATION_ONLY", "1");ntr::input_poll();
    if(ntr::input_keyinput()!=0xfff || ntr::input_state().left_x)return 25;
    _putenv_s("SM64DS_PC_TEST_POWER_STAR", "");
#endif
    std::puts("input smoke: analog travel, isolated input and timed stick replay pass");
    return 0;
}
