// dActor_c-hierarchy vtables (gate 9), per the vtable law (clsn_vtable.cpp):
// MSVC slot order. include/fBase_c.h declares the dtor LAST, so MSVC and
// the ROM agree on slots 0..15 and diverge only at the tail -- the header
// was built for exactly this. Lifecycle slots forward to the class's own
// overrides where they exist and to the fBase_c/dActor_c defaults where
// they do not; the tail traps.
//
// Base-class vtable symbols the ctor chain installs and then overwrites
// (never dispatched between installs) are plain storage.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "fBase_c.h"
#if defined(SM64DS_GAME_TARGET)
#include "hal/dsmt_scene.h"
#include "daStar_c.h"
#include "daStarBase_c.h"
#include "SharedFilePtr.h"
#endif
#include "daObjYajirusi_c.h"
#include "dClipper.h"
#include "Player.h"

#define PC_OV002_TEXSEQ_HANDLE_LIST(X) \
    X(e1c8) X(e1d0) X(e1e0) X(e1e8) X(e230) X(e250) X(e270) X(e288) \
    X(e2f0) X(e3b0) X(e3c8) X(e3e0) X(e3e8) X(e400) X(e408) X(e430) \
    X(e438) X(e450) X(e458) X(e460) X(e478) X(e4b8) X(e4d0) X(e4e8) \
    X(e500) X(e538) X(e540) X(e600) X(e620) X(e640) X(e660) X(e670) \
    X(e680) X(e6a0) X(e6b8) X(e6e0) X(e708) X(e728) X(e738) X(e770) \
    X(e788) X(e7e8) X(e800) X(e898) X(e8e8) X(e910) X(e958) X(e9d0) \
    X(e9e8) X(ea10) X(ea30) X(ea70) X(ea88) X(eac8) X(eaf0) X(eb10) \
    X(eb38) X(eb58) X(eb70) X(eb90) X(eba8) X(ebc8) X(ec00) X(ec10) \
    X(ec40) X(ec98) X(eca8) X(ecb8)

/* ArrowSignRight is the historical host-gate name for this actor. */
using ArrowSignRight = daObjYajirusi_c;

// The lifecycle definitions are MSVC methods (ArrowSignRight.h/fBase_c.h
// real classes). Every shim calls QUALIFIED -- never virtual.
extern "C" int _ZN14ArrowSignRight13InitResourcesEv(char *self)
{ return ((ArrowSignRight *)self)->ArrowSignRight::InitResources(); }

static int __fastcall sl_init(void *self, void *)
{ return _ZN14ArrowSignRight13InitResourcesEv((char *)self); }
static int __fastcall sl_cleanup(void *self, void *)
{ return ((ArrowSignRight *)self)->ArrowSignRight::CleanupResources(); }
static int __fastcall sl_behavior(void *self, void *)
{ return ((ArrowSignRight *)self)->ArrowSignRight::Behavior(); }
static int __fastcall sl_render(void *self, void *)
{ return ((ArrowSignRight *)self)->ArrowSignRight::Render(); }
static int __fastcall sl_binit(void *self, void *)
{ return ((fBase_c *)self)->fBase_c::BeforeInitResources(); }
static void __fastcall sl_ainit(void *self, void *, u32 a)
{ ((fBase_c *)self)->fBase_c::AfterInitResources(a); }
static int __fastcall sl_bclean(void *self, void *)
{ return ((fBase_c *)self)->fBase_c::BeforeCleanupResources(); }
static void __fastcall sl_aclean(void *self, void *, u32 a)
{ ((fBase_c *)self)->fBase_c::AfterCleanupResources(a); }
static int __fastcall sl_bbeh(void *self, void *)
{ return ((fBase_c *)self)->fBase_c::BeforeBehavior(); }
static void __fastcall sl_abeh(void *self, void *, u32 a)
{ ((fBase_c *)self)->fBase_c::AfterBehavior(a); }
static int __fastcall sl_bren(void *self, void *)
{ return ((fBase_c *)self)->fBase_c::BeforeRender(); }
static void __fastcall sl_aren(void *self, void *, u32 a)
{ ((fBase_c *)self)->fBase_c::AfterRender(a); }
static int __fastcall sl_pdes(void *self, void *)
{ ((fBase_c *)self)->fBase_c::OnPendingDestroy(); return 0; }
static int __fastcall sl_heap(void *self, void *)
{ return ((fBase_c *)self)->fBase_c::OnHeapCreated(); }

#define ATRAP(n) \
    static void __fastcall a_trap##n(void *, void *) { \
        fprintf(stderr, "FATAL: ArrowSignRight vtable slot %d trap\n", n); \
        abort(); }
ATRAP(13) ATRAP(14) ATRAP(16) ATRAP(17)

extern "C" void *_ZTV14ArrowSignRight[20] = {
    (void *)sl_init,     /* 0  InitResources */
    (void *)sl_binit,    /* 1  BeforeInitResources */
    (void *)sl_ainit,    /* 2  AfterInitResources */
    (void *)sl_cleanup,  /* 3  CleanupResources */
    (void *)sl_bclean,   /* 4  BeforeCleanupResources */
    (void *)sl_aclean,   /* 5  AfterCleanupResources */
    (void *)sl_behavior, /* 6  Behavior */
    (void *)sl_bbeh,     /* 7  BeforeBehavior */
    (void *)sl_abeh,     /* 8  AfterBehavior */
    (void *)sl_render,   /* 9  Render */
    (void *)sl_bren,     /* 10 BeforeRender */
    (void *)sl_aren,     /* 11 AfterRender */
    (void *)sl_pdes,     /* 12 OnPendingDestroy */
    (void *)a_trap13, (void *)a_trap14,
    (void *)sl_heap,     /* 15 OnHeapCreated */
    (void *)a_trap16, (void *)a_trap17,
    0, 0,
};

// ExclamationSwitch still uses a literal ROM-vtable boundary. The dBase_c and
// dActor_c installs are now compiler-owned by the native constructor sources.
extern "C" {
void *_ZTV17ExclamationSwitch[20];
}

