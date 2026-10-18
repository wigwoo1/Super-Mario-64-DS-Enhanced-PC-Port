#include "Player.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define PC_PLAYER_STATE_METHOD(guest, method)
#define PC_PLAYER_STATE_WORKER(guest, worker) extern "C" int worker(Player *self);
#include "player_state_methods.inc"
#undef PC_PLAYER_STATE_METHOD
#undef PC_PLAYER_STATE_WORKER

extern "C" void func_ov002_020e1e70(char *self);
extern "C" void func_ov002_020e200c(char *self);
extern "C" void func_ov002_020e1c20(char *self);
extern "C" void pc_player_jump_callback(Player *self, const int *row) {
    // These character-specific jump helpers are a separate table from State.
    // All four original descriptors have zero adjustment and direct targets.
    if (row[1] != 0) {
        std::fprintf(stderr, "Unsupported Player jump adjustment %08x\n", row[1]);
        std::abort();
    }
    switch ((unsigned)row[0]) {
    case 0x020e1e70: func_ov002_020e1e70((char *)self); return;
    case 0x020e200c: func_ov002_020e200c((char *)self); return;
    case 0x020e1c20: func_ov002_020e1c20((char *)self); return;
    default:
        std::fprintf(stderr, "Unmapped Player jump callback %08x\n", row[0]);
        std::abort();
    }
}

template<int (*Worker)(Player *)>
static int __fastcall native_worker(Player *self, void *) { return Worker(self); }

static uintptr_t native_method(int (Player::*method)())
{
    static_assert(sizeof(method) == sizeof(uintptr_t), "native Player member pointer");
    uintptr_t address;
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

extern "C" const char *pc_player_state_name(const Player *player)
{
    if (!player || !player->mState) return "none";
    const uintptr_t address=player->mState->mMain.address;
#define PC_PLAYER_STATE_METHOD(guest, method) \
    if(address==guest || address==native_method(&Player::method)) return #method;
#define PC_PLAYER_STATE_WORKER(guest, worker) \
    if(address==guest || address==reinterpret_cast<uintptr_t>(&native_worker<worker>)) return #worker;
#include "player_state_methods.inc"
#undef PC_PLAYER_STATE_METHOD
#undef PC_PLAYER_STATE_WORKER
    return "unknown";
}

extern "C" void pc_rebase_player_state(Player::State *state)
{
    if (!state) return;
    PcMemberCallback<int> *callbacks[] = {&state->mInit, &state->mMain, &state->mCleanup};
    for (auto callback : callbacks) {
        if (callback->address < 0x02004000u || callback->address >= 0x02400000u) continue;
        if (callback->adjustment != 0) {
            std::fprintf(stderr, "Unsupported Player state adjustment at %08x\n", callback->address);
            std::abort();
        }
        switch (callback->address) {
#define PC_PLAYER_STATE_METHOD(guest, method) \
            case guest: callback->address = native_method(&Player::method); break;
#define PC_PLAYER_STATE_WORKER(guest, worker) \
            case guest: callback->address = reinterpret_cast<uintptr_t>(&native_worker<worker>); break;
#include "player_state_methods.inc"
#undef PC_PLAYER_STATE_METHOD
#undef PC_PLAYER_STATE_WORKER
        default:
            std::fprintf(stderr, "Unmapped Player state callback %08x\n", callback->address);
            std::abort();
        }
    }
}
