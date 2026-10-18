// Side-effect boundary for smoke_actor only. These calls are not included in
// any prospective game target; the native audio backend remains separate.
#include "types.h"

extern "C" void smoke_sound_play_bank0(unsigned int, void *) {}
extern "C" bool smoke_sound_play_sub(unsigned int, unsigned int,
                                      unsigned int, int, bool) { return false; }
#pragma comment(linker, "/alternatename:__ZN5Sound9PlayBank0EjRK7Vector3=_smoke_sound_play_bank0")
#pragma comment(linker, "/alternatename:__ZN5Sound7PlaySubEjjj5Fix12IiEb=_smoke_sound_play_sub")

namespace Sound {
void PlayBank3(unsigned int, const Vector3 &) {}
void PlayBank0(unsigned int, const Vector3 &) {}
}

extern "C" {
void _ZN5Sound13PlayCharVoiceEjjRK7Vector3(unsigned int, unsigned int, void *) {}
void func_02012694(unsigned int, const Vector3 *) {}
}

// These old call sites use the Itanium-style symbol spelling as a C++ name.
void _ZN5Sound9PlayBank0EjRK7Vector3(unsigned int, void *) {}
bool _ZN5Sound7PlaySubEjjj5Fix12IiEb(unsigned int, unsigned int,
                                     unsigned int, int, bool) { return false; }
