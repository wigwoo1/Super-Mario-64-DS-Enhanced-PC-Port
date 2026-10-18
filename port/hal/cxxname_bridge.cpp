// Bridges for TUs that declare C-NAMED symbols at C++ linkage (an extern
// declaration outside the file's extern "C" block): the reference mangles
// as ?_ZN...@@YA..., so a same-shape C++ definition here forwards to the
// real implementation. The forward hop goes through a differently-named
// extern "C" helper because one TU cannot name both linkages of the same
// identifier.
#include <new>
#include <cstdio>
#include <cstdlib>
#include "ntr/rt.h"

#include "dBgW.h"
#include "dCc_c.h"
#include "OAM.h"
#include "OamAttr.h"
#include "math/Matrix.h"
#include "dCamera_c.h"
#include "PathPtr.h"
#include "daStar_c.h"
#include "daKpa_c.h"
#include "daKpaTail_c.h"
#include "dMgJump3DMario_c.h"
#include "dScMgTrampoline_c.h"
#include "dScMgTrampoline2_c.h"
#include "ModelAnim.h"

/* Four shared {held, pressed} key pairs live in input_state.asm. The pressed
   label is the halfword at base+2, rather than a separate array. */

#ifdef SMOKE_ACTOR_LINK_FIXTURES
template <typename T>
static T actor_smoke_unexpected(const char *symbol)
{
    fprintf(stderr, "FATAL: actor smoke reached out-of-scope path %s\n", symbol);
    abort();
    return T();
}

static void actor_smoke_unexpected_void(const char *symbol)
{
    fprintf(stderr, "FATAL: actor smoke reached out-of-scope path %s\n", symbol);
    abort();
}
#endif

namespace cstd {
unsigned int strlen(const char *);
long long ldiv_result(void);
}
void CopyToViewMat(const Matrix4x3 *source);

extern "C" int _ZN4dBgW9IsEnabledEv(void *self)
{
    return ((dBgW *)self)->dBgW::IsEnabled();
}

extern "C" int _ZN4dBgW7DisableEv(void *self)
{
    return ((dBgW *)self)->dBgW::Disable();
}

extern "C" void _ZN4dBgW16UpdatePosAndAngsERS_P8dActor_cR5dBgPiR7Vector3P10Vector3_16S8_(
    dBgW *collider, dActor_c *actor, dBgPi *result, Vector3 *position,
    Vector3_16 *motionAngle, Vector3_16 *angle)
{
    dBgW::UpdatePosAndAngs(*collider, actor, *result, *position,
                           motionAngle, angle);
}

extern "C" unsigned int _ZN4cstd6strlenEPKc(const char *text)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    unsigned int length = 0;
    while (text[length]) ++length;
    return length;
#else
    return cstd::strlen(text);
#endif
}

extern "C" long long _ZN4cstd11ldiv_resultEv(void)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    return actor_smoke_unexpected<long long>("cstd::ldiv_result");
#else
    return cstd::ldiv_result();
#endif
}

extern "C" void _Z13CopyToViewMatPK9Matrix4x3(const Matrix4x3 *source)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)source;
    actor_smoke_unexpected_void("CopyToViewMat");
#else
    CopyToViewMat(source);
#endif
}

extern "C" int _ZN9dCamera_c11ChangeStateEPNS_5StateE(
    dCamera_c *camera, dCamera_c::State *state)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)camera; (void)state;
    return actor_smoke_unexpected<int>("dCamera_c::ChangeState");
#else
    return camera->dCamera_c::ChangeState(state);
#endif
}

extern "C" void _ZN9dCamera_c25SaveCameraStateBeforeTalkEv(dCamera_c *camera)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)camera;
    actor_smoke_unexpected_void("dCamera_c::SaveCameraStateBeforeTalk");
#else
    camera->SaveCameraStateBeforeTalk();
#endif
}

extern "C" unsigned int _ZNK7PathPtr5LoopsEv(const PathPtr *path)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)path;
    return actor_smoke_unexpected<unsigned int>("PathPtr::Loops");
#else
    return path->Loops();
#endif
}

extern "C" unsigned int _ZNK7PathPtr13GetPullFactorEv(const PathPtr *path)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)path;
    return actor_smoke_unexpected<unsigned int>("PathPtr::GetPullFactor");
#else
    return path->GetPullFactor();
#endif
}

extern "C" void _ZN7PathPtr6FromIDEj(PathPtr *path, unsigned int id)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)path; (void)id;
    actor_smoke_unexpected_void("PathPtr::FromID");
#else
    path->PathPtr::FromID(id);
#endif
}

extern "C" unsigned int _ZNK7PathPtr8NumNodesEv(const PathPtr *path)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)path;
    return actor_smoke_unexpected<unsigned int>("PathPtr::NumNodes");
#else
    return path->PathPtr::NumNodes();
#endif
}

extern "C" void _ZNK7PathPtr7GetNodeER7Vector3j(
    const PathPtr *path, Vector3 *node, unsigned int index)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)path; (void)node; (void)index;
    actor_smoke_unexpected_void("PathPtr::GetNode");
#else
    path->PathPtr::GetNode(*node, index);
#endif
}

extern "C" unsigned int _ZNK7PathPtr9GetUnk004Ev(const PathPtr *path)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)path;
    return actor_smoke_unexpected<unsigned int>("PathPtr::GetUnk004");
