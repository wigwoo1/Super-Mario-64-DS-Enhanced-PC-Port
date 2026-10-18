//cpp
/* Compiler-emitted below from the written bodies (D0, dBgPi thunks); also
 * SetObjAndLine -- its callers still declare unconverted Vec3* spellings,
 * so its mark stays up here and the definition does not bind over the
 * plurality.
// @symbol _ZN9dBgCh_Lin13SetObjAndLineERK7Vector3S2_P8dActor_c
// @symbol _ZN9dBgCh_LinD0Ev
// @symbol _ZThn16_N9dBgCh_LinD0Ev
// @symbol _ZThn16_N9dBgCh_LinD1Ev
*/
/* dBgCh_Lin -- the line-segment collision query: dBgCh plus a dBgPi hit
 * record at 0x10 plus a non-polymorphic dM3dGLin at 0x38 (the ROM's own
 * __vmi_class_type_info lists all three; _ZTS9dBgCh_Lin @ 0x02099290,
 * _ZTV9dBgCh_Lin @ 0x020992a4, dBgPi secondary VTable_dBgPi_dBgCh_LinThunk
 * @ 0x020992b4). lineEnd is seeded from the line end and overwritten with
 * the world-space collision point on a hit.
 *
 * Deferred codegen emits the plain members in reverse definition order
 * and the lifecycle groups in their fixed order, so the file defines
 * ctor and dtor first. The dBgPi-base destructor thunks (_ZThn16_) are
 * compiler-emitted after C1. */
#include "dBgCh_Lin.h"

extern "C" {
void func_020353b0(void *ray, void *actor);
void func_ov002_020fea4c(int *a, int *b);       /* dM3dGLin::GetEnd */
void func_ov002_020fea68(int *a, int *b);       /* dM3dGLin::GetStart */
Fix12i Vec3_Dist(const Vector3 *a, const Vector3 *b);
}

// @symbol _ZN9dBgCh_LinC1Ev
dBgCh_Lin::dBgCh_Lin()
{
    lineEnd.z = 0;
    lineEnd.y = lineEnd.z;
    lineEnd.x = lineEnd.y;
    clsnDist = 0;
}

// @symbol _ZN9dBgCh_LinD1Ev
dBgCh_Lin::~dBgCh_Lin()
{
}

void dBgCh_Lin::SetObjAndLine(const Vector3 &start, const Vector3 &end,
                              dActor_c *actor)
{
    Vector3 mid;
    dM3dGLin::Set(start, end);
    func_020353b0(this, actor);
    func_02037608();
    mid.x = start.x + end.x;
    mid.y = start.y + end.y;
    mid.z = start.z + end.z;
    mid.x >>= 1;
    mid.y >>= 1;
    mid.z >>= 1;
    mBoundSphere.Set(mid, (clsnDist >> 1) + 0x1000);
}

// @symbol _ZN9dBgCh_Lin13func_02037608Ev
void dBgCh_Lin::func_02037608()
{
    Vector3 a;
    Vector3 b;
    hasClsn = 0;
    /* through the REFERENCE: a pointer-level upcast makes mwcc emit the
       null-checked MI adjustment (movs/addne), the ROM's is unconditional */
    func_ov002_020fea4c((int *)&b, (int *)&(dM3dGLin &)*this);
    lineEnd = b;
    func_ov002_020fea68((int *)&a, (int *)&(dM3dGLin &)*this);
    clsnDist = Vec3_Dist(&lineEnd, &a);
    dBgPi::Reset();
}

// @symbol _ZN9dBgCh_Lin10SetClsnPosERK7Vector3
void dBgCh_Lin::SetClsnPos(const Vector3 &pos)
{
    lineEnd = pos;
}

/* Stays a mangled free definition: a real C++ method returning Vector3 by
   value cannot reproduce this shape -- mwcc builds the temporary on the
   stack instead of writing straight through the hidden return pointer, same
   "wall 6az" class as a Fix12<int> by-value parameter. The declaration in
   dBgCh_Lin.h is the real one and callers may use it.

   Returns lineEnd by value. Named for its role AFTER a hit, when
   SetObjAndLine's collision-point write has overwritten the field; see
   dBgCh_Lin.h for why the storage keeps the name `lineEnd`. */
// @symbol _ZN9dBgCh_Lin10GetClsnPosEv
extern "C" void _ZN9dBgCh_Lin10GetClsnPosEv(Vector3 *res, dBgCh_Lin *self)
{
    res->x = self->lineEnd.x;
    res->y = self->lineEnd.y;
    res->z = self->lineEnd.z;
}
