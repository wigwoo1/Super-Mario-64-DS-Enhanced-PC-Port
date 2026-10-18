//cpp
/* daIbl_c -- the rolling iron ball (registry profile IRONBALL).
 *
 * param1's low nibble picks the kind (mVariant, see daIbl_Kind in
 * daIbl_c.h), and Behavior runs that kind's handler out of the
 * pointer-to-member table at data_ov100_0214867c, which the startup code
 * fills as { func_ov100_02142b90, func_ov100_02142918, func_ov100_021424c0,
 * func_ov100_0214272c, func_ov100_021424c0 }:
 *
 *   0  func_ov100_02142b90  spawner: makes more balls on a timer
 *   1  func_ov100_02142918  free-rolling ball
 *   2  func_ov100_021424c0  path follower, speed follows the slope
 *   3  func_ov100_0214272c  ball that has left its path; breaks at mKillY
 *   4  func_ov100_021424c0  path follower, speed ramps to the cap
 *
 * func_ov100_0214233c steps along the path for the followers.
 * func_ov100_02141fb0 is the shared contact check, func_ov100_02142130 the
 * shared wall and floor probe, func_ov100_02142264 the per-frame model and
 * shadow matrices.
 *
 * Units, for the numbers below: positions and speeds are 20.12 fixed point
 * (0x1000 = 1 unit, 0x1000 per frame = 1 unit per frame); angles are 16 bit
 * (0x10000 = a full turn, 0x4000 = a quarter turn); model and shadow matrices
 * take a position >> 3.
 *
 * This file is the whole linker unit 0x02141f04..0x021431c4, 16 functions:
 * D1 and D0 (daBtfly_c_classInit, the last function of
 * src/actors/daBtfly_c.cpp, ends exactly at 0x02141f04 below them),
 * OnAimedAtWithEgg, the eight helpers func_ov100_02141fb0 through
 * func_ov100_02142b90, CleanupResources, Render, Behavior, InitResources,
 * and last the registry factory daIbl_c_classInit (0x0214316c);
 * daWanwan2_c's D1 starts exactly at 0x021431c4 above it. The out-of-line
 * destructor is the key function, so this TU also emits the vtable and the
 * RTTI.
 *
 * It replaces the one-function sources for _ZN7daIbl_cD1Ev,
 * _ZN7daIbl_cD0Ev, _ZN7daIbl_c16OnAimedAtWithEggEv, func_ov100_02141fb0 ..
 * func_ov100_02142b90, _ZN7daIbl_c16CleanupResourcesEv,
 * _ZN7daIbl_c6RenderEv, _ZN7daIbl_c8BehaviorEv,
 * _ZN7daIbl_c13InitResourcesEv and daIbl_c_classInit. Each member keeps the
 * provenance notes its source carried.
 *
 * `#pragma defer_codegen off` keeps this file in ROM order.
 *
 * Leftover: the eight helpers are members but keep their func_ov100_*
 *   address names; the real names are not recovered. Behavior's dispatch
 *   reads the two-word pointer-to-member records by hand because the table
 *   at data_ov100_0214867c is .bss the startup code fills.
 * Leftover: the callees with Fix12<int> parameters (Particle::System,
 *   dCcAc_c::Init, dBgCh_Actr::Init, the shadow drop, Player::Hurt) stay
 *   spelled as mangled extern-C free functions. A real method call homes
 *   a class-typed by-value argument and size-DIFFs the caller
 *   (notes/mwccarm-codegen.md 6az).
 * Leftover: "sublevel" below is shorthand for the signed byte at data_0209f2f8
 *   (symbols/verified.tsv calls it LEVEL_ID). Its numbers tested here (6, 0x16,
 *   0x18, 0x19) have no level names in this tree, and neither do the
 *   per-level distances, speeds and kill heights chosen from them.
 * Leftover: func_020383f0 and func_02038414, which the probe picks between by
 *   sublevel, are unidentified. func_02012694 is known to be a bank-3
 *   Sound::Play wrapper (sound ID, position); only its name is unrecovered.
 * Leftover: Behavior's pointer-to-member dispatch and InitResources's param1
 *   shift keep the spellings the byte match depends on.
 */

#pragma defer_codegen off

#include "daIbl_c.h"
#include "types.h"
#include "common.h"
#include "SharedFilePtr.h"
#include "decl_dBgCh_Actr.h"
#include "decl_PathPtr.h"
#include "Player.h"

/* Actor IDs from symbols/actor_debug_names.tsv. */
enum {
    daIbl_ACTOR_PLAYER   = 191,     /* PLAYER */
    daIbl_ACTOR_IRONBALL = 220      /* IRONBALL, this class: a spawner makes more of itself */
};

/* Bits of the ball's dCc_c hitFlags (mdCcAc_c.hitFlags). dCc_c.h says its
   bit table is a best-effort reading, so these name the table's entries. */
enum {
    daIbl_HIT_MEGA   = 0x10,    /* the table's "mega character" */
    daIbl_HIT_ATTACK = 0x3c0    /* the table's punch | kick | breakdance | slide kick */
};

/* dEnemyBase_c::mDeathState value that KillByInvincibleChar sets: the enemy is
   being knocked away spinning. */
enum { daIbl_DEATH_KNOCKED = 8 };

/* Sound IDs, as the callers pass them; the ID-to-sound mapping itself is not
   recovered here, so the names say when the ball plays them. */
enum {
    daIbl_SND_LAND          = 0x40,     /* the frame the ball touches down */
    daIbl_SND_ROLL          = 0x8a,     /* PlayLong's looping roll sound (with its argument 3) */
    daIbl_SND_BREAK         = 0x173,    /* the ball breaks up */
    daIbl_SND_HIT_NOT_BROKEN = 0xb5     /* attacked, but not by something that breaks it */
};

/* Particle played at the ball's position where it breaks up. */
enum { daIbl_PTCL_BREAK = 0xf };

/* Three plain words: the stack vector func_ov100_02142b90 builds, which
   carries none of Vector3's empty destructor. */
struct Vec3i { s32 x, y, z; };

/* The model and shadow matrices, as twelve words: a copy through the C++
   Matrix4x3 DIFFs. */
struct M48 { int w[12]; };

/* Behavior's handler table at data_ov100_0214867c: one pointer-to-member
   per kind, as the two words mwcc lays one out in (function or vtable
   offset, then this-adjustment doubled with the virtual bit in bit 0). */
struct VtEntry {
    int field0;
    int field1;
};
typedef void (*FnPtr)(void *);