#else
    return path->PathPtr::GetUnk004();
#endif
}

extern "C" void _ZN8daStar_cD1Ev(daStar_c *actor)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)actor;
    actor_smoke_unexpected_void("daStar_c::~daStar_c");
#else
    actor->daStar_c::~daStar_c();
#endif
}

extern "C" void _ZN6daKpa_cD1Ev(daKpa_c *actor)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)actor;
    actor_smoke_unexpected_void("daKpa_c::~daKpa_c");
#else
    actor->daKpa_c::~daKpa_c();
#endif
}

extern "C" void _ZN11daKpaTail_cD1Ev(daKpaTail_c *actor)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)actor;
    actor_smoke_unexpected_void("daKpaTail_c::~daKpaTail_c");
#else
    actor->daKpaTail_c::~daKpaTail_c();
#endif
}

extern "C" void _ZN16dMgJump3DMario_cC1Ev(dMgJump3DMario_c *object)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)object;
    actor_smoke_unexpected_void("dMgJump3DMario_c::dMgJump3DMario_c");
#else
    ::new (object) dMgJump3DMario_c();
#endif
}

extern "C" void _ZN16dMgJump3DMario_cD1Ev(dMgJump3DMario_c *object)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)object;
    actor_smoke_unexpected_void("dMgJump3DMario_c::~dMgJump3DMario_c");
#else
    object->~dMgJump3DMario_c();
#endif
}

extern "C" void _ZN17dScMgTrampoline_c9StateDoneEv(dScMgTrampoline_c *o)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)o; actor_smoke_unexpected_void("dScMgTrampoline_c::StateDone");
#else
    o->dScMgTrampoline_c::StateDone();
#endif
}
extern "C" void _ZN17dScMgTrampoline_c13StateWaitExitEv(dScMgTrampoline_c *o)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)o; actor_smoke_unexpected_void("dScMgTrampoline_c::StateWaitExit");
#else
    o->dScMgTrampoline_c::StateWaitExit();
#endif
}
extern "C" void _ZN17dScMgTrampoline_c12StateResultsEv(dScMgTrampoline_c *o)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)o; actor_smoke_unexpected_void("dScMgTrampoline_c::StateResults");
#else
    o->dScMgTrampoline_c::StateResults();
#endif
}
extern "C" void _ZN17dScMgTrampoline_c9StatePlayEv(dScMgTrampoline_c *o)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)o; actor_smoke_unexpected_void("dScMgTrampoline_c::StatePlay");
#else
    o->dScMgTrampoline_c::StatePlay();
#endif
}
extern "C" void _ZN17dScMgTrampoline_c10StateIntroEv(dScMgTrampoline_c *o)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)o; actor_smoke_unexpected_void("dScMgTrampoline_c::StateIntro");
#else
    o->dScMgTrampoline_c::StateIntro();
#endif
}
extern "C" void _ZN18dScMgTrampoline2_c9StateDoneEv(dScMgTrampoline2_c *o)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)o; actor_smoke_unexpected_void("dScMgTrampoline2_c::StateDone");
#else
    o->dScMgTrampoline2_c::StateDone();
#endif
}
extern "C" void _ZN18dScMgTrampoline2_c13StateWaitExitEv(dScMgTrampoline2_c *o)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)o; actor_smoke_unexpected_void("dScMgTrampoline2_c::StateWaitExit");
#else
    o->dScMgTrampoline2_c::StateWaitExit();
#endif
}
extern "C" void _ZN18dScMgTrampoline2_c12StateResultsEv(dScMgTrampoline2_c *o)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)o; actor_smoke_unexpected_void("dScMgTrampoline2_c::StateResults");
#else
    o->dScMgTrampoline2_c::StateResults();
#endif
}
extern "C" void _ZN18dScMgTrampoline2_c9StatePlayEv(dScMgTrampoline2_c *o)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)o; actor_smoke_unexpected_void("dScMgTrampoline2_c::StatePlay");
#else
    o->dScMgTrampoline2_c::StatePlay();
#endif
}
extern "C" void _ZN18dScMgTrampoline2_c10StateIntroEv(dScMgTrampoline2_c *o)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)o; actor_smoke_unexpected_void("dScMgTrampoline2_c::StateIntro");
#else
    o->dScMgTrampoline2_c::StateIntro();
#endif
}

// The ov098 file table's second column is addressed through its own symbol
// (base+4 on the DS -- an offset alias hosts cannot express). Column b gets
// its own 0xc-stride view; the smoke seeds both views with the same
// SharedFilePtr pointers.
extern "C" {
char data_ov098_0213c384[0x18];
char data_ov098_0213c388[0x18];
}

// ---- gate-9 method bridges (C name -> MSVC method), the gx_upload pattern -
#include "dBgActor_c.h"
#include "dBgCh_Actr.h"
#include "dBgCh_Gnd.h"
#include "dBgCh_Lin.h"
#include "dBgPi.h"
#include "dBgCh_SphCrr.h"
#include "dExtShadowModel_c.h"
#include "dScMgBase_c.h"
#include "Particle__SysTracker.h"
#include "dExtFrameCtrl_c.h"
#include "G2x.h"
#include "fBase_c.h"
#include "dWipe_c.h"
#include "dFdBrightness_c.h"
#include "dActor_c.h"
#include "Player.h"
#include "Sound.h"
#include "Model.h"

