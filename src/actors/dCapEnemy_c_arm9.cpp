//cpp
/* dCapEnemy_c, arm9 half (0x02005d94..0x0200651c).
 *
 * src/actors/dCapEnemy_c.cpp is the ov002 half and already owns that path, so
 * this file is the other module. rombuild refuses one complete path in two
 * modules. D2 and D0 stay in the ov002 TU.
 *
 * D1 (0x0200651c) and C2 (0x02006554) keep their single-function files. The
 * destructor is this class's key function -- a member ~dCapEnemy_c() is the
 * only definition that emits _ZTV11dCapEnemy_c -- and a member dtor or ctor
 * in this multi-function object would also emit D0/D2/C1 as STB_GLOBAL,
 * homed to the ov002 TU, which plan_many cannot deadstrip.
 *
 * Source order is reverse ROM order.
 */
#include "dCapEnemy_c.h"

struct Vector3_16 { s16 x, y, z; };

extern "C" {
extern void *_ZN5Model8LoadFileER13SharedFilePtr(void *sfp);
extern int _ZN9ModelBase7SetFileEP8BMD_Fileii(void *this_, void *file, int a, int b);
extern int _ZN13SharedFilePtr7ReleaseEv(void *sfp);
extern void *data_ov002_020ff028[];

extern unsigned char data_0209f2d8;
extern unsigned char data_0208a0e0;
extern void *data_0209f394[];
extern int *_ZN8dActor_c13ClosestPlayerEv(void *self);

extern struct dActor_c *_ZN8dActor_c5SpawnEjjRK7Vector3PK10Vector3_16as(
    unsigned int a1, unsigned int a2, const struct Vector3 *a3,
    const struct Vector3_16 *a4, int a5, int a6);

extern void Vec3_Add(Vector3 *out, const Vector3 *a, const Vector3 *b);

/* UpdateCapPos's stack temps are POD. Vector3's empty destructor and
   Matrix4x3's embedded Vector3 change this function's frame. */
struct CapVec { int x, y, z; };
struct CapMtx { int a[12]; };
extern void Vec3_Asr(CapVec *vF, const CapVec *v, int amount);
extern void Matrix4x3_FromTranslation(CapMtx *mF, int x, int y, int z);
extern void Matrix4x3_ApplyInPlaceToRotationXYZExt(CapMtx *mF, short x, short y, short z);
extern CapMtx data_020a0e68;
}

// @symbol _ZN11dCapEnemy_c14UnloadCapModelEv
void dCapEnemy_c::UnloadCapModel()
{
    s32 idx = mCapId & 7;
    if (idx >= 6) return;
    _ZN13SharedFilePtr7ReleaseEv(data_ov002_020ff028[idx]);
}

// @symbol _ZN11dCapEnemy_c6AddCapEj
int dCapEnemy_c::AddCap(unsigned int param)
{
    mIsDormant = 0;
    if (param >= 6) {
        mCapId = 6;
        return 6;
    }

    mCapId = (int)param % 3;

    if (param >= 3) {
        mCapBank = 1;
        if (mHadBank1Cap == 0) mHadBank1Cap = 1;
    } else {
        mCapBank = 0;
    }

    void *file = _ZN5Model8LoadFileER13SharedFilePtr(data_ov002_020ff028[mCapId]);
    int ok = _ZN9ModelBase7SetFileEP8BMD_Fileii(&mModel, file, 1, -1);
    if (ok == 0) {
        mCapId = 6;
        return _ZN13SharedFilePtr7ReleaseEv(data_ov002_020ff028[mCapId]);
    }

    *(int *)(((long long)(int)((char *)this + 0xb0))) &= ~1;
    mCapIcon.func_ov001_020ab228((char *)this, mCapId, mHadBank1Cap, mCapBank != 0);

    int result = func_02005e28();
    if (result != 0) {
        unsigned char *p = (unsigned char *)(((long long)(int)((char *)this + 0x113)));
        result = *p | 0x80;
        *p = result;
    }
    return result;
}