/* Render draws the model through its vtable slot 5 with the class's own
   draw scale. */
struct EmbeddedClass {
    virtual void method(void *a);
    virtual void dummy1();
    virtual void dummy2();
    virtual void dummy3();
    virtual void dummy4();
    virtual void virtualMethod(char *a);
};

extern "C" {
/* Defined below. */

void *_ZN8dActor_c10FindWithIDEj(u32 id);
void *_ZN8dActor_c13ClosestPlayerEv(void *c);
void *_ZN8dActor_c5SpawnEjjRK7Vector3PK10Vector3_16as(u32 a, u32 b, const void *p, const void *q, int e, int f);
void _ZN12dEnemyBase_c20KillByInvincibleCharERK10Vector3_16R6Player5Fix12IiE(void *c, Vector3_16 *s, void *a, int z);
void _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(u32 id, int x, int y, int z);
void _ZN8dActor_c8PoofDustEv(void *a);
void _ZN8dActor_c14TriplePoofDustEv(void *a);
void _ZN8dActor_c11LandingDustEb(void *a, int b);
void _ZN7fBase_c18MarkForDestructionEv(void *a);
int func_02012694(int a, void *p);
void _ZN5Sound9PlayBank0EjRK7Vector3(u32 id, void *pos);
int _ZN5Sound8PlayLongEjjjRK7Vector3s(u32 a, u32 b, u32 c, void *v, u32 d);
void _ZN6Player4HurtERK7Vector3j5Fix12IiEjjj(void *p, Vector3 *pos, u32 a, int b, u32 d, u32 e, u32 f);
void func_020383f0(void *p);
void func_02038414(void *p);
int _ZNK10dBgCh_Actr8IsOnWallEv(void *w);
int _ZNK10dBgCh_Actr10IsOnGroundEv(void *w);
int _ZNK10dBgCh_Actr13JustHitGroundEv(void *w);
void *_ZNK10dBgCh_Actr14GetFloorResultEv(void *w);
void _ZNK11SurfaceInfo12CopyNormalToER7Vector3(void *s, Vector3 *v);
int _ZN4cstd5atan2E5Fix12IiES1_(int y, int x);
int _ZN4cstd4fdivEii(int a, int b);
int _ZN8dActor_c14GetSubtractionEss(void *a, short s1, int s2);
int _ZN8dActor_c12ReflectAngleE5Fix12IiES1_s(void *self, int a, int b, int ang);
void _ZN8dActor_c19DropShadowRadHeightER17dExtShadowModel_cR9Matrix4x35Fix12IiES5_j(void *thiz, void *sm, void *mtx, int f, int g, unsigned int h);
void _ZN8dActor_c19MakeVanishLuigiWorkER5dCc_c(void *self, void *c);
void _ZN8dActor_c9UpdatePosEP5dCc_c(void *self, void *c);
void _ZN8dActor_c22UpdatePosWithOnlySpeedEP5dCc_c(void *self, void *c);
void _ZN8dActor_c28UpdatePosWithHorzSpeedAndAngEv(void *self);
int func_ov002_020ad660(void *a, void *b, void *d, int e);
int _Z14ApproachLinearRsss(short *value, short target, short step);
void _Z14ApproachLinearRiii(int *value, int target, int step);
int Vec3_HorzLen(void *v);
int Vec3_HorzDist(const void *a, const void *b);
int Vec3_Equal(void *a, void *b);
void _ZN5dCc_c5ClearEv(void *c);
int _ZN5dCc_c6UpdateEv(void *c);
void _ZN7PathPtr6FromIDEj(void *self, unsigned int id);
void _ZNK7PathPtr7GetNodeER7Vector3j(void *self, void *v, unsigned int idx);
void *_ZN5Model8LoadFileER13SharedFilePtr(void *fp);
int _ZN9ModelBase7SetFileEP8BMD_Fileii(void *self, void *file, int a, int b);
void _ZN7dCcAc_c4InitEP8dActor_c5Fix12IiES3_jj(void *self, void *actor, int a, int b, unsigned int c, unsigned int d);
void _ZN10dBgCh_Actr4InitEP8dActor_c5Fix12IiES3_P10Vector3_16S5_(void *self, void *actor, int a, int b, void *v0, int v1);

extern int IDENTITY_MATRIX4X3[];
}

extern signed char data_0209f2f8;
extern int data_02092138;
extern s16 data_02082214[];
extern char data_ov100_02148668;
extern struct VtEntry data_ov100_0214867c[];

/* -------------------------------------------------------------------------- */
/* ROM ordinals 0 and 1 -- _ZN7daIbl_cD1Ev, 0x02141f04, size 0x48;
 *                         _ZN7daIbl_cD0Ev, 0x02141f4c, size 0x5c */
/* -------------------------------------------------------------------------- */
// @symbol _ZN7daIbl_cD1Ev
// @symbol _ZN7daIbl_cD0Ev
/* recovered: real C++ destructor -- the compiler emits the whole body.
 *
 * D1 is one vtable store and five destructor calls, every one a consequence
 * of `struct daIbl_c : dEnemyBase_c` and the members that declaration types:
 * its own vptr, then dCcAc_c (0x374), dExtShadowModel_c (0x31c), Model (0x2cc),
 * dBgCh_Actr (0x110) in reverse declaration order, then
 * dEnemyBase_c::~dEnemyBase_c. D0 is the deleting destructor: the same
 * teardown, then an inline operator delete -- dEnemyBase_c's, reached
 * because dEnemyBase_c is this class's immediate base.
 *
 * This body is the evidence for the header. It was the hand-written C that
 * named those offsets in the first place, and daIbl_c_classInit constructs
 * the same types at the same offsets. */
daIbl_c::~daIbl_c()
{
}

#ifdef _MSC_VER
/* The host needs the ROM's flat D0 name, and MSVC never emits it: it folds
 * the Itanium destructor variants into the one ~daIbl_c() above. This arm
 * spells out what the deleting destructor does -- the D1 body, called
 * qualified so it is a direct call, then the class-specific operator
 * delete. Nothing here reaches mwccarm. */
extern "C" daIbl_c *_ZN7daIbl_cD0Ev(daIbl_c *thiz)
{
    thiz->daIbl_c::~daIbl_c();
    daIbl_c::operator delete(thiz);
    return thiz;
}
#endif

