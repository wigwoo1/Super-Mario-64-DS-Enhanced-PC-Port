// PC-game-only bridges for recovered APIs called through literal Itanium names.
// Keep these out of smoke_actor: they forward to the production sound/GX and
// collision implementations, which that isolated actor harness intentionally
// does not link.
#include <new>
#include <atomic>
#include <cstdio>
#include <cstdlib>

#include "Sound.h"
#include "OAM.h"
#include "G2x.h"
#include "dCcAcPos_c.h"
#include "dActor_c.h"
#include "dCamera_c.h"
#include "daStarBase_c.h"
#include "dEnemyBase_c.h"
#include "dCapEnemy_c.h"

extern "C" int pc_actor_yoshi_try_eat(void *actor)
{ return static_cast<dActor_c *>(actor)->OnYoshiTryEat(); }
extern "C" void pc_actor_turn_into_egg(void *actor, void *player)
{ static_cast<dActor_c *>(actor)->OnTurnIntoEgg(*static_cast<Player *>(player)); }
#include "daStar_c.h"
#include "daBtn_c.h"
#include "daDossy_c.h"
#include "daJango_c.h"
#include "Player.h"
#include "G3X.h"
#include "G3i.h"
#include "Model.h"
#include "ModelAnim.h"
#include "BlendModelAnim.h"
#include "MaterialChanger.h"
#include "TextureSequence.h"
#include "SharedFilePtr.h"

struct PcSharedFilePtrView { unsigned short fileID; unsigned char numRefs; unsigned char pad; void *filePtr; };
#include "ModelBase.h"
#include "Memory.h"
#include "TextureTransformer.h"
#include "PathPtr.h"
#include "dCc_c.h"
#include "dScene_c.h"
#include "dBase_c.h"
#include "dFdWipe_c.h"
#include "TextureSequence.h"
#include "Message.h"
#include "SaveData.h"
#include "Event.h"
#include "ModelAnim2.h"
#include "dBgCh_Actr.h"
#include "dBgCh.h"
#include "dExtShadowModel_c.h"
#include "Particle__SysTracker.h"
#include "Particle__System.h"
#include "dPa_c.h"
#include "dClipper.h"
#include "dMg3DEspModel_c.h"
#include "Heap.h"
#include "Timer.h"
#include "daObjKey_c.h"
#include "dEntObj_c.h"
#include "dScMgMCarlo_c.h"
#include "dScMgMCarlo2_c.h"
#include "dScStage_c.h"
#include "ntr/rt.h"
#include "ntr/mmio.h"

// Callback vtables are owned by the runtime tracker. The simple subclasses
// use the common particle queue setup; the water/clip bookkeeping callbacks
// do not enqueue a new system themselves.
void dPa_c::level_c::fitWaterSimpleCallback_c::SpawnParticles(Particle::System& s)
{ simpleCallback_c::SpawnParticles(s); }
void dPa_c::level_c::fitWaterCallback_c::SpawnParticles(Particle::System&) {}
void dPa_c::level_c::checkWaterCallback_c::SpawnParticles(Particle::System&) {}
void dPa_c::level_c::bubbleCallback_c::SpawnParticles(Particle::System& s)
{ simpleCallback_c::SpawnParticles(s); }
void dPa_c::level_c::splashCallback_c::SpawnParticles(Particle::System& s)
{ simpleCallback_c::SpawnParticles(s); }
void dPa_c::level_c::clipCallback_c::SpawnParticles(Particle::System&) {}
void dPa_c::level_c::cleanParticleCallback_c::SpawnParticles(Particle::System&) {}
void dPa_c::level_c::checkWaterRippleCallback_c::SpawnParticles(Particle::System&) {}

extern "C" void _ZN5dPa_c7level_c20edStarKiraCallback_cC1Ev(void *self)
{ ::new (self) dPa_c::level_c::edStarKiraCallback_c(); }

extern "C" int _ZN6Player22IsBeingShotOutOfCannonEv(void *self)
{ return ((Player *)self)->Player::IsBeingShotOutOfCannon(); }

extern "C" void _ZN8dClipper13Func_020156DCEitii(
    dClipper *self, int aspect, unsigned short fov, int nearZ, int farZ)
{ self->Func_020156DC(aspect, fov, nearZ, farZ); }

extern "C" void _ZN15dMg3DEspModel_cC1Ev(void *self)
{ ::new (self) dMg3DEspModel_c(); }

// These three helpers implement the ARM soft-float math used for scale
// interpolation in the Kuri King actor. The host uses IEEE single precision;
// the conversion retains the game's 12 fractional bits.
extern "C" int func_01ffa344(int bits)
{
    union { unsigned int u; float f; } value;
    value.u = (unsigned int)bits;
    return (int)(value.f * 4096.0f);
}
extern "C" int func_01ffa594(int left, int right)
{
    union { unsigned int u; float f; } a, b, result;
    a.u = (unsigned int)left; b.u = (unsigned int)right;
    result.f = a.f - b.f;
    return (int)result.u;
}
extern "C" int func_01ff9378(int left, int right)
{
    union { unsigned int u; float f; } a, b, result;
    a.u = (unsigned int)left; b.u = (unsigned int)right;
    result.f = a.f + b.f;
    return (int)result.u;
}

extern "C" void func_0205256c(int *m, int s, int c)
{
    m[0] = c; m[1] = 0; m[2] = -s;
    m[3] = 0; m[4] = 0x1000; m[5] = 0;
    m[6] = s; m[7] = 0; m[8] = c;
}
extern "C" void Matrix3x3_SetRotationZ(int *m, int s, int c)
{
    m[0] = c; m[1] = s; m[2] = 0;
    m[3] = -s; m[4] = c; m[5] = 0;
    m[6] = 0; m[7] = 0; m[8] = 0x1000;
}

// This ARM routine performs the DS IPC reset handshake. It has no equivalent
// on the host; the PC runtime handles application shutdown independently.
extern "C" void func_01ffdb28(void) {}

namespace Memory {
void *Allocate(unsigned int size);
void *Allocate(unsigned int size, int align);
}
extern "C" void func_020731dc(void *object, void *destructor, void **node);

namespace {
std::atomic<unsigned int> g_host_fiq_mask(0x40u);
std::atomic<unsigned int> g_host_mpu_region7(0);
}