// @symbol _ZN11dCapEnemy_c12UpdateCapPosERK7Vector3RK10Vector3_16
void dCapEnemy_c::UpdateCapPos(const Vector3& pos, const Vector3_16& rot)
{
    char* self = (char*)this;
    /* The flags byte is read through an inline cast on each access. A local
       CapFlags* at +0x17f makes mwcc materialize the (non-immediate) offset
       through the literal pool (ldr r3,[pc] / ldrb r0,[r5,r3]) and the
       function grows a word; the cartridge folds it (ldrb r0,[r5,#0x17f]). */
    struct CapFlags {
        unsigned char b0 : 1;
        unsigned char b1 : 1;
        unsigned char b2 : 1;
        unsigned char b3to7 : 5;
    };
    if (((CapFlags*)(self + 0x17f))->b2 != 0 || ((CapFlags*)(self + 0x17f))->b1 == 0) {
        if ((*(unsigned char*)(self + 0x113) & 0xf) < 6) {
            unsigned char* p = (unsigned char*)(((long long)(int)(self + 0x113)));
            *p = *p | 0x80;
        }
    } else {
        unsigned char* p = (unsigned char*)(((long long)(int)(self + 0x113)));
        *p = *p & 7;
    }
    if (*(unsigned char*)(self + 0x113) >= 6) return;

    CapVec sum, asr;
    Vec3_Add((Vector3 *)&sum, (Vector3 *)(self + 0x5c), &pos);
    Vec3_Asr(&asr, &sum, 3);
    Matrix4x3_FromTranslation(&data_020a0e68, asr.x, asr.y, asr.z);
    Matrix4x3_ApplyInPlaceToRotationXYZExt(&data_020a0e68, rot.x, rot.y, rot.z);
    *(CapMtx*)(self + 0x130) = data_020a0e68;
}

// @symbol _ZN11dCapEnemy_c14RenderCapModelEPK7Vector3
void dCapEnemy_c::RenderCapModel(const Vector3 * v)
{
  if((unsigned char)((char*)this)[0x113] >= 6) return;
  struct CapModel {
    virtual void f0(); virtual void f1(); virtual void f2();
    virtual void f3(); virtual void f4();
    virtual void render(const Vector3*);
  };
  CapModel* o=(CapModel*)((char*)&mModel);
  o->render(v);
}

// @symbol _ZN11dCapEnemy_c10ReleaseCapERK7Vector3
struct dActor_c *dCapEnemy_c::ReleaseCap(const Vector3 & v_)
{
    unsigned char *c = (unsigned char *)this;
    const struct Vector3 *v = &v_;
    struct dActor_c *ret = 0;

    if ((mCapId & 7) < 6) {
        func_02005ed8();
        if (mCapId < 6u) {
            CapVec out;
            Vec3_Add((Vector3 *)&out, (const Vector3 *)&mPosX, v);
            ret = dActor_c::Spawn(0x10d, 0x1012 | (mCapId << 8), *(const Vector3 *)&out, (const Vector3_16 *)&mAngleX, mAreaId, -1);
            if (mCapBank != 0) {
                *(unsigned char *)((int)c + 0x113) |= 8;
            } else {
                *(unsigned char *)((int)c + 0x113) |= 0x80;
            }
        } else {
            ret = (struct dActor_c *)c;
        }
    }
    return ret;
}

// @symbol _ZN11dCapEnemy_c16GetCapEatenOffItERK7Vector3
int dCapEnemy_c::GetCapEatenOffIt(const Vector3 & v_)
{
    unsigned char *c = (unsigned char *)this;
    const Vector3 *v = &v_;
    CapVec local;
    unsigned char *p;
    local.x = v->x;
    local.y = v->y;
    local.z = v->z;
    p = (unsigned char *)ReleaseCap(*(const Vector3 *)&local);
    if (p != 0 && p != c) {
        unsigned char idx;
        *(int *)(p + 0xd0) = mEatingPlayer;
        *(unsigned char **)(*(unsigned char **)(c + 0xd0) + 0x360) = p;
        *(int *)(((int)p + 0xb0)) |= 0x20000;
        *(int *)(((int)c + 0xb0)) &= ~0x20000;
        mEatingPlayer = 0;
        if (mCapBank == 0) {
            idx = mCapId & 7;
            mCapIcon.func_ov001_020ab228((char *)c, idx, 0, 0);
        }
        return 1;
    }
    return 0;
}