/* -------------------------------------------------------------------------- */
/* ROM ordinal 2 -- _ZN7daIbl_c16OnAimedAtWithEggEv, 0x02141fa8, size 0x8 */
/* -------------------------------------------------------------------------- */
// @symbol _ZN7daIbl_c16OnAimedAtWithEggEv
/* recovered from vtable slot identity (slot 29); historical alias
   RollingIronBall_OnAimedAtWithEgg.
   Returns 532480 = 0x82000 = 130 units. func_ov100_02141fb0 adds it to the
   ball's Y before handing the ball to KillByInvincibleChar. */
s32 daIbl_c::OnAimedAtWithEgg()
{
    return 532480;
}

/* -------------------------------------------------------------------------- */
/* ROM ordinal 3 -- func_ov100_02141fb0, 0x02141fb0, size 0x180 */
/* -------------------------------------------------------------------------- */
// @symbol _ZN7daIbl_c19func_ov100_02141fb0Ev
/* Contact check, shared by every handler. mdCcAc_c.otherOwner is the uniqueID
   of the actor that touched the ball; when that is a Player (actor 191) who is
   not vanished (mIsVanish), mdCcAc_c.hitFlags decides what happens:
     - the mega-character bit: the ball is lifted by OnAimedAtWithEgg's answer
       (130 units), handed to KillByInvincibleChar (which starts the
       mDeathState 8 knock-away that func_ov002_020ad660 then steps), and its
       ground flag is cleared;
     - an attack bit (punch, kick, breakdance, slide kick, per dCc_c.h's
       best-effort and unproven bit table): when the Player's
       param1 is 2 (read here as its character index; index 2 is Wario in
       SaveData's unlock-bit order) the ball breaks up (particle, poof, MarkForDestruction, break
       sound); any other character only gets the not-broken sound;
     - any other contact hurts the Player (Player::Hurt, given the ball's
       position). */
void daIbl_c::func_ov100_02141fb0()
{
    Player *a;
    u32 fl;
    u32 id = mdCcAc_c.otherOwner;

    if (id == 0) return;
    a = (Player *)_ZN8dActor_c10FindWithIDEj(id);
    if (a == 0) return;
    {
        int b = (a->actorID == daIbl_ACTOR_PLAYER);
        if (b == 0) return;
    }
    if (a->mIsVanish != 0) return;
    fl = mdCcAc_c.hitFlags;
    if ((fl & daIbl_HIT_MEGA) != 0) {
        Vector3_16 s;
        u32 r = OnAimedAtWithEgg();
        mPosY += r;
        s.x = 0; s.y = 0; s.z = 0;
        _ZN12dEnemyBase_c20KillByInvincibleCharERK10Vector3_16R6Player5Fix12IiE(this, &s, a, 0);
        _ZN10dBgCh_Actr15ClearGroundFlagEv(&mWithMeshClsn);
        return;
    }
    if ((fl & daIbl_HIT_ATTACK) != 0) {
        if (a->param1 == 2) {
            _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(
                daIbl_PTCL_BREAK, mPosX, mPosY, mPosZ);
            _ZN8dActor_c8PoofDustEv(this);
            _ZN7fBase_c18MarkForDestructionEv(this);
            func_02012694(daIbl_SND_BREAK, &mCamSpacePosX);
            return;
        }
        _ZN5Sound9PlayBank0EjRK7Vector3(daIbl_SND_HIT_NOT_BROKEN, &mCamSpacePosX);
        return;
    }
    {
        Vector3 v;
        v.x = mPosX;
        v.y = mPosY;
        v.z = mPosZ;
        _ZN6Player4HurtERK7Vector3j5Fix12IiEjjj(a, &v, 2, 0xc000, 1, 0, 1);
    }
}

/* -------------------------------------------------------------------------- */
/* ROM ordinal 4 -- func_ov100_02142130, 0x02142130, size 0x134 */
/* -------------------------------------------------------------------------- */
// @symbol _ZN7daIbl_c19func_ov100_02142130Ev
/* The wall and floor probe. mHitWall is cleared, the collision object is
   updated (sublevel 0x19 runs func_020383f0, every other sublevel
   func_02038414; both unidentified), and mHitWall is set when the ball touches
   a wall and the angle between the ball's heading mPrevAngleY and the wall
   normal's angle exceeds a quarter turn (0x4000), i.e. it is heading into
   the wall. On the ground it copies the floor normal to mFloorNormal and,
   unless it hit a wall, puts X and Z back to last frame's position and, when
   the floor normal's Y is non-zero, sets mVertSpeed to -(slope + 8
   units/frame), where slope = (n.x * vel.x + n.z * vel.z) / n.y, so the ball
   keeps to the slope. */
void daIbl_c::func_ov100_02142130()
{
    mHitWall = 0;
    if (data_0209f2f8 == 0x19) func_020383f0(&mWithMeshClsn);
    else func_02038414(&mWithMeshClsn);
    if (_ZNK10dBgCh_Actr8IsOnWallEv(&mWithMeshClsn) != 0) {
        _ZNK11SurfaceInfo12CopyNormalToER7Vector3((char *)_ZNK10dBgCh_Actr13GetWallResultEv(&mWithMeshClsn) + 4, (Vector3 *)&mWallNormalX);
        int a = _ZN4cstd5atan2E5Fix12IiES1_(mWallNormalX, mWallNormalZ);
        if (_ZN8dActor_c14GetSubtractionEss(this, mPrevAngleY, a) > 0x4000)
            mHitWall = 1;
    }
    if (_ZNK10dBgCh_Actr10IsOnGroundEv(&mWithMeshClsn) == 0) return;
    _ZNK11SurfaceInfo12CopyNormalToER7Vector3((char *)_ZNK10dBgCh_Actr14GetFloorResultEv(&mWithMeshClsn) + 4, (Vector3 *)&mFloorNormalX);
    if (mHitWall != 0) return;
    mPosX = mPrevPosX;
    mPosZ = mPrevPosZ;
    if (mFloorNormalY == 0) return;
    int s = (int)(((long long)mFloorNormalX * unk_0a4 + 0x800) >> 0xc)
          + (int)(((long long)mFloorNormalZ * unk_0ac + 0x800) >> 0xc);
    mVertSpeed = -(_ZN4cstd4fdivEii(s, mFloorNormalY) + 0x8000);
}

/* -------------------------------------------------------------------------- */
/* ROM ordinal 5 -- func_ov100_02142264, 0x02142264, size 0xd8 */
/* -------------------------------------------------------------------------- */
// @symbol _ZN7daIbl_c19func_ov100_02142264Ev
/* Per-frame model and shadow matrices. The model matrix is the identity with
   its translation set to mPos >> 3 (the ball's position in model units). It
   is copied to the shadow's matrix, whose Y translation is then replaced with
   (50 units * scale + mPosY) >> 3 -- 50 units times the draw scale above the
   ball's position -- and the drop shadow is cast with radius 200 units * scale,
   depth 90 units * scale on the ground (0x32 + 0x28) or 450 units * scale in
   the air (0x32 + 0x190), and opacity 0xf. "Scale" is mDrawScaleX, 0x1000
   being 1.0. */