#if defined(SM64DS_GAME_TARGET)
// The retail startup constructs this singleton. A copied BSS allocation has
// no native vtable; SetFaders must receive a real, constructed native object.
extern "C" { dWipe_c data_0209f61c; }
// MSVC does not reuse the base tail padding: the native object is 48 bytes.
// All C/C++ symbol spellings bind here instead of the smaller ROM BSS copy.
static_assert(sizeof(dWipe_c)==48,"Native scene-wipe ABI changed");
#endif

// The recovered dWipe_c body keeps the cartridge's two-argument ARM ABI under
// its raw symbol name. The native vtable needs an MSVC member entry, so route
// that entry to the recovered implementation. For the brightness path, adapt
// the raw three-argument forwarder to the recovered one-argument member.
extern "C" int _ZN7dWipe_c15SetBackwardTimeEj(
    dWipe_c *self, unsigned int frames, unsigned int extra);
extern "C" int _ZN15dFdBrightness_c15SetBackwardTimeEj(
    dFdBrightness_c *self, unsigned int frames, unsigned int extra);

int dWipe_c::SetBackwardTime(u32 frames)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)frames;
    return actor_smoke_unexpected<int>("dWipe_c::SetBackwardTime");
#else
    return _ZN7dWipe_c15SetBackwardTimeEj(this, frames, 0);
#endif
}

extern "C" int _ZN15dFdBrightness_c15SetBackwardTimeEj(
    dFdBrightness_c *self, unsigned int frames, unsigned int extra)
{
#ifdef SMOKE_ACTOR_LINK_FIXTURES
    (void)self; (void)frames; (void)extra;
    return actor_smoke_unexpected<int>("dFdBrightness_c::SetBackwardTime");
#else
    (void)extra;
    return self->dFdBrightness_c::SetBackwardTime(frames);
#endif
}

#if defined(SM64DS_GAME_TARGET)
unsigned char _ZN3OAM11GetObjWidthEii(int shape, int sizeBits)
{ return ::OAM::GetObjWidth(shape, sizeBits); }
#pragma comment(linker, "/alternatename:?_ZN3OAM11GetObjWidthEii@@YAEHH@Z=_ZN3OAM11GetObjWidthEii")
unsigned char _ZN3OAM12GetObjHeightEii(int shape, int sizeBits)
{ return ::OAM::GetObjHeight(shape, sizeBits); }
#pragma comment(linker, "/alternatename:?_ZN3OAM12GetObjHeightEii@@YAEHH@Z=_ZN3OAM12GetObjHeightEii")
int _ZN3OAM16LoadAffineParamsEP7OamAttrPiP9Matrix2x2(
    OamAttr *attr, int *count, Matrix2x2 *matrix)
{ return ::OAM::LoadAffineParams(attr, count, matrix); }
#pragma comment(linker, "/alternatename:?_ZN3OAM16LoadAffineParamsEP7OamAttrPiP9Matrix2x2@@YAHPAUOamAttr@@PAHPAUMatrix2x2@@@Z=_ZN3OAM16LoadAffineParamsEP7OamAttrPiP9Matrix2x2")
extern "C" void _ZN3OAM6RenderEbP7OamAttriiiiP9Matrix2x2(
    bool draw, OamAttr *attr, int x, int y, int palette, int priority,
    Matrix2x2 *matrix)
{ ::OAM::Render(draw, attr, x, y, palette, priority, matrix); }
#pragma comment(linker, "/alternatename:?_ZN3OAM6RenderEbP7OamAttriiiiP9Matrix2x2@@YAX_NPAUOamAttr@@HHHHPAUMatrix2x2@@@Z=_ZN3OAM6RenderEbP7OamAttriiiiP9Matrix2x2")
#endif

#if defined(SM64DS_GAME_TARGET)
#pragma comment(linker, "/alternatename:?WillHitFrame@dExtFrameCtrl_c@@QBEHH@Z=__ZNK15dExtFrameCtrl_c12WillHitFrameEi")
#endif

bool ApproachLinear(short &value, short target, short step);
namespace GX {
void SetBankForBG(unsigned short bank);
void SetBankForOBJ(unsigned short bank);
}

