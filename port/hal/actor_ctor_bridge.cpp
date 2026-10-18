// ABI constructor bridges and the few gate-9 vtable fillers that still lack a
// real body.
//
// Gate 9 belongs to smoke_actor alone, so these cannot live in the gate-8
// bridge -- smoke_clsn links that one and never builds these classes.
//
// The production TUs supply actor, collision and shadow methods; keep any
// genuinely missing virtual slot loud instead of shadowing a real body.
#include "common.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>

#include "dExtShadowModel_c.h"
#include "dBgW_KcMbg.h"
#include "dBgActor_c.h"
#include "SolidHeap.h"
#include "SolidHeapAllocator.h"
#include "Memory.h"

[[noreturn]] static void actor_trap(const char *who)
{
    fprintf(stderr, "FATAL: %s dispatched, but the port defines it only so "
                    "MSVC can emit the vtable (hal/actor_ctor_bridge.cpp).\n", who);
    abort();
}

#define ACTOR_TRAP(sig, who) sig { actor_trap(who); }

extern "C" {
// Zero-initialized ARM9 actor-factory state. The game startup path assigns
// the callbacks before actor spawning; the smoke target only needs storage.
void *data_020a4b58 = 0;
void *data_020a4b5c = 0;
unsigned char data_020a4b4c = 0;
unsigned short data_020a4b50 = 0xffff;

void _ZN10dBgW_KcMbgC1Ev(void *self) { ::new (self) dBgW_KcMbg(); }
void _ZN17dExtShadowModel_cC1Ev(void *self) { ::new (self) dExtShadowModel_c(); }
SolidHeapAllocator *_ZN18SolidHeapAllocatorC1EPvj(
    SolidHeapAllocator *self, void *heapEnd, u32 flags)
{ return ::new ((void *)self) SolidHeapAllocator(heapEnd, flags); }
SolidHeap *_ZN9SolidHeapC1EPvjP4HeapP18SolidHeapAllocator(
    SolidHeap *self, void *start, u32 size, Heap *root,
    SolidHeapAllocator *allocator)
{ return ::new ((void *)self) SolidHeap(start, size, root, allocator); }
}

// ---------------------------------------------------------------------------
// dBgActor_c's constructor is real C++ on this branch. The gate keeps the one
// missing fBase_c slot required by the emitted MSVC vtable explicit.

// Gate 9 includes the native dActor_c constructor from src/. That definition
// owns construction, including mActorListNode(this); no HAL wrapper is needed.

#if !defined(SM64DS_GAME_TARGET)
ACTOR_TRAP(dActor_c::~dActor_c(), "dActor_c::~dActor_c")
#endif

// The real source TUs above supply the dActor_c, fBase_c, dBgActor_c,
// The absent resource-init slot and return vector remain loud vtable fillers
// for this actor-only smoke gate.
ACTOR_TRAP(int fBase_c::InitResources(), "fBase_c::InitResources")
#if defined(SM64DS_GAME_TARGET)
// The recovered ARM ABI function returns through an explicit output pointer.
// Native virtual dispatch needs MSVC's ordinary Vector3-by-value method.
extern "C" void _ZN8dActor_c25OnAimedAtWithEggReturnVecEv(Vector3 *, dActor_c *);
Vector3 dActor_c::OnAimedAtWithEggReturnVec()
{
    Vector3 result;
    _ZN8dActor_c25OnAimedAtWithEggReturnVecEv(&result, this);
    return result;
}
#else
ACTOR_TRAP(Vector3 dActor_c::OnAimedAtWithEggReturnVec(), "dActor_c::OnAimedAtWithEggReturnVec")
#endif

// The raw Itanium spelling ArrowSignRight_Spawn.c still calls: cdecl, `this`
// as an ordinary first argument, where the real method is __thiscall.
extern "C" void *_ZTV5Model[8];
extern "C" void _ZN10dBgActor_cC2Ev(void *self)
{
    ::new (self) dBgActor_c();
    // The real constructor builds its Model sub-object at +0xd4 by calling
    // Model::Model() directly, which stores MSVC's own five-slot table -- it
    // never goes through the _ZN5ModelC1Ev bridge in hal/ctor_bridge.cpp, so
    // the dual-numbered synthetic table that bridge installs is bypassed here.
    // Re-stamp it: gate 9 dispatches Model's virtuals by ROM slot number, and
    // ROM numbering carries two destructor slots where MSVC carries one.
    *(void **)((char *)self + 0xd4) = _ZTV5Model;
}

// Preserve the actor's owning heap. Memory supplies the default only for null.
extern "C" void _ZN6Memory10DeallocateEPvP4Heap(void *p, void *heap)
{ Memory::Deallocate(p, static_cast<Heap *>(heap)); }

// dBase_c sits between fBase_c and dActor_c; its own two undefined slots come
// along with the same vtable emission.
/* Native actors need the normal base destructor chain during lifecycle
   cleanup. Their allocation is released by fBase_c::operator delete. */
fBase_c::~fBase_c()
{
}
