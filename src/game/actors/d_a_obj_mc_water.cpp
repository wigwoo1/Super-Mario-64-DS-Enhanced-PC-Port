//cpp
/* ov009/daObjMcWater_c -- CASTLE_WATER (338), the castle water plane.
 *
 * ov009 is mixed (DOCK_POLE / CASTLE_WATER / METAL_NET / FLAG / BIRD).
 * RTTI names this class daObjMcWater_c; the debug table names CASTLE_WATER.
 * This is the water plane, not daObjMc_Metalnet_c / daMcFlag_c / daSBird_c.
 *
 * This one file is the production source for the whole class. It licenses
 * the contiguous .text run 0x02111a70..0x02111dc4 in ov009
 * (config/tu_manifest.d/ov009/daObjMcWater_c.json).
 *
 * WRITTEN IN REVERSE ROM ORDER. mwccarm emits one .text section per function
 * in the reverse of source order, so the highest-address ROM function is
 * written FIRST. The inline destructor in include/daObjMcWater_c.h emits
 * the retail D1/D0 pair first and emits no D2 body.
 *
 * deslop leftovers:
 * - TextureTransformer::SetFile / dBgW_KcMbg::SetFile 6az: InitResources
 *   passes Fix12<int> by value; the header method form size-DIFFs.
 * - SharedFilePtr +4: InitResources' TextureTransformer::Prepare reads the
 *   BMD at data_ov009_02113c68 +4; SharedFilePtr.h has no fields.
 * - data_ov009_02113c68 / 02113c70 / 02112bc4 / 02112c38 model/KCL/BTA/CLPS
 *   handles and data_ov009_02113d8c / 02113de0 / 02113e34 spawn tables;
 *   this TU consumes them, overlay .bss/.data owns them.
 * - S14: g_profile_MC_WATER stays outside the licensed .text.
 * - func_ov009_02111b1c stays a C-linkage free function; the cartridge
 *   gives it a global symbol other config still names.
 * - no Player.h.
 *
 * Absorbed these legacy one-function sources (ROM address order), all deleted
 * by tools/tu_promote.py:
 *   [0] 0x02111a70  src/_ZN14daObjMcWater_cD1Ev.cpp
 *   [1] 0x02111abc  src/_ZN14daObjMcWater_cD0Ev.cpp
 *   [2] 0x02111b1c  src/func_ov009_02111b1c.cpp
 *   [3] 0x02111bd4  src/_ZN14daObjMcWater_c16CleanupResourcesEv.cpp
 *   [4] 0x02111c18  src/_ZN14daObjMcWater_c6RenderEv.cpp
 *   [5] 0x02111c4c  src/_ZN14daObjMcWater_c8BehaviorEv.cpp
 *   [6] 0x02111c74  src/_ZN14daObjMcWater_c13InitResourcesEv.cpp
 *   [7] 0x02111d8c  src/daObjMcWater_c_classInit.c
 */

#include "common.h"
#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#endif
#include "daObjMcWater_c.h"
#include "SharedFilePtr.h"
#include "dBgW.h"

struct BMD_File;
struct BTA_File;
struct KCL_File;
struct CLPS_Block;

extern "C" {
void func_ov009_02111b1c(daObjMcWater_c *self);
void *pc_overlay9_address(unsigned int address);
void func_02017acc(void *handle, unsigned int fileID);
void _ZN18TextureTransformer7SetFileER8BTA_Filei5Fix12IiEj(
    void *self, void *bta, int a, int fix, unsigned b);
void _ZN10dBgW_KcMbg7SetFileEP8KCL_FileRK9Matrix4x35Fix12IiEsR10CLPS_Block(
    void *self, void *kcl, void *mtx, int fix, short s, void *clps);

extern unsigned char data_0209f2d8[];
extern int data_0209caa0[];
extern int data_0209f32c;
}

