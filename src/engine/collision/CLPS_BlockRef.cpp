//cpp
/* engine/collision/CLPS_BlockRef.cpp — CLPS_BlockRef TU
 * (arm9 0x0203821c..0x02038234)
 *
 * CLPS_BlockRef is the one-word value handle dBgW_Kc carries: a raw
 * CLPS_Block pointer with value semantics. The TU is the three member
 * definitions below; mwccarm emits the constructor's C1,C2 pair and the
 * destructor's D2,D1 pair (no D0 -- nothing deletes a CLPS_BlockRef, and
 * the type is non-polymorphic). Source order is constructor, destructor,
 * then operator= so the deferred emission order lands the ROM's
 * operator=, D1, C1 layout (mwccarm emits .text in reverse source order).
 */
#include "CLPS_BlockRef.h"

// @symbol _ZN13CLPS_BlockRefC1Ev
CLPS_BlockRef::CLPS_BlockRef() : ptr(0) {}

// @symbol _ZN13CLPS_BlockRefD1Ev
CLPS_BlockRef::~CLPS_BlockRef() {}

// @symbol _ZN13CLPS_BlockRefaSER10CLPS_Block
CLPS_BlockRef &CLPS_BlockRef::operator=(CLPS_Block &block)
{
    ptr = &block;
    return *this;
}
