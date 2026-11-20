"""Bind the rolling log and volcano flame to the native actor ABI."""
import struct


COLLISION_SURFACE = "data_ov064_0211bbcc"
TRAVEL_LIMIT = "data_ov022_02112c98"


def replace(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


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
            static_cast<Player *>(other)->Burn();
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
    if (collider->owner && actor)
        func_ov080_0212758c(static_cast<daObjMaruta_c *>(collider->owner),
                           reinterpret_cast<char *>(actor));
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