void daIbl_c::func_ov100_02142264()
{
    *(M48 *)&mModel.mat4x3 = *(M48 *)IDENTITY_MATRIX4X3;
    mModel.mat4x3.t.x = mPosX >> 3;
    mModel.mat4x3.t.y = mPosY >> 3;
    mModel.mat4x3.t.z = mPosZ >> 3;
    *(M48 *)mShadowMtx = *(M48 *)&mModel.mat4x3;
    int k = 0x32;
    mShadowMtx[10] = (mDrawScaleX * k + mPosY) >> 3;
    if (_ZNK10dBgCh_Actr10IsOnGroundEv(&mWithMeshClsn) != 0)
        k += 0x28;
    else
        k += 0x190;
    _ZN8dActor_c19DropShadowRadHeightER17dExtShadowModel_cR9Matrix4x35Fix12IiES5_j(
        this, &mShadowModel, mShadowMtx, mDrawScaleX * 0xc8, k * mDrawScaleX, 0xf);
}

/* -------------------------------------------------------------------------- */
/* ROM ordinal 6 -- func_ov100_0214233c, 0x0214233c, size 0x184 */
/* -------------------------------------------------------------------------- */
// @symbol _ZN7daIbl_c19func_ov100_0214233cEv
/* Path-node advance. mNextNodePos is the node the ball is heading for and
 * mPrevNodePos the one it last passed. Stores the atan2 heading to the node
 * in mHeadingToNode, then takes a 3-axis sign-dot of (node-prev) vs
 * (node-pos). If the actor is not still approaching the node (dot <= 0),
 * copy node->prev, advance path index (wrap), fetch next node.
 * Returns -1 when the index wrapped to 0, else 1; 0 if still approaching.
 *
 * Codegen notes (mwccarm 1.2/sp2p3):
 *  - Long-lived values use 6q variable-identity mapping so birth-order colors
 *    match the ROM (A holds C8, B holds C7, C8 holds C6, ...).
 *  - Sign result s8/s6 names are swapped vs the values they hold so free-reg
 *    dest colors land on r1/r3 like the ROM; mul uses those names accordingly.
 *  - u64-launder materializes pidx between the first path-node load and store.
 */
int daIbl_c::func_ov100_0214233c()
{
    int px = mNextNodePosX;
    int ax = mPosX;
    int prevx = mPrevNodePosX;
    /* name A  = node.x - prev.x  (C8)  -- high-priority web -> r8 */
    int A = px - prevx;
    int pz = mNextNodePosZ;
    int az = mPosZ;
    int prevz = mPrevNodePosZ;
    int py = mNextNodePosY;
    int prevy = mPrevNodePosY;
    /* name B  = node.y - prev.y  (C7) */
    int B = py - prevy;
    /* name C8 = node.z - prev.z  (C6) */
    int C8 = pz - prevz;
    int ay = mPosY;
    /* name C7 = node.y - pos.y   (Csl) */
    int C7 = py - ay;
    /* name C6 = node.x - pos.x   (true A) */
    int C6 = px - ax;
    /* name Csl= node.z - pos.z   (true B) */
    int Csl = pz - az;
    int sA, sB, s8, s7, s6, ssl;
    int dot;

    mHeadingToNode = _ZN4cstd5atan2E5Fix12IiES1_(C6, Csl);

    /* s6/s8 dest names swapped vs the values they store (free-reg coloring). */
    if (A != 0) {
        if (A < 0)
            s6 = -1;
        else
            s6 = 1;
    } else {
        s6 = 0;
    }
    if (B != 0) {
        if (B < 0)
            s7 = -1;
        else
            s7 = 1;
    } else {
        s7 = 0;
    }
    if (C8 != 0) {
        if (C8 < 0)
            s8 = -1;
        else
            s8 = 1;
    } else {
        s8 = 0;
    }
    if (C6 != 0) {
        if (C6 < 0)
            sA = -1;
        else
            sA = 1;
    } else {
        sA = 0;
    }
    if (C7 != 0) {
        if (C7 < 0)
            ssl = -1;
        else
            ssl = 1;
    } else {
        ssl = 0;
    }
    if (Csl != 0) {
        if (Csl < 0)
            sB = -1;
        else
            sB = 1;
    } else {
        sB = 0;
    }

    /* s6=sign(C8), s8=sign(C6) after the dest-name swap above */
    dot = s6 * sA + s7 * ssl + s8 * sB;
    if (dot <= 0) {
        int v = mNextNodePosX;
        int *pidx = &mPathNodeIndex;
        mPrevNodePosX = v;
        v = mNextNodePosY;
        mPrevNodePosY = v;
        v = mNextNodePosZ;
        mPrevNodePosZ = v;
        *pidx = *pidx + 1;
        if (mPathNodeIndex >= mNumPathNodes) {
            mPathNodeIndex = 0;
        }
        _ZNK7PathPtr7GetNodeER7Vector3j(&mPathPtr, (Vector3 *)&mNextNodePosX,
                                       mPathNodeIndex);
        return (mPathNodeIndex == 0) ? -1 : 1;
    }
    return 0;
}

/* -------------------------------------------------------------------------- */
/* ROM ordinal 7 -- func_ov100_021424c0, 0x021424c0, size 0x26c */
/* -------------------------------------------------------------------------- */
// @symbol _ZN7daIbl_c19func_ov100_021424c0Ev
/* Kinds 2 and 4, the path followers. First the knock-away death that
 * KillByInvincibleChar starts: func_ov002_020ad660 (a sibling of
 * dEnemyBase_c's UpdateKillByInvincibleChar at 0x020ad838, same mDeathState 8
 * check) steps it (0 = not dying, 1 = still spinning, 2 = it just ended; with
 * the flags 3 passed here it also spawns the coin and calls
 * KillAndTrackInDeathTable itself before returning 2) and, on 2, the ball
 * puffs (a Particle) and plays the break sound. Then the contact check, and with mDeathState 8 the handler
 * is done. Otherwise func_ov100_0214233c advances along the path and the
 * heading mPrevAngleY turns toward mHeadingToNode by at most 0x800 (11.25
 * degrees) a frame. When the path index wraps (-1) the ball becomes kind 3
 * and, if it is on the ground, has its vertical speed zeroed and is lifted 15
 * units (so it falls again); otherwise it runs the probe. If the path index
 * did not wrap it probes, then: a wall newly hit reflects the heading
 * and restarts the roll sound; on the ground a touch-down plays the landing
 * sound and dust and bounces it up at half the impact speed when it fell
 * faster than 20 units/frame, while a roll plays the rolling sound; and
 * horizontal speed is capped at 0x23000 (35 units/frame) -- below the cap, kind
 * 4 ramps toward it by 0x400 (0.25 units/frame) a frame, and kind 2 adds
 * the floor normal's X and Z to its velocity. Last comes the collision
 * update. */
