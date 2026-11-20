"""Bind the volcano's platforms and flame to the native actor ABI."""
import struct


COLLISION_SURFACE = "data_ov064_0211bbcc"
TRAVEL_LIMIT = "data_ov022_02112c98"
BATTAN_SURFACE = "data_ov064_0211ba4c"
FLAMETHROWER_TABLES = ("data_ov095_02136f80", "data_ov095_02136f98", "data_ov095_02136fb0")


def replace(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


def transform_flamethrower(root, text):
    rom = (root / 'extracted/overlays/overlay_0095.bin').read_bytes()
    base = 0x02135700
    assert struct.unpack_from('<IHHIiiii', rom, 0x021376cc - base) == (
        0x02136ed4, 318, 316, 2, 0, 0x500000, 0x1000000, 0)
    for symbol in ('daObjFlamethrower_c_classInit', '_ZN19daObjFlamethrower_c13InitResourcesEv',
                   '_ZN19daObjFlamethrower_c8BehaviorEv'):
        assert f'// @symbol {symbol}\n' in text
    return r'''//cpp
#include "daObjFlamethrower_c.h"
#include "Particle__System.h"
#include "Player.h"
#include "Sound.h"
#include "ntr/rt.h"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <new>

extern "C" {
extern signed char data_0209f2f8;
extern short data_02082214[];
extern const s16 data_ov095_02136f80[], data_ov095_02136f98[];
extern const s32 data_ov095_02136fb0[];
void *_ZN7fBase_cnwEj(unsigned int);
unsigned short DecIfAbove0_Short(unsigned short *);
void Matrix4x3_FromRotationXYZExt(void *, int, int, int);
void MulVec3Mat4x3(const Vector3 *, const Matrix4x3 *, Vector3 *);
void Vec3_Add(Vector3 *, const Vector3 *, const Vector3 *);
void _ZN8dCcPos_c4InitERK7Vector35Fix12IiES4_jj(dCcPos_c *, const Vector3 *, int, int, u32, u32);
void func_02022774(Fix12i, Fix12i, Fix12i, s16, s16);
void func_020226fc(Fix12i, Fix12i, Fix12i, s16, s16);
void func_02022864(Fix12i, Fix12i, Fix12i, s16, s16);
void func_020227ec(Fix12i, Fix12i, Fix12i, s16, s16);
}

// These ARM9 wrappers keep their scale callback and shared emitter ID by offset.
static_assert(offsetof(Particle::SysTracker, mCallback_7b4) == 0x7b4);
static_assert(offsetof(Particle::SysTracker, mCallback_7c4) == 0x7c4);
static_assert(offsetof(Particle::SysTracker, mCallback_7d4) == 0x7d4);
static_assert(offsetof(Particle::SysTracker, mCallback_7e4) == 0x7e4);

namespace {
constexpr int kHaunt = 12;
constexpr unsigned kPhaseFrames = 60;

void position_segment(daObjFlamethrower_c &jet, int segment) {
    Vector3 offset{}, rotated{};
    offset.z = data_ov095_02136fb0[segment];
    MulVec3Mat4x3(&offset, &jet.mTransform, &rotated);
    const Vector3 base = {jet.mPosX, jet.mPosY, jet.mPosZ};
    Vec3_Add(&jet.mFlamePositions[segment], segment == 0 ? &base : &jet.mFlamePositions[segment - 1], &rotated);
}
}

// @symbol daObjFlamethrower_c_classInit
extern "C" void *daObjFlamethrower_c_classInit() {
    void *storage = _ZN7fBase_cnwEj(sizeof(daObjFlamethrower_c));
    return storage ? ::new (storage) daObjFlamethrower_c{} : nullptr;
}

// @symbol _ZN19daObjFlamethrower_c13InitResourcesEv
int daObjFlamethrower_c::InitResources() {
    const int count = data_0209f2f8 == kHaunt ? 9 : 12;
    Matrix4x3_FromRotationXYZExt(&mTransform, mAngleX, mAngleY, mAngleZ);
    for (int i = 0; i < count; ++i) {
        position_segment(*this, i);
        const int scale = data_ov095_02136f80[i];
        _ZN8dCcPos_c4InitERK7Vector35Fix12IiES4_jj(&mColliders[i], &mFlamePositions[i],
            scale * 20, scale * 40, 0x200002, 0);
    }
    return 1;
}

// @symbol _ZN19daObjFlamethrower_c8BehaviorEv
int daObjFlamethrower_c::Behavior() {
    const bool in_haunt = data_0209f2f8 == kHaunt;
    const bool fixed_pair = (param1 & 0xff) == 0xff;
    const bool aimed = !fixed_pair || in_haunt;
    const int count = in_haunt ? 9 : 12;
    if (mState == 0 && mActiveFlames != 0) --mActiveFlames;
    if (mState == 1 && mActiveFlames < count - 1) ++mActiveFlames;
    if (DecIfAbove0_Short(&mTimer) == 0) {
        mState ^= 1;
        mTimer = kPhaseFrames;
    }
    if (mActiveFlames) {
        const Vector3 sound_position = {mCamSpacePosX, mCamSpacePosY, mCamSpacePosZ};
        mSoundHandle = Sound::PlayLong(mSoundHandle, 3, 0x180, sound_position, 0);
    }
    const bool visible = (mFlags & 8) == 0;
    const bool trace = std::getenv("SM64DS_PC_LAVA_VOLCANO_TRACE") != nullptr;
    const auto tick = static_cast<unsigned long long>(ntr::rt_frame());
    if (trace && tick % 10 == 0) {
        const auto &tip = mFlamePositions[mActiveFlames ? mActiveFlames - 1 : 0];
        std::fprintf(stderr, "[native flamethrower] tick=%llu uid=%u level=%d phase=%u timer=%u active=%u visible=%u tip=%d,%d,%d\n",
            tick, uniqueID, int(data_0209f2f8), unsigned(mState), unsigned(mTimer), unsigned(mActiveFlames),
            unsigned(visible), int(tip.x), int(tip.y), int(tip.z));
    }
    if (!visible) return 1;

    if (aimed) Matrix4x3_FromRotationXYZExt(&mTransform, mAngleX, mAngleY, mAngleZ);
    for (int i = 0; i < count; ++i) {
        auto &collider = mColliders[i];
        if (i < mActiveFlames) {
            if (aimed) {
                position_segment(*this, i);
                if (in_haunt) {
                    const auto wave = data_02082214[((i * 0x1000) >> 4) * 2];
                    mFlamePositions[i].y = mPosY + int((static_cast<long long>(wave) * -0xaa000 + 0x800) >> 12);
                }
                collider.pos = mFlamePositions[i];
            }
            if (auto *other = collider.otherOwner ? dActor_c::FindWithID(collider.otherOwner) : nullptr;
                other && other->actorID == 191) {
                auto *player = static_cast<Player *>(other);
                const int health = player->GetHealth();
                const int accepted = player->Burn();
                if (trace && accepted) {
                    std::fprintf(stderr, "[native flamethrower hit] tick=%llu uid=%u segment=%d player=%u health=%d\n",
                        tick, uniqueID, i, player->uniqueID, health);
                }
            }
            if (data_0209ee74 && data_0209ee74->mManager) {
                const auto &pos = mFlamePositions[i];
                const Fix12i y = pos.y + 0x32000;
                const s16 scale = data_ov095_02136f80[i];
                const s16 distortion = data_ov095_02136f98[i];
                if (fixed_pair) {
                    func_02022864(pos.x, y, pos.z, scale, 0);
                    func_020227ec(pos.x, y, pos.z, scale, distortion);
                } else {
                    func_02022774(pos.x, y, pos.z, scale, 0);
                    func_020226fc(pos.x, y, pos.z, scale, distortion);
                }
            }
        }
        collider.Clear();
        if (i < mActiveFlames) collider.dCc_c::Update();
    }
    return 1;
}
'''


def transform_lava_emitter(text):
    text = replace(text, '#include "Player.h"', '''#include "Player.h"
#include "Particle__System.h"
#include "ntr/rt.h"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <new>

// The original six-argument particle wrapper passes this callback by offset.
static_assert(offsetof(Particle::SysTracker, mCallback_808) == 0x808);''')
    text = replace(text, '    return new daObjLava_c();', '''    void *storage = _ZN7fBase_cnwEj(sizeof(daObjLava_c));
    return storage ? ::new (storage) daObjLava_c{} : nullptr;''')
    text = replace(text, '    Vector3 *pos = (Vector3 *)&ClosestPlayer()->mPosX;', '''    const auto *player = ClosestPlayer();
    if (!player || !data_0209ee74 || !data_0209ee74->mManager) return 1;
    const auto *pos = reinterpret_cast<const Vector3 *>(&player->mPosX);
    const auto previous = mEffectHandle;''')
    return replace(text, '''        pos->x, pos->y, pos->z, 0);
    return 1;''', '''        pos->x, pos->y, pos->z, 0);
    const auto tick = ntr::rt_frame();
    if (std::getenv("SM64DS_PC_LAVA_VOLCANO_TRACE") && tick % 20 == 0) {
        const auto *entry = data_0209ee74->mContents.FindData(mEffectHandle);
        const bool lava_callback = entry && entry->callback == &data_0209ee74->mCallback_808;
        std::fprintf(stderr, "[native lava effect] tick=%llu uid=%u player=%u handle=%u previous=%u system=%p callback=%u x=%d y=%d z=%d\\n",
            static_cast<unsigned long long>(tick), uniqueID, player->uniqueID, mEffectHandle, previous,
            entry ? static_cast<void *>(entry->system) : nullptr, lava_callback ? 1u : 0u,
            pos->x, pos->y, pos->z);
    }
    return 1;''')


def transform_lava_bubble(root, text):
    overlay = (root / 'extracted/overlays/overlay_0064.bin').read_bytes()
    base = 0x02115ee0
    assert struct.unpack_from('<IHHIiiii', overlay, 0x0211bec8 - base) == (
        0x02118b10, 214, 58, 2, 0x64000, 0x64000, 0x1000000, 0x640000)
    for address, target in ((0x0211be90, 0x02118760), (0x0211be98, 0x02118644),
                            (0x0211bea0, 0x021187d0), (0x0211bea8, 0x0211873c)):
        assert struct.unpack_from('<II', overlay, address - base) == (target, 0)
    text = replace(text, '#include "Player.h"', '''#include "Player.h"
#include "Particle__System.h"
#include "ntr/rt.h"
#include <cstdio>
#include <cstdlib>
#include <new>''')
    text = replace(text, 'void ApproachLinear(int &value, int target, int step);',
                   'int ApproachLinear(int &value, int target, int step);')
    begin = text.index('/* Eight-byte pointer-to-member records.')
    end = text.index('// @symbol daBbl_c_classInit', begin)
    text = text[:begin] + '''daBbl_c::State data_ov064_0211c7b8 = {
    &daBbl_c::func_ov064_021187d0, &daBbl_c::func_ov064_02118760
};
daBbl_c::State data_ov064_0211c7c8 = {
    &daBbl_c::func_ov064_0211873c, &daBbl_c::func_ov064_02118644
};
}

''' + text[end:]
    text = text[:text.index('/* Declaration order is the retail __sinit order:')]
    for table in ('data_ov064_0211c7b8', 'data_ov064_0211c7c8'):
        text = replace(text, f'func_ov064_021187ec((State *){table});',
                       f'func_ov064_021187ec(&{table});')
    text = replace(text, '    return new daBbl_c();', '''    void *storage = _ZN7fBase_cnwEj(sizeof(daBbl_c));
    return storage ? ::new (storage) daBbl_c{} : nullptr;''')
    text = replace(text, '''    if (mJumps != 0) {
        func_ov064_021187ec(&data_ov064_0211c7c8);''', '''    if (std::getenv("SM64DS_PC_LAVA_VOLCANO_TRACE"))
        std::fprintf(stderr, "[native lava bubble] init tick=%llu uid=%u jumps=%u x=%d y=%d z=%d\\n",
            static_cast<unsigned long long>(ntr::rt_frame()), uniqueID, unsigned(mJumps), mPosX, mPosY, mPosZ);
    if (mJumps != 0) {
        func_ov064_021187ec(&data_ov064_0211c7c8);''')
    text = replace(text, '''                    if (isPlayer)
                        ((Player *)other)->Burn();''', '''                    if (isPlayer) {
                        const int accepted = static_cast<Player *>(other)->Burn();
                        if (std::getenv("SM64DS_PC_LAVA_VOLCANO_TRACE"))
                            std::fprintf(stderr, "[native lava bubble] hit tick=%llu uid=%u player=%u accepted=%d\\n",
                                static_cast<unsigned long long>(ntr::rt_frame()), uniqueID, other->uniqueID, accepted);
                    }''')
    text = replace(text, '''        dActor_c::Spawn(0xd6, 0, *(const Vector3 *)spawnPos,
                        (const Vector3_16 *)&mPrevAngleX, mAreaId, -1);''', '''        auto *child = dActor_c::Spawn(0xd6, 0, *(const Vector3 *)spawnPos,
                        (const Vector3_16 *)&mPrevAngleX, mAreaId, -1);
        if (child && std::getenv("SM64DS_PC_LAVA_VOLCANO_TRACE"))
            std::fprintf(stderr, "[native lava bubble] launch tick=%llu parent=%u child=%u\\n",
                static_cast<unsigned long long>(ntr::rt_frame()), uniqueID, child->uniqueID);''')
    text = replace(text, '''    if (mWithMeshClsn.IsOnGround() != 0) {
        if (mHorzSpeed == 0)''', '''    if (mWithMeshClsn.IsOnGround() != 0) {
        if (std::getenv("SM64DS_PC_LAVA_VOLCANO_TRACE"))
            std::fprintf(stderr, "[native lava bubble] bounce tick=%llu uid=%u initial=%u y=%d speed=%d\\n",
                static_cast<unsigned long long>(ntr::rt_frame()), uniqueID, mHorzSpeed == 0 ? 1u : 0u,
                mPosY, mVertSpeed);
        if (mHorzSpeed == 0)''')
    return replace(text, '''    if ((u16)mStateTimer == 0)
        MarkForDestruction();''', '''    if (std::getenv("SM64DS_PC_LAVA_VOLCANO_TRACE") && ntr::rt_frame() % 10 == 0) {
        std::fprintf(stderr, "[native lava bubble] flight tick=%llu uid=%u x=%d y=%d z=%d vy=%d fire=%p smoke=%p\\n",
            static_cast<unsigned long long>(ntr::rt_frame()), uniqueID, mPosX, mPosY, mPosZ, mVertSpeed,
            static_cast<void *>(Particle::System::FromUniqueID(mFireParticle)),
            static_cast<void *>(Particle::System::FromUniqueID(mSmokeParticle)));
    }
    if ((u16)mStateTimer == 0) {
        if (std::getenv("SM64DS_PC_LAVA_VOLCANO_TRACE"))
            std::fprintf(stderr, "[native lava bubble] expired tick=%llu uid=%u\\n",
                static_cast<unsigned long long>(ntr::rt_frame()), uniqueID);
        MarkForDestruction();
    }''')


def transform_battan(root, text):
    overlay = (root / 'extracted/overlays/overlay_0023.bin').read_bytes()
    profile = struct.unpack_from('<IHHIiiii', overlay, 0x02111fc0 - 0x021111a0)
    assert profile == (0x02111728, 84, 292, 0x02000000, 0, 0x500000, 0x1000000, 0x1000000)
    assert struct.unpack_from('<I', overlay, 0x02111af8 - 0x021111a0) == (1558,)
    assert struct.unpack_from('<I', overlay, 0x02111b08 - 0x021111a0) == (1559,)
    surface = (root / 'extracted/overlays/overlay_0064.bin').read_bytes()
    assert struct.unpack_from('<4sHH', surface, 0x0211ba4c - 0x02115ee0) == (b'CLPS', 8, 1)
    text = '#include <new>\n' + text
    text = replace(text, 'extern "C" {\nextern char', 'struct dCamera_c;\nextern "C" {\nextern char')
    text = replace(text, 'extern int _ZN8dActor_c18DropShadowScaleXYZ', 'extern void _ZN8dActor_c18DropShadowScaleXYZ')
    text = replace(text, 'dActor_c *, dExtShadowModel_c *, Matrix4x3 *, Fix12i, Fix12i, Fix12i, u32);',
                   'dActor_c *, dExtShadowModel_c *, Matrix4x3 *, Fix12i, Fix12i, Fix12i, u8);')
    text = replace(text, '''extern void _ZN8dActor_c10EarthquakeERK7Vector35Fix12IiE(
    dActor_c *, BattanVector3 *, Fix12i);''', '''extern dCamera_c *data_0209f318;
extern void func_0200d8c8(dCamera_c *, const BattanVector3 *, Fix12i);''')
    text = replace(text, 'extern void func_020393d4(int *, int);\n', '')
    begin = text.index('struct BattanModelFilePtr :')
    end = text.index('// @symbol daObjFm_Battan_c_classInit', begin)
    text = text[:begin] + '''struct BattanFilePtr : SharedFilePtr { u32 words[2]; };
static_assert(sizeof(BattanFilePtr) == 8);
extern "C" BattanFilePtr data_ov023_02112088 = {};
extern "C" BattanFilePtr data_ov023_02112080 = {};

''' + text[end:]
    text = replace(text, '    return new daObjFm_Battan_c();', '''    void *storage = _ZN7fBase_cnwEj(sizeof(daObjFm_Battan_c));
    return storage ? ::new (storage) daObjFm_Battan_c{} : nullptr;''')
    text = replace(text, '''    mModel.SetFile((BMD_File *)Model::LoadFile(data_ov023_02112088), 1, -1);
    mShadowModel.InitCuboid();''', '''    mHasBattanResources = false;
    static const bool resourcesReady = [] {
        data_ov023_02112088.Construct(1558);
        data_ov023_02112080.Construct(1559);
        return true;
    }();
    (void)resourcesReady;
    auto *model = static_cast<BMD_File *>(Model::LoadFile(data_ov023_02112088));
    if (!model || !mModel.SetFile(model, 1, -1)) {
        if (model) data_ov023_02112088.Release();
        return 0;
    }
    auto *file = reinterpret_cast<KCL_File *>(dBgW_Kc::LoadFile(data_ov023_02112080));
    if (!file) {
        data_ov023_02112088.Release();
        return 0;
    }
    mShadowModel.InitCuboid();''')
    text = replace(text, '    KCL_File *file = (KCL_File *)dBgW_Kc::LoadFile(data_ov023_02112080);\n', '')
    begin = text.index('    /* Stores dBgW::UpdatePosWithTransform')
    end = text.index('    mAngVelX = 0;', begin)
    text = text[:begin] + '''    mMeshCollider.beforeClsnCallback = [](dBgW *collider, dActor_c *actor,
            dBgPi *result, Vector3 *position, Vector3_16 *motion, Vector3_16 *angles) {
        dBgW::UpdatePosWithTransform(*collider, actor, *result, *position, motion, angles);
    };
    if (!mMeshCollider.Enable(this)) {
        data_ov023_02112080.Release();
        data_ov023_02112088.Release();
        return 0;
    }
    mHasBattanResources = true;

''' + text[end:]
    text = replace(text, '            BattanVector3 offset;', '            BattanVector3 offset{};')
    text = replace(text, '''            _ZN8dActor_c10EarthquakeERK7Vector35Fix12IiE(
                this, &pos, 0x04000000);''', '''            if (data_0209f318) func_0200d8c8(data_0209f318, &pos, 0x04000000);''')
    text = replace(text, '''    mMeshCollider.Disable();
    data_ov023_02112088.Release();
    data_ov023_02112080.Release();''', '''    if (mHasBattanResources) {
        mMeshCollider.Disable();
        data_ov023_02112088.Release();
        data_ov023_02112080.Release();
        mHasBattanResources = false;
    }''')
    text = replace(text, '    return _ZN8dActor_c18DropShadowScaleXYZ', '    _ZN8dActor_c18DropShadowScaleXYZ')
    text = replace(text, '        0x258000, 0x32000, -depth, 0xf);',
                   '        0x258000, 0x32000, -depth, 0xf);\n    return 1;')
    from battlefield_platform_host import observe
    return observe(text, 'daObjFm_Battan_c', 'state,timer',
                   ['int(mState)', 'int(mStateTimer)'])


def transform(root, text, name):
    if name == "d_a_obj_maruta.cpp":
        return transform_log_base(text)
    overlay = (root / 'extracted/overlays/overlay_0022.bin').read_bytes()
    base = 0x021111a0
    assert struct.unpack_from('<3I', overlay, 0x02112c9c-base) == (0x02114668, 0x02114670, 0x0211bbcc)
    assert struct.unpack_from('<4I', overlay, 0x02114424-base) == (0x021126ac, 0, 0x02112710, 0)
    surface = (root / 'extracted/overlays/overlay_0064.bin').read_bytes()
    assert struct.unpack_from('<4sH', surface, 0x0211bbcc-0x02115ee0) == (b'CLPS', 8)

    text = replace(text, '#include "decl_Platform.h"\n', '')
    text = replace(text, '#include "decl_common.h"\n', '')
    text = replace(text, '#include "decl_Actor.h"', '''#include <cstdio>
#include <cstdlib>
#include <new>
#include "ntr/rt.h"
#include "daObjFl_Ring_c.h"
#include "SharedFilePtr.h"
#include "decl_Actor.h"''')
    begin = text.index('/* daObjFlMaruta_c\'s collaborators. */')
    end = text.index('#pragma defer_codegen off', begin)
    text = text[:begin] + '''void func_020393a4(dBgW_KcMbg *p, int v);
extern int data_ov022_02112c98[];
int RandomIntInternal(int *seed);
extern int data_0209e650;
extern CLPS_Block data_ov064_0211bbcc;
}

struct ResourceDescriptor {
    SharedFilePtr *model;
    SharedFilePtr *collision;
    CLPS_Block *clps;
};
struct LavaLogHandle : SharedFilePtr { u32 words[2]; };
static_assert(sizeof(LavaLogHandle) == 8);
static LavaLogHandle logModel, logCollision;
extern "C" ResourceDescriptor data_ov022_02112c9c = {
    &logModel, &logCollision, &data_ov064_0211bbcc
};
extern "C" int func_ov080_021270dc(daObjMaruta_c *, ResourceDescriptor *);
extern "C" int func_ov080_0212714c(daObjMaruta_c *, int *);
extern "C" int func_ov080_021274ac(daObjMaruta_c *, ResourceDescriptor *);
extern "C" daObj_volcanoCannon_c::State data_ov022_02114690 = {
    &daObj_volcanoCannon_c::InitFlight, &daObj_volcanoCannon_c::UpdateFlight
};

''' + text[end:]
    text = replace(text, 'func_ov080_021270dc(((void *)this), data_ov022_02112c9c)',
                   'func_ov080_021270dc(this, &data_ov022_02112c9c)')
    text = replace(text, 'func_ov080_0212714c(((char *)this), data_ov022_02112c98)',
                   'func_ov080_0212714c(this, data_ov022_02112c98)')
    text = replace(text, '    return func_ov080_021274ac(((void *)this), data_ov022_02112c9c);', '''    static const bool handlesReady = [] {
        logModel.Construct(1531);
        logCollision.Construct(1532);
        return true;
    }();
    (void)handlesReady;
    return func_ov080_021274ac(this, &data_ov022_02112c9c);''')
    begin = text.index('extern "C" int *daObjFlMaruta_c_classInit(void)')
    end = text.index('// @symbol _ZN21daObj_volcanoCannon_cD1Ev', begin)
    text = text[:begin] + '''extern "C" void *daObjFlMaruta_c_classInit()
{
    void *storage = _ZN7fBase_cnwEj(sizeof(daObjFlMaruta_c));
    return storage ? ::new (storage) daObjFlMaruta_c{} : nullptr;
}

''' + text[end:]
    begin = text.index('// @symbol func_ov022_02112654')
    end = text.index('// @symbol _ZN21daObj_volcanoCannon_c11ChangeState', begin)
    text = text[:begin] + '''int daObj_volcanoCannon_c::UpdateFlight()
{
    if (mPosY < mKillPosY || !mKillTimer) {
        MarkForDestruction();
        return 1;
    }
    if (mCylinderClsn.otherOwner) {
        auto *other = dActor_c::FindWithID(mCylinderClsn.otherOwner);
        if (other && other->actorID == 191) {
            auto *player = static_cast<Player *>(other);
            const int health = player->GetHealth();
            const int accepted = player->Burn();
            if (std::getenv("SM64DS_PC_LAVA_VOLCANO_TRACE"))
                std::fprintf(stderr, "[native volcano flame] hit tick=%llu uid=%u ring=%u player=%u health=%d accepted=%d\\n",
                    static_cast<unsigned long long>(ntr::rt_frame()), uniqueID, mSpawnerId,
                    player->uniqueID, health, accepted);
            MarkForDestruction();
        }
    }
    return 1;
}

int daObj_volcanoCannon_c::InitFlight()
{
    const auto random = static_cast<unsigned>(RandomIntInternal(&data_0209e650)) >> 8;
    mKillTimer = 0x3c;
    mPrevAngleY = static_cast<s16>((random & 0xf) << 12);
    mHorzSpeed = ((random & 0x1f) + 0x14) * 4096;
    mVertAccel = -0x4000;
    mVertSpeed = ((random & 0x3f) + 0x28) * 4096;
    if (std::getenv("SM64DS_PC_LAVA_VOLCANO_TRACE"))
        std::fprintf(stderr, "[native volcano flame] spawn tick=%llu uid=%u ring=%u\\n",
            static_cast<unsigned long long>(ntr::rt_frame()), uniqueID, mSpawnerId);
    return 1;
}

''' + text[end:]
    text = replace(text, '''    if (mSpawner)
        *(u16 *)((char *)mSpawner + 0x324) -= 1;''', '''    auto *owner = mSpawnerId ? dActor_c::FindWithID(mSpawnerId) : nullptr;
    if (owner && owner->actorID == 71) {
        auto *ring = static_cast<daObjFl_Ring_c *>(owner);
        if (ring->mSpawnCount) --ring->mSpawnCount;
    }
    if (std::getenv("SM64DS_PC_LAVA_VOLCANO_TRACE"))
        std::fprintf(stderr, "[native volcano flame] retire tick=%llu uid=%u ring=%u\\n",
            static_cast<unsigned long long>(ntr::rt_frame()), uniqueID, mSpawnerId);''')
    particle = '''                mParticleID, 0x129, mPosX, mPosY, mPosZ, 0);'''
    text = replace(text, particle, particle + '''
            if (mParticleID && std::getenv("SM64DS_PC_LAVA_VOLCANO_TRACE") && ntr::rt_frame()%20 == 0)
                std::fprintf(stderr, "[native volcano flame] emission tick=%llu uid=%u particle=%u y=%d\\n",
                    static_cast<unsigned long long>(ntr::rt_frame()), uniqueID, mParticleID, mPosY);''')
    begin = text.index('extern "C" int *daObj_volcanoCannon_c_classInit(void)')
    return text[:begin] + '''extern "C" void *daObj_volcanoCannon_c_classInit()
{
    void *storage = _ZN7fBase_cnwEj(sizeof(daObj_volcanoCannon_c));
    return storage ? ::new (storage) daObj_volcanoCannon_c{} : nullptr;
}
'''


def transform_log_base(text):
    text = replace(text, '#include "common.h"', '''#include <cstdio>
#include <cstdlib>
#include "ntr/rt.h"
#include "hal/actor_render_probe.h"
#include "common.h"''')
    text = replace(text, '    arg->model->Release();\n    arg->collision->Release();', '''    if (self->mHasMarutaResources) {
        arg->model->Release();
        arg->collision->Release();
        self->mHasMarutaResources = false;
    }''')
    text = replace(text, 's32 daObjMaruta_c::Render()\n{',
                   's32 daObjMaruta_c::Render()\n{\n    pc_render_probe::Scope probe(this,0,&mModel.mat4x3);')
    text = replace(text, '    char *c = (char *)self;\n', '')
    text = text.replace('(u8 *)((int)c + 0x33e)', '&self->mHitTimer')
    text = text.replace('*(s16 *)((c + 0x300) + 0x3c)', 'self->mRollAngVel')
    text = text.replace('(s16 *)(int)(c + 0x33c)', '&self->mRollAngVel')
    text = text.replace('(s16 *)(int)(c + 0x8c)', '&self->mAngleX')
    text = text.replace('*(u16 *)((c + 0x300) + 0x40)', 'self->mSoundTimer')
    text = text.replace('(u16 *)(c + 0x340)', '&self->mSoundTimer')
    text = text.replace('(t >> 6) << 12', '(t >> 6) * 4096')
    anchor = '// @symbol func_ov080_021274ac\n'
    text = replace(text, anchor, '''static void carryWithLog(dBgW *collider, dActor_c *actor, dBgPi *result,
                         Vector3 *position, Vector3_16 *motion, Vector3_16 *angles)
{
    dBgW::UpdatePosWithVelocity(*collider, actor, *result, *position, motion, angles);
}

''' + anchor)
    begin = text.index('    self->mModel.SetFile(', text.index(anchor))
    end = text.index('    self->mRestPosX', begin)
    text = text[:begin] + '''    self->mHasMarutaResources = false;
    auto *model = static_cast<BMD_File *>(Model::LoadFile(*arg->model));
    if (!model || !self->mModel.SetFile(model, 1, -1)) {
        if (model) arg->model->Release();
        return 0;
    }
    auto *collision = reinterpret_cast<KCL_File *>(dBgW_Kc::LoadFile(*arg->collision));
    if (!collision) {
        arg->model->Release();
        return 0;
    }
    self->func_ov080_02127094();
    self->UpdateClsnPosAndRot();
    _ZN10dBgW_KcMbg7SetFileEP8KCL_FileRK9Matrix4x35Fix12IiEsR10CLPS_Block(
        &self->mMeshCollider, collision, &self->mClsnMat, 0x1000, self->mAngleY, arg->clps);
    self->mMeshCollider.beforeClsnCallback = carryWithLog;
    // Rider contact can start the roll while the cylinder is stationary.
    func_020393c4(reinterpret_cast<int *>(&self->mMeshCollider),
                 reinterpret_cast<int>(&func_ov080_02127658));
    self->mHasMarutaResources = true;
''' + text[end:]
    text = replace(text, '    t = (*(volatile u16 *)(a1 + 0xc) == 0xbf);',
                   '    t = (reinterpret_cast<dActor_c *>(a1)->actorID == 191);')
    text = replace(text, '    self->mHitByPlayer = 0;\n    return 1;', '''    if (std::getenv("SM64DS_PC_LAVA_VOLCANO_TRACE") && ntr::rt_frame()%10 == 0)
        std::fprintf(stderr, "[native lava log] tick=%llu uid=%u x=%d y=%d z=%d roll=%d contact=%u collision=%u\\n",
            static_cast<unsigned long long>(ntr::rt_frame()), self->uniqueID,
            self->mPosX, self->mPosY, self->mPosZ, int(self->mRollAngVel),
            unsigned(self->mHitByPlayer), unsigned(self->mMeshCollider.IsEnabled()));
    self->mHitByPlayer = 0;
    return 1;''')
    return text