extern "C" {
// MWCC's 8-byte virtual member pointers store a byte offset and a virtual
// dispatch tag. MSVC cannot call that representation directly. Translate the
// three Process arguments to the host object's same-numbered vtable slots.
struct RomVirtualMember { int vtable_offset; int tag; };
extern int data_0209f394[8];
extern signed char data_0209f2f8;
void _ZN7fBase_c18MarkForDestructionEv(void *self)
{ static_cast<fBase_c *>(self)->fBase_c::MarkForDestruction(); }
int _ZN7fBase_c7ProcessEMS_FivEMS_FbvEMS_FvjE(
    void *self, RomVirtualMember action, RomVirtualMember before,
    RomVirtualMember after)
{
    const char *trace = std::getenv("SM64DS_PC_TRACE");
    const bool tracing = trace && trace[0] && trace[0] != '0';
    if (tracing) {
        std::fprintf(stderr, "[sm64ds] fBase Process: begin self=%p slots=%d/%d/%d\n",
                     self, action.vtable_offset, before.vtable_offset,
                     after.vtable_offset);
        std::fflush(stderr);
    }
    const RomVirtualMember members[] = {action, before, after};
    for (const auto &member : members) {
        if (member.tag != 1 || member.vtable_offset < 0 ||
            member.vtable_offset % static_cast<int>(sizeof(void *)) != 0 ||
            member.vtable_offset >= 0x100) {
            std::fprintf(stderr, "invalid ARM virtual member pointer (%d, %d)\n",
                         member.vtable_offset, member.tag);
            std::abort();
        }
    }
    void **vtable = *static_cast<void ***>(self);
    const auto action_fn = reinterpret_cast<int (__thiscall *)(void *)>(
        vtable[action.vtable_offset / sizeof(void *)]);
    const auto before_fn = reinterpret_cast<int (__thiscall *)(void *)>(
        vtable[before.vtable_offset / sizeof(void *)]);
    const auto after_fn = reinterpret_cast<void (__thiscall *)(void *, unsigned)>(
        vtable[after.vtable_offset / sizeof(void *)]);

    if (tracing) {
        std::fprintf(stderr,
                     "[sm64ds] fBase Process: dispatch self=%p vtable=%p action=%p before=%p after=%p slots=%d/%d/%d\n",
                     self, vtable, action_fn, before_fn, after_fn,
                     action.vtable_offset / 4, before.vtable_offset / 4,
                     after.vtable_offset / 4);
        std::fflush(stderr);
    }

    auto *observed_player=reinterpret_cast<int *>(data_0209f394[0]);
    int observed_y=observed_player?observed_player[0x60/4]:0;
    auto observe_y=[&](const char *phase) {
        if(!observed_player || data_0209f2f8!=6 || !std::getenv("SM64DS_PC_ACTOR_Y_TRACE"))return;
        const int y=observed_player[0x60/4];
        static unsigned changes=0;
        if(y!=observed_y && changes++<500)
            std::fprintf(stderr,"[actor Y] tick=%llu id=%u action=%d phase=%s y=%d->%d ground=%d prev_y=%d\n",
                static_cast<unsigned long long>(ntr::rt_frame()),static_cast<fBase_c *>(self)->actorID,
                action.vtable_offset,phase,observed_y,y,observed_player[0x644/4],observed_player[0x6c/4]);
        observed_y=y;
    };
    int result = before_fn(self);
    observe_y("before");
    if (tracing) {
        const unsigned char *bytes = static_cast<const unsigned char *>(self);
        const int direct_before = static_cast<fBase_c *>(self)->fBase_c::BeforeBehavior();
        std::fprintf(stderr,
                     "[sm64ds] fBase Process: before returned %d direct=%d slot=%p state=%u kill=%u flags=%u\n",
                     result, direct_before, vtable[before.vtable_offset / sizeof(void *)],
                     bytes[0x0e], bytes[0x0f], bytes[0x13]);
        std::fflush(stderr);
    }
    unsigned status = 0;
    if (result != 0) {
        result = action_fn(self);
        observe_y("action");
        if (tracing) {
            std::fprintf(stderr, "[sm64ds] fBase Process: action returned %d\n", result);
            std::fflush(stderr);
        }
        status = result == -1 ? 3u : (result == 1 ? 2u : 1u);
    }
    after_fn(self, status);
    observe_y("after");
    if (tracing) {
        std::fprintf(stderr, "[sm64ds] fBase Process: after returned status=%u\n", status);
        std::fflush(stderr);
    }
    return result;
}

void _ZN10dBgActor_c19UpdateClsnPosAndRotEv(void *self)
{ ((dBgActor_c *)self)->dBgActor_c::UpdateClsnPosAndRot(); }
void _ZN10dBgActor_c21UpdateModelPosAndRotYEv(void *self)
{ ((dBgActor_c *)self)->dBgActor_c::UpdateModelPosAndRotY(); }
void _ZN10dBgActor_c14KillByMegaCharER6Player(void *self, Player *player)
{ ((dBgActor_c *)self)->dBgActor_c::KillByMegaChar(*player); }
int _ZN8dActor_c13DistToCPlayerEv(dActor_c *self)
{ return self->dActor_c::DistToCPlayer(); }
dActor_c *_ZN8dActor_c5SpawnEjjRK7Vector3PK10Vector3_16as(
    unsigned int actorID, unsigned int spawnParam, const Vector3 *pos,
    const Vector3_16 *rot, signed char areaID, short deathTableID)
{ return dActor_c::Spawn(actorID, spawnParam, *pos, rot, areaID, deathTableID); }
// Legacy C callers consume a full int. MSVC bool only defines AL, leaving the
// old state pointer in EAX's upper bytes and turning false into a nonzero int.
int _ZN6Player7IsStateERNS_5StateE(Player *self, Player::State *state)
{ return self->Player::IsState(*state) ? 1 : 0; }
void _ZN7fBase_c9SceneNodeC1Ev(void *self)
{ ::new (self) fBase_c::SceneNode(); }
void _ZN9dBgCh_GndC1Ev(void *self)
{ ::new (self) dBgCh_Gnd(); }
void _ZN9dBgCh_GndD1Ev(void *self)
{ ((dBgCh_Gnd *)self)->~dBgCh_Gnd(); }
void _ZN9dBgCh_Gnd12SetObjAndPosERK7Vector3P8dActor_c(
    void *self, const Vector3 *pos, dActor_c *actor)
{ ((dBgCh_Gnd *)self)->dBgCh_Gnd::SetObjAndPos(*pos, actor); }
int _ZN9dBgCh_Gnd10DetectClsnEv(void *self)
{ return ((dBgCh_Gnd *)self)->dBgCh_Gnd::DetectClsn(); }
void _ZN9dBgCh_Lin13SetObjAndLineERK7Vector3S2_P8dActor_c(
    void *self, const Vector3 *start, const Vector3 *end, dActor_c *actor)
{ ((dBgCh_Lin *)self)->dBgCh_Lin::SetObjAndLine(*start, *end, actor); }
bool _ZN9dBgCh_Lin10DetectClsnEv(void *self)
{ return ((dBgCh_Lin *)self)->dBgCh_Lin::DetectClsn(); }
void _ZN5dBgPiD1Ev(void *self)
{ ((dBgPi *)self)->~dBgPi(); }
void _ZN5dBgPiC1Ev(void *self)
{ ::new (self) dBgPi(); }
void _ZNK5dBgPi6CopyToERS_(const void *self, void *dst)
{ ((const dBgPi *)self)->CopyTo(*(dBgPi *)dst); }
void _ZN5dBgPiaSERKS_(void *self, const void *other)
{ *((dBgPi *)self) = *((const dBgPi *)other); }
void _ZN5dBgPi5ResetEv(void *self)
{ ((dBgPi *)self)->dBgPi::Reset(); }
int _ZNK5dBgPi16GetColliderIndexEv(const void *self)
{ return ((const dBgPi *)self)->dBgPi::GetColliderIndex(); }
int _ZNK5dBgPi7IsValidEv(const void *self)
{ return ((const dBgPi *)self)->dBgPi::IsValid(); }
int _ZNK5dBgPi7IsHitByEiP8dActor_cP4dBgW(
    const void *self, int uid, dActor_c *owner, dBgW *collider)
{ return ((const dBgPi *)self)->dBgPi::IsHitBy(uid, owner, collider); }
unsigned _ZNK5dBgPi9GetClsnIDEv(const void *self)
{ return ((const dBgPi *)self)->dBgPi::GetClsnID(); }
int _ZNK10dBgCh_Actr8IsOnWallEv(const void *self)
{ return ((const dBgCh_Actr *)self)->dBgCh_Actr::IsOnWall(); }
int _ZNK10dBgCh_Actr10IsOnGroundEv(const void *self)
{ return ((const dBgCh_Actr *)self)->dBgCh_Actr::IsOnGround(); }
void *_ZNK10dBgCh_Actr13GetWallResultEv(const void *self)
{ return ((const dBgCh_Actr *)self)->dBgCh_Actr::GetWallResult(); }
void *_ZNK10dBgCh_Actr14GetFloorResultEv(const void *self)
{ return ((const dBgCh_Actr *)self)->dBgCh_Actr::GetFloorResult(); }
void _ZN12dBgCh_SphCrr14SetFloorResultERK5dBgPi(void *self, const dBgPi *src)
{ ((dBgCh_SphCrr *)self)->dBgCh_SphCrr::SetFloorResult(*src); }
void *_ZN5Model8LoadFileER13SharedFilePtr(void *fp)
{
    void *file = Model::LoadFile(*(SharedFilePtr *)fp);
    if (std::getenv("SM64DS_PC_TRACE") && fp) {
        const unsigned int fileID = *(const unsigned int *)fp;
        const unsigned int *words = (const unsigned int *)file;
        std::fprintf(stderr, "[sm64ds] Model::LoadFile id=%04x handle=%p result=%p",
                     fileID, fp, file);
        if (words) {
            for (int i = 0; i < 8; ++i)
                std::fprintf(stderr, " %08x", words[i]);
        }
        std::fputc('\n', stderr);
    }
    return file;
}

}