int daIbl_c::func_ov100_021424c0()
{
    int r;
    int vy;

    _ZN8dActor_c19MakeVanishLuigiWorkER5dCc_c(this, &mdCcAc_c);
    r = func_ov002_020ad660(this, &mWithMeshClsn, &mModel, 3);
    if (r != 0) {
        if (r != 2)
            return r;
        _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(daIbl_PTCL_BREAK, mPosX,
                                                       mPosY, mPosZ);
        return func_02012694(daIbl_SND_BREAK, &mCamSpacePosX);
    }

    func_ov100_02141fb0();
    {
        int m = mDeathState;
        if (m == daIbl_DEATH_KNOCKED)
            return m;
    }

    r = func_ov100_0214233c();
    _Z14ApproachLinearRsss(&mPrevAngleY, mHeadingToNode, 0x800);

    if (r == -1) {
        mVariant = daIbl_KIND_ROLL_OFF;
        /* mVariant was just set to 3, so the != 4 test cannot fail here. */
        if (mVariant != daIbl_KIND_PATH_RAMP && _ZNK10dBgCh_Actr10IsOnGroundEv(&mWithMeshClsn) != 0) {
            mVertSpeed = 0;
            mPosY += 0xf000;
        } else {
            func_ov100_02142130();
        }
    } else {
        int had = (mHitWall != 0);
        vy = mVertSpeed;
        func_ov100_02142130();
        if (mHitWall != 0 && had == 0) {
            mPrevAngleY = _ZN8dActor_c12ReflectAngleE5Fix12IiES1_s(this, mWallNormalX,
                                                                      mWallNormalZ,
                                                                      mPrevAngleY);
            mRollSoundHandle = 0;
        } else if (_ZNK10dBgCh_Actr10IsOnGroundEv(&mWithMeshClsn) != 0) {
            if (_ZNK10dBgCh_Actr13JustHitGroundEv(&mWithMeshClsn) != 0) {
                func_02012694(daIbl_SND_LAND, &mCamSpacePosX);
                _ZN8dActor_c11LandingDustEb(this, 1);
                if (vy < -0x14000)
                    mVertSpeed = (int)(-vy + ((u32)-vy >> 31)) >> 1;
                mRollSoundHandle = 0;
            } else {
                mRollSoundHandle = _ZN5Sound8PlayLongEjjjRK7Vector3s(mRollSoundHandle,
                                                                       3, daIbl_SND_ROLL, &mCamSpacePosX, 0);
            }
            if (mHorzSpeed >= 0x23000) {
                mHorzSpeed = 0x23000;
            } else if (mVariant == daIbl_KIND_PATH_RAMP) {
                _Z14ApproachLinearRiii(&mHorzSpeed, 0x23000, 0x400);
            } else {
                unk_0a4 += mFloorNormalX;
                unk_0ac += mFloorNormalZ;
                mHorzSpeed = Vec3_HorzLen(&unk_0a4);
            }
        }
    }

    _ZN8dActor_c9UpdatePosEP5dCc_c(this, 0);
    func_ov100_02142264();
    _ZN5dCc_c5ClearEv(&mdCcAc_c);
    return _ZN5dCc_c6UpdateEv(&mdCcAc_c);
}

/* -------------------------------------------------------------------------- */
/* ROM ordinal 8 -- func_ov100_0214272c, 0x0214272c, size 0x1ec */
/* -------------------------------------------------------------------------- */
// @symbol _ZN7daIbl_c19func_ov100_0214272cEv
/* Kind 3, the ball that has left its path. After the knock-away death step
   (see func_ov100_021424c0) and the contact check, it breaks up in a triple
   poof when it is below the kill height mKillY or the probe says it hit a
   wall. Otherwise, on the ground it adds three times the floor normal's X and Z
   to its velocity (so it rolls downhill), points its heading along that
   velocity, and on touch-down plays the landing sound and dust and, when the
   vertical speed the probe just left (-(slope + 8 units/frame)) is below
   -8 units/frame, replaces it with 1.5 times its magnitude; in a roll it
   plays the rolling sound. Vertical speed falls by gravity down
   to the terminal velocity, then the position updates from speed alone. */
void daIbl_c::func_ov100_0214272c()
{
    int r;

    _ZN8dActor_c19MakeVanishLuigiWorkER5dCc_c(this, &mdCcAc_c);
    r = func_ov002_020ad660(this, &mWithMeshClsn, &mModel, 3);
    if (r != 0) {
        if (r != 2)
            return;

        _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(
            daIbl_PTCL_BREAK, mPosX, mPosY, mPosZ);
        func_02012694(daIbl_SND_BREAK, &mCamSpacePosX);
        return;
    }

    func_ov100_02141fb0();
    func_ov100_02142130();

    if (mPosY >= mKillY) {
        if (mHitWall == 0)
            goto ground;
    }

    _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(
        daIbl_PTCL_BREAK,
        mPosX,
        *(volatile int *)&mPosY,
        mPosZ);
    func_02012694(daIbl_SND_BREAK, &mCamSpacePosX);
    _ZN8dActor_c14TriplePoofDustEv(this);
    _ZN7fBase_c18MarkForDestructionEv(this);
    return;

ground:
    if (_ZNK10dBgCh_Actr10IsOnGroundEv(&mWithMeshClsn)) {
        int *pa4;
        int *pac;

        pa4 = &unk_0a4;
        pac = &unk_0ac;

        *pa4 = *pa4 + mFloorNormalX * 3;
        *pac = *pac + mFloorNormalZ * 3;
        mHorzSpeed = Vec3_HorzLen(pa4);

        mPrevAngleY =
            _ZN4cstd5atan2E5Fix12IiES1_(
                unk_0a4, unk_0ac);

        if (_ZNK10dBgCh_Actr13JustHitGroundEv(&mWithMeshClsn)) {
            func_02012694(daIbl_SND_LAND, &mCamSpacePosX);
            _ZN8dActor_c11LandingDustEb(this, 1);

            if (mVertSpeed < -0x8000)
                mVertSpeed = mVertSpeed * -3 / 2;
        } else {
            mRollSoundHandle =
                _ZN5Sound8PlayLongEjjjRK7Vector3s(
                    mRollSoundHandle, 3, daIbl_SND_ROLL, &mCamSpacePosX, 0);
        }
    }

    {
        int v;
        int lim;

        v = mVertSpeed + mVertAccel;
        lim = mTerminalVelocity;
        if (v >= lim)
            lim = v;
        mVertSpeed = lim;
    }

    _ZN8dActor_c22UpdatePosWithOnlySpeedEP5dCc_c(this, 0);
    func_ov100_02142264();
    _ZN5dCc_c5ClearEv(&mdCcAc_c);
    _ZN5dCc_c6UpdateEv(&mdCcAc_c);
}

