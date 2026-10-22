#include "Player.h"
#include "ntr/rt.h"
#include "ntr/audio.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
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

static const char *native_state_name(const Player::State *state)
{
    if (!state) return "none";
    const uintptr_t address=state->mMain.address;
#define PC_PLAYER_STATE_METHOD(guest, method) \
    if(address==guest || address==native_method(&Player::method)) return #method;
#define PC_PLAYER_STATE_WORKER(guest, worker) \
    if(address==guest || address==reinterpret_cast<uintptr_t>(&native_worker<worker>)) return #worker;
#include "player_state_methods.inc"
#undef PC_PLAYER_STATE_METHOD
#undef PC_PLAYER_STATE_WORKER
    return "unknown";
}

extern "C" const char *pc_player_state_name(const Player *player)
{
    return native_state_name(player?player->mState:nullptr);
}

extern "C" void pc_trace_jump_input(const void *object,const char *where)
{
    const char *range=std::getenv("SM64DS_PC_JUMP_INPUT_TRACE");
    unsigned begin=0,end=0;
    if(!range || std::sscanf(range,"%u:%u",&begin,&end)!=2)return;
    const auto tick=ntr::rt_frame();if(tick<begin || tick>end)return;
    extern unsigned char gActivePlayerSlot;
    extern unsigned short data_0209f49c, data_0209f49e;
    const auto *p=static_cast<const Player *>(object);
    const auto *c=static_cast<const unsigned char *>(object);
    const unsigned slot=gActivePlayerSlot;
    const auto held=*reinterpret_cast<const unsigned short *>(reinterpret_cast<const char *>(&data_0209f49c)+slot*24);
    const auto pressed=*reinterpret_cast<const unsigned short *>(reinterpret_cast<const char *>(&data_0209f49e)+slot*24);
    std::fprintf(stderr,"[jump input] tick=%llu where=%s slot=%u held=%04x pressed=%04x state=%s flags=%04x crouch=%u phase=%u step=%u air=%u y=%d speed=%d\n",
        tick,where,slot,held,pressed,pc_player_state_name(p),*reinterpret_cast<const unsigned short *>(c+0x6ce),
        *reinterpret_cast<const unsigned short *>(c+0x6b0),unsigned(p->mStatePhase),unsigned(p->mStateStep),unsigned(p->mIsAirborne),p->mPosY,p->mHorzSpeed);
}

extern "C" void pc_trace_player_state_change(const Player *p,const Player::State *oldState,const Player::State *newState,const void *caller)
{
    const char *range=std::getenv("SM64DS_PC_PLAYER_STATE_TRACE");
    unsigned begin=0,end=0;
    if(!range || std::sscanf(range,"%u:%u",&begin,&end)!=2)return;
    const auto tick=ntr::rt_frame();if(tick<begin || tick>end)return;
    const auto image=reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    std::fprintf(stderr,"[native player transition] tick=%llu old=%s new=%s state=%p main=%08x caller=%08x pos=%d,%d,%d speed=%d airborne=%u step=%u phase=%u work=%u loop=%u audio=%u\n",
        tick,native_state_name(oldState),native_state_name(newState),newState,
        newState?unsigned(newState->mMain.address):0,unsigned(reinterpret_cast<uintptr_t>(caller)-image+0x400000),
        p->mPosX,p->mPosY,p->mPosZ,p->mHorzSpeed,unsigned(p->mIsAirborne),
        unsigned(p->mStateStep),unsigned(p->mStatePhase),unsigned(p->mStateWork),unsigned(p->mLoopingSoundHandle),unsigned(ntr::audio_ready()));
    void *frames[10];
    const auto count=CaptureStackBackTrace(0,10,frames,nullptr);
    std::fprintf(stderr,"[native player stack] tick=%llu",tick);
    for(unsigned i=0;i<count;++i)std::fprintf(stderr," %08x",unsigned(reinterpret_cast<uintptr_t>(frames[i])-image+0x400000));
    std::fprintf(stderr,"\n");
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