Vector3 dBgCh_Lin::GetClsnPos() { return lineEnd; }

#include "dBgW_Kc.h"
#include "ModelBase.h"
extern "C" {
void *_ZN7dBgW_Kc8LoadFileER13SharedFilePtr(void *fp)
{ return dBgW_Kc::LoadFile(*(SharedFilePtr *)fp); }
void _ZN9ModelBase7SetFileEP8BMD_Fileii(void *self, void *bmd, int a, int b)
{
    if (std::getenv("SM64DS_PC_TRACE")) {
        const unsigned int *words = (const unsigned int *)bmd;
        std::fprintf(stderr, "[sm64ds] ModelBase::SetFile self=%p vptr=%p bmd=%p a=%d b=%d",
                     self, self ? *(void **)self : nullptr, bmd, a, b);
        if (words) {
            for (int i = 0; i < 12; ++i)
                std::fprintf(stderr, " %08x", words[i]);
        }
        std::fputc('\n', stderr);
    }
    ((ModelBase *)self)->ModelBase::SetFile((BMD_File *)bmd, a, b);
}
}
#pragma comment(linker, "/alternatename:?data_ov098_0213c380@@3PADA=_data_ov098_0213c380")
#pragma comment(linker, "/alternatename:?data_ov098_0213c384@@3PADA=_data_ov098_0213c384")
// Some recovered overlay callers declare this method as void because they
// discard the result. Alias that call-site ABI to the recovered int method.
#pragma comment(linker, "/alternatename:?SetFile@ModelBase@@QAEXPAUBMD_File@@HH@Z=?SetFile@ModelBase@@QAEHPAUBMD_File@@HH@Z")
// dScStage_c's recovered methods are defined in local shadow declarations
// that are non-virtual. The native stage vtable refers to the same bodies as
// virtual members, so alias those three host decorations to the existing code.
#pragma comment(linker, "/alternatename:?InitResources@dScStage_c@@UAEHXZ=?InitResources@dScStage_c@@QAEHXZ")
#pragma comment(linker, "/alternatename:?Behavior@dScStage_c@@UAEHXZ=?Behavior@dScStage_c@@QAEHXZ")
#pragma comment(linker, "/alternatename:?Render@dScStage_c@@UAEHXZ=?Render@dScStage_c@@QAEHXZ")
#pragma comment(linker, "/alternatename:?Render@Model@@QAEXPBUVector3@@@Z=?Render@Model@@UAEXPBUVector3@@@Z")
#pragma comment(linker, "/alternatename:?RenderAll@dExtShadowModel_c@@YAXXZ=?RenderAll@dExtShadowModel_c@@SAXXZ")
#pragma comment(linker, "/alternatename:?GetBG1ScrPtr@G2@@SAPAXXZ=?GetBG1ScrPtr@G2@@YAPAXXZ")
#pragma comment(linker, "/alternatename:?ST_WAIT@Player@@2UState@1@A=_data_ov002_02110154")
#pragma comment(linker, "/alternatename:?_ZN6Player7SetAnimEji5Fix12IiEj@@YAXPAXIHHI@Z=__ZN6Player7SetAnimEji5Fix12IiEj")
#pragma comment(linker, "/alternatename:?_ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_@@YAXIHHH@Z=__ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_")
#pragma comment(linker, "/alternatename:?_ZN8Particle6System3NewEjj5Fix12IiES2_S2_PK11Vector3_16fPNS_8CallbackE@@YAHIIHHHPAX0@Z=__ZN8Particle6System3NewEjj5Fix12IiES2_S2_PK11Vector3_16fPNS_8CallbackE")