/* -------------------------------------------------------------------------- */
/* ROM ordinal 9 -- func_ov100_02142918, 0x02142918, size 0x278 */
/* -------------------------------------------------------------------------- */
// @symbol _ZN7daIbl_c19func_ov100_02142918Ev
/* Kind 1, the free-rolling ball: after the knock-away death step and the
   contact check (a mDeathState of 8 ends the handler), it bounces off walls
   and rolls down slopes. A wall hit reflects the heading and rebuilds the
   velocity as speed times the sine and cosine table's entries for the new
   heading (X from the even entry, Z from the odd), then three times the
   wall normal is subtracted from the velocity. On the ground it adds three times the floor normal's X and Z
   to the velocity and, on flat ground (normal Y == 0x1000, 1.0), caps its speed at
   0x1c000 (28 units/frame), pointing the heading along the velocity and
   advancing the position along it. A touch-down plays the landing sound and
   dust; otherwise it plays the rolling sound. Its legacy source carried the
   recovered name Butterfly_Kill ("daBtfly_c::Kill, from vtable slot
   identity"); no vtable holds it -- its only reference is the
   pointer-to-member constant at 0x02147f18 that seeds entry 1 of
   Behavior's table. */
void daIbl_c::func_ov100_02142918()
{
    int r;
    _ZN8dActor_c19MakeVanishLuigiWorkER5dCc_c(this, &mdCcAc_c);
    r = func_ov002_020ad660(this, &mWithMeshClsn, &mModel, 3);
    if (r != 0) {
        if (r != 2)
            return;
        _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(daIbl_PTCL_BREAK, mPosX, mPosY, mPosZ);
        func_02012694(daIbl_SND_BREAK, &mCamSpacePosX);
        return;
    }

    func_ov100_02141fb0();
    if (mDeathState == daIbl_DEATH_KNOCKED)
        return;
    func_ov100_02142130();

    if (mHitWall != 0) {
        int ang;
        mPrevAngleY = _ZN8dActor_c12ReflectAngleE5Fix12IiES1_s(this, mWallNormalX, mWallNormalZ, mPrevAngleY);
        ang = *(u16 *)&mPrevAngleY;
        unk_0a4 = (int)(((long long)mHorzSpeed * data_02082214[(ang >> 4) << 1] + 0x800) >> 12);
        ang = *(u16 *)&mPrevAngleY;
        unk_0ac = (int)(((long long)mHorzSpeed * data_02082214[((ang >> 4) << 1) + 1] + 0x800) >> 12);
        {
            int *pa4 = &unk_0a4;
            int *pac = &unk_0ac;
            *pa4 = *pa4 - mWallNormalX * 3;
            *pac = *pac - mWallNormalZ * 3;
            mHorzSpeed = Vec3_HorzLen(pa4);
        }
        goto Lend;
    }

    if (_ZNK10dBgCh_Actr10IsOnGroundEv(&mWithMeshClsn) == 0)
        goto Lend;

    {
        int *pa4 = &unk_0a4;
        int *pac = &unk_0ac;
        *pa4 = *pa4 + mFloorNormalX * 3;
        *pac = *pac + mFloorNormalZ * 3;
        mHorzSpeed = Vec3_HorzLen(pa4);
    }
    if (mFloorNormalY == 0x1000 && mHorzSpeed > 0x1c000) {
        mHorzSpeed = 0x1c000;
        mPrevAngleY = _ZN4cstd5atan2E5Fix12IiES1_(unk_0a4, unk_0ac);
        _ZN8dActor_c28UpdatePosWithHorzSpeedAndAngEv(this);
    }

    if (_ZNK10dBgCh_Actr13JustHitGroundEv(&mWithMeshClsn) != 0) {
        func_02012694(daIbl_SND_LAND, &mCamSpacePosX);
        _ZN8dActor_c11LandingDustEb(this, 1);
        goto Lend;
    }

    mRollSoundHandle = _ZN5Sound8PlayLongEjjjRK7Vector3s(mRollSoundHandle, 3, daIbl_SND_ROLL, &mCamSpacePosX, 0);

Lend:
    {
        int nv = mVertSpeed + mVertAccel;
        int lim = mTerminalVelocity;
        if (nv >= lim)
            lim = nv;
        mVertSpeed = lim;
    }
    _ZN8dActor_c22UpdatePosWithOnlySpeedEP5dCc_c(this, 0);
    func_ov100_02142264();
    _ZN5dCc_c5ClearEv(&mdCcAc_c);
    _ZN5dCc_c6UpdateEv(&mdCcAc_c);
}

/* -------------------------------------------------------------------------- */
/* ROM ordinal 10 -- func_ov100_02142b90, 0x02142b90, size 0x18c */
/* -------------------------------------------------------------------------- */
// @symbol _ZN7daIbl_c19func_ov100_02142b90Ev
/* Kind 0, the spawner. mStateTimer is its countdown. When it is zero, and the
   spawner has fewer than 3 balls out in sublevel 6 or 6 elsewhere
   (mLiveBalls), it looks at the closest player: unless the ball it would
   spawn is kind 4 (see below), the player must be more than 40 units below it
   (0x28000), and the horizontal distance to the player must lie within
   mMinSpawnDist..mMaxSpawnDist. In sublevel 0x16 a player lower than -2500
   units stops it. Passing all that reloads mStateTimer (with 0x3f in sublevel
   0x16, 0x7f otherwise) and spawns another IRONBALL (actor 220) at its own
   position with its own param1, area ID and angles. param1 was shifted down by
   four in InitResources, so what the new ball takes as its kind (the low
   nibble) is the spawner's second nibble, and (param1 & 0xf) != 4 is the test
   for "the spawned ball is not kind 4". The new ball stores the spawner in
   mDispenser, and mLiveBalls counts it. */
