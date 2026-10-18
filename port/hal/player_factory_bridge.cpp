#include "dActor_c.h"
#include "MaterialChanger.h"
#include "dBgCh_Actr.h"
#include "dCc_c.h"
#include "SaveData.h"
#include "ModelAnim2.h"
#include "Player.h"

#include <new>

extern "C" {
extern unsigned int data_020a4b60[];
extern unsigned short data_020a4b54;
}

extern "C" {
void _ZN8dActor_cC2Ev(void *self)
{
    dActor_c *actor = ::new (self) dActor_c();
    /* The host C++ constructor covers the native object layout and vtables.
       The DS constructor also copies the pending spawn parameter and actor ID
       from the factory context, which native placement-new does not know about. */
    actor->param1 = data_020a4b60[0];
    actor->actorID = data_020a4b54;
}

void *_ZN15MaterialChangerC1Ev(void *self)
{
    return ::new (self) MaterialChanger();
}

void _ZN15MaterialChangerD1Ev(void *self)
{
    static_cast<MaterialChanger *>(self)->~MaterialChanger();
}

void *_ZN10dBgCh_ActrC1Ev(void *self)
{
    return ::new (self) dBgCh_Actr();
}

/* The overlay's ARM/Itanium symbol spellings are called by recovered C
   functions. The native C++ implementations use MSVC member names, so provide
   small ABI bridges for those three edges. */
int _ZN8dActor_c16JumpedOnByPlayerER5dCc_cR6Player(
    dActor_c *self, dCc_c &clsn, Player &player)
{
    return self->JumpedOnByPlayer(clsn, player);
}

int _ZN8SaveData16CanPlayerHaveCapEv(void)
{
    return SaveData::CanPlayerHaveCap();
}

void *_ZN10ModelAnim2C1Ev(void *self)
{
    return ::new (self) ModelAnim2();
}
}
