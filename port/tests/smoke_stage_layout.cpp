#include "dScStage_c.h"
#include "Player.h"
#include <cstddef>
#include <cstdio>

// Recovered code and native C++ members must address the same Stage fields.
static_assert(sizeof(dPa_c::level_c::simpleCallback_c) == 8, "simple callback ABI");
static_assert(sizeof(dPa_c::level_c::scaleCallback_c) == 12, "scale callback ABI");
static_assert(sizeof(Particle::SysTracker) == 0x81c, "particle tracker ABI");
static_assert(offsetof(dScStage_c, mModel) == 0x86c, "Stage model ABI");
static_assert(offsetof(dScStage_c, mTexAnimSlots) == 0x8bc, "Stage texture slot ABI");
static_assert(offsetof(dScStage_c, mMeshCollider) == 0x91c, "Stage collider ABI");
static_assert(sizeof(dScStage_c) == 0x9c8, "Stage allocation extent ABI");
static_assert(sizeof(Player::State) == 24, "Player state ABI");
static_assert(offsetof(Player::State, mMain) == 8, "Player main callback ABI");
static_assert(offsetof(Player::State, mCleanup) == 16, "Player cleanup callback ABI");
static_assert(sizeof(Player) == 0x768, "Player allocation extent ABI");

struct CallbackProbe {
    int value;
    int Init() { return ++value; }
    int Cleanup() { return 0; }
    void Enter() { value += 10; }
    int Script(unsigned char *command, int first, int last) {
        return value + *command + first + last;
    }
};

static int __fastcall c_worker_adapter(CallbackProbe *self, void *)
{
    return self->value + 100;
}

int main()
{
    CallbackProbe probe{0};
    PcMemberCallback<int> records[3] = {};
    records[0] = &CallbackProbe::Init;
    records[1] = &CallbackProbe::Cleanup;
    records[2].address = reinterpret_cast<uintptr_t>(&c_worker_adapter);
    if ((&probe->*records[0])() != 1 || (&probe->*records[1])() != 0 ||
        (&probe->*records[2])() != 101) {
        std::puts("FAIL: callback table index, this pointer, or cleanup veto");
        return 1;
    }
    PcMemberCallback<void> enter{};
    enter = &CallbackProbe::Enter;
    (&probe->*enter)();
    PcMemberCallback<int, unsigned char *, int, int> script{};
    script = &CallbackProbe::Script;
    unsigned char command = 3;
    if (probe.value != 11 || (&probe->*script)(&command, 4, 5) != 23) {
        std::puts("FAIL: void callback or script arguments");
        return 1;
    }
    std::puts("PASS: Stage/Player layouts and native callback dispatch");
}