extern "C" void _ZN7dBgW_Kc7SetFileEP8KCL_FileR10CLPS_Block(
    void *self, void *kcl, void *clps)
{
    ((dBgW_Kc *)self)->dBgW_Kc::SetFile((KCL_File *)kcl,
                                                  *(CLPS_Block *)clps);
}

// The 3DS image names these ROM vtables and shared model templates by their
// Itanium data symbols. Host objects use MSVC's vftable name and the gate's
// template BMD storage, so bind the original spellings to those same objects.
#pragma comment(linker, "/alternatename:_data_02099368=??_7dBgPi@@6B@")
#pragma comment(linker, "/alternatename:??1dBgCh_Gnd@@QAE@XZ=??1dBgCh_Gnd@@UAE@XZ")
#pragma comment(linker, "/alternatename:??1ModelAnim@@QAE@XZ=??1ModelAnim@@UAE@XZ")

// operator new support: the game heap pointer for actors (the smoke seeds
// it with the root heap), and the zero-fill veneer -- its DS chain rides
// arguments through registers, so the host supplies the semantics direct.
extern "C" {
extern void *GAME_HEAP_PTR;
}
// Both ROM symbol names are address 020a0eac. Separate host storage makes
// actor allocation follow the current default heap instead of the game heap.
#pragma comment(linker, "/alternatename:_data_020a0eac_c=_GAME_HEAP_PTR")
#pragma comment(linker, "/alternatename:?data_020a0eac@@3PAUHeap@@A=_GAME_HEAP_PTR")
#pragma comment(linker, "/alternatename:_data_020a0eac=_GAME_HEAP_PTR")
extern "C" void hal_m43_roty(void *m, int a);
void Matrix4x3_FromRotationY(void *m, int a) { hal_m43_roty(m, a); }

#include "dBgW.h"
extern "C" int _ZN4dBgW6EnableEP8dActor_c(void *self, void *actor)
{
    return ((dBgW *)self)->dBgW::Enable((dActor_c *)actor);
}
extern "C" {
/* player-list globals ClosestPlayer scans: empty world -> null result */
int data_0208e37c[2];
int data_0208e380[2];
int data_0209b450[2];
int data_0209b458[2];
int data_0209f21c[8];
int data_0209f394[8];
}
#pragma comment(linker, "/alternatename:?data_0209f394@@3PAPAXA=_data_0209f394")

extern "C" void _ZN6Memory10DeallocateEPv(void *p);
extern "C" void Memory_Deallocate(void *p) { _ZN6Memory10DeallocateEPv(p); }
extern "C" int hal_f0204424c(char *c);
int func_0204424c(int c) { return hal_f0204424c((char *)c); }