// ---- fBase_c::fBase_c() transcription ---------------------------------
// The ROM ctor is a hand-asm block (src/actors/ActorBase.cpp); this is its
// C transcription, field for field against the disassembly there. The spawn
// CONTEXT globals it reads (pending actor ID, area byte, the spawn-info
// pointer table for the two processing-list priorities) are storage here;
// the smoke seeds them the way func_02010e78/dBase_c::Spawn would.
extern "C" {
void _ZN7fBase_c9SceneNodeC1Ev(void *node);
int func_0203b438(void *a, void *b, void *c);
int func_02043810(void *p);

int data_02099edc[8];           /* the transient fBase_c vtable install */
#pragma comment(linker, "/alternatename:__ZTV7fBase_c=_data_02099edc")
int data_02099e70[1];           /* next unique actor id */
int data_020a4b60[1];
unsigned short data_020a4b54;   /* PENDING ACTOR ID (the spawn context) */
unsigned char data_020a4b48;    /* pending area byte */
int data_020a4b64[1];
int data_020a4ba8[4];           /* fBase_c processing-list root */
int data_020a4b6c[8];           /* the scene tree root the ctor links into */
void *data_020a4bb8_storage[512];
void **data_020a4bb8 = data_020a4bb8_storage;  /* actorID -> SpawnInfo* */
// Lifecycle fixtures seed one actor descriptor directly. Full bootstrap
// needs the scene/Player overlay resources and native factories only in game.
#if defined(SM64DS_GAME_TARGET)
#define DECLARE_PC_TEXSEQ_HANDLE(suffix) extern unsigned char data_ov002_0210##suffix[8];
PC_OV002_TEXSEQ_HANDLE_LIST(DECLARE_PC_TEXSEQ_HANDLE)
#undef DECLARE_PC_TEXSEQ_HANDLE
extern unsigned char data_ov002_0210e488[8];
extern unsigned char data_ov002_0210e4a0[8];
extern unsigned char data_ov002_0210e468[8];
extern unsigned char data_ov002_0210e470[8];
extern unsigned char data_ov002_0210e280[8];
extern unsigned char data_ov002_0210e228[8];
extern unsigned char data_ov002_0210e498[8];
extern unsigned short *data_0209d3b8;
extern "C" unsigned int data_0209d3b0 = 0;
extern void *data_ov002_020ff2f0[100];
extern unsigned char data_ov002_0210e798[8];
extern unsigned char data_ov002_0210e4f0[8];
extern unsigned char data_ov002_0210e3d8[8];
extern unsigned char data_ov002_0210e6c0[8];
extern unsigned char data_ov002_0210eb98[8];
extern unsigned char data_ov002_0210eb88[8];
extern unsigned char data_ov002_0210e4c0[8];
extern unsigned char data_ov002_02110aa4[8];
extern unsigned char data_ov002_0210a0fc[8];
extern unsigned char data_ov002_0210a104[8];
extern unsigned char data_ov002_0210a114[8];
extern unsigned char data_ov002_0210e840[8];
extern unsigned char data_ov002_0210e798[8];
extern unsigned char data_ov002_0210e4f0[8];
/* overlay_data emits raw ROM bytes; spell this table as words so its entries
   can be safely rebased in place during native startup. */
extern unsigned int data_ov002_020ff480[736];
#define PC_PLAYER_ANIM_HANDLE(address, symbol) extern unsigned char symbol[8];
#include "player_anim_handles.inc"
#undef PC_PLAYER_ANIM_HANDLE
void *data_ov002_020ff790[4];
extern unsigned char data_ov002_021104b4[24];
extern PcMemberCallback<void> data_ov002_0211075c[7];
void func_ov002_020c72a4(void *self);
extern unsigned char data_ov002_0210e168[8];
extern unsigned char data_ov002_0210a68c[16];
extern unsigned char data_ov002_0210a69c[16];
extern unsigned char data_ov002_0210a6ac[16];
extern unsigned char data_ov002_0210ec50[8], data_ov002_0210ebb8[8];
extern unsigned char data_ov002_0210ec60[8], data_ov002_0210e818[8];
extern unsigned char data_ov002_0210e690[8], data_ov002_0210eb20[8];
extern unsigned char data_ov002_0210e758[8], data_ov002_0210e7a8[8];
extern unsigned char data_ov002_0210ec38[8], data_ov002_0210eb00[8];
extern unsigned char data_ov002_0210e790[8], data_ov002_0210e7f0[8];
extern void *_ZN13SharedFilePtr9ConstructEj(void *self, unsigned int fileID);
extern void hal_fill_model_vtable(void);
extern "C" void *daPly_c_classInit(void);
extern "C" void *dCamera_c_classInit(void);
extern "C" void pc_sync_actor_profile_table_cpp(void);
extern "C" void __sinit_ov002_02100560(void);
extern "C" void __sinit_ov002_021019d0(void);
extern "C" void __sinit_ov002_0210804c(void);
#pragma comment(linker, "/alternatename:_data_ov002_0210a83c=??_7Player@@6B@")

static unsigned int pc_player_method_address(int (Player::*method)())
{
    unsigned int address = 0;
    memcpy(&address, &method, sizeof(address));
    return address;
}

static void __fastcall pc_level_enter_ground(Player *self, void *)
{
    func_ov002_020c72a4(self);
}

static void pc_fill_player_level_enter_callbacks()
{
    // Exact retail initializer order, decoded from its seven constant records.
    data_ov002_0211075c[0] = &Player::func_ov002_020c75f0;
    data_ov002_0211075c[1] = &Player::func_ov002_020c7350;
    data_ov002_0211075c[2] = &Player::func_ov002_020c71e0;
    data_ov002_0211075c[3] = &Player::func_ov002_020c7194;
    data_ov002_0211075c[4].address =
        reinterpret_cast<uintptr_t>(&pc_level_enter_ground);
    data_ov002_0211075c[4].adjustment = 0;
    data_ov002_0211075c[5] = &Player::func_ov002_020c70ac;
    data_ov002_0211075c[6] = &Player::func_ov002_020c6fe4;
}


static void *pc_resolve_player_anim_handle(unsigned int address)
{
    // Each generated global owns separate host storage. Preserve the exact
    // DS-address-to-symbol mapping instead of assuming contiguous native BSS.
    switch (address) {
#define PC_PLAYER_ANIM_HANDLE(address, symbol) case address: return symbol;
#include "player_anim_handles.inc"
#undef PC_PLAYER_ANIM_HANDLE
    default: return 0;
    }
}

static void *pc_resolve_player_texseq_handle(unsigned int address)
{
    switch (address) {
#define PC_CASE_TEXSEQ_HANDLE(suffix) case 0x0210##suffix: return data_ov002_0210##suffix;
        PC_OV002_TEXSEQ_HANDLE_LIST(PC_CASE_TEXSEQ_HANDLE)
#undef PC_CASE_TEXSEQ_HANDLE
    default:
        return 0;
    }
}

/* The retail ACTOR_SPAWN_TABLE contains ARM9 virtual addresses to profile
   records. Those values cannot be dereferenced by the Windows process. Keep
   the first boot profile in host storage so the initial BOOT scene constructor
   can read its execute/draw priorities after func_0201a054 selects it. */
struct PcActorProfile {
    void *factory;
    unsigned short behaviorPriority;
    unsigned short renderPriority;
};
#if defined(SM64DS_GAME_TARGET)
extern "C" void *daChScene_c_classInit(void);
extern "C" void *daDoor_c_classInit(void);
extern "C" unsigned char g_profile_STAR_GATE[];
extern "C" void *dMap_c_classInit(void);
extern "C" void *daDemo_c_classInit(void);
extern "C" void *daC_Jugem_c_classInit(void);
extern "C" void *dScDSMT_c_classInit(void);
static PcActorProfile pc_dsmt_profile = {(void *)&dScDSMT_c_classInit,1,3};
extern "C" void *dScMiniGm_c_classInit(void);
// Overlay 5's original 8-byte profile: native factory, priorities 5 and 8.
static PcActorProfile pc_minigame_menu_profile = {(void *)&dScMiniGm_c_classInit,5,8};
extern "C" void *dScMgLuigi_c_classInit(void);
static PcActorProfile pc_wanted_profile = {(void *)&dScMgLuigi_c_classInit,366,366};
extern "C" void pc_demo_init_jugem_resources(void);
// local extern: these profile POD types are private to daCoin_c.cpp. Keep
// their canonical native factory/metadata records instead of duplicating them.
extern "C" unsigned char g_profile_COIN[];
extern "C" unsigned char g_profile_RED_COIN[];
extern "C" unsigned char g_profile_BLUE_COIN[];
extern "C" unsigned char g_profile_KURIBO[], g_profile_KURIBO_S[], g_profile_KURIBO_L[];
extern "C" void pc_goomba_init_resources();
extern "C" void pc_bombking_init_resources();
extern "C" void pc_buddy_init_resources(), pc_shutter_init_resources();
extern "C" void pc_chomp_init_resources(), pc_pile_init_resources(), pc_wanwan_fence_init_resources();
extern "C" void pc_lava_bridge_init_resources();
extern "C" unsigned char g_profile_FL_LONDON[];
extern "C" void *daObjFl_Seesaw_c_classInit();
extern "C" void pc_fl_seesaw_init_resources();
extern "C" unsigned char g_profile_WANWAN[], g_profile_PILE[], g_profile_WANWAN_SHUTTER[];
extern "C" void *daRedBombhei_c_classInit();
extern "C" void *daCnn_c_classInit(), *daWbm_c_classInit();
extern "C" void *daWarpkun_c_classInit();
extern "C" void *daObjMc_Metalnet_c_classInit();
extern "C" void *daObjMcWater_c_classInit();
extern "C" void pc_cannon_init_resources(), pc_waterbomb_init_resources();
extern "C" unsigned char g_profile_CANNON_SHUTTER[];
extern "C" unsigned char g_profile_BOMBKING[];
extern "C" unsigned char g_profile_TREE[];
extern "C" unsigned char g_profile_SBIRD[];
extern "C" unsigned char g_profile_KINOPIO[];
extern "C" void pc_toad_init_resources();
extern "C" unsigned char g_profile_BOMBHEI[];
extern "C++" { extern SharedFilePtr data_ov102_0214e9c0, data_ov102_0214e9c8; }
extern "C" void *daYegg_c_classInit();
extern "C" void *daSoundObj_c_classInit();
extern "C" void pc_sound_obj_init_callbacks(void);
// MAP derives directly from dBase_c: its ROM profile is eight bytes.
static PcActorProfile pc_map_profile = {(void *)&dMap_c_classInit, 335, 335};
extern "C" void pc_door_init_tables(void);
struct PcNativeActorProfile {
    void *factory;
    unsigned short behaviorPriority, renderPriority;
    unsigned int flags;
    int clipOffsetY, clipRadius, clipDistance, farDistance;
};
extern "C" void *da1up_c_classInit_ONEUPKINOKO();
extern "C" void *daObjTatefuda_c_classInit();
extern "C" void *daObjC1Hikari_c_classInit();
extern "C" void *daSetSE_c_classInit();
extern "C" void *daObjHeart_c_classInit();
extern "C" void *daObjTh_Fall_Block_c_classInit();
extern "C" void *daTrsTrap_c_classInit_KAIDAN();
extern "C" void *daTrs_c_classInit_TERESA();
extern "C" void *daTrs_c_classInit_BOSS_TERESA();
extern "C" void *daTrsIcon_c_classInit();
extern "C" void *daTBasket_c_classInit();
static PcNativeActorProfile pc_boo_profile = {
    (void *)&daTrs_c_classInit_TERESA,209,49,6,0x64000,0x64000,0x1000000,0xa00000
};
static PcNativeActorProfile pc_big_boo_profile = {
    (void *)&daTrs_c_classInit_BOSS_TERESA,210,50,6,0xc8000,0xc8000,0x1000000,0xa00000
};
static PcNativeActorProfile pc_boo_icon_profile = {
    (void *)&daTrsIcon_c_classInit,211,51,0,0,0x1000,0,0
};
static PcNativeActorProfile pc_boo_basket_profile = {
    (void *)&daTBasket_c_classInit,212,52,2,0x64000,0x64000,0x1000000,0xa00000
};
extern "C" void *daTrsTrap_c_classInit_BOOKSHELF();
extern "C" void *daBook_c_classInit_BOOK_SWITCH(), *daBook_c_classInit_SHOOT_BOOK();
extern "C" void *daChair_c_classInit(), *daPiano_c_classInit();
static PcNativeActorProfile pc_haunted_chair_profile = {
    (void *)&daChair_c_classInit,326,326,2,0x64000,0x64000,0x1000000,0x1000000
};
static PcNativeActorProfile pc_mad_piano_profile = {
    (void *)&daPiano_c_classInit,249,107,3,0x60000,0x200000,0x1000000,0x1000000
};
static PcNativeActorProfile pc_bookshelf_profile = {
    (void *)&daTrsTrap_c_classInit_BOOKSHELF,158,55,0,0x64000,0x64000,0x1000000,0xa00000
};
static PcNativeActorProfile pc_book_switch_profile = {
    (void *)&daBook_c_classInit_BOOK_SWITCH,213,54,2,0x64000,0x64000,0x1000000,0x1000000
};
static PcNativeActorProfile pc_shoot_book_profile = {
    (void *)&daBook_c_classInit_SHOOT_BOOK,325,327,2,0x64000,0x64000,0x1000000,0x1000000
};
extern "C" void *daTrsTrap_c_classInit_MERRYGOROUND();
extern "C" void *daTrsTrap_c_classInit_TERESAPIT();
static PcNativeActorProfile pc_kaidan_profile = {
    (void *)&daTrsTrap_c_classInit_KAIDAN,65,53,0,0x64000,0x64000,0x1000000,0xa00000
};
static PcNativeActorProfile pc_teresapit_profile = {
    (void *)&daTrsTrap_c_classInit_TERESAPIT,160,57,0,0x64000,0x64000,0x1000000,0xa00000
};
static PcNativeActorProfile pc_merry_go_round_profile = {
    (void *)&daTrsTrap_c_classInit_MERRYGOROUND,159,56,0,0x64000,0x64000,0x1000000,0xa00000
};
static PcNativeActorProfile pc_th_fall_block_profile = {
    (void *)&daObjTh_Fall_Block_c_classInit,66,209,2,0,0x80000,0x1000000,0
};
extern "C" void *daObjYajirusi_c_classInit_YAJIRUSI_L(),*daObjYajirusi_c_classInit_YAJIRUSI_R();
static PcNativeActorProfile pc_arrow_left_profile = {
    (void *)&daObjYajirusi_c_classInit_YAJIRUSI_L,299,188,2,0xc0000,0xe0000,0x1000000,0x1000000
};
static PcNativeActorProfile pc_arrow_right_profile = {
    (void *)&daObjYajirusi_c_classInit_YAJIRUSI_R,300,189,2,0xc0000,0xe0000,0x1000000,0x1000000
};
// Original ov002 HEART descriptor at02109c50, native constructor.
static PcNativeActorProfile pc_healing_heart_profile = {
    (void *)&daObjHeart_c_classInit,297,170,3,0x60000,0x60000,0x1000000,0x1000000
};
// Original ov002 SET_SE descriptor at0210b47c, native constructor.
static PcNativeActorProfile pc_ambient_sound_profile = {
    (void *)&daSetSE_c_classInit,350,350,0x64800000,0,0x1000,0,0
};
// Original ov010 C1_HIKARI descriptor at02112b84, native constructor.
static PcNativeActorProfile pc_castle_light_profile = {
    (void *)&daObjC1Hikari_c_classInit,37,324,3,0,0x7d0000,0xfa0000,0
};
extern "C" void *daObjBlockS_c_classInit();
// Original ov098 BLOCK_S descriptor at0213c510 with its native factory.
static PcNativeActorProfile pc_crate_profile = {
    (void *)&daObjBlockS_c_classInit,194,37,0x82,0x32000,0x64000,0x1000000,0x1000000
};
// Original ov002 TATEFUDA descriptor, with the native constructor.
static PcNativeActorProfile pc_signpost_profile = {
    (void *)&daObjTatefuda_c_classInit,184,167,0x82,0x40000,0x80000,0x1000000,0x1000000
};
// Original ONEUPKINOKO descriptor with its native factory.
static PcNativeActorProfile pc_oneup_profile = {
    (void *)&da1up_c_classInit_ONEUPKINOKO,276,140,3,0x32000,0x12c000,0x1000000,0x1000000
};
extern "C" void *daObj1UpLogo_c_classInit();
static PcNativeActorProfile pc_oneup_logo_profile = {
    (void *)&daObj1UpLogo_c_classInit,331,333,0x4800000,0,0xa000,0,0
};
extern "C" void *daObjHatenaBlock_c_classInit_ITEM_BLOCK();
extern "C" void *daObjHatenaBlock_c_classInit_HATENA_BLOCK();
extern "C" unsigned char g_profile_POWER_UP_ITEM[];
extern "C" unsigned char g_profile_HATENA_SWITCH[];
extern "C" void *daFeather_c_classInit();
static PcNativeActorProfile pc_feather_profile = {
    (void *)&daFeather_c_classInit,345,345,2,0x60000,0x200000,0x1000000,0x1000000
};
// Original ov102 HATENA_BLOCK descriptor, with the native constructor.
static PcNativeActorProfile pc_question_block_profile = {
    (void *)&daObjHatenaBlock_c_classInit_HATENA_BLOCK,20,38,2,0x60000,0x100000,0x1000000,0x1000000
};
// Original ov102 ITEM_BLOCK descriptor; only its constructor is native.
static PcNativeActorProfile pc_item_block_profile = {
    (void *)&daObjHatenaBlock_c_classInit_ITEM_BLOCK,21,39,2,0x60000,0x100000,0x1000000,0x1000000
};
static_assert(sizeof(PcNativeActorProfile) == 0x1c, "native actor descriptor");
extern "C" void *daRNk_c_classInit(), *daRFlag_c_classInit();
// Original ov062 race actor and finish flag descriptors, native constructors.
static PcNativeActorProfile pc_koopa_race_profile = {
    (void *)&daRNk_c_classInit,188,29,0x00800002,0xc8000,0xc8000,0x17d0000,0x17d0000
};
static PcNativeActorProfile pc_koopa_flag_profile = {
    (void *)&daRFlag_c_classInit,205,30,2,0x190000,0x190000,0x13e8000,0x13e8000
};
extern "C" void *daObjRotateUpdownLift_c_classInit_UPDOWN_LIFT();
extern "C" void *daObjSeesaw_c_classInit_BOMB_SEESAW();
extern "C" void *daObjBSwdoor_c_classInit();
// Original ov091/095/014 descriptors; only the constructor pointer is rebased.
static PcNativeActorProfile pc_updown_lift_profile = {
    (void *)&daObjRotateUpdownLift_c_classInit_UPDOWN_LIFT,29,297,0x01000002,0,0x200000,0x1000000,0x1000000
};
static PcNativeActorProfile pc_battlefield_seesaw_profile = {
    (void *)&daObjSeesaw_c_classInit_BOMB_SEESAW,39,173,2,0,0x400000,0x1000000,0
};
static PcNativeActorProfile pc_battlefield_gate_profile = {
    (void *)&daObjBSwdoor_c_classInit,40,137,3,0x120000,0x400000,0x1000000,0
};
extern "C" void *daObjSwitch_c_classInit_HANSWITCH();
extern "C" void *daObjSwitch_c_classInit_STAR_SWITCH();
// Original ov002 descriptors at02109900/0210991c, with native factories.
static PcNativeActorProfile pc_floor_switch_profile = {
    (void *)&daObjSwitch_c_classInit_HANSWITCH,11,135,2,0,0x100000,0x1000000,0
};
static PcNativeActorProfile pc_star_switch_profile = {
    (void *)&daObjSwitch_c_classInit_STAR_SWITCH,12,136,2,0,0x100000,0x1000000,0
};
// Original ov002 big-brick actor: five distinct profile IDs share one class.
extern "C" void *daObjBlockL_c_classInit_BK_FUTA();
extern "C" void *daObjBlockL_c_classInit_BLOCK_LL();
extern "C" void *daObjBlockL_c_classInit_SW_BLOCK_L();
extern "C" void *daObjBlockL_c_classInit_DP_BLOCK_L();
extern "C" void *daObjBlockL_c_classInit_BLOCK_L();
extern "C" void pc_block_init_resources(void);
static PcNativeActorProfile pc_block_profile = {
    (void *)&daObjBlockL_c_classInit_BLOCK_L,15,32,0x2u,
    0x64000,0xc8000,0x1800000,0x1800000
};
static PcNativeActorProfile pc_big_block_profile = {
    (void *)&daObjBlockL_c_classInit_BLOCK_LL,17,34,0x2u,
    0x96000,0x12c000,0x1800000,0x1800000
};
static PcNativeActorProfile pc_switch_block_profile = {
    (void *)&daObjBlockL_c_classInit_SW_BLOCK_L,19,36,0x2u,
    0x64000,0xc8000,0x1800000,0x1800000
};
static PcNativeActorProfile pc_heavy_block_profile = {
    (void *)&daObjBlockL_c_classInit_DP_BLOCK_L,16,33,0x2u,
    0x96000,0x12c000,0x4000000,0x4000000
};
static PcNativeActorProfile pc_fortress_wall_profile = {
    (void *)&daObjBlockL_c_classInit_BK_FUTA,46,199,0x2u,
    0x64000,0xc8000,0x1800000,0x1800000
};
// WARPKUN, original ov002 descriptor at 0210ac98; invisible warp cylinder.
static PcNativeActorProfile pc_warp_profile = {
    (void *)&daWarpkun_c_classInit,347,347,3,0,0xa000,0x960000,0
};
static PcNativeActorProfile pc_buddy_profile = { (void *)&daRedBombhei_c_classInit,181,46,3,0x40000,0x80000,0x1000000,0x1000000 };
// ov098 descriptors 0213c67c/0213c74c, with native constructor pointers.
static PcNativeActorProfile pc_cannon_profile = { (void *)&daCnn_c_classInit,207,47,3,0xc8000,0xc8000,0x1000000,0 };
static PcNativeActorProfile pc_waterbomb_profile = { (void *)&daWbm_c_classInit,208,48,2,0x64000,0x64000,0x1000000,0x640000 };
// MC_METALNET (339), reconstructed ov009 profile. This net surrounds the
// castle-grounds cannon and removes itself only when the save has all 150
// stars (or its author-selected unlock bit is set).
static PcNativeActorProfile pc_metalnet_profile = {
    (void *)&daObjMc_Metalnet_c_classInit,339,339,0x2u,
    0,0x500000,0x1000000,0
};
extern "C" void *daObjC0_Switch_c_classInit(), *daObjC0Water_c_classInit();
// Original ov012 C0_SWITCH / C0_WATER descriptor metadata.
static PcNativeActorProfile pc_basement_pillar_profile = {
    (void *)&daObjC0_Switch_c_classInit,34,228,2,0x60000,0x250000,0x1000000,0
};
static PcNativeActorProfile pc_basement_water_profile = {
    (void *)&daObjC0Water_c_classInit,35,229,0,0,0x1000000,0,0
};
// CASTLE_WATER (338), recovered from the mixed ov009 actor profile registry.
static PcNativeActorProfile pc_castle_water_profile = {
    (void *)&daObjMcWater_c_classInit,338,338,0,
    0,0x00320000,0x01f40000,0x00050000
};
// FL_SEESAW (77), exact ov022 descriptor metadata from 0x02114010.
static PcNativeActorProfile pc_fl_seesaw_profile = {
    (void *)&daObjFl_Seesaw_c_classInit,77,285,0x00000002u,
    0,0x00650000,0x02000000,0
};
static void *pc_power_star_factory() { return new daStar_c(); }
static void *pc_silver_star_factory() { return new daStar_c(); }
static void *pc_star_marker_factory() { return new daStarBase_c(); }
extern "C" void *daObjMarioCap_c_classInit();
// Original ov002 descriptors at 0210aa40/aa5c/aa78/95cc, native factories.
static PcNativeActorProfile pc_power_star_profile = {
    (void *)&pc_power_star_factory,178,64,0x20000002u,
    0x32000,0x1f4000,0x1000000,0x1000000
};
static PcNativeActorProfile pc_silver_star_profile = {
    (void *)&pc_silver_star_factory,179,65,0x28000002u,
    0x32000,0x1f4000,0x1194000,0x1000000
};
static PcNativeActorProfile pc_star_marker_profile = {
    (void *)&pc_star_marker_factory,180,66,0x28000002u,
    0,0xc8000,0x1000000,0x1000000
};
static PcNativeActorProfile pc_cap_reward_profile = {
    (void *)&daObjMarioCap_c_classInit,269,97,0x800002u,
    0x32000,0x32000,0x1000000,0x1000000
};
// YOSHI_EGG (9), recovered ov002 profile at 0x0210ad90.
static PcNativeActorProfile pc_yoshi_egg_profile = {
    (void *)&daYegg_c_classInit, 9, 157, 0x2u,
    0x32000, 0x46000, 0x1000000, 0x1000000
};
static PcNativeActorProfile pc_demo_profile = {
    (void *)&daDemo_c_classInit, 352, 352, 0x64800002u,
    0x3e8000, 0x3e8000, 0x2710000, 0x2710000
};
static PcNativeActorProfile pc_lakitu_profile = {
    (void *)&daC_Jugem_c_classInit, 235, 90, 0,
    0x64000, 0x12c000, 0x1000000, 0x1000000
};
extern "C" void *daMip_c_classInit();
extern "C" void *daObj_Mip_Key_c_classInit();
extern "C" void pc_rabbit_init_resources();
static PcNativeActorProfile pc_rabbit_profile = {
    (void *)&daMip_c_classInit,187,83,0x8282u,
    0x64000,0xc8000,0x1000000,0x1000000
};
static PcNativeActorProfile pc_rabbit_key_profile = {
    (void *)&daObj_Mip_Key_c_classInit,229,84,2,
    0x64000,0x64000,0x1000000,0x1000000
};
// Recovered registry metadata; dActor_c reads every word of this descriptor.
static PcNativeActorProfile pc_exit_profile = {
    (void *)&daChScene_c_classInit, 349, 349, 0, 0, 0, 0, 0
};
extern "C" void *daPicGate_c_classInit(void);
static PcNativeActorProfile pc_picture_gate_profile = {
    (void *)&daPicGate_c_classInit, 307, 304, 3, 4096, 4096, 0x1000000, 4096
};
static PcNativeActorProfile pc_door_profile = {
    (void *)&daDoor_c_classInit, 353, 353, 0x60800002u,
    0x7d000, 0x7d000, 0x1f40000, 0
};
// SOUND_OBJ (359), recovered from the original ov002 profile at 0x0210c064.
static PcNativeActorProfile pc_sound_obj_profile = {
    (void *)&daSoundObj_c_classInit, 359, 359, 0x24800000u,
    0, 0x1000, 0, 0
};
#endif
/* BOOT/TITLE/STAGE dispatch their explicit native factories in the scene
   creation veneer; their descriptors still supply scheduler priorities. */
static void *pc_unused_scene_factory(void) { return 0; }
static PcActorProfile pc_boot_profile = {
    (void *)&pc_unused_scene_factory, 0, 2
};
static PcActorProfile pc_title_profile = {
    (void *)&pc_unused_scene_factory, 0, 2
};
#if defined(SM64DS_GAME_TARGET)
// local extern: registry factories return an opaque actor pointer; no shared
// header declares this original allocation boundary.
extern "C" void *dScStage_c_classInit(void);
extern "C" void *dScStarSel_c_classInit(void);
static PcActorProfile pc_star_select_profile = {
    (void *)&dScStarSel_c_classInit, 4,7
};
static PcActorProfile pc_stage_profile = {
    (void *)&dScStage_c_classInit, 3, 6
};
#else
static PcActorProfile pc_stage_profile = {
    (void *)&pc_unused_scene_factory, 3, 6
};
#endif
static PcNativeActorProfile pc_player_profile = {
    (void *)&daPly_c_classInit, 191, 18, 0x2u,
    0xfa000, 0xfa000, 0x2ee0000, 0x2ee0000
};
static PcActorProfile pc_camera_profile = {
    (void *)&dCamera_c_classInit, 0, 2
};
static bool pc_player_overlay_data_initialized;
extern "C" void pc_actor_spawn_table_init(void)
{
    data_020a4bb8_storage[0] = &pc_boot_profile;
    data_020a4bb8_storage[2] = &pc_title_profile;
    data_020a4bb8_storage[3] = &pc_stage_profile;
#if defined(SM64DS_GAME_TARGET)
    data_020a4bb8_storage[4] = &pc_star_select_profile;
#endif
    data_020a4bb8_storage[191] = &pc_player_profile;
    /* CAMERA is the built-in ARM9 actor profile 332 (0x14c). Stage's
       LoadViewObjects publishes this actor through data_0209f318; without a
       native profile, that global stays null and camera consumers fault. */
    data_020a4bb8_storage[332] = &pc_camera_profile;
#if defined(SM64DS_GAME_TARGET)
    data_020a4bb8_storage[235] = &pc_lakitu_profile;
    data_020a4bb8_storage[187] = &pc_rabbit_profile;
    data_020a4bb8_storage[229] = &pc_rabbit_key_profile;
    pc_rabbit_init_resources();
    pc_buddy_init_resources();pc_shutter_init_resources();
    pc_chomp_init_resources();pc_pile_init_resources();pc_wanwan_fence_init_resources();
    pc_lava_bridge_init_resources();
    data_020a4bb8_storage[73] = g_profile_FL_LONDON;
    data_020a4bb8_storage[15] = &pc_block_profile;
    data_020a4bb8_storage[16] = &pc_heavy_block_profile;
    data_020a4bb8_storage[17] = &pc_big_block_profile;
    data_020a4bb8_storage[19] = &pc_switch_block_profile;
    data_020a4bb8_storage[21] = &pc_item_block_profile;
    data_020a4bb8_storage[20] = &pc_question_block_profile;
    data_020a4bb8_storage[26] = g_profile_HATENA_SWITCH;
    data_020a4bb8_storage[37] = &pc_castle_light_profile;
    data_020a4bb8_storage[350] = &pc_ambient_sound_profile;
    data_020a4bb8_storage[297] = &pc_healing_heart_profile;
    data_020a4bb8_storage[299] = &pc_arrow_left_profile;
    data_020a4bb8_storage[300] = &pc_arrow_right_profile;
    data_020a4bb8_storage[66] = &pc_th_fall_block_profile;
    data_020a4bb8_storage[65] = &pc_kaidan_profile;
    data_020a4bb8_storage[209] = &pc_boo_profile;
    data_020a4bb8_storage[210] = &pc_big_boo_profile;
    data_020a4bb8_storage[211] = &pc_boo_icon_profile;
    data_020a4bb8_storage[212] = &pc_boo_basket_profile;
    data_020a4bb8_storage[158] = &pc_bookshelf_profile;
    data_020a4bb8_storage[213] = &pc_book_switch_profile;
    data_020a4bb8_storage[325] = &pc_shoot_book_profile;
    data_020a4bb8_storage[326] = &pc_haunted_chair_profile;
    data_020a4bb8_storage[249] = &pc_mad_piano_profile;
    data_020a4bb8_storage[159] = &pc_merry_go_round_profile;
    data_020a4bb8_storage[160] = &pc_teresapit_profile;
    data_020a4bb8_storage[306] = g_profile_POWER_UP_ITEM;
    data_020a4bb8_storage[345] = &pc_feather_profile;
    data_020a4bb8_storage[276] = &pc_oneup_profile;
    data_020a4bb8_storage[184] = &pc_signpost_profile;
    data_020a4bb8_storage[194] = &pc_crate_profile;
    data_020a4bb8_storage[331] = &pc_oneup_logo_profile;
    data_020a4bb8_storage[11] = &pc_floor_switch_profile;
    data_020a4bb8_storage[12] = &pc_star_switch_profile;
    data_020a4bb8_storage[29] = &pc_updown_lift_profile;
    data_020a4bb8_storage[39] = &pc_battlefield_seesaw_profile;
    data_020a4bb8_storage[40] = &pc_battlefield_gate_profile;
    data_020a4bb8_storage[188] = &pc_koopa_race_profile;
    data_020a4bb8_storage[205] = &pc_koopa_flag_profile;
    data_020a4bb8_storage[46] = &pc_fortress_wall_profile;
    pc_fl_seesaw_init_resources();
    data_020a4bb8_storage[77] = &pc_fl_seesaw_profile;
    data_020a4bb8_storage[219] = g_profile_WANWAN;
    data_020a4bb8_storage[27] = g_profile_PILE;
    data_020a4bb8_storage[41] = g_profile_WANWAN_SHUTTER;
    pc_cannon_init_resources();pc_waterbomb_init_resources();
    data_020a4bb8_storage[207] = &pc_cannon_profile;
    data_020a4bb8_storage[208] = &pc_waterbomb_profile;
    data_020a4bb8_storage[339] = &pc_metalnet_profile;
    data_020a4bb8_storage[338] = &pc_castle_water_profile;
    data_020a4bb8_storage[34] = &pc_basement_pillar_profile;
    data_020a4bb8_storage[35] = &pc_basement_water_profile;
    data_020a4bb8_storage[286] = g_profile_TREE;
    data_020a4bb8_storage[347] = &pc_warp_profile;
    data_020a4bb8_storage[343] = g_profile_SBIRD;
    pc_toad_init_resources();
    data_020a4bb8_storage[185] = g_profile_KINOPIO;
    data_020a4bb8_storage[181] = &pc_buddy_profile;
    const char *qa_no_shutter=getenv("SM64DS_PC_TEST_NO_SHUTTER");
    const char *qa_only=getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(!(qa_only && !strcmp(qa_only,"1") && qa_no_shutter && !strcmp(qa_no_shutter,"1")))
        data_020a4bb8_storage[14] = g_profile_CANNON_SHUTTER;
    // Save & Quit returns to scene1 even when startup used the development
    // course picker. Keep its native factory available for that transition;
    // the boot scene still independently chooses the startup destination.
    data_020a4bb8_storage[1] = &pc_dsmt_profile;
    data_020a4bb8_storage[5] = &pc_minigame_menu_profile;
    data_020a4bb8_storage[366] = &pc_wanted_profile;
    data_020a4bb8_storage[0x120] = g_profile_COIN;
    data_020a4bb8_storage[0x121] = g_profile_RED_COIN;
    data_020a4bb8_storage[0x122] = g_profile_BLUE_COIN;
    data_020a4bb8_storage[200] = g_profile_KURIBO;
    data_020a4bb8_storage[9] = &pc_yoshi_egg_profile;
    data_020a4bb8_storage[201] = g_profile_KURIBO_S;
    data_020a4bb8_storage[202] = g_profile_KURIBO_L;
    data_020a4bb8_storage[178] = &pc_power_star_profile;
    data_020a4bb8_storage[179] = &pc_silver_star_profile;
    data_020a4bb8_storage[180] = &pc_star_marker_profile;
    data_020a4bb8_storage[269] = &pc_cap_reward_profile;
    pc_goomba_init_resources();
    pc_bombking_init_resources();
    _ZN13SharedFilePtr9ConstructEj(&data_ov102_0214e9c0,0x2b8);
    _ZN13SharedFilePtr9ConstructEj(&data_ov102_0214e9c8,0x2b7);
    data_020a4bb8_storage[189] = g_profile_BOMBKING;
    data_020a4bb8_storage[206] = g_profile_BOMBHEI;
    data_020a4bb8_storage[349] = &pc_exit_profile;
    data_020a4bb8_storage[335] = &pc_map_profile;
    data_020a4bb8_storage[352] = &pc_demo_profile;
    data_020a4bb8_storage[353] = &pc_door_profile;
    data_020a4bb8_storage[354] = g_profile_STAR_GATE;
    data_020a4bb8_storage[307] = &pc_picture_gate_profile;
    pc_sound_obj_init_callbacks();
    data_020a4bb8_storage[359] = &pc_sound_obj_profile;
#endif
    data_020a4bb8 = data_020a4bb8_storage;
    pc_sync_actor_profile_table_cpp();
    hal_fill_model_vtable();
    /* Player's SharedFilePtr resources and state-member tables are constructed
       by overlay 2's __sinit routine on the DS. Static linking the code does
       not run that overlay-owned initializer, so run it once after providing
       host storage for the overlay globals. */
    if (!pc_player_overlay_data_initialized) {
#if defined(SM64DS_GAME_TARGET)
        pc_demo_init_jugem_resources();
#endif
        __sinit_ov002_02100560();
        __sinit_ov002_021019d0();
        // The map projection subtracts/adds this constructor-owned screen
        // center (128,0,96). Zero BSS is not its initialized value.
        __sinit_ov002_0210804c();
        /* Shared animation records are constructor-owned on the DS. Native
           host static linking does not run that overlay startup automatically. */
#define PC_INIT_PLAYER_ANIM_HANDLE(suffix, file_id) \
        _ZN13SharedFilePtr9ConstructEj(data_ov002_0210##suffix, file_id)
        // e168 was initialized above by the original constructor to archive
        // animation0x8099 (including Yoshi's slide). A legacy0x4c0 override
        // replaced it with a Mario head BMD and crashed the bone-animation reader.
        /* e1c8 is also a live texture-sequence handle (0x80b1). The player
           texture initializer owns this record, so don't replace it with a
           BCA animation handle here. */
        PC_INIT_PLAYER_ANIM_HANDLE(e468, 0x4c2);
        PC_INIT_PLAYER_ANIM_HANDLE(e470, 0x4c3);
        PC_INIT_PLAYER_ANIM_HANDLE(e280, 0x4c4);
        PC_INIT_PLAYER_ANIM_HANDLE(e488, 0x4c5);
        PC_INIT_PLAYER_ANIM_HANDLE(e228, 0x4c6);
        PC_INIT_PLAYER_ANIM_HANDLE(e498, 0x4c7);
        PC_INIT_PLAYER_ANIM_HANDLE(e4a0, 0x4c8);
        PC_INIT_PLAYER_ANIM_HANDLE(e840, 0x4cf);
#undef PC_INIT_PLAYER_ANIM_HANDLE
        /* The LevelEnter state initializer copies ARM9 addresses from ROM
           constants. Replace its three destination member pointers with the
           linked host methods; ChangeState dispatches through this record. */
        *(unsigned int *)(data_ov002_021104b4 + 0x00) =
            pc_player_method_address(&Player::St_LevelEnter_Init);
        *(unsigned int *)(data_ov002_021104b4 + 0x08) =
            pc_player_method_address(&Player::St_LevelEnter_Main);
        *(unsigned int *)(data_ov002_021104b4 + 0x10) =
            pc_player_method_address(&Player::St_LevelEnter_Cleanup);
        pc_fill_player_level_enter_callbacks();
        /* Player constructor side tables store ARM9 pointers; map their
           ordinary FS handles through the locally generated ROM catalog. */
        extern void pc_fs_init_handle_table(void);
        pc_fs_init_handle_table();
        /* The DS path resolver fills these globals before overlay static
           initializers. Build their host equivalent from the generated local
           handle catalog, so SharedFilePtr constructors can resolve IDs. */
        data_0209d3b8 = (unsigned short *)calloc(0x80a, sizeof(unsigned short));
        data_0209d3b0 = 0x80a;
        if (!data_0209d3b8) abort();
        for (unsigned int handleId = 0; handleId < 0x80a; ++handleId) {
            unsigned int fileId = 0;
            extern unsigned int pc_fs_handle_to_file_id(unsigned int);
            fileId = pc_fs_handle_to_file_id(handleId);
            data_0209d3b8[handleId] = (unsigned short)fileId;
        }
        /* Overlay 2 stores ARM9 addresses in this 100-entry texture-sequence
           table. Rebase them before Player's setup code dereferences handles. */
        for (int i = 0; i < 100; ++i) {
            const unsigned int address = (unsigned int)(uintptr_t)data_ov002_020ff2f0[i];
            void *handle = pc_resolve_player_texseq_handle(address);
            if (handle)
                data_ov002_020ff2f0[i] = handle;
            else {
                const char *trace = getenv("SM64DS_PC_TRACE");
                if (trace && trace[0] && trace[0] != '0')
                    fprintf(stderr, "[sm64ds] unresolved player texseq entry[%d]=%08x\n",
                            i, address);
            }
            const char *trace = getenv("SM64DS_PC_TRACE");
            if (trace && trace[0] && trace[0] != '0')
                fprintf(stderr, "[sm64ds] player texseq slot[%d] guest=%08x host=%p file=%04x\n",
                        i, address, (void *)(uintptr_t)data_ov002_020ff2f0[i],
                        *(unsigned short *)((uintptr_t)data_ov002_020ff2f0[i]));
        }
        /* The 1-Up logo handle is a class-specific global constructor in
           the actor TU. That TU is not part of the PC boot source closure,
           but Player's resource path consumes its model handle directly. */
        _ZN13SharedFilePtr9ConstructEj(data_ov002_02110aa4, 0x436);
        /* Animation IDs index a 736-entry ROM table. Rebase each SharedFilePtr
           address into the statically linked ov002 handle block. */
        for (int i = 0; i < 736; ++i) {
            const unsigned int address = data_ov002_020ff480[i];
            void *handle = pc_resolve_player_anim_handle(address);
            if (handle)
                data_ov002_020ff480[i] = (unsigned int)(uintptr_t)handle;
            else {
                const char *trace = getenv("SM64DS_PC_TRACE");
                if (trace && trace[0] && trace[0] != '0')
                    fprintf(stderr, "[sm64ds] unresolved player anim entry[%d]=%08x\n",
                            i, address);
            }
        }
        data_ov002_020ff790[0] = data_ov002_0210e798;
        data_ov002_020ff790[1] = data_ov002_0210e798;
        data_ov002_020ff790[2] = data_ov002_0210e4f0;
        data_ov002_020ff790[3] = data_ov002_0210e798;
        /* These three four-entry tables name the body, cap, and shell model
           handles through ARM9 addresses in the extracted overlay bytes. */
        void **modelHandles = (void **)data_ov002_0210a68c;
        modelHandles[0] = data_ov002_0210ec50;
        modelHandles[1] = data_ov002_0210ebb8;
        modelHandles[2] = data_ov002_0210ec60;
        modelHandles[3] = data_ov002_0210e818;
        modelHandles = (void **)data_ov002_0210a69c;
        modelHandles[0] = data_ov002_0210e690;
        modelHandles[1] = data_ov002_0210eb20;
        modelHandles[2] = data_ov002_0210e758;
        modelHandles[3] = data_ov002_0210e7a8;
        modelHandles = (void **)data_ov002_0210a6ac;
        modelHandles[0] = data_ov002_0210ec38;
        modelHandles[1] = data_ov002_0210eb00;
        modelHandles[2] = data_ov002_0210e790;
        modelHandles[3] = data_ov002_0210e7f0;
        pc_player_overlay_data_initialized = true;
#if defined(SM64DS_GAME_TARGET)
        pc_door_init_tables();
#endif
        const char *trace = getenv("SM64DS_PC_TRACE");
        if (trace && trace[0] && trace[0] != '0') {
            unsigned char *handles[] = {
                data_ov002_0210e798, data_ov002_0210e4f0,
                data_ov002_0210e3d8, data_ov002_0210e6c0,
                data_ov002_0210eb98, data_ov002_0210eb88,
                data_ov002_0210e4c0,
            };
            for (int i = 0; i < 7; ++i)
                fprintf(stderr, "[sm64ds] ov002 Player handle[%d]=%04x storage=%p\n",
                        i, *(unsigned short *)handles[i], (void *)handles[i]);
        }
    }
}

#endif // SM64DS_GAME_TARGET

void *_ZN7fBase_cC2Ev(char *self)
{
    *(void **)self = data_02099edc;
    _ZN7fBase_c9SceneNodeC1Ev(self + 0x14);
    *(void **)(self + 0x24) = self;             /* sceneNode.actor */
    for (int off = 0x28; off <= 0x38; off += 0x10) {
        *(void **)(self + off) = 0;
        *(void **)(self + off + 4) = 0;
        *(void **)(self + off + 8) = self;
        *(unsigned short *)(self + off + 0xc) = 0;
        *(unsigned short *)(self + off + 0xe) = 0;
    }
    int id = data_02099e70[0];
    *(int *)(self + 4) = id;
    data_02099e70[0] = id + 1;
    *(int *)(self + 8) = data_020a4b60[0];
    *(unsigned short *)(self + 0xc) = data_020a4b54;
    *(unsigned char *)(self + 0x12) = data_020a4b48;
    func_0203b438(data_020a4b6c, self + 0x14, (void *)(size_t)data_020a4b64[0]);
    {
        unsigned short *info = (unsigned short *)data_020a4bb8[
            *(unsigned short *)(self + 0xc)];
        *(unsigned short *)(self + 0x28 + 0xc) = info[2];   /* prio at +4 */
        *(unsigned short *)(self + 0x28 + 0xe) = info[2];
        *(unsigned short *)(self + 0x38 + 0xc) = info[3];   /* prio at +6 */
        *(unsigned short *)(self + 0x38 + 0xe) = info[3];
    }
    {
        char *parent = (char *)(size_t)func_02043810(data_020a4b6c);
        if (parent) {
            unsigned char pf = *(unsigned char *)(parent + 0x13);
            if (pf & 3)
                *(unsigned char *)(self + 0x13) |= 2;
            if (pf & 0xC)
                *(unsigned char *)(self + 0x13) |= 8;
        }
    }
    return self;
}
} /* extern "C" */

// ---- gate-9 storage and bridges -------------------------------------------
extern "C" {
void *_ZTV17dExtShadowModel_c[8];

// ov098's SharedFilePtr entry table for the arrow signs: three-pointer
// entries {model, kcl, ?}. The smoke seeds the pointers with its own
// SharedFilePtr objects (on DS these live prebuilt in the overlay's data).
void *data_ov098_0213c380[6];
} /* extern "C" */

extern "C" {
/* asm primitive: plain 48-byte block copy (with writeback, unlike the
   FIFO-fixed variant) */
void Copy48Bytes(int *src, int *dst) { for (int i = 0; i < 12; ++i) dst[i] = src[i]; }
// Shared Matrix4x3 scratch: twelve words. A four-word allocation lets model
// translation writes corrupt the neighboring spawn context and actor list.
int data_020a0e68[12];
static_assert(sizeof(data_020a0e68) == 48, "shared Matrix4x3 extent");
short data_0208e378;
short *data_0209b45c;      /* spawn default rotation ptr (null = none) */
short *data_0209b460;      /* spawn default position ptr */
signed char data_0209b44c_c;
int data_0209b468[4];      /* actor list head the ctor links into */
}
#pragma comment(linker, "/alternatename:?data_0209b44c@@3CA=_data_0209b44c_c")
#pragma comment(linker, "/alternatename:_data_0209b44c=_data_0209b44c_c")

// The C base constructor stores this address before the derived spawn replaces
// it with ArrowSignRight's manual host vtable. No base virtual dispatch occurs
// during that interval, but the storage must exist for the raw relocation.
extern "C" void *_ZTV10dBgActor_c[32] = {};


extern "C" {
unsigned char data_0209f2d8_c;   /* mega-char state byte: none */
int data_0209ceec[4];            /* shadow scale globals */
int data_0209cef4[4];
}
#pragma comment(linker, "/alternatename:_data_0209f2d8=_data_0209f2d8_c")
#pragma comment(linker, "/alternatename:?data_020a0e68@@3UMatrix4x3@@A=_data_020a0e68")

extern "C" {
void Matrix4x3_FromRotationY(void *m, int a);
void hal_m43_roty(void *m, int a) { Matrix4x3_FromRotationY(m, a); }
}

extern "C" {
/* asm primitive: 4x3 fx32 Y-rotation from (sin, cos):
   rows {c,0,-s},{0,1,0},{s,0,c},{0,0,0} */
void func_02052820(int *m, int s, int c)
{
    m[0] = c;      m[1] = 0; m[2] = -s;
    m[3] = 0;      m[4] = 0x1000; m[5] = 0;
    m[6] = s;      m[7] = 0; m[8] = c;
    m[9] = 0;      m[10] = 0; m[11] = 0;
}
}

// Cleanup-path heap wrappers: unnamed on the DS side here; the semantics
// are the gate-3a Memory layer's. Destroy is never reached in the gate
// (the smoke does not tear its heap-owning actor down), so it traps.
extern "C" {
void Memory_Deallocate(void *p);
void Heap_Destroy(void *h)
{
    (void)h;
    fprintf(stderr, "FATAL: Heap_Destroy reached (unwired)\n");
    abort();
}
}

extern "C" {
int func_0204424c(char *c);
int hal_f0204424c(char *c) { return func_0204424c(c); }
}
extern "C" { int data_020a4b68[1]; }
extern "C" {
signed char data_0209f2f8;
signed char data_0209f2f4;
unsigned char data_0209f208;
unsigned char data_0209f1f4;
unsigned char data_0209f2bc;
short data_0209f308;
unsigned int data_0209f34c;
unsigned int data_0209f4f8[4096];
int data_0209f358[4];
char *data_0209f314;
unsigned char data_0209f37c[12];
int data_0209f40c[12];
}

#pragma comment(linker, "/alternatename:?data_0209f34c@@3HA=_data_0209f34c")

extern "C" {
void func_02052800(int *m, int s, int c)
{
    m[0] = 0x1000; m[1] = 0;     m[2] = 0;
    m[3] = 0;      m[4] = c;     m[5] = s;
    m[6] = 0;      m[7] = -s;    m[8] = c;
    m[9] = 0;      m[10] = 0;    m[11] = 0;
}
void func_0205283c(int *m, int s, int c)
{
    m[0] = c;      m[1] = s;     m[2] = 0;
    m[3] = -s;     m[4] = c;     m[5] = 0;
    m[6] = 0;      m[7] = 0;     m[8] = 0x1000;
    m[9] = 0;      m[10] = 0;    m[11] = 0;
}
}

/* BSS globals owned by the actor/stage runtime. Scalar types and the clipper
   extent come from their production declarations; the save flags cover all
   valid course IDs, with host room for the full byte-indexed course domain. */
extern "C" {
u32 data_0209b464;
#if !defined(SM64DS_GAME_TARGET)
u8 data_0209cab4[0x40];
#endif
s32 data_0209e650;
u8 data_0209f20c;
u8 data_0209f220;
u8 data_0209f274;
u8 data_0209f294;
u8 data_0209f2c0;
u8 data_0209f2c4;
void *data_0209f318;
dClipper data_0209f43c;
void *data_0209ee74;
fBase_c *data_0209f5c0;
s32 data_0209fc68;
}