// @symbol _ZN11dCapEnemy_c15RespawnIfHasCapEv
struct dActor_c * dCapEnemy_c::RespawnIfHasCap()
{
    struct dActor_c *r;
    func_02005ed8();
    if ((((unsigned char *)this)[0x113] & 0xf) >= 6) return 0;
    r = _ZN8dActor_c5SpawnEjjRK7Vector3PK10Vector3_16as(
        actorID, param1,
        (const struct Vector3 *)((unsigned char *)&mPosX),
        (const struct Vector3_16 *)((unsigned char *)&mAngleX),
        mAreaId, -1);
    if (!r) return r;
    *(unsigned char *)((char *)r + 0x111) = 1;
    *(int *)((char *)r + 0xf4) = *(int *)((char *)r + 0xb0);
    *(unsigned char *)((char *)r + 0x108) = 0;
    {
        int *p = (int *)((unsigned long long)((char *)r + 0xb0));
        *p &= ~1;
        *p &= ~0x10000000;
    }
    return r;
}

/* The u64 no-op mask sits on the plain 0xf4 read, not on the mFlags store.
   Demoting that load out of its fold-onto-[r4,#0xf4] value-numbering class is
   what lets the RMW-pool chain lead the tail. The masked mCapId RMW is the
   usual 6g launder. */
// @symbol _ZN11dCapEnemy_c11GetCapStateEv
int dCapEnemy_c::GetCapState() {
    if (mIsDormant == 0) {
        return 2;
    }

    unsigned char val = *(unsigned char *)&data_0209f2d8;
    int check = (val == 1) ? 1 : 0;
    if (check == 0) {
        int *p = _ZN8dActor_c13ClosestPlayerEv(this);
        if (p != 0) {
            int s = mCapId & 7;
            int ps = p[2];
            if (s == ps) {
                return 0;
            }
        }
    }

    unsigned int b = mCapIcon.mFlags;
    b = (b << 0x1e) >> 0x1f;
    if (b == 0) {
        return 0;
    }

    mIsDormant = 0;
    mFlags = *(int *)((long long)(int)((char *)this + 0xf4));
    *(unsigned char *)((int)this + 0x113) &= 7;
    return 1;
}

// @symbol _ZN11dCapEnemy_c21DestroyIfCapNotNeededEv
int dCapEnemy_c::DestroyIfCapNotNeeded()
{
  if (((unsigned char *)this)[0x113] >= 6) return 1;
  if ((int)(data_0209f2d8 == 1) != 0) return 1;
  unsigned char *p = (unsigned char *)_ZN8dActor_c13ClosestPlayerEv(this);
  if (p == 0) { func_02005ed8(); return 0; }
  if (((unsigned char *)this)[0x113] != p[0x6d9]) return 1;
  func_02005ed8();
  return 0;
}

// @symbol _ZN11dCapEnemy_c13func_02005ed8Ev
void dCapEnemy_c::func_02005ed8() {
    if ( ((unsigned)((unsigned char *)this)[0x17f] << 29) >> 31 )
        return;
    mCapIcon.Unlink();
}

// @symbol _ZN11dCapEnemy_c13func_02005e28Ev
int dCapEnemy_c::func_02005e28()
{
    int b;
    int i;
    int n;
    b = (int)(data_0209f2d8 == 1);
    if (b != 0) {
        n = data_0208a0e0;
        for (i = 0; i < n; i++) {
            int *p = (int*)data_0209f394[i];
            if (p != 0) {
                if (p[2] == ((unsigned char *)this)[0x113]) return 1;
            }
        }
        return 0;
    } else {
        int *p = _ZN8dActor_c13ClosestPlayerEv(this);
        int r1;
        if (p != 0) r1 = p[2];
        else r1 = ((unsigned char *)this)[0x113];
        return (r1 == ((unsigned char *)this)[0x113]) ? 1 : 0;
    }
}

// @symbol _ZN11dCapEnemy_c12Unk_02005d94Ev
void dCapEnemy_c::Unk_02005d94()
{
    int b1;
    int b2;
    if (!(((unsigned char *)this)[0x113] & 0x80)) return;
    b1 = (mFlags & 8) ? 1 : 0;
    if (b1 == 0) {
        volatile unsigned char *q = &data_0209f2d8;
        b2 = (q[0] == 1) ? 1 : 0;
        if (b2 == 0) return;
    }
    *(unsigned char *)((int)((unsigned char *)this) + 0x113) &= 7;
    if (func_02005e28())
        *(unsigned char *)((int)((unsigned char *)&mCapId)) |= 0x80;
}
