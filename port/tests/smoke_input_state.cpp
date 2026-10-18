#include "ntr/input.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
extern unsigned char data_020a0e58[], data_020a0e5a[], data_020a0e5c[], data_020a0e5e[];
extern unsigned char pc_input_state_end[];
unsigned char data_020a0e44;
unsigned short data_020a0e48[4], data_020a0e50[4];
void func_0203bc7c();
void func_0203bc50();
unsigned short func_0203dae4(int player) {
    const unsigned short remote[] = {0, 1, 0x40, 0x100};
    return remote[player];
}
}

int main() {
    const auto base = reinterpret_cast<uintptr_t>(data_020a0e58);
    if (reinterpret_cast<uintptr_t>(data_020a0e5a) != base + 2 ||
        reinterpret_cast<uintptr_t>(data_020a0e5c) != base + 4 ||
        reinterpret_cast<uintptr_t>(data_020a0e5e) != base + 6 ||
        reinterpret_cast<uintptr_t>(pc_input_state_end) != base + 16) return 1;
    _putenv_s("SM64DS_PC_AUTOMATION_ONLY", "1");
    _putenv_s("SM64DS_PC_AUTOCONFIRM_POLL", "0");
    _putenv_s("SM64DS_PC_AUTODOWN_POLL", "0");
    _putenv_s("SM64DS_PC_TEST_BUTTONS", "1:8");
    ntr_input_set_menu_navigation(0);
    auto *keys = reinterpret_cast<unsigned short *>(data_020a0e58);
    for (int poll = 1; poll <= 7; ++poll) {
        ntr::input_poll();
        func_0203bc7c();
        if (keys[0] != (poll <= 5 ? 8 : 0) || keys[1] != (poll == 1 ? 8 : 0)) return 2;
        if (*reinterpret_cast<unsigned short *>(data_020a0e5a) != keys[1]) return 3;
        for (int player = 1; player < 4; ++player) {
            const unsigned short expected[] = {0, 1, 0x40, 0x100};
            if (keys[player * 2] != expected[player] ||
                keys[player * 2 + 1] != (poll == 1 ? expected[player] : 0)) return 4;
        }
    }
    std::memset(data_020a0e58, 0xab, 16);
    func_0203bc50();
    for (int i = 0; i < 16; ++i) if (data_020a0e58[i]) return 5;
    std::puts("PASS: original input update/reset share all four held/pressed pairs and interior labels");
}
