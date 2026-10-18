//cpp
/* Heap -- abstract base of the ROM's mHeap::Heap_t family (RTTI at
   0x02099ce4 spells the cartridge name; SolidHeap and ExpandingHeap derive
   from it). TU claims 0x0203c24c..0x0203c33c, the class's own contiguous run
   in delinks order: the game-heap bootstrap, the four tail-call veneers, the
   scratch-heap push/pop pair, and the default-heap setter. Written
   back-to-front: mwccarm emits .text in reverse source order under default
   deferred codegen. */
#include "Heap.h"
#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#endif

/* 0x020a0eac, already named in config/arm9/symbols.txt. */
extern "C" Heap* GAME_HEAP_PTR;
namespace Memory { extern Heap* defaultHeapPtr; }   /* 0x020a0ea0 */
/* 0x020a0ea8 -- where SetupSolidHeapAsDefault parks the outgoing default heap.
   Unnamed in config/arm9/symbols.txt; left as the address symbol rather than
   minted here, since naming it is a claim of its own. */
extern "C" Heap* data_020a0ea8;

// @symbol _ZN4Heap10SetDefaultEv
/* Install this heap as the process-wide default and hand back the one it
   replaced, so a caller can put it back. */
Heap* Heap::SetDefault()
{
    Heap* previous = Memory::defaultHeapPtr;
    Memory::defaultHeapPtr = this;
    return previous;
}

// @symbol _ZN4Heap23SetupSolidHeapAsDefaultEjPS_i
/* Push a solid scratch heap in front of the current default, remembering the
   old one so RestoreFromTemporary can put it back. One level deep only: a
   second call overwrites the saved pointer and the first default is lost.

   Note the order. The outgoing default is read straight out of the global
   rather than from SetDefault's return value, even though SetDefault returns
   exactly that -- so the save happens before the swap, not after it. */
void* Heap::SetupSolidHeapAsDefault(u32 size, Heap* root, int align)
{
    Heap* heap = CreateSolidHeap(size, root, align);
    if (!heap)
        return 0;

    data_020a0ea8 = Memory::defaultHeapPtr;
    heap->SetDefault();
    return heap;
}

// @symbol _ZN4Heap28InitializeSolidHeapAsDefaultEjPS_i
/* Three-word tail-call veneer to SetupSolidHeapAsDefault above:
   `ldr ip,[pc] / bx ip / .word 0x0203c2e4'. Static, by the same argument-count
   test as its target: three declared parameters, three arguments in the body,
   no room for a `this'. The veneer sits twelve bytes BEFORE what it jumps to
   -- the pair is adjacent and the thunk is still a long-form ldr/bx, which is
   what these veneers look like throughout: they are not distance-driven. */
void* Heap::InitializeSolidHeapAsDefault(u32 size, Heap* root, int align)
{
    return SetupSolidHeapAsDefault(size, root, align);
}

// @symbol _ZN4Heap20RestoreFromTemporaryEv
/* Undoes SetupSolidHeapAsDefault: reinstate the heap that was default before
   the scratch heap was pushed, and drop the saved pointer. */
void Heap::RestoreFromTemporary()
{
    data_020a0ea8->SetDefault();
    data_020a0ea8 = 0;
}

// @symbol _ZN4Heap9_AllocateEji
/* Three-word tail-call veneer to Allocate(u32, int) (0x0203c6cc). */
void* Heap::_Allocate(u32 size, int align)
{
    return Allocate(size, align);
}

// @symbol _ZN4Heap8AllocateEj
/* Convenience overload that forwards to Allocate(u32, int) with the default
   alignment of 4. */
void* Heap::Allocate(u32 size)
{
    return Allocate(size, 4);
}

// @symbol _ZN4Heap11_DeallocateEPv
/* Three-word tail-call veneer to Deallocate (0x0203c538). */
void Heap::_Deallocate(void* ptr)
{
    Deallocate(ptr);
}

// @symbol _ZN4Heap7_SizeofEPv
/* Three-word tail-call veneer to Sizeof (0x0203c454). */
int Heap::_Sizeof(void* ptr)
{
    return Sizeof(ptr);
}

// @symbol _ZN4Heap18InitializeGameHeapEjPS_
/* Carve the game heap out of `root' and publish it. Alignment 4 is hard-coded
   here; callers do not get a say. */
void Heap::InitializeGameHeap(u32 size, Heap* root)
{
#if defined(SM64DS_PLATFORM_PC)
    const char *trace = getenv("SM64DS_PC_TRACE");
    if (trace && trace[0] && trace[0] != '0') {
        fprintf(stderr, "[sm64ds] heap init: size=%u root=%p default=%p\\n",
                size, root, Memory::defaultHeapPtr);
        fflush(stderr);
    }
#endif
    GAME_HEAP_PTR = CreateExpandingHeap(size, root, 4);
#if defined(SM64DS_PLATFORM_PC)
    if (trace && trace[0] && trace[0] != '0') {
        fprintf(stderr, "[sm64ds] heap init: game heap=%p\\n", GAME_HEAP_PTR);
        fflush(stderr);
    }
#endif
}