/* The retail overlay owns these handles and point tables in ov009 .bss/.data.
 * Static constructors do not run when the native PC build links the overlay's
 * image as data, so initialize the exact recovered addresses on first use. */
static void pc_water_initialize_overlay_data()
{
    static bool initialized;
    if (initialized) return;
    initialized = true;

    void *modelHandle = pc_overlay9_address(0x02113c68);
    void *collisionHandle = pc_overlay9_address(0x02113c70);
    func_02017acc(modelHandle, 0x693);
    ((SharedFilePtr *)collisionHandle)->Construct(0x694);

    static const unsigned int splashPoints[3][7][3] = {
        {
            {0xfe94e000, (unsigned int)-0x5a000, 0xff60a000},
            {0xfea20000, (unsigned int)-0x5a000, 0xff52e000},
            {(unsigned int)-0x1540000, (unsigned int)-0x5a000, 0xff43e000},
            {0xfeb56000, (unsigned int)-0x5a000, 0xff344000},
            {0xfebb0000, (unsigned int)-0x5a000, 0xff236000},
            {0xfebe2000, (unsigned int)-0x5a000, 0xff114000},
            {0xfebe2000, (unsigned int)-0x5a000, 0xfeff2000},
        },
        {
            {0xfeb2e000, 0xffbd2000, (unsigned int)-0x960000},
            {0xfebf6000, 0xffbd2000, 0xff5ce000},
            {0xfec96000, 0xffbd2000, 0xff4e8000},
            {0xfed0e000, 0xffbd2000, 0xff3ee000},
            {0xfed7c000, 0xffbd2000, 0xff2f4000},
            {0xfed9a000, 0xffbd2000, 0xff1e6000},
            {0xfed72000, 0xffbd2000, 0xff0ce000},
        },
        {
            {0xfead4000, (unsigned int)-0x320000, 0xff682000},
            {0xfebb0000, (unsigned int)-0x320000, 0xff5ba000},
            {0xfec5a000, (unsigned int)-0x320000, (unsigned int)-0xc30000},
            {0xfecdc000, (unsigned int)-0x320000, 0xff4d4000},
            {0xfed54000, (unsigned int)-0x320000, 0xff2cc000},
            {0xfed54000, (unsigned int)-0x320000, 0xff25e000},
            {0xfed68000, (unsigned int)-0x320000, 0xff1aa000},
        },
    };
    static const unsigned int tableAddresses[3] = {
        0x02113d8c, 0x02113de0, 0x02113e34,
    };
    for (int table = 0; table < 3; ++table) {
        unsigned int *points = (unsigned int *)pc_overlay9_address(tableAddresses[table]);
        for (int point = 0; point < 7; ++point)
            for (int axis = 0; axis < 3; ++axis)
                points[point * 3 + axis] = splashPoints[table][point][axis];
    }
}

static void **pc_water_model_handle()
{ return (void **)pc_overlay9_address(0x02113c68); }
static SharedFilePtr *pc_water_collision_handle()
{ return (SharedFilePtr *)pc_overlay9_address(0x02113c70); }
static BTA_File *pc_water_bta()
{ return (BTA_File *)pc_overlay9_address(0x02112bc4); }
static CLPS_Block *pc_water_clps()
{ return (CLPS_Block *)pc_overlay9_address(0x02112c38); }

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
// @symbol daObjMcWater_c_classInit
/* Reconstructed source-style name; exact original SM64DS spelling is not
 * preserved. Historical alias: CastleWater_Spawn. */
