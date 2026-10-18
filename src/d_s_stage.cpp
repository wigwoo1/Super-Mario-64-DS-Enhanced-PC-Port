//cpp
// @symbol dScStage_c_classInit
#include "dScStage_c.h"
#include <new>

#if defined(SM64DS_PLATFORM_PC)
#include "hal/native_scalar_delete.h"
/* Scheduler call records carry the original ARM vtable slot offsets. Build a
   small host table in that exact order rather than exposing MSVC's deleting-
   destructor layout to those call sites. */
static int __fastcall pc_stage_init(void *self, void *) { return ((dScStage_c *)self)->dScStage_c::InitResources(); }
static bool __fastcall pc_stage_before_init(void *self, void *) { return ((dScStage_c *)self)->dScStage_c::BeforeInitResources(); }
static void __fastcall pc_stage_after_init(void *self, void *, u32 ok) { ((dScStage_c *)self)->dScStage_c::AfterInitResources(ok); }
static int __fastcall pc_stage_cleanup(void *self, void *) { return ((dScStage_c *)self)->dScStage_c::CleanupResources(); }
static int __fastcall pc_stage_before_cleanup(void *self, void *) { return ((dScStage_c *)self)->dScStage_c::BeforeCleanupResources(); }
static void __fastcall pc_stage_after_cleanup(void *self, void *, u32 ok) { ((dScStage_c *)self)->dScStage_c::AfterCleanupResources(ok); }
static int __fastcall pc_stage_behavior(void *self, void *) { return ((dScStage_c *)self)->dScStage_c::Behavior(); }
static int __fastcall pc_stage_before_behavior(void *self, void *) { return ((dScStage_c *)self)->dScStage_c::BeforeBehavior(); }
static void __fastcall pc_stage_after_behavior(void *self, void *, u32 ok) { ((dScStage_c *)self)->dScStage_c::AfterBehavior(ok); }
static int __fastcall pc_stage_render(void *self, void *) { return ((dScStage_c *)self)->dScStage_c::Render(); }
static int __fastcall pc_stage_before_render(void *self, void *) { return ((dScStage_c *)self)->dScStage_c::BeforeRender(); }
static void __fastcall pc_stage_after_render(void *self, void *, u32 ok) { ((dScStage_c *)self)->dScStage_c::AfterRender(ok); }
static void __fastcall pc_stage_pending(void *self, void *) { ((dScStage_c *)self)->dScStage_c::OnPendingDestroy(); }
static int __fastcall pc_stage_v34(void *self, void *, u32 a, u32 b) { return ((fBase_c *)self)->fBase_c::Virtual34(a,b); }
static int __fastcall pc_stage_v38(void *self, void *, u32 a, u32 b) { return ((fBase_c *)self)->fBase_c::Virtual38(a,b); }
static bool __fastcall pc_stage_heap(void *self, void *) { return ((fBase_c *)self)->fBase_c::OnHeapCreated(); }
static void *__fastcall pc_stage_dtor(void *self, void *, unsigned flags) {
    return pc_native_scalar_delete<dScStage_c>(self, flags);
}
static void *pc_stage_vtable[18] = {
    (void *)pc_stage_init, (void *)pc_stage_before_init, (void *)pc_stage_after_init,
    (void *)pc_stage_cleanup, (void *)pc_stage_before_cleanup, (void *)pc_stage_after_cleanup,
    (void *)pc_stage_behavior, (void *)pc_stage_before_behavior, (void *)pc_stage_after_behavior,
    (void *)pc_stage_render, (void *)pc_stage_before_render, (void *)pc_stage_after_render,
    (void *)pc_stage_pending, (void *)pc_stage_v34, (void *)pc_stage_v38, (void *)pc_stage_heap,
    (void *)pc_stage_dtor, (void *)pc_stage_dtor
};
#endif


extern "C" {
void *_ZN7fBase_cnwEj(unsigned int size);
void *_ZN7fBase_cC2Ev(fBase_c *base);
void _ZN8Particle10SysTrackerC1Ev(Particle::SysTracker *tracker);
void *_ZN5ModelC1Ev(Model *model);
void *_ZN7dBgW_KcC1Ev(dBgW_Kc *meshCollider);
extern void *_ZTV7dBase_c[];
extern void *_ZTV8dScene_c[];
extern void *_ZTV10dScStage_c[];

/* This is the allocation boundary for dScStage_c, not a compiler-emitted C3
   constructor. Its base and member construction order remains explicit until
   the surrounding original translation unit can own a real dScStage_c constructor. */
/* Reconstructed source-style name: SM64DS proves dScStage_c through RTTI,
 * allocation size, vtable identity, and the STAGE registry profile; later EAD
 * lineage supplies classInit. Exact original spelling is not preserved.
 * Historical alias: Stage_Spawn. The project's dScStage_c implementation type is
 * retained here because this wave reconstructs registry symbols, not classes. */
dScStage_c *dScStage_c_classInit()
{
#if defined(SM64DS_PLATFORM_PC)
    /* Let the host compiler provide the native MSVC vtable and construct the
       embedded members with their host ABI. The explicit ARM vptr stores below
       are only valid for the original DS image. */
    dScStage_c *stage = new dScStage_c();
    if (stage) {
        *(void ***)stage = pc_stage_vtable;
        /* The ARM object is allocated from a zeroed game heap, while native
           operator new does not clear the recovered trailing retry latch.
           InitResources reads this field before it writes it; stale heap data
           can therefore make a single-player stage wait for a network event. */
        *(s32 *)((char *)stage + 0x9c4) = 0;
        /* Recovered stage methods address this embedded Model at the DS
           offset directly. The PC compiler's member layout must not change
           that fixed-address construction contract. */
        new ((char *)stage + 0x86c) Model();
        /* The collision registry stores the embedded mesh collider itself,
           and collision dispatch calls its slot-6 vtable entry. Construct at
           the recovered DS offset just as the native stage methods address it. */
        new ((char *)stage + 0x91c) dBgW_Kc();
    }
    return stage;
#else
    dScStage_c *stage = (dScStage_c *)_ZN7fBase_cnwEj(sizeof(dScStage_c));
    if (stage) {
        _ZN7fBase_cC2Ev(stage);
        *(void ***)stage = _ZTV7dBase_c;
        *(void ***)stage = _ZTV8dScene_c;
        stage->pauseFlags |= 1;
        stage->pauseFlags |= 4;
        *(void ***)stage = _ZTV10dScStage_c;
        _ZN8Particle10SysTrackerC1Ev(&stage->mSysTracker);
        _ZN5ModelC1Ev(&stage->mModel);
        _ZN7dBgW_KcC1Ev(&stage->mMeshCollider);
    }
    return stage;
#endif
}
}
