//cpp
/* dCapEnemy_c -- the ov002 half of the class.
 *
 * ov002/dCapEnemy_c  (2 function(s))  .text 0x020aedbc .. 0x020aee40
 *
 * dCapEnemy_c's members do NOT all live in one module. Ten ordinary members,
 * the constructor and D1 are in arm9 (0x02005d94 .. 0x02006588); only the
 * base-object destructor D2 (0x020aedbc) and the deleting destructor D0
 * (0x020aedf4) are here in ov002. This file is therefore the ov002 half only.
 *
 * FUNCTION ORDER IS DELIBERATELY THE REVERSE OF THE ROM'S -- mwccarm 2004/b56
 * emits one .text section per function in the REVERSE of source order, so the
 * highest-address ROM function is written first.
 *
 * The destructor is never defined in this TU: a single out-of-line
 * `~dCapEnemy_c()` would emit the whole D1/D2/D0 group, and the D1 copy --
 * homed in arm9 at 0x0200651c -- comes out STB_GLOBAL, which isolation will
 * not deadstrip as a duplicate. D2 and D0 are spelled out below as two
 * extern "C" ABI bodies instead, D0 then D2 so the reversal emits D2 below
 * D0 as the cartridge has it. With no key function defined the compiler
 * emits no vtable, RTTI or stray D1 here; the cartridge copies stay
 * ROM-provided.
 */
#include "dCapEnemy_c.h"
#include "decl_common.h"
#include "decl_Model.h"

extern "C" void _ZN10dCapIcon_cD1Ev(void *);
extern "C" void _ZN12dEnemyBase_cD2Ev(void *);

/* -------------------------------------------------------------------------- */
/* ROM ordinal 1 -- _ZN11dCapEnemy_cD0Ev, 0x020aedf4, size 0x4c */
/* -------------------------------------------------------------------------- */
// @symbol _ZN11dCapEnemy_cD0Ev
/* The DELETING destructor: store this vtable, destroy the members in reverse
 * declaration order (CapIcon at 0x164, Model at 0x114), chain to
 * dEnemyBase_c's base-object destructor, then hand the object back to the
 * actor heap.
 */
extern "C" dCapEnemy_c *_ZN11dCapEnemy_cD0Ev(dCapEnemy_c *self)
{
    *(int *)self = (int)_ZTV11dCapEnemy_c;
    _ZN10dCapIcon_cD1Ev(&self->mCapIcon);
    _ZN5ModelD1Ev(&self->mModel);
    _ZN12dEnemyBase_cD2Ev(self);
    _ZN6Memory10DeallocateEPvP4Heap(self, data_020a0eac);
    return self;
}

/* -------------------------------------------------------------------------- */
/* ROM ordinal 0 -- _ZN11dCapEnemy_cD2Ev, 0x020aedbc, size 0x38 */
/* -------------------------------------------------------------------------- */
// @symbol _ZN11dCapEnemy_cD2Ev
/* The base-object variant, and the one every derived cap enemy chains to.
 * It is byte-identical to D1 at arm9 0x0200651c -- dCapEnemy_c has no virtual
 * bases -- modulo the three relocated `bl` words, which is why this address
 * spent so long carrying the placeholder name func_ov002_020aedbc while arm9
 * 0x0200651c wore the D2 name that belongs here.
 *
 * Which is which is settled by how the ROM reaches them, not by their bytes:
 * word 16 of _ZTV11dCapEnemy_c (0x021082c4) holds 0x0200651c, so that one is
 * D1, and word 17 holds 0x020aedf4, which is D0. This one has no vtable word
 * at all and is reached only by `bl` from daTrs_c's and daKrb_c's destructors
 * tearing down their base sub-object, which is exactly what D2 is for.
 */
extern "C" dCapEnemy_c *_ZN11dCapEnemy_cD2Ev(dCapEnemy_c *self)
{
    *(int *)self = (int)_ZTV11dCapEnemy_c;
    _ZN10dCapIcon_cD1Ev(&self->mCapIcon);
    _ZN5ModelD1Ev(&self->mModel);
    _ZN12dEnemyBase_cD2Ev(self);
    return self;
}
