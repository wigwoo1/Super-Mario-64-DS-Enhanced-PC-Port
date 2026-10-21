// Production behavior bridges for small player/runtime helpers used by the
// actor slice. The names retain the decomp symbol spelling at C call sites.
#include "Player.h"
#include "dCc_c.h"
#include "SharedFilePtr.h"
#include <cstring>
#include <new>

extern "C" {
extern unsigned char data_0209caa0[];
extern unsigned char data_0209f2d8;
extern unsigned char data_0209f2f8;
extern short data_02092144[];
extern dCc_c *data_0209cee8;
dCc_c *data_0209cee8 = nullptr;

void _ZN15dExtFrameCtrl_c8LoadFileER13SharedFilePtr(void *ptr)
{
    dExtFrameCtrl_c::LoadFile(*static_cast<SharedFilePtr *>(ptr));
}

#if !defined(SM64DS_GAME_TARGET)
// The small actor smoke slice has no camera. The shipping game must resolve
// this name to dCamera_c.cpp's original player-scoped state restoration.
void func_0200d81c(void *, int) {}
#endif
}

extern "C" {
int _ZN8SaveData16HasPlayerLostCapEv(void)
{
    if (data_0209f2d8 == 1) return 0;
    const unsigned character = data_0209caa0[0x41];
    if (character == 3) return 0;
    const unsigned flags1 = *reinterpret_cast<unsigned *>(data_0209caa0 + 4);
    return (flags1 & (0x01000000u << character)) != 0;
}

int _ZN6Player4HealEi(void *self, int amount)
{
    return static_cast<Player *>(self)->Player::Heal(amount);
}

int _ZN6Player9GetHealthEv(void *self)
{
    return static_cast<Player *>(self)->Player::GetHealth();
}

#if !defined(SM64DS_GAME_TARGET)
void *_Znwj(unsigned int size)
{
    return ::operator new(size, std::nothrow);
}
#endif

int _ZN4cstd6strcmpEPKcS1_(const char *a, const char *b)
{
    return std::strcmp(a, b);
}

}

#if !defined(SM64DS_GAME_TARGET)
int Player::GetHealth()
{
    return static_cast<signed char>(data_02092144[mPlayerNo] >> 8);
}

int Player::IsInAir()
{
    return mIsAirborne ? (mIsInAirState != 0) : 0;
}

void dCc_c::Unlink()
{
    if (prev) prev->next = next;
    else if (data_0209cee8 == this) data_0209cee8 = next;
    if (next) next->prev = prev;
    prev = nullptr;
    next = nullptr;
}

extern "C" void func_0206e330(void *dst, int val, unsigned int n);
extern "C" void *func_0206e2f8(void *dst, int val, unsigned int n)
{
    func_0206e330(dst, val, n);
    return dst;
}
#endif

#pragma comment(linker, "/alternatename:?_ZN6Player4HurtERK7Vector3j5Fix12IiEjjj@@YAHPAX0IHIII@Z=__ZN6Player4HurtERK7Vector3j5Fix12IiEjjj")
#pragma comment(linker, "/alternatename:?data_ov002_021101cc@@3UState@@A=_data_ov002_021101cc")

