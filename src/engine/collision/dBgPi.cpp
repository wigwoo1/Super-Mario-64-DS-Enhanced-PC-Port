//cpp
// @symbol _ZNK5dBgPi9GetClsnIDEv
// @symbol _ZNK5dBgPi16GetColliderIndexEv
// @symbol _ZN5dBgPi16SetColliderIndexEi
// @symbol _ZNK5dBgPi7IsHitByEiP8dActor_cP4dBgW
// @symbol _ZNK5dBgPi7IsValidEv
// @symbol _ZNK5dBgPi11HasColliderEv
// @symbol _ZN5dBgPi9RecordHitEsP11SurfaceInfo
// @symbol _ZN5dBgPi11SetColliderEiiP8dActor_cP4dBgW
// @symbol _ZN5dBgPiaSERKS_
// @symbol _ZNK5dBgPi6CopyToERS_
// @symbol _ZN5dBgPi5ResetEv
// @symbol _ZN5dBgPiD2Ev
// @symbol _ZN5dBgPiD0Ev
// @symbol _ZN5dBgPiD1Ev
// @symbol _ZN5dBgPiC1Ev
// @symbol _ZN5dBgPiC2Ev
/* dBgPi -- a polygon hit produced by the background-collision system:
 * the surface record (dBgPc base at 0x04) plus which triangle, which
 * registered collider and which actor were involved, 0x28 bytes.
 * ROM's own RTTI name (_ZTS5dBgPi @ 0x0209920c, vtable _ZTV5dBgPi @
 * 0x02099368). The three checkers dBgCh_Gnd/Lin/SphCrr each embed one
 * as their result sub-object at 0x10.
 *
 * Deferred codegen emits the plain members in reverse definition order
 * and each lifecycle group in its fixed order (D2,D0,D1 / C1,C2), so
 * the file defines ctor and dtor first. The accessors between GetClsnID
 * and Reset were enrolled as anonymous func_ shards; they are real
 * members now -- colliderIdx/owner/collider are what the DetectClsn
 * loops record via SetCollider, and IsHitBy re-checks that identity
 * against the collider table.
 *
 * The vtable/RTTI are emitted (the destructor is the key function) and
 * licensed deadstrip-data against their canonical homes.
 *
 * Leftovers: RecordHit still calls the five-word surface copy by its
 * func_ name -- the helper belongs to the SurfaceInfo TU next door.
 */
#include "dBgPi.h"

extern "C" void func_02037de8(int *dst, int *src);

dBgPi::dBgPi()
{
    Reset();
}

dBgPi::~dBgPi()
{
}

void dBgPi::Reset()
{
    triangleID = 0xffff;
    colliderIdx = 0x18;
    clsnID = -1;
    owner = 0;
    collider = 0;
}

void dBgPi::CopyTo(dBgPi &dst) const
{
    *reinterpret_cast<u64 *>(&dst.surface.clps) =
        *reinterpret_cast<const u64 *>(&surface.clps);
    dst.surface.normal.x = surface.normal.x;
    dst.surface.normal.y = surface.normal.y;
    dst.surface.normal.z = surface.normal.z;
    dst.triangleID = triangleID;
    dst.colliderIdx = colliderIdx;
    dst.clsnID = clsnID;
    dst.owner = owner;
    dst.collider = collider;
}

dBgPi &dBgPi::operator=(const dBgPi &other)
{
    /* CLPS is one 64-bit record. This spelling also preserves its two loads
       before either store when source and destination might overlap. */
    *reinterpret_cast<u64 *>(&surface.clps) =
        *reinterpret_cast<const u64 *>(&other.surface.clps);
    surface.normal.x = other.surface.normal.x;
    surface.normal.y = other.surface.normal.y;
    surface.normal.z = other.surface.normal.z;
    triangleID = other.triangleID;
    colliderIdx = other.colliderIdx;
    clsnID = other.clsnID;
    owner = other.owner;
    collider = other.collider;
    return *this;
}

void dBgPi::SetCollider(int idx, int uid, dActor_c *actor, dBgW *clsn)
{
    SetColliderIndex(idx);
    clsnID = uid;
    owner = actor;
    collider = clsn;
}

void dBgPi::RecordHit(s16 triID, SurfaceInfo *src)
{
    triangleID = triID;
    func_02037de8(reinterpret_cast<int *>(&surface), reinterpret_cast<int *>(src));
}

int dBgPi::HasCollider() const
{
    return colliderIdx != 0x18;
}

int dBgPi::IsValid() const
{
    return colliderIdx != 0x18 && triangleID != 0xffff;
}

int dBgPi::IsHitBy(int uid, dActor_c *actor, dBgW *clsn) const
{
    return clsnID == uid && owner == actor && collider == clsn;
}

void dBgPi::SetColliderIndex(int idx)
{
    colliderIdx = idx;
}

int dBgPi::GetColliderIndex() const
{
    return colliderIdx;
}

u32 dBgPi::GetClsnID() const
{
    return clsnID;
}