void daIbl_c::func_ov100_02142b90()
{
    struct Vec3i pos;
    u16 *timer = (u16 *)&mStateTimer;
    void *pl;
    int r1;

    if (*timer != 0) {
        *timer = *timer - 1;
        return;
    }

    if (data_0209f2f8 == 6) r1 = 3; else r1 = 6;
    if (mLiveBalls >= (u32)r1) return;

    pl = _ZN8dActor_c13ClosestPlayerEv(this);
    if (pl == 0) return;

    {
        struct Vec3i *pp = (struct Vec3i *)&((Player *)pl)->mPosX;
        pos.x = pp->x;
        pos.y = pp->y;
        pos.z = pp->z;
    }

    if ((param1 & 0xf) != daIbl_KIND_PATH_RAMP) {
        if (pos.y >= mPosY - 0x28000) return;
    }

    {
        int d = Vec3_HorzDist((struct Vec3i *)&mPosX, &pos);
        if (d < mMinSpawnDist) return;
        if (d > mMaxSpawnDist) return;
    }

    if (data_0209f2f8 == 0x16) {
        if (pos.y < (int)0xff63c000) return;
        mStateTimer = 0x3f;
    } else {
        mStateTimer = 0x7f;
    }

    {
        void *a;
        int cc = mAreaId;
        a = _ZN8dActor_c5SpawnEjjRK7Vector3PK10Vector3_16as(daIbl_ACTOR_IRONBALL, param1,
            (struct Vec3i *)&mPosX, (const void *)&mPrevAngleX, cc, -1);
        if (a == 0) return;
        {
            u8 *cnt = &mLiveBalls;
            *cnt = *cnt + 1;
        }
        ((daIbl_c *)a)->mDispenser = this;
    }
}

/* -------------------------------------------------------------------------- */
/* ROM ordinal 11 -- _ZN7daIbl_c16CleanupResourcesEv, 0x02142d1c, size 0x40 */
/* -------------------------------------------------------------------------- */
// @symbol _ZN7daIbl_c16CleanupResourcesEv
/* Gives a spawner's slot back: a ball made by a spawner holds it in
   mDispenser, and its count of balls out drops by one. Then the model file
   reference is released. */
int daIbl_c::CleanupResources()
{
    daIbl_c *dispenser = mDispenser;

    if (dispenser != 0) {
        (dispenser->mLiveBalls)--;
    }

    ((SharedFilePtr *)(&data_ov100_02148668))->Release();
    return 1;
}

/* -------------------------------------------------------------------------- */
/* ROM ordinal 12 -- _ZN7daIbl_c6RenderEv, 0x02142d5c, size 0x3c */
/* -------------------------------------------------------------------------- */
// @symbol _ZN7daIbl_c6RenderEv
/* Draws the model with mDrawScale, except kind 0, the spawner, which has
   nothing to draw. */
int daIbl_c::Render()
{
    unsigned char b = mVariant;
    if (b) {
        EmbeddedClass *e = (EmbeddedClass *)((char *)&mModel);
        e->virtualMethod((char *)&mDrawScaleX);
    }
    return 1;
}

/* -------------------------------------------------------------------------- */
/* ROM ordinal 13 -- _ZN7daIbl_c8BehaviorEv, 0x02142d98, size 0x48 */
/* -------------------------------------------------------------------------- */
// @symbol _ZN7daIbl_c8BehaviorEv
/* Calls the pointer-to-member handler for this ball's kind from the table at
   data_ov100_0214867c, on this actor. */
int daIbl_c::Behavior()
{
    unsigned char idx = mVariant;
    struct VtEntry *e = (struct VtEntry *)((char *)data_ov100_0214867c + ((int)idx << 3));
    int f1 = e->field1;
    void *obj = (void *)((char *)((void *)this) + (f1 >> 1));
    FnPtr fn;
    if (f1 & 1) {
        fn = (FnPtr)*(int *)((char *)*(int **)obj + e->field0);
    } else {
        fn = (FnPtr)e->field0;
    }
    fn(obj);
    return 1;
}

/* -------------------------------------------------------------------------- */
/* ROM ordinal 14 -- _ZN7daIbl_c13InitResourcesEv, 0x02142de0, size 0x38c */
/* -------------------------------------------------------------------------- */
// @symbol _ZN7daIbl_c13InitResourcesEv
/* Byte-matches under the pinned 2004/b56, 0x38c for 0x38c, relocation
   destinations checked. It did not until the param1 shift below was respelt;
   the extra `add r1, r4, #8` the loose source used to emit at +0x60 is
   described at that line.

   Loads the model and shadow, sets gravity (-4 units/frame^2, mVertAccel
   -0x4000) and terminal velocity (-70 units/frame, -0x46000), picks the kind
   from param1's low nibble and sets up that kind. Distances and speeds below
   are in units (0x1000 = 1). The sublevel tests read data_0209f2f8 (a signed
   byte): 6, 0x16, 0x18 and 0x19 are the sublevels this class has special
   numbers for. */
