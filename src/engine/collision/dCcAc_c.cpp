//cpp
/* dCcAc_c -- collision cylinder attached to an dActor_c. Cartridge class:
   _ZTV7dCcAc_c at 0x0208e6d4, _ZTS7dCcAc_c at 0x0208e668, _ZTI7dCcAc_c at
   0x0208e698 (si_class, base dCc_c). TU claims 0x0201490c..0x02014a20, the
   whole class run in delinks order. Written back-to-front: mwccarm emits
   .text in reverse source order under default deferred codegen. */
#include "dCcAc_c.h"
#include "dActor_c.h"

// @symbol _ZN7dCcAc_cC2Ev
dCcAc_c::dCcAc_c() : owner(0)
{
}

// @symbol _ZN7dCcAc_cD1Ev
dCcAc_c::~dCcAc_c()
{
}

// @symbol _ZN7dCcAc_c6GetPosEv
/* Slot 2. Returns the OWNER's position (dActor_c + 0x5c), not a field of this
   object -- a moving cylinder tracks its dActor_c instead of storing a copy.
   The (Vector3 *)&mPosX pun stays until dActor_c::Pos() is on the shared
   header. */
Vector3 &dCcAc_c::GetPos()
{
    return *(Vector3 *)&owner->mPosX;
}

// @symbol _ZN7dCcAc_c10GetOwnerIDEv
/* Slot 3. owner->uniqueID, at dActor_c + 4 -- the same offset dBgW
   reads for its own ownerUniqueID. */
u32 dCcAc_c::GetOwnerID()
{
    return owner->uniqueID;
}

// @symbol _ZN7dCcAc_c4InitEP8dActor_c5Fix12IiES3_jj
/* Stays a mangled free definition: the real signature carries Fix12<int> and
   wall 6az (notes/mwccarm-codegen.md) homes class-typed by-value parameters.
   The declaration in dCcAc_c.h is the real one and callers may use it. */
extern "C" void _ZN5dCc_c4InitE5Fix12IiES1_jj(dCc_c *self, int radius, int height, u32 flags, u32 vulnFlags);

extern "C" void _ZN7dCcAc_c4InitEP8dActor_c5Fix12IiES3_jj(dCcAc_c *self, dActor_c *actor, int radius, int height, u32 flags, u32 vulnFlags)
{
    self->owner = actor;
    _ZN5dCc_c4InitE5Fix12IiES1_jj(self, radius, height, flags, vulnFlags);
}