namespace cstd {
int div(int, int);
int mod(int, int);
}

struct EventStaticHostProxy {
    static int GetBit(unsigned int bit) { return Event::GetBit(bit); }
};
#pragma comment(linker, "/alternatename:?GetBit@Event@@SAHI@Z=?GetBit@EventStaticHostProxy@@SAHI@Z")
#pragma comment(linker, "/alternatename:?GetBG3CharPtr@G2S@@YAPAXXZ=?GetBG3CharPtr@G2S@@YAIXZ")

namespace Sound {
void LoadGroupAndSetBank(int, int);
void ResetPlayerVoiceGroup();
void LoadAndSetMusic_Layer2(unsigned int);
void LoadAndSetMusic_Layer3(unsigned int);
void StopLoadedMusic_Layer3();
void LoadAndSetMusic_Layer1(int);
void StopLoadedMusic_Layer1(unsigned int);
unsigned int PlayBank3_2D(unsigned int);
void EndMusic(unsigned int, unsigned int);
void Play(unsigned int, unsigned int, const Vector3 &);
void PlayBank3(unsigned int, const Vector3 &);
}

int ApproachLinear2(short &, short, short);
int ApproachLinear2(int &, int, int);
void UpdateAngle(short &, short, int, short);

// daJango's matched overlay body uses literal Itanium-name declarations as
// ordinary C++ free functions. Recreate those MSVC call-site signatures and
// forward into the recovered class methods.
char *_ZN8dActor_c13ClosestPlayerEv(char *self)
{ return reinterpret_cast<char *>(reinterpret_cast<daJango_c *>(self)->ClosestPlayer()); }
int _ZN9daJango_c19func_ov062_0211c658EPv(daJango_c *self, void *state)
{ return self->func_ov062_0211c658(state); }
int _ZN9daJango_c19func_ov062_0211b51cEv(daJango_c *self)
{ return self->func_ov062_0211b51c(); }
int _ZNK10dBgCh_Actr8IsOnWallEv(char *self)
{ return static_cast<dBgCh_Actr *>(static_cast<void *>(self))->IsOnWall(); }

// Message::Update operates on file-global state, but the recovered body is
// emitted as a non-static C++ member. This proxy gives no-this callers the
// same host implementation without changing its source.
struct MessageUpdateProxy { void Update(); };
struct MessageUpdateStaticProxy { static void Update(); };
#pragma comment(linker, "/alternatename:?Update@MessageUpdateProxy@@QAEXXZ=?Update@Message@@QAEXXZ")
#pragma comment(linker, "/alternatename:?Update@Message@@SAXXZ=?Update@MessageUpdateStaticProxy@@SAXXZ")
#pragma comment(linker, "/alternatename:?SetFog@G3X@@YAX_NHHH@Z=?SetFog@G3X@@SAX_NHHH@Z")
#pragma comment(linker, "/alternatename:?PlayBank0@Sound@@SAXIABUVector3@@@Z=?PlayBank0@Sound@@YAXIABUVector3@@@Z")
#pragma comment(linker, "/alternatename:?LoadAndSetMusic_Layer1@Sound@@SAXH@Z=?LoadAndSetMusic_Layer1@Sound@@YAXH@Z")
#pragma comment(linker, "/alternatename:?StopLoadedMusic_Layer1@Sound@@SAXI@Z=?StopLoadedMusic_Layer1@Sound@@YAXI@Z")
#pragma comment(linker, "/alternatename:?UnsetPlayerVoiceGroup@Sound@@SAXXZ=?UnsetPlayerVoiceGroup@Sound@@YAXXZ")
#pragma comment(linker, "/alternatename:?Render@OAM@@SAH_NPAUOamAttr@@HHHHPAUMatrix2x2@@@Z=?Render@OAM@@SAX_NPAUOamAttr@@HHHHPAUMatrix2x2@@@Z")
#pragma comment(linker, "/alternatename:?GetBG0CharPtr@G2S@@YAPADXZ=?GetBG0CharPtr@G2S@@YAIXZ")
#pragma comment(linker, "/alternatename:?GetBG0ScrPtr@G2S@@YAPADXZ=?GetBG0ScrPtr@G2S@@YAIXZ")
#pragma comment(linker, "/alternatename:__ZN5Sound13Func_02048eb4Ev=?Func_02048eb4@Sound@@SAXXZ")
#pragma comment(linker, "/alternatename:__ZN5Sound13Func_02048ec4Ev=?Func_02048ec4@Sound@@SAXXZ")
#pragma comment(linker, "/alternatename:__ZN5Sound13Func_02048ee4Ev=?Func_02048ee4@Sound@@SAXXZ")

void MessageUpdateStaticProxy::Update()
{ (reinterpret_cast<MessageUpdateProxy *>(0))->Update(); }