// gate 9: the Model vtable gains real slots at RUNTIME (targets without the
// Model methods keep the zero storage; nothing dispatches there). The host
// headers reserve both DS destructor slots as virtuals, so DoSetFile is slot
// 2, followed by UpdateVerts, Virtual10, and Render in slots 3-5. Keep this
// aligned with ModelBase::SetFile's virtual dispatch; shifting these one slot
// early makes SetFile invoke UpdateVerts on an uninitialized model.
static void __fastcall mv_destroy(void *self, void *)
{ ((Model *)self)->Model::~Model(); }
static void __fastcall mv_delete(void *self, void *)
{ ((Model *)self)->Model::~Model(); Model::operator delete(self); }
static void __fastcall mva_destroy(void *self, void *)
{ ((ModelAnim *)self)->ModelAnim::~ModelAnim(); }
static void __fastcall mva_delete(void *self, void *)
{ ((ModelAnim *)self)->ModelAnim::~ModelAnim(); ModelAnim::operator delete(self); }
static int __fastcall mv_dosetfile(void *self, void *, char *f, int a, int b)
{ return ((Model *)self)->Model::DoSetFile(f, a, b); }
static void __fastcall mv_updateverts(void *self, void *)
{ ((Model *)self)->Model::UpdateVerts(); }
static void __fastcall mv_virtual10(void *self, void *, void *m)
{ ((Model *)self)->Model::Virtual10(*(Matrix4x3 *)m); }
static void __fastcall mv_render(void *self, void *, const void *s)
{ ((Model *)self)->Model::Render((const Vector3 *)s); }
static void __fastcall mva_updateverts(void *self, void *)
{ ((ModelAnim *)self)->ModelAnim::UpdateVerts(); }
static void __fastcall mva_virtual10(void *self, void *, void *m)
{ ((ModelAnim *)self)->ModelAnim::Virtual10(*(Matrix4x3 *)m); }
static void __fastcall mva_render(void *self, void *, const void *s)
{ ((ModelAnim *)self)->ModelAnim::Render((const Vector3 *)s); }
static void __fastcall mva_virtual18(void *self, void *, u32 mat, const void *s)
{ ((ModelAnim *)self)->ModelAnim::Virtual18(mat, (const Vector3 *)s); }
extern "C" {
extern void *_ZTV5Model[8];
extern void *_ZTV9ModelAnim[10];
void hal_fill_model_vtable(void)
{
    _ZTV5Model[0] = (void *)mv_destroy;
    _ZTV5Model[1] = (void *)mv_delete;
    _ZTV5Model[2] = (void *)mv_dosetfile;
    _ZTV5Model[3] = (void *)mv_updateverts;
    _ZTV5Model[4] = (void *)mv_virtual10;
    _ZTV5Model[5] = (void *)mv_render;
    _ZTV9ModelAnim[0] = (void *)mva_destroy;
    _ZTV9ModelAnim[1] = (void *)mva_delete;
    _ZTV9ModelAnim[2] = (void *)mv_dosetfile;
    _ZTV9ModelAnim[3] = (void *)mva_updateverts;
    _ZTV9ModelAnim[4] = (void *)mva_virtual10;
    _ZTV9ModelAnim[5] = (void *)mva_render;
    _ZTV9ModelAnim[6] = (void *)mva_virtual18;
    /* Slot 5 too: TUs that dispatch through LOCAL shadow classes count in
       ROM/Itanium numbering (two dtor slots), which lands Render at 5.
       Model.h-compiled TUs land it at 4. The object serves both. */
}
}

#include "dExtShadowModel_c.h"
extern "C" int _ZN17dExtShadowModel_c10InitCuboidEv(dExtShadowModel_c *self)
{ return self->dExtShadowModel_c::InitCuboid(); }
extern "C" {
void hal_fill_shadow_vtable(void) {}   /* shadow system deferred */
}

// Raw Itanium spellings retained by C and local-shadow callers after the real
// definitions migrated to C++. Each wrapper uses an explicit qualified call so
// a synthetic host vtable cannot recurse back into the shim.
#include "dActor_c.h"
#include "Heap.h"
extern "C" {
Player *_ZN8dActor_c13ClosestPlayerEv(dActor_c *self)
{ return self->dActor_c::ClosestPlayer(); }
void Heap_Destroy(void *heap);
}
void Heap::_Destroy() { Heap_Destroy(this); }

// fBase_c's local static-class shadow and the migrated namespace function are
// both cdecl with the same two pointer arguments; only their decorations differ.
#pragma comment(linker, "/alternatename:?Deallocate@Memory@@SAXPAXPAUHeap@@@Z=?Deallocate@Memory@@YAXPAXPAVHeap@@@Z")

extern "C" {
void *_ZN5Model23AddToCommonModelDataArrER8BMD_File(void *file)
{ return Model::AddToCommonModelDataArr(*(BMD_File *)file); }
void *func_0203cc0c(unsigned size);
void _ZN6Memory10DeallocateEPv(void *p);
/* the DS global operator new/delete route through the Memory layer */
/* _ZN6Memory16operator_delete2EPv now lives in hal/ctor_bridge.cpp:
   every target that builds a migrated model-family constructor needs
   it, not just the ones that link this file. */
}

#include "fListNode.h"
#include "Heap.h"
#include "Particle__Manager.h"
extern "C" {
void _ZN11fLiNdBaPr_cD1Ev(void *self)
{ ((fLiNdBaPr_c *)self)->~fLiNdBaPr_c(); }
}
#pragma comment(linker, "/alternatename:?_ZN7dBgW_Kc7SetFileEP8KCL_FileR10CLPS_Block@@YAXPAX00@Z=__ZN7dBgW_Kc7SetFileEP8KCL_FileR10CLPS_Block")
#pragma comment(linker, "/alternatename:__ZN10dBgW_KcMbg7SetFileEP8KCL_FileRK9Matrix4x35Fix12IiEsR10CLPS_Block=?_ZN10dBgW_KcMbg7SetFileEP8KCL_FileRK9Matrix4x35Fix12IiEsR10CLPS_Block@@YAXPAX0PBUMatrix4x3@@HF0@Z")
#pragma comment(linker, "/alternatename:?DisappearPoofDustAt@daObjYajirusi_c@@QAEXABUVector3@@@Z=?DisappearPoofDustAt@dActor_c@@QAEXABUVector3@@@Z")
#pragma comment(linker, "/alternatename:?MarkForDestruction@daObjYajirusi_c@@QAEXXZ=?MarkForDestruction@fBase_c@@QAEXXZ")
#pragma comment(linker, "/alternatename:__ZN17dExtShadowModel_c9InitModelEP9Matrix4x35Fix12IiES3_S3_j=?_ZN17dExtShadowModel_c9InitModelEP9Matrix4x35Fix12IiES3_S3_j@@YAXPAUdExtShadowModel_c@@HHHHE@Z")
#pragma comment(linker, "/alternatename:?func_0203564c@@YAPADPAD@Z=_func_0203564c")

