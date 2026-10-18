#pragma once

// The manual native vtables must accept MSVC's scalar deleting-destructor
// flag. A qualified destructor call avoids dispatching back into the wrapper.
template<class T>
void *pc_native_scalar_delete(void *self, unsigned flags)
{
    static_cast<T *>(self)->T::~T();
    if (flags & 1u) T::operator delete(self);
    return self;
}
