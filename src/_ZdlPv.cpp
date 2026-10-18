//cpp
// @symbol _ZdlPv
/* operator delete(void*) at 0x0203cbf0 -- global deallocator. Loads
 * Memory::defaultHeapPtr and tail-calls Heap::_Deallocate(ptr).
 *
 * Language-mode migration: the compiler mangles `operator delete(void*)` to
 * `_ZdlPv`. The library declaration carries `throw()`, so the definition must
 * match that exception-spec. `#pragma exceptions off` keeps this a five-
 * instruction tail call under both match.py's default C++ flags and the ROM
 * build's `-Cpp_exceptions off` (without it, match flags emit EH glue and the
 * body grows to 0x44).
 *
 * Heap::_Deallocate is a three-word veneer to Heap::Deallocate; see
 * include/Heap.h. */
#pragma exceptions off
#include "Heap.h"
#if defined(SM64DS_PLATFORM_PC)
#include "Memory.h"
#include <cstdlib>
extern "C" int pc_game_arena_contains(const void *);
#endif

namespace Memory {
extern Heap* defaultHeapPtr;
#if defined(SM64DS_PLATFORM_PC)
void Deallocate(void *ptr);
#endif
}

void operator delete(void* ptr) throw()
{
#if defined(SM64DS_PLATFORM_PC)
    if (!ptr) return;
    if (pc_game_arena_contains(ptr)) Memory::Deallocate(ptr);
    else std::free(ptr);  // PC global new and native STL use the CRT allocator.
#else
    Memory::defaultHeapPtr->_Deallocate(ptr);
#endif
}