extern "C" {
dActor_c *_ZN8dActor_c10FindWithIDEj(unsigned id)
{ return dActor_c::FindWithID(id); }
int _ZN6Player7TryGrabER8dActor_c(Player *self, dActor_c *actor)
{ return self->TryGrab(*actor); }
void _ZN8dActor_c17HugeLandingDustAtER7Vector3b(dActor_c *, Vector3 *, bool);
void _ZN8dActor_c13LandingDustAtER7Vector3b(dActor_c *, Vector3 *, bool);
}
void dActor_c::HugeLandingDustAt(Vector3 &pos, bool doRaycast)
{ _ZN8dActor_c17HugeLandingDustAtER7Vector3b(this, &pos, doRaycast); }
void dActor_c::LandingDustAt(Vector3 &pos, bool doRaycast)
{ _ZN8dActor_c13LandingDustAtER7Vector3b(this, &pos, doRaycast); }
extern "C" void *_ZN8Particle7Manager9AddSystemEiR7Vector3(
    void *self, unsigned int definitionID, Vector3 *position)
{
    return ((Particle::Manager *)self)->Particle::Manager::AddSystem(
        (int)definitionID, *position);
}
#pragma comment(linker, "/alternatename:_STAR_MARKERS=_data_0209f40c")
#pragma comment(linker, "/alternatename:?_ZN8dActor_c10FindWithIDEj@@YAPAXI@Z=__ZN8dActor_c10FindWithIDEj")

extern "C" int _ZN4cstd3divEii(int a, int b)
{ return b == 0 ? (a < 0 ? -1 : 1) : a / b; }
extern "C" int _ZN4cstd3modEii(int a, int b)
{ return b == 0 ? a : a % b; }

extern "C" {
void _ZN9dBgCh_LinC1Ev(void *self)
{ ::new (self) dBgCh_Lin(); }
void _ZN9dBgCh_LinD1Ev(void *self)
{ ((dBgCh_Lin *)self)->~dBgCh_Lin(); }
#if defined(SM64DS_GAME_TARGET)
void _ZN15dExtFrameCtrl_c7AdvanceEv(dExtFrameCtrl_c *self)
{ self->dExtFrameCtrl_c::Advance(); }
int _ZN15dExtFrameCtrl_c8FinishedEv(dExtFrameCtrl_c *self)
{ return self->dExtFrameCtrl_c::Finished(); }
// Animation-event callers also consume a full int, as the retail ABI did.
int _ZNK15dExtFrameCtrl_c12WillHitFrameEi(
    const dExtFrameCtrl_c *self, int frame)
{ return self->dExtFrameCtrl_c::WillHitFrame(frame) ? 1 : 0; }
void *_ZN11dScMgBase_cC2Ev(void *self)
{ return ::new (self) dScMgBase_c(); }
void _ZN8Particle10SysTrackerC1Ev(void *self)
{ ::new (self) Particle::SysTracker(); }
#endif
void _ZN8dActor_c9UpdatePosEP5dCc_c(dActor_c *self, dCc_c *clsn)
{ self->dActor_c::UpdatePos(clsn); }
#if defined(SM64DS_GAME_TARGET)
int _ZN6Player17SetNoControlStateEhih(Player *self, unsigned char a,
                                   int b, unsigned char c)
{ return self->Player::SetNoControlState(a, b, c); }
bool _Z14ApproachLinearRsss(short *value, short target, short step)
{ return ApproachLinear(*value, target, step); }
void _ZN3G2x13SetBlendAlphaEPVttttj(volatile unsigned short *reg,
                                    unsigned short a, unsigned short b,
                                    unsigned short c, unsigned int d)
{ G2x::SetBlendAlpha(reg, a, b, c, d); }
void _ZN2GX12SetBankForBGEt(unsigned short bank)
{ GX::SetBankForBG(bank); }
void _ZN2GX13SetBankForOBJEt(unsigned short bank)
{ GX::SetBankForOBJ(bank); }
#endif
void _ZN5Sound9PlayBank0EjRK7Vector3(unsigned int id, const Vector3 *pos)
{ Sound::PlayBank0(id, *pos); }
// Legacy callers declare this as void, while the recovered implementation
// returns an unused status value. On x86 the return register is ignored, so
// route that signature to the same C-name bridge used by the recovered calls.
#pragma comment(linker, "/alternatename:?PlayBank2_2D@Sound@@YAXI@Z=__ZN5Sound12PlayBank2_2DEj")
#pragma comment(linker, "/alternatename:?PlayBank2_2D@Sound@@SAXI@Z=__ZN5Sound12PlayBank2_2DEj")
void _ZN7Vector3D1Ev() {}
}

