//cpp
// @symbol _ZN4Heap8AllocateEji
/* Heap::Allocate(u32, int) at 0x0203c6cc -- the allocation entry point: call
 * slot 3, and if it comes back empty on a fail-fast heap, do not return the
 * NULL to the caller, Crash() instead.
 *
 * RETURN TYPE: void*, from the definition. This file used to declare `int
 * Heap::Allocate(...)' while the sibling overload -- now in
 * src/engine/heap/Heap.cpp and forwarding to this one -- declared the same
 * function `void*'. Two files
 * disagreeing about one signature is exactly the debt the shadow structs
 * create. ExpandingHeap::VAllocate and SolidHeap::VAllocate both return void*,
 * so void* it is, and the two files now agree because they share a header.
 *
 * Was a cast of `this' to a local `struct Base' with three anonymous virtuals
 * padding `m' out to index 3, plus an address-cast read of `&unk_010'. */
#include "decl_common.h"
#include "Heap.h"
#ifdef SM64DS_PLATFORM_PC
#include "Memory.h"
#include <stdio.h>
extern "C" Heap* GAME_HEAP_PTR;
#endif

void* Heap::Allocate(u32 size, int align)
{
    void* block = VAllocate(size, align);
    if (block == 0 && (flags & 0x4000)) {
#ifdef SM64DS_PLATFORM_PC
        fprintf(stderr, "[heap failure] heap=%p default=%p game=%p request=%u align=%d flags=%x arena=%p size=%u parent=%p max=%u left=%u\n",
                this, Memory::defaultHeapPtr, GAME_HEAP_PTR, size, align, flags,
                heapStart, heapSize, parentHeap, VMaxAllocatableSize(), VMemoryLeft());
        fflush(stderr);
#endif
        Crash();
    }
    return block;
}