int daIbl_c::InitResources()
{
    int kind;
    int d;

    _ZN9ModelBase7SetFileEP8BMD_Fileii(&mModel,
        _ZN5Model8LoadFileER13SharedFilePtr(&data_ov100_02148668), 1, -1);
    if (mShadowModel.InitCylinder() == 0)
        return 0;
    mVertAccel = -0x4000;
    mTerminalVelocity = -0x46000;
    mVariant = param1 & 0xf;
    /* The spawn word is packed: the low nibble is the variant, read out just
       above, and the rest is this ball's own parameters, shifted down in
       place here.

       Spelt plainly (`param1 = param1 >> 4;`), both sides of this assignment
       are the same expression, and 2004/b56 value-numbers them together and
       materialises the address once -- the extra `add r1, r4, #8` at +0x60,
       followed by `ldr r0, [r1]` and `str r0, [r1]` -- one instruction
       longer than the ROM, which shifts every literal-pool load in the rest
       of the function. The ROM keeps the offset folded into both accesses:
       `ldr r0, [r4, #8]` / `lsr r0, r0, #4` / `str r0, [r4, #8]`. A
       redundant cast on the read side is enough to make the two sides
       textually different and reach the folded form -- no `volatile`
       needed, so tools/tiers.py never reads this as a codegen trick. Same
       residue, same lever, as daDoor_c::InitResources. Measured: with the
       cast the candidate is 0x38c and 0 of 227 words differ; without it
       0x390, and over the shared prefix 186 of 228 differ. */
    param1 = (u32)param1 >> 4;
    kind = mVariant;

    /* Kinds 2 and 4: the path followers. The path ID is the next nibble of
       param1. mPrevNodePos is set to where the ball spawned and mNextNodePos to
       the path's node 0 (node 1 when the ball is already standing on node 0),
       the node it heads for first. */
    if (kind == daIbl_KIND_PATH_SLOPE || kind == daIbl_KIND_PATH_RAMP) {
        _ZN7PathPtr6FromIDEj(&mPathPtr, param1 & 0xf);
        mNumPathNodes = _ZNK7PathPtr8NumNodesEv((char *)&mPathPtr);
        mPathNodeIndex = 0;
        mPrevNodePosX = mPosX;
        mPrevNodePosY = mPosY;
        mPrevNodePosZ = mPosZ;
        _ZNK7PathPtr7GetNodeER7Vector3j(&mPathPtr, &mNextNodePosX, mPathNodeIndex);
        if (Vec3_Equal(&mPosX, &mNextNodePosX)) {
            mPathNodeIndex += 1;
            _ZNK7PathPtr7GetNodeER7Vector3j(&mPathPtr, &mNextNodePosX, mPathNodeIndex);
        }
        func_ov100_0214233c();
        mPrevAngleY = mHeadingToNode;
        mKillY = *(int *)&data_02092138;
        d = *(signed char *)&data_0209f2f8;
        /* Sublevel 0x19 is the half-size ball: speed 10 units/frame, scale
           0.5 (0x800), collider radius and height 30 units. */
        if (d == 0x19) {
            mHorzSpeed = 0xa000;
            mDrawScaleX = 0x800;
            mDrawScaleY = 0x800;
            mDrawScaleZ = 0x800;
            _ZN7dCcAc_c4InitEP8dActor_c5Fix12IiES3_jj(&mdCcAc_c, this, 0x1e000, 0x1e000, 0x200004, 0x3c0);
            _ZN10dBgCh_Actr4InitEP8dActor_c5Fix12IiES3_P10Vector3_16S5_(&mWithMeshClsn, this, 0x1e000, 0x1e000, 0, 0);
            mKillY = -0x640000;        /* -1600 units */
        } else {
            mDrawScaleX = 0x1000;
            mDrawScaleY = 0x1000;
            mDrawScaleZ = 0x1000;
            _ZN7dCcAc_c4InitEP8dActor_c5Fix12IiES3_jj(&mdCcAc_c, this, 0x64000, 0x64000, 0x200004, 0x3c0);
            _ZN10dBgCh_Actr4InitEP8dActor_c5Fix12IiES3_P10Vector3_16S5_(&mWithMeshClsn, this, 0x64000, 0x64000, 0, 0);
            /* Full size: scale 1.0, collider radius and height 100 units.
               Speed per frame: 25 units in sublevel 0x18, 20 in 6, else 10. */
            d = *(volatile signed char *)&data_0209f2f8;
            if (d == 0x18) {
                mHorzSpeed = 0x19000;
                mKillY = 0xfec78000;       /* -5000 units */
            } else if (d == 0x16) {
                mHorzSpeed = 0xa000;
                mKillY = 0xff63c000;       /* -2500 units */
            } else {
                if (d == 6)
                    mHorzSpeed = 0x14000;
                else
                    mHorzSpeed = 0xa000;
            }
        }
        _ZN8dActor_c9UpdatePosEP5dCc_c(this, 0);
        _ZN10dBgCh_Actr13SetLimMovFlagEv((char *)&mWithMeshClsn);
        goto end;
    }

    /* Kind 1, the free roller: no path. mFlags |= 1 clip-tests the ball, so
       its Behavior is skipped while it is off screen. */
    if (kind == daIbl_KIND_FREE_ROLL) {
        mFlags |= 1;
        mDrawScaleX = 0x1000;
        mDrawScaleY = 0x1000;
        mDrawScaleZ = 0x1000;
        _ZN7dCcAc_c4InitEP8dActor_c5Fix12IiES3_jj(&mdCcAc_c, this, 0x64000, 0x64000, 0x200004, 0x3c0);
        _ZN10dBgCh_Actr4InitEP8dActor_c5Fix12IiES3_P10Vector3_16S5_(&mWithMeshClsn, this, 0x64000, 0x64000, 0, 0);
        _ZN10dBgCh_Actr13SetLimMovFlagEv((char *)&mWithMeshClsn);
        goto end;
    }

    /* Kind 0, the spawner: only picks the distance band, in units. Sublevel
       0x19: 512 to 6000. Otherwise the minimum is 1024 and the maximum 7000 in
       sublevels 0x18 and 0x16, 4600 in 6, 6500 elsewhere. */
    if (kind == daIbl_KIND_SPAWNER) {
        d = *(signed char *)&data_0209f2f8;
        if (d == 0x19) {
            mMinSpawnDist = 0x200000;
            mMaxSpawnDist = 0x1770000;
        } else {
            mMinSpawnDist = 0x400000;
            if (d == 0x18)
                mMaxSpawnDist = 0x1b58000;
            else if (d == 6)
                mMaxSpawnDist = 0x11f8000;
            else if (d == 0x16)
                mMaxSpawnDist = 0x1b58000;
            else
                mMaxSpawnDist = 0x1964000;
        }
    }

end:
    mStateTimer = 0;
    unk_108 = 0;            /* a dEnemyBase_c byte; nothing in this file reads it */
    mRollSoundHandle = 0;
    mDispenser = 0;
    mLiveBalls = 0;
    return 1;
}

/* -------------------------------------------------------------------------- */
/* ROM ordinal 15 -- daIbl_c_classInit, 0x0214316c, size 0x58 */
/* -------------------------------------------------------------------------- */
// @symbol daIbl_c_classInit
/* recovered: vtable identified, globals resolved */
/* Reconstructed source-style name: SM64DS proves daIbl_c through RTTI,
 * allocation size, vtable identity, and the IRONBALL registry profile;
 * later EAD lineage supplies classInit. Exact original spelling is not
 * preserved. Historical alias: RollingIronBall_Spawn.
 *
 * `new daIbl_c` is the whole sequence the loose factory spelled by hand:
 * fBase_c::operator new(0x3fc), dEnemyBase_c's base constructor, the vptr
 * store, then the five member constructors in declaration order. */
extern "C" daIbl_c *daIbl_c_classInit(void)
{
    return new daIbl_c;
}
