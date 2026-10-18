#include "dScMgLuigi_c.h"
#include "ntr/minigame_callback.h"
#include <cstddef>
#include <cstring>
#include <cstdio>

struct Probe { int value; void enter() { value += 3; } void slot(int x) { value += x; } };
static void enter(void *p) { static_cast<Probe *>(p)->enter(); }
static void slot(void *p, int x) { static_cast<Probe *>(p)->slot(x); }
int main() {
    static_assert(sizeof(dMgState_c) == 0x28, "state controller");
    static_assert(offsetof(dMgState_c, mBehavior) == 8, "behavior record");
    static_assert(offsetof(dMgState_c, mRender) == 0x10, "render record");
    static_assert(offsetof(dMgState_c, mState) == 0x18, "state number");
    static_assert(offsetof(dMgState_c, unk_024) == 0x24, "gate");
    static_assert(sizeof(dScMgBase_c) == 0x4660, "base");
    static_assert(offsetof(dScMgBase_c, mStateController) == 0xcc, "controller offset");
    static_assert(offsetof(dScMgBase_c, mTouchOptions) == 0xf4, "touch options");
    static_assert(offsetof(dScMgLuigi_c, mState) == 0x4f78, "Wanted state");
    static_assert(offsetof(dScMgLuigi_c, mPenalty) == 0x4660, "Wanted penalty");
    static_assert(sizeof(dScMgLuigi_c) == 0x545c, "Wanted size");
    struct Guarded { unsigned before; PcMinigameCallback<> record; unsigned after; } g{123,{},456};
    PcMinigameCallback<> source{enter};
    // Match the original state handlers' two-word copy, including null records.
    std::memcpy(&g.record, &source, 8);
    Probe p{}; g.record(&p);
    if(p.value!=3 || g.before!=123 || g.after!=456) return 1;
    PcMinigameCallback<int> with_slot{slot}; with_slot(&p,7);
    if(p.value!=10) return 2;
    struct Adjusted { int pad; Probe object; } a{};
    PcMinigameCallback<> adjusted{enter,offsetof(Adjusted,object)}; adjusted(&a);
    if(a.object.value!=3) return 3;
    PcMinigameCallback<> empty;
    std::memcpy(&g.record,&empty,8); g.record(&p);
    if(g.record || !(g.record==0) || p.value!=10 || g.after!=456) return 4;
    std::puts("minigame layout and native two-word callback dispatch passed");
}