extern "C" {
// Stage TUs still call these static Message routines by their literal ARM
// names. Forward those names to the recovered host member implementations.
void _ZN7Message21DisplayLevelClearTextEta(int msgID, unsigned char course)
{ Message::DisplayLevelClearText((u16)msgID, (s8)course); }
void _ZN7Message19DisplaySaveMenuTextEt(int msgID)
{ Message::DisplaySaveMenuText((u16)msgID); }

void __register_global_object(void *object, void *destructor, void **node)
{ func_020731dc(object, destructor, node); }
void func_02017e0c(void *self, unsigned int fileID);
void *func_02017e34(void *self);
void pc_sharedfile_handle_construct(void *self, unsigned int fileID)
{ func_02017e0c(self, fileID); }
void pc_sharedfile_handle_destroy(void *self)
{ (void)func_02017e34(self); }
// PC builds have no ARM CPSR/CP15 registers. Preserve the save/restore
// contract for callers that guard shared state; DMA/IRQ scheduling is owned
// by the host runtime.
unsigned int _ZN3IRQ10DisableAllEv(void)
{ return ntr::rt_irq_disable(); }
unsigned int _ZN3IRQ10RestoreAllEj(unsigned int state)
{ return ntr::rt_irq_restore(state); }
void _ZN3IRQ11SetFIQStateEb(int state)
{ if (state) g_host_fiq_mask.fetch_or(0x40u); else g_host_fiq_mask.fetch_and(~0x40u); }
unsigned int _ZN3IRQ11SetFIQStateEb_false(void)
{ return g_host_fiq_mask.fetch_or(0x40u) & 0x40u; }
void _ZN4CP1510EnableDTCMEv(void) {}
void _ZN4CP1514MPUDataRegion1Ej(unsigned int) {}
void _ZN4CP1514MPUDataRegion7Ej(unsigned int value)
{ g_host_mpu_region7.store(value); }
unsigned int _ZN4CP1517MPUGetDataRegion7Ev(void)
{ return g_host_mpu_region7.load(); }
unsigned int _ZN4CP1518GetDTCMBaseAddressEv(void) { return 0x027c0000u; }
void _ZN4CP1516WaitForInterruptEv(void) {}
struct IRQ {
    static unsigned int Disable();
    static void Restore(unsigned int);
    static unsigned int DisableIRQs(unsigned int);
    static unsigned int SetIRQs(unsigned int);
};
unsigned int IRQ::Disable() { return ntr::rt_irq_disable(); }
void IRQ::Restore(unsigned int state) { (void)ntr::rt_irq_restore(state); }
unsigned int IRQ::DisableIRQs(unsigned int mask) {
    const unsigned int old = static_cast<unsigned int>(ntr::io_read(0x04000210u, 4));
    ntr::io_write(0x04000210u, old & ~mask, 4);
    return old;
}

extern "C" {
void _ZN3OAM5FlushEv(void) { OAM::Flush(); }
void _ZN3OAM5ResetEv(void) { OAM::Reset(); }
}
unsigned int IRQ::SetIRQs(unsigned int mask) {
    const unsigned int old = static_cast<unsigned int>(ntr::io_read(0x04000210u, 4));
    ntr::io_write(0x04000210u, mask, 4);
    return old;
}

struct CP15 {
    static void WaitForInterrupt();
    static void InvalidateDataCache(unsigned int, unsigned int);
};
void CP15::WaitForInterrupt() {}
void CP15::InvalidateDataCache(unsigned int, unsigned int) {}

extern "C" unsigned int _ZN4Heap7_SizeofEPv(Heap *heap, void *ptr)
{ return static_cast<unsigned int>(heap->Sizeof(ptr)); }
extern "C" unsigned int _ZN4Heap10ReallocateEPvj(
    Heap *heap, void *ptr, unsigned int size)
{ return heap->Reallocate(ptr, size); }
void _ZN4Heap18InitializeRootHeapEv(void)
{ Heap::InitializeRootHeap(); }
void _ZN4Heap9SetNodeIDEj(Heap *heap, unsigned int id)
{ heap->SetNodeID(id); }
void _ZN4Heap18InitializeGameHeapEjPS_(unsigned int size, Heap *root)
{ Heap::InitializeGameHeap(size, root); }
void *_ZN6Memory8AllocateEj(unsigned int size)
{ return Memory::Allocate(size); }
void *_ZN6Memory8AllocateEji(unsigned int size, int align)
{ return Memory::Allocate(size, align); }
void _ZN4Heap11_DeallocateEPv(Heap *self, void *ptr)
{ self->_Deallocate(ptr); }
void *_ZN4Heap9_AllocateEji(Heap *self, unsigned int size, int align)
{ return self->_Allocate(size, align); }
bool _ZN4Heap6IntactEv(Heap *self) { return self->Intact(); }
void _ZN4Heap6RescueEv(Heap *self) { self->Rescue(); }
unsigned int _ZN4Heap21MaxAllocationUnitSizeEv(Heap *self)
{ return self->MaxAllocationUnitSize(); }
SolidHeapAllocator *_ZN4Heap24CreateSolidHeapAllocatorEPvjj(
    void *address, unsigned int size, unsigned int flags)
{ return Heap::CreateSolidHeapAllocator(address, size, flags); }


// The boot closure calls the original CodeWarrior static with a literal
// Itanium name. MSVC cannot emit that ABI name for the same recovered method.
void _ZN5Model17UpdateFileOffsetsER8BMD_File(BMD_File *file)
{ Model::UpdateFileOffsets(*file); }
void _ZN5Model12SetPolygonIDEi(Model *self, int id)
{ self->SetPolygonID(id); }
void _ZN5Model14SetPolygonModeEi(Model *self, int mode)
{ self->SetPolygonMode(mode); }
void _ZN5Model12HideMaterialEii(Model *self, int bone, int list)
{ self->HideMaterial(bone, list); }
void _ZN5Model12ShowMaterialEii(Model *self, int bone, int list)
{ self->ShowMaterial(bone, list); }
void _ZN5Model6RenderEPK7Vector3(Model *self, const Vector3 *scale)
{ self->Model::Render(scale); }
void _ZN5ModelD1Ev(Model *self) { self->~Model(); }
void _ZN14BlendModelAnim7AdvanceEv(BlendModelAnim *self) { self->Advance(); }
void _ZN15MaterialChanger7PrepareER8BMD_FileR8BMA_File(
    BMD_File *model, BMA_File *animation)
{ MaterialChanger::Prepare(*model, *animation); }
void _ZN15TextureSequence7PrepareER8BMD_FileR8BTP_File(
    BMD_File *model, BTP_File *animation)
{ TextureSequence::Prepare(*model, *animation); }
void _ZN15ModelComponents6RenderEP9Matrix4x3P7Vector3(
    ModelComponents *self, Matrix4x3 *matrix, Vector3 *scale)
{ self->Render(matrix, scale); }
void _ZN17dExtShadowModel_c12InitCylinderEv(dExtShadowModel_c *self)
{ self->InitCylinder(); }
void _ZN15TextureSequenceD1Ev(TextureSequence *self)
{ self->~TextureSequence(); }
void _ZN18TextureTransformerD1Ev(TextureTransformer *self)
{ self->~TextureTransformer(); }
void _ZN9ModelAnimD1Ev(ModelAnim *self) { self->~ModelAnim(); }
void _ZN17dExtShadowModel_cD1Ev(dExtShadowModel_c *self)
{ self->~dExtShadowModel_c(); }
void _ZN14BlendModelAnimD1Ev(BlendModelAnim *self)
{ self->~BlendModelAnim(); }
void _ZN14BlendModelAnimC1Ev(BlendModelAnim *self)
{ ::new (self) BlendModelAnim(); }
int _ZN15dExtFrameCtrl_c8GetFlagsEv(dExtFrameCtrl_c *self)
{ return self->GetFlags(); }
void _ZN17dExtShadowModel_c9RenderAllEv(void)
{ dExtShadowModel_c::RenderAll(); }
int _ZN8dActor_c9TrackStarEjj(dActor_c *self, unsigned int star, unsigned int type)
{ return self->TrackStar(star, type); }
void _ZN8dActor_c11UntrackStarERa(dActor_c *self, signed char *star)
{ self->UntrackStar(*star); }
void _ZN9dCamera_c9SetFlag_3Ev(dCamera_c *self) { self->SetFlag_3(); }
dActor_c *_ZN8dActor_c15FindWithActorIDEjPS_(unsigned int actorID, dActor_c *after)
{ return dActor_c::FindWithActorID(actorID, after); }
dActor_c *_ZN8dActor_c13SpawnSoundObjEj(dActor_c *self, unsigned int param)
{
    // Recovered callers retain the sound actor (for example the cap intro).
    // The cartridge wrapper's last Spawn call returns it in r0, although its
    // reconstructed void declaration would discard that value on the host.
    return dActor_c::Spawn(0x167,param,
        *reinterpret_cast<const Vector3 *>(&self->mPosX),nullptr,self->mAreaId,-1);
}
dActor_c *_ZN8dActor_c4NextEPKS_(const dActor_c *after)
{ return dActor_c::Next(after); }
Player *_ZN8dActor_c14FarthestPlayerEv(dActor_c *self)
{ return self->FarthestPlayer(); }
int _ZN8dActor_c14GetSubtractionEss(dActor_c *self, short a, short b)
{ return self->GetSubtraction(a, b); }
dActor_c *_ZN8dActor_c11SpawnNumberERK7Vector3jbtPS_(
    dActor_c *self, const Vector3 *pos, unsigned int value, bool packLowNibble,
    unsigned short delay, dActor_c *owner)
{ return self->SpawnNumber(*pos, value, packLowNibble, delay, owner); }
void _ZN8dActor_c10PoofDustAtERK7Vector3(dActor_c *self, const Vector3 *pos)
{ self->PoofDustAt(*pos); }
void _ZN8dActor_c16TriplePoofDustAtERK7Vector3(dActor_c *self, const Vector3 *pos)
{ self->TriplePoofDustAt(*pos); }
int _ZN8dActor_c18GetBitInDeathTableEv(dActor_c *self)
{ return self->GetBitInDeathTable(); }
void _ZN8dActor_c17TrackInDeathTableEv(dActor_c *self)
{ self->TrackInDeathTable(); }
void _ZN8dActor_c24KillAndTrackInDeathTableEv(dActor_c *self)
{ self->KillAndTrackInDeathTable(); }
dActor_c *_ZN8dActor_c19UntrackAndSpawnStarERajRK7Vector3h(
    dActor_c *self, signed char *trackStarID, unsigned int starID,
    const Vector3 *spawnPos, unsigned char howToSpawnStar)
{ return self->UntrackAndSpawnStar(*trackStarID, starID, *spawnPos, howToSpawnStar); }
void _ZN11dCapEnemy_c12UpdateCapPosERK7Vector3RK10Vector3_16(
    dCapEnemy_c *self, const Vector3 *pos, const Vector3_16 *rot)
{ self->UpdateCapPos(*pos, *rot); }
int _ZN11dCapEnemy_c11GetCapStateEv(dCapEnemy_c *self)
{ return self->GetCapState(); }
int _ZNK10dBgCh_Actr13JustHitGroundEv(const dBgCh_Actr *self)
{ return self->JustHitGround(); }
int _ZNK10dBgCh_Actr14GetResultFlag1Ev(const dBgCh_Actr *self)
{ return self->GetResultFlag1(); }
int _ZNK10dBgCh_Actr12TouchesWaterEv(const dBgCh_Actr *self)
{ return self->TouchesWater(); }
int _ZN6Player9DropActorEv(Player *self)
{ return self->DropActor(); }
char *pc_closest_player(char *self)
{ return reinterpret_cast<char *>(reinterpret_cast<dActor_c *>(self)->ClosestPlayer()); }
#pragma comment(linker, "/alternatename:?_ZN8dActor_c13ClosestPlayerEv@@YAPADPAD@Z=?pc_closest_player@@YAPADPAD@Z")
void _ZN12dEnemyBase_c12UpdateWMClsnER10dBgCh_Actrj(
    dEnemyBase_c *self, dBgCh_Actr *collision, unsigned int selector)
{ self->UpdateWMClsn(*collision, selector); }
int _ZN12dEnemyBase_c14UpdateYoshiEatER10dBgCh_Actr(
    dEnemyBase_c *self, dBgCh_Actr *collision)
{ return self->UpdateYoshiEat(*collision); }
void _ZN8daStar_c19func_ov002_020e7218EPci(
    daStar_c *self, char *player, int gate)
{ self->func_ov002_020e7218(player, gate); }
void _ZN7daBtn_c19func_ov079_02123804EP8dActor_c(
    daBtn_c *self, dActor_c *other)
{ self->func_ov079_02123804(other); }
void _ZN9daDossy_c19func_ov065_0211956cEP8dActor_c(
    daDossy_c *self, dActor_c *other)
{ self->func_ov065_0211956c(other); }
void _ZN9daDossy_c19func_ov065_02119594EP8dActor_c(
    daDossy_c *self, dActor_c *other)
{ self->func_ov065_02119594(other); }
int pc_bgactor_is_on_wall(char *self)
{ return reinterpret_cast<dBgCh_Actr *>(self)->IsOnWall(); }
int pc_jango_change_state(daJango_c *self, void *state)
{ return self->func_ov062_0211c658(state); }
int pc_jango_update_state(daJango_c *self)
{ return self->func_ov062_0211b51c(); }
#pragma comment(linker, "/alternatename:?_ZNK10dBgCh_Actr8IsOnWallEv@@YAHPAD@Z=?pc_bgactor_is_on_wall@@YAHPAD@Z")
#pragma comment(linker, "/alternatename:?_ZN9daJango_c19func_ov062_0211c658EPv@@YAHPAUdaJango_c@@PAX@Z=?pc_jango_change_state@@YAHPAUdaJango_c@@PAX@Z")
#pragma comment(linker, "/alternatename:?_ZN9daJango_c19func_ov062_0211b51cEv@@YAHPAUdaJango_c@@@Z=?pc_jango_update_state@@YAHPAUdaJango_c@@@Z")
void _ZN7Message7EndTalkEv(void)
{ Message::EndTalk(); }

// The stage sources retain CodeWarrior's raw global names at C call sites,
// while the recovered definitions are ordinary C++ functions under MSVC.
struct NativeOverlayDispatch {
    static void LoadLevel(int level);
    static void UnloadLevel(int level);
};
#pragma comment(linker, "/alternatename:?LoadLevel@NativeOverlayDispatch@@SAXH@Z=?LoadLevelOverlays@@YAXH@Z")
#pragma comment(linker, "/alternatename:?UnloadLevel@NativeOverlayDispatch@@SAXH@Z=?UnloadLevelOverlays@@YAXH@Z")
extern "C" {
void _Z17LoadLevelOverlaysi(int level) { NativeOverlayDispatch::LoadLevel(level); }
void _Z19UnloadLevelOverlaysi(int level) { NativeOverlayDispatch::UnloadLevel(level); }
}

void _ZN12daStarBase_c7CollectEv(daStarBase_c *self)
{ self->Collect(); }
void _ZN12daStarBase_c27LinkSilverStarAndStarMarkerEPc(
    daStarBase_c *self, char *star)
{ self->LinkSilverStarAndStarMarker(star); }

// Recovered callers spell these as CodeWarrior member symbols and pass the
// former `this` pointer explicitly. The real implementations remain the
// production C++ methods; these thunks adapt only the host ABI.
void _ZN5Model14LoadAndSetFileEtii(Model *self, unsigned short file, int a, int b)
{ self->LoadAndSetFile(file, a, b); }
void _ZN18TextureTransformer6UpdateER15ModelComponents(
    TextureTransformer *self, ModelComponents *model)
{ self->Update(*model); }
void _ZN18TextureTransformer7PrepareER8BMD_FileR8BTA_File(
    BMD_File *model, BTA_File *animation)
{ TextureTransformer::Prepare(*model, *animation); }
void *_ZN7PathPtrC1Ev(PathPtr *self)
{ return ::new (self) PathPtr(); }
void _ZN5dCc_c5ClearEv(dCc_c *self) { self->Clear(); }
void _ZN5dCc_c6UpdateEv(dCc_c *self) { self->Update(); }
void _ZN8dScene_c14StartSceneFadeEjjt(unsigned int scene, unsigned int param,
                                    unsigned short color)
{ dScene_c::StartSceneFade(scene, param, color); }
void _ZN8dScene_c20SetAndStopColorFaderEv(void)
{ dScene_c::SetAndStopColorFader(); }
void _ZN8dScene_c20Initialise3dGraphicsEv(void)
{ dScene_c::Initialise3dGraphics(); }
void _ZN8dScene_c18PrepareToSpawnBootEv(void)
{ dScene_c::PrepareToSpawnBoot(); }
int _ZN8dScene_c15SetSceneToSpawnEjj(unsigned int scene, unsigned int param)
{ return dScene_c::SetSceneToSpawn(scene, param); }
int _ZN8dScene_c16SpawnIfNecessaryEv(void)
{ return dScene_c::SpawnIfNecessary(); }
void _ZN9ModelBase12ApplyOpacityEjj(ModelBase *self, unsigned int opacity,
                                    unsigned int extra)
{ self->ApplyOpacity(opacity, extra); }
void _ZN8dActor_c22UpdatePosWithOnlySpeedEP5dCc_c(dActor_c *self, dCc_c *collision)
{ self->UpdatePosWithOnlySpeed(collision); }
void _ZN12dEnemyBase_c20KillByInvincibleCharERK10Vector3_16R6Player5Fix12IiE(
    dEnemyBase_c *self, const Vector3_16 *velocity, Player *player,
    Fix12<int> unused)
{ self->KillByInvincibleChar(*velocity, *player, unused); }
void _ZN10dScStage_c14LoadGraphics2DEbi(bool e3d, int level)
{ dScStage_c::LoadGraphics2D(e3d, level); }
Heap *_ZN4Heap10SetDefaultEv(Heap *self) { return self->SetDefault(); }
void _ZN9dFdWipe_c14LoadAndSetFileEt(dFdWipe_c *self, unsigned short file)
{ self->model.LoadAndSetFile(file, 0, -1); }
void _ZN9dFdWipe_cC1Ev(dFdWipe_c *self)
{ ::new (self) dFdWipe_c(); }
void _ZN9dFdWipe_cD1Ev(dFdWipe_c *self) { self->~dFdWipe_c(); }
void _ZN18TextureTransformerC1Ev(TextureTransformer *self)
{ ::new (self) TextureTransformer(); }
void _ZN5Sound6Player19SetPlayableSeqCountEii(int player, int count)
{ Sound::Player::SetPlayableSeqCount(player, count); }
int _ZN6Player11ShowMessageER7fBase_cjPK7Vector3hh(
    Player *self, fBase_c *actor, unsigned int message, const Vector3 *pos,
    unsigned char a, unsigned char b)
{ return self->Player::ShowMessage(*actor, message, pos, a, b); }
int _ZN6Player12GetTalkStateEv(Player *self)
{ return self->Player::GetTalkState(); }
int _ZN6Player8HasNoCapEv(Player *self)
{ return self->Player::HasNoCap(); }
void _ZN6Player18SetNewHatCharacterEjjb(
    Player *self, unsigned int character, unsigned int previous, bool update)
{ self->Player::SetNewHatCharacter(character, previous, update); }
void _ZN6Player18TurnOffToonShadingEj(Player *self, unsigned int model)
{ self->Player::TurnOffToonShading(model); }
int _ZN6Player12Unk_020c9e5cEh(Player *self, unsigned char state)
{ return self->Player::Unk_020c9e5c(state); }
void _ZN10ModelAnim24CopyERKS_Pcj(
    ModelAnim2 *self, const ModelAnim2 *source, char *file, unsigned int otherFile)
{ self->Copy(*source, file, otherFile); }
void _ZN15TextureSequence6UpdateER15ModelComponents(
    TextureSequence *self, ModelComponents *model)
{ self->Update(*model); }
void _ZN15TextureSequenceC1Ev(TextureSequence *self)
{ ::new (self) TextureSequence(); }
void *_ZN15TextureSequence8LoadFileER13SharedFilePtr(SharedFilePtr *file)
{
    PcSharedFilePtrView *view = reinterpret_cast<PcSharedFilePtrView *>(file);
    const char *trace = std::getenv("SM64DS_PC_TRACE");
    if (trace && trace[0] && trace[0] != '0') {
        std::fprintf(stderr, "[sm64ds] texture sequence begin handle=%p id=%04x refs=%u\n",
                     (void *)file, view->fileID, view->numRefs);
        std::fflush(stderr);
    }
    void *result = TextureSequence::LoadFile(*file);
    if (trace && trace[0] && trace[0] != '0') {
        if (result) {
            const unsigned char *p = static_cast<const unsigned char *>(result);
            std::fprintf(stderr,
                "[sm64ds] texture sequence load handle=%p id=%04x refs=%u result=%p header=%02x%02x%02x%02x %02x%02x%02x%02x\n",
                (void *)file, view->fileID, view->numRefs, result,
                p[3], p[2], p[1], p[0], p[7], p[6], p[5], p[4]);
            std::fflush(stderr);
        } else {
            std::fprintf(stderr, "[sm64ds] texture sequence load failed handle=%p id=%04x refs=%u\n",
                         (void *)file, view->fileID, view->numRefs);
            std::fflush(stderr);
        }
    }
    return result;
}
void _ZN7Message11PrepareTalkEv(void) { Message::PrepareTalk(); }
void _ZN7Message15ResetAllGlobalsEv(void) { Message::ResetAllGlobals(); }
void _ZN7Message11DisplayTextEt(unsigned short message)
{ Message::DisplayText(message); }
void _ZN7Message7AddCharEc(char ch) { Message::AddChar(ch); }
void _ZN7Message7DisplayEj(Message *self, unsigned int message)
{ self->Display(message); }
void _ZN7Message10LoadTextVSEv(void) { Message::LoadTextVS(); }
void _ZN7Message13DisplaySavingEt(unsigned short message)
{ Message::DisplaySaving(message); }
void _ZN7Message16DisplayPauseTextEth(unsigned short message, unsigned char course)
{ Message::DisplayPauseText(message, course); }
void _ZN7Message17DisplayVsExitTextEt(unsigned short message)
{ Message::DisplayVsExitText(message); }
void _ZN7Message18DisplayPauseTextVSEt(unsigned short message)
{ Message::DisplayPauseTextVS(message); }
void _ZN7Message19DisplayDontSaveTextEt(unsigned short message)
{ Message::DisplayDontSaveText(message); }
void _ZN7Message21DisplaySaveStatusTextEt(unsigned short message)
{ Message::DisplaySaveStatusText(message); }
void _ZN7Message22DisplayOptionsMenuTextEt(unsigned short message)
{ Message::DisplayOptionsMenuText(message); }
void _ZN7Message25DisplayControllerModeTextEt(unsigned short message)
{ Message::DisplayControllerModeText(message); }
void _ZN7Message6UpdateEv(void)
{ MessageUpdateStaticProxy::Update(); }
void _ZN8Particle10SysTracker10InitialiseEv(Particle::SysTracker *self)
{ self->Initialise(); }
void _ZN17dExtShadowModel_c8CleanAllEv(void)
{ dExtShadowModel_c::CleanAll(); }
void _ZN5dBgCh19StartDetectingWaterEv(dBgCh *self)
{ self->StartDetectingWater(); }
void _ZN10dBgCh_Actr19StartDetectingWaterEv(dBgCh_Actr *self)
{ self->StartDetectingWater(); }
void _ZN10dBgCh_Actr15ClearGroundFlagEv(dBgCh_Actr *self)
{ self->ClearGroundFlag(); }
void _ZN10dBgCh_Actr12Unk_0203589cEv(dBgCh_Actr *self)
{ self->Unk_0203589c(); }
void _ZN10dBgCh_Actr15ClearLimMovFlagEv(dBgCh_Actr *self)
{ self->ClearLimMovFlag(); }
void _ZN10dBgCh_Actr16UpdateContinuousEv(dBgCh_Actr *self)
{ self->UpdateContinuous(); }
int _ZN6Player7IsInAirEv(Player *self)
{ return self->IsInAir(); }
int _ZN6Player15IsCollectingCapEv(Player *self)
{ return self->IsCollectingCap(); }
int _ZN6Player9IsOnShellEv(Player *self)
{ return self->IsOnShell(); }
void _ZN6Player12Unk_020ca488Ev(Player *self)
{ self->Unk_020ca488(); }
int _ZN6Player9StartTalkER7fBase_cb(Player *self, fBase_c *actor, bool enabled)
{ return self->StartTalk(*actor, enabled); }
int _ZN6Player12Unk_020ca8f8Ev(Player *self)
{ return self->Unk_020ca8f8(); }
int _ZN6Player12ShowMessage2ER7fBase_cjPK7Vector3hh(
    Player *self, fBase_c *actor, unsigned int message, const Vector3 *position,
    unsigned char first, unsigned char second)
{ return self->ShowMessage2(*actor, message, position, first, second); }
int _ZN6Player20IsStateEnteringLevelEv(Player *self)
{ return self->IsStateEnteringLevel(); }
int _ZN6Player16St_Shell_CleanupEv(Player *self)
{ return self->St_Shell_Cleanup(); }
int _ZN6Player14St_OnWall_InitEv(Player *self)
{ return self->St_OnWall_Init(); }
int _ZN6Player17St_PunchKick_InitEv(Player *self)
{ return self->St_PunchKick_Init(); }
void _ZN8dActor_c11LandingDustEb(dActor_c *self, int raycast)
{ self->LandingDust(raycast != 0); }
int _ZN6Player19func_ov002_020bd664EPhii(Player *self, unsigned char *command, int begin, int end)
{ return self->func_ov002_020bd664(command, begin, end); }
void _ZN6Player19func_ov002_020bf88cEv(Player *self)
{ self->func_ov002_020bf88c(); }
int _ZN6Player19func_ov002_020c04e0Ev(Player *self)
{ return self->func_ov002_020c04e0(); }
int _ZN6Player19func_ov002_020c0cbcEv(Player *self)
{ return self->func_ov002_020c0cbc(); }
void _ZN6Player19func_ov002_020be3b0Ev(Player *self)
{ self->func_ov002_020be3b0(); }
void _ZN6Player19func_ov002_020bda48Ev(Player *self)
{ self->func_ov002_020bda48(); }
void _ZN10dBgCh_Actr18StopDetectingWaterEv(dBgCh_Actr *self)
{ self->StopDetectingWater(); }
void _ZN5dBgCh19StartDetectingToxicEv(dBgCh *self)
{ self->StartDetectingToxic(); }
void _ZN5dBgCh21StopDetectingOrdinaryEv(dBgCh *self)
{ self->StopDetectingOrdinary(); }
void _ZN10dBgCh_Actr20UpdateDiscreteNoLavaEv(dBgCh_Actr *self)
{ self->UpdateDiscreteNoLava(); }
void _ZN10dBgCh_Actr20UpdateExtraContinousEv(dBgCh_Actr *self)
{ self->UpdateExtraContinous(); }
void _ZN10dBgCh_Actr22ClearJustHitGroundFlagEv(dBgCh_Actr *self)
{ self->ClearJustHitGroundFlag(); }
void _ZN10dBgCh_Actr22UpdateContinuousNoLavaEv(dBgCh_Actr *self)
{ self->UpdateContinuousNoLava(); }
void _ZN10dBgCh_Actr22UpdateDiscreteNoLava_2Ev(dBgCh_Actr *self)
{ self->UpdateDiscreteNoLava_2(); }
void _ZN10dBgCh_Actr13SetLimMovFlagEv(dBgCh_Actr *self)
{ self->SetLimMovFlag(); }
void _Z11UpdateAngleRssis(short *angle, short target, int divisor, short maxStep)
{ UpdateAngle(*angle, target, divisor, maxStep); }

void _ZN3G3X6SetFogEbiii(bool enable, int a, int b, int c)
{ ::G3X::SetFog(enable, a, b, c); }
void _ZN3G3X11SetFogTableEPv(void *table)
{ ::G3X::SetFogTable(table); }
void _ZN3G3X13SetClearColorEtiiib(
    unsigned short color, int depth, int fogColor, int alpha, bool polygon)
{ ::G3X::SetClearColor(color, depth, fogColor, alpha, polygon); }
void _ZN3G3i7LookAt_EPK7Vector3S2_S2_bP9Matrix4x3(
    const Vector3 *at, const Vector3 *up, const Vector3 *eye, bool draw,
    Matrix4x3 *matrix)
{ ::G3i::LookAt_(at, up, eye, draw, matrix); }

int _ZN6Player11ChangeStateERNS_5StateE(Player *self, Player::State *state)
{ return self->Player::ChangeState(*state); }
extern "C" int func_020bd828(void *self)
{ return static_cast<Player *>(self)->CanPause(); }
void _ZN8dScene_c22ResetHardwareRegistersEv(void)
{ dScene_c::ResetHardwareRegisters(); }
unsigned int _ZNK6Player14GetBodyModelIDEjb(
    const Player *self, unsigned int index, int isBody)
{ return self->Player::GetBodyModelID(index, isBody != 0); }
int _ZN6Player12FinishedAnimEv(Player *self)
{ return self->Player::FinishedAnim(); }
}
int _ZNK6Player14GetBodyModelIDEjb(
    void *self, unsigned int index, int isBody)
{ return static_cast<Player *>(self)->Player::GetBodyModelID(index, isBody != 0); }
int _ZN6Player12FinishedAnimEv(void *self)
{ return static_cast<Player *>(self)->Player::FinishedAnim(); }
extern "C" {
int _ZN6Player6IsAnimEj(Player *self, unsigned int anim)
{ return self->Player::IsAnim(anim); }
void _ZN6Player16InitWingFeathersEb(void *self, int enabled)
{ static_cast<Player *>(self)->Player::InitWingFeathers(enabled != 0); }
void _ZN10daObjKey_c19func_ov089_0213115cEi(void *self, int anim)
{ static_cast<daObjKey_c *>(self)->func_ov089_0213115c(anim); }
void _ZN8daStar_c19func_ov002_020e7090Ei(int arg, void *self)
{ static_cast<daStar_c *>(self)->func_ov002_020e7090(arg); }

void _ZN15UnknownVsPlayerC1Ev(UnknownVsPlayer *self)
{ ::new (self) UnknownVsPlayer(); }
void _ZN15UnknownVsPlayerD1Ev(UnknownVsPlayer *self)
{ self->~UnknownVsPlayer(); }
void _ZN18dMgMCarloCardObj_cC1Ev(dMgMCarloCardObj_c *self)
{ ::new (self) dMgMCarloCardObj_c(); }
void _ZN18dMgMCarloCardObj_cD1Ev(void *self)
{ static_cast<dMgMCarloCardObj_c *>(self)->~dMgMCarloCardObj_c(); }
void _ZN19dMgMCarlo2CardObj_cC1Ev(dMgMCarlo2CardObj_c *self)
{ ::new (self) dMgMCarlo2CardObj_c(); }
void _ZN19dMgMCarlo2CardObj_cD1Ev(void *self)
{ static_cast<dMgMCarlo2CardObj_c *>(self)->~dMgMCarlo2CardObj_c(); }

long long _ZN5Timer7GetTimeEv(Timer *self)
{ return self->Timer::GetTime(); }
void _ZN5Timer10ResetTimerEv(Timer *self)
{ self->Timer::ResetTimer(); }
void _ZN5Timer10StartTimerEv(Timer *self)
{ self->Timer::StartTimer(); }
void _ZN5Timer9StopTimerEv(Timer *self)
{ self->Timer::StopTimer(); }

unsigned int _ZN5Sound13PlayCharVoiceEjjRK7Vector3(
    unsigned int a, unsigned int b, const Vector3 *pos)
{ return Sound::PlayCharVoice(a, b, *pos); }
}
void _ZN5Sound13PlayCharVoiceEjjRK7Vector3(
    unsigned int a, unsigned int b, void *pos)
{ (void)Sound::PlayCharVoice(a, b, *static_cast<const Vector3 *>(pos)); }
extern "C" {

// The callers keep the decomp's C-linkage spelling. The implementation is the
// reconstructed C++ namespace function compiled under its MSVC decorated name.
void _ZN5Sound16LoadInitialGroupEi(int group)
{ Sound::LoadInitialGroup(group); }
void _ZN5Sound19LoadGroupAndSetBankEii(int group, int bank)
{ Sound::LoadGroupAndSetBank(group, bank); }
void _ZN5Sound21ResetPlayerVoiceGroupEv(void)
{ Sound::ResetPlayerVoiceGroup(); }
void _ZN5Sound22LoadAndSetMusic_Layer3Ej(unsigned int id)
{ Sound::LoadAndSetMusic_Layer3(id); }
void _ZN5Sound22StopLoadedMusic_Layer3Ev(void)
{ Sound::StopLoadedMusic_Layer3(); }
void _ZN5Sound22StopLoadedMusic_Layer1Ej(unsigned int id)
{ Sound::StopLoadedMusic_Layer1(id); }
void _ZN5Sound22LoadAndSetMusic_Layer2Ej(unsigned int id)
{ Sound::LoadAndSetMusic_Layer2(id); }
void _ZN5Sound22StopLoadedMusic_Layer2Ev(void)
{ Sound::StopLoadedMusic_Layer2(); }
unsigned int _ZN5Sound12PlayBank2_2DEj(unsigned int id)
{ return Sound::PlayBank2_2D(id); }
unsigned int _ZN5Sound12PlayBank3_2DEj(unsigned int id)
{ return Sound::PlayBank3_2D(id); }
void _ZN5Sound8EndMusicEjj(unsigned int a, unsigned int b)
{ Sound::EndMusic(a, b); }
void _ZN5Sound6Play2DEjj(unsigned int bank, unsigned int id)
{ Sound::Play2D(bank, id); }
void _ZN5Sound4PlayEjjRK7Vector3(
    unsigned int bank, unsigned int id, const Vector3 *pos)
{ Sound::Play(bank, id, *pos); }
void _ZN5Sound9PlayBank3EjRK7Vector3(unsigned int id, const Vector3 *pos)
{ Sound::PlayBank3(id, *pos); }

unsigned char _ZN8SaveData13GetCoinRecordEj(unsigned int course)
{ return SaveData::GetCoinRecord(course); }
int _ZN8SaveData15SaveCurrentFileEv(void) { return SaveData::SaveCurrentFile(); }
void _ZN8SaveData17SetCharacterIntroEi(int character)
{ SaveData::SetCharacterIntro(character); }
int _ZN8SaveData19IsCharacterUnlockedEj(unsigned int character)
{ return SaveData::IsCharacterUnlocked(character); }
void _ZN8SaveData21SetCoinRecordIfHigherEah(signed char course, unsigned char coins)
{ SaveData::SetCoinRecordIfHigher(static_cast<signed char>(course), coins); }
unsigned char _ZN8SaveData26CountStarsCollectedInLevelEj(unsigned int course)
{ return SaveData::CountStarsCollectedInLevel(course); }
int _ZN8SaveData22NumGlowingRabbitsFoundEv(void)
{ return SaveData::NumGlowingRabbitsFound(); }

int _ZN5Event6GetBitEj(unsigned int bit) { return Event::GetBit(bit); }
void _ZN5Event6SetBitEj(unsigned int bit) { Event::SetBit(bit); }
int _ZN5Event8ClearBitEj(unsigned int bit) { return Event::ClearBit(bit); }

fBase_c *_ZN7dBase_c5SpawnEjP7fBase_cii(
    unsigned int actor, fBase_c *parent, int a, int b)
{ return dBase_c::Spawn(actor, parent, a, b); }

void _ZN3G2x18SetBlendBrightnessEPVtts(
    volatile unsigned short *regs, unsigned short value, short amount)
{ G2x::SetBlendBrightness(regs, value, amount); }
void _ZN3G2x12SetBGyAffineEPVtP9Matrix2x2iiii(
    volatile unsigned short *regs, Matrix2x2 *matrix,
    int a, int b, int c, int d)
{ G2x::SetBGyAffine(regs, matrix, a, b, c, d); }

void _ZN10dCcAcPos_cC1Ev(void *self)
{ ::new (self) dCcAcPos_c(); }
void _ZN10dCcAcPos_cD1Ev(void *self)
{ static_cast<dCcAcPos_c *>(self)->~dCcAcPos_c(); }
Vector3 *_ZN10dCcAcPos_c6GetPosEv(dCcAcPos_c *self)
{ return &self->GetPos(); }
void _ZN10dCcAcPos_c21SetPosRelativeToActorERK7Vector3(
    dCcAcPos_c *self, const Vector3 *offset)
{ self->SetPosRelativeToActor(*offset); }

Particle::SysTracker::Contents::Entry *_ZNK8Particle10SysTracker8Contents8FindDataEj(
    const Particle::SysTracker::Contents *self, unsigned int id)
{ return self->FindData(id); }
unsigned int _ZN8Particle10SysTracker8Contents6CreateEjR7Vector3PK11Vector3_16fPN5dPa_c7level_c10callback_cE(
    Particle::SysTracker::Contents *self, unsigned int definition,
    Vector3 *position, const Vector3_16f *direction,
    dPa_c::level_c::callback_c *callback)
{ return self->Create(definition, *position, direction, callback); }

int _Z15ApproachLinear2Rsss(short *value, short target, short step)
{ return ApproachLinear2(*value, target, step); }
extern "C" int _Z15ApproachLinear2Riii(int *value, int target, int step)
{ return ApproachLinear2(*value, target, step); }
}

// These recovered minigame declarations use char pointers for byte arithmetic;
// canonical SDK definitions return void pointers (or the same 32-bit address).
#pragma comment(linker,"/alternatename:?GetBG2CharPtr@G2@@YAPADXZ=?GetBG2CharPtr@G2@@YAPAXXZ")
#pragma comment(linker,"/alternatename:?GetBG2ScrPtr@G2@@YAPADXZ=?GetBG2ScrPtr@G2@@YAPAXXZ")
#pragma comment(linker,"/alternatename:?GetBG3ScrPtr@G2@@YAPADXZ=?GetBG3ScrPtr@G2@@YAPAXXZ")
#pragma comment(linker,"/alternatename:?GetBG3ScrPtr@G2S@@YAPADXZ=?GetBG3ScrPtr@G2S@@YAIXZ")
