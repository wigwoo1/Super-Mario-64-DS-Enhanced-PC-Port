#include "hal/stage_graph_state.h"
#include "ntr/mmio.h"
#include "ntr/rt.h"
#include "ntr/video.h"
#include <cstdio>
#include <cstddef>
extern "C" void UpdateMinimap(const int *, int, int, int, int);
extern "C" { void *data_0209d4a8 = nullptr; }
extern "C" { unsigned char data_0209f2c4 = 0; }
extern "C" {
int data_ov002_0211116c[3] = {};
int data_ov002_02111160[3] = {};
void _ZN7Vector3D1Ev() {}
void __sinit_ov002_0210804c();
}
static bool registered_center = false;
extern "C" void func_020731dc(void *object, void *destructor, void *node)
{
    registered_center = object == data_ov002_0211116c &&
        destructor == reinterpret_cast<void *>(_ZN7Vector3D1Ev) && node == data_ov002_02111160;
}

static bool runtime_ok = true;
static int presentations = 0;
static unsigned seen_ticks = 0;
static void game()
{
    for (int i=0; i<3; ++i) {
        int matrix[] = {0x800+i*0x100, 0, 0, 0x800};
        UpdateMinimap(matrix,64,64,-64,-32);
        ntr::rt_vblank_wait();
    }
}
static bool presented(uint64_t)
{
    const int expected = (0x800 + (ntr::rt_frame()-1)*0x100) >> 4;
    runtime_ok &= NTR_MMIO(uint16_t,0x04001030) == expected;
    seen_ticks |= 1u << ntr::rt_frame();
    ++presentations;
    return true;
}

int main()
{
    if (!ntr::io_init()) return 2;
    __sinit_ov002_0210804c();
    if (!registered_center || data_ov002_0211116c[0] != 128*4096 ||
        data_ov002_0211116c[1] != 0 || data_ov002_0211116c[2] != 96*4096) return 9;
    static_assert(offsetof(PcStageGraphState, matrix) == 4, "matrix offset");
    static_assert(offsetof(PcStageGraphState, center_x) == 20, "center offset");
    void *vtable[] = {nullptr, nullptr, (void *)pc_stage_graph_apply, nullptr};
    data_0209f3c4.vtable = vtable;
    const int matrix[] = {0x800, 0x400, -0x400, 0x800};
    UpdateMinimap(matrix, 64, 64, 64-128, 64-96);
    auto apply = reinterpret_cast<int (*)(void *)>(vtable[2]);
    if (apply(&data_0209f3c4) != 1) return 3;
    if (NTR_MMIO(uint16_t,0x04001030) != 128 ||
        NTR_MMIO(uint16_t,0x04001032) != 64 ||
        (int16_t)NTR_MMIO(uint16_t,0x04001034) != -64 ||
        NTR_MMIO(uint16_t,0x04001036) != 128) return 4;
    if ((int32_t)NTR_MMIO(uint32_t,0x04001038) != -6144 ||
        (int32_t)NTR_MMIO(uint32_t,0x0400103c) != 12288) return 5;
    if (data_0209f3c4.vtable != vtable) return 6;
    data_0209d4a8 = &data_0209f3c4;
    ntr::rt_run(game, presented, 3, pc_stage_graph_prepare);
    if (!runtime_ok || presentations < 3 || seen_ticks != 14) return 7;
    data_0209d4a8 = nullptr;
    NTR_MMIO(uint16_t,0x04001030) = 123;
    pc_stage_graph_prepare();
    if (NTR_MMIO(uint16_t,0x04001030) != 123) return 8;
    data_0209f2c4 = 1;
    pc_stage_graph_prepare();
    // An inactive Stage's old pause flag must not replace another scene.
    if (ntr::video_pause_screen() || NTR_MMIO(uint16_t,0x04001030) != 123) return 10;
    ntr::video_set_minigame_active(true);
    pc_stage_graph_prepare();
    if (!ntr::video_dual_screen_active() || ntr::video_pause_screen() || ntr::video_minimap_visible()) return 12;
    ntr::video_set_minigame_active(false);
    // An actual active Stage still opens its original pause interface.
    data_0209d4a8 = &data_0209f3c4;
    pc_stage_graph_prepare();
    if (!ntr::video_pause_screen()) return 13;
    data_0209f2c4 = 0;
    pc_stage_graph_prepare();
    if (ntr::video_pause_screen()) return 11;
    std::puts("PASS: Stage minimap callback, active pause, and stale-pause isolation for minigames");
}