extern "C" daObjMcWater_c *daObjMcWater_c_classInit()
{
    return new daObjMcWater_c();
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
// @symbol _ZN14daObjMcWater_c13InitResourcesEv
int daObjMcWater_c::InitResources()
{
    pc_water_initialize_overlay_data();
#if defined(SM64DS_PLATFORM_PC)
    const char *trace = getenv("SM64DS_PC_WATER_TRACE");
    if (trace && trace[0] == '1')
        fprintf(stderr, "[castle water] InitResources pos=%d,%d,%d area=%d model=%p collision=%p bta=%p clps=%p\n",
            mPosX, mPosY, mPosZ, mAreaId,
            (void *)pc_water_model_handle(), (void *)pc_water_collision_handle(),
            (void *)pc_water_bta(), (void *)pc_water_clps());
#endif
    int b = (int)(data_0209f2d8[0] == 1);
    if (b == 0) {
        if (data_0209caa0[2] & 0x80000) {
            mPosY = -0x2bc000;
        }
    }
    {
        BMD_File *bmd = (BMD_File *)Model::LoadFile(*(SharedFilePtr *)pc_water_model_handle());
        mModel.SetFile(bmd, 1, 0x14);
    }
    TextureTransformer::Prepare(*(BMD_File *)pc_water_model_handle()[1], *pc_water_bta());
    /* 6az: header SetFile takes Fix12<int> by value. */
    _ZN18TextureTransformer7SetFileER8BTA_Filei5Fix12IiEj(
        &mTexTransformer, pc_water_bta(), 0, 0x1000, 0);
    UpdateModelPosAndRotY();
    UpdateClsnPosAndRot();
    {
        KCL_File *kcl = (KCL_File *)dBgW_Kc::LoadFile(*pc_water_collision_handle());
        /* 6az: header SetFile takes Fix12<int> by value. */
        _ZN10dBgW_KcMbg7SetFileEP8KCL_FileRK9Matrix4x35Fix12IiEsR10CLPS_Block(
            &mMeshCollider, kcl, &mClsnMat, 0x1000, mAngleY, pc_water_clps());
    }
    mMeshCollider.Enable(this);
    {
        int v = mPosY - 0x64000;
        if (data_0209f32c > v) data_0209f32c = v;
    }
    func_ov009_02111b1c(this);
    return 1;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
// @symbol _ZN14daObjMcWater_c8BehaviorEv
int daObjMcWater_c::Behavior()
{
    mTexTransformer.speed = 0x1000;
    mTexTransformer.Advance();
    return 1;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
// @symbol _ZN14daObjMcWater_c6RenderEv
int daObjMcWater_c::Render()
{
    mTexTransformer.Update(mModel.data);
    mModel.Render(0);
    return 1;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
// @symbol _ZN14daObjMcWater_c16CleanupResourcesEv
int daObjMcWater_c::CleanupResources()
{
    ((SharedFilePtr *)pc_water_model_handle())->Release();
    pc_water_collision_handle()->Release();
    if (mMeshCollider.IsEnabled())
        mMeshCollider.Disable();
    return 1;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
// @symbol func_ov009_02111b1c
extern "C" void func_ov009_02111b1c(daObjMcWater_c *self)
{
    Vector3 *vec;
    int i;
    int b = (int)(data_0209f2d8[0] == 1);
    if (b != 0) {
        if (self->mPosX > 0) return;
        vec = (Vector3 *)pc_overlay9_address(0x02113de0);
    } else {
        vec = (Vector3 *)pc_overlay9_address(
            (data_0209caa0[2] & 0x80000) ? 0x02113e34 : 0x02113d8c);
    }
    for (i = 0; i < 7; i++) {
        dActor_c::Spawn(0xc5, 0, vec[i], 0, self->mAreaId, -1);
    }
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
// @symbol _ZN14daObjMcWater_cD1Ev
// @symbol _ZN14daObjMcWater_cD0Ev
/* NOT WRITTEN HERE ON PURPOSE. The inline `~daObjMcWater_c() {}` in the
   header is the whole source of both variants: from an inline body mwcc emits
   D1 and then D0 -- the cartridge's own order -- and no leaf D2. Writing the
   body out of line here instead flips them to D0-before-D1 and the isolation
   step rejects the object.
 */
