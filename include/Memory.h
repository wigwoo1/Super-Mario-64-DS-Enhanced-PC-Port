#ifndef MEMORY_H
#define MEMORY_H
#include "types.h"

struct Heap;

namespace Memory {

extern Heap *defaultHeapPtr;
void Deallocate(void *ptr, Heap *heap);

void* operator_new2(u32 size);
void  operator_delete2(void* ptr);

}

#endif
