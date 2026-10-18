//cpp
/* engine/collision/dBgPc.cpp — dBgPc TU (arm9 0x02037ee4..0x02037f44)
 *
 * dBgPc is dBgPi's non-polymorphic base (the SurfaceInfo record at +0x04);
 * the cartridge's RTTI records that hierarchy but the class itself has no
 * vtable. The TU is the two member definitions below: mwccarm emits the
 * destructor as D2+D1 (D0 also emits and deadstrips — nothing ever deletes
 * a dBgPc) and the constructor as C1+C2, byte-identical here because a
 * non-polymorphic base has no vptr to store.
 */
#include "dBgPc.h"

/* The constructor is defined before the destructor so the deferred
 * emission order lands the D2,D1 group below the C1,C2 group, matching
 * the ROM's layout (mwccarm emits .text in reverse source order). */
// @symbol _ZN5dBgPcC1Ev
// @symbol _ZN5dBgPcC2Ev
dBgPc::dBgPc()
{
    surface.clps.w0 = 0xfc0;
    surface.clps.w1 = 0xff;
    surface.normal.z = 0;
    surface.normal.y = surface.normal.z;
    surface.normal.x = surface.normal.y;
}

// @symbol _ZN5dBgPcD2Ev
// @symbol _ZN5dBgPcD1Ev
dBgPc::~dBgPc()
{
}
