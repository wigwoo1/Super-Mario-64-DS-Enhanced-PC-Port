"""Bind the original arena ice to native callbacks and locally loaded files."""
import re
import struct

from battlefield_platform_host import observe


ICE_SURFACES = ("data_ov057_0211153c", "data_ov057_0211155c", "data_ov057_0211159c")


def transform_ice_factory(text):
    factory, = re.findall(r'int \*?(daObjEwbIce_c_classInit_EWB_ICE_[ABC])\(void\)', text)
    return f'''#include "daObjEwbIce_c.h"
#include <cstddef>
#include <new>
extern "C" void *_ZN7fBase_cnwEj(unsigned int);
static_assert(sizeof(daObjEwbIce_c) == 0x33c);
static_assert(offsetof(daObjEwbIce_c, mState) == 0x320);
static_assert(offsetof(daObjEwbIce_c, mSpawnIndex) == 0x32d);
extern "C" void *{factory}() {{
    void *storage = _ZN7fBase_cnwEj(sizeof(daObjEwbIce_c));
    return storage ? ::new (storage) daObjEwbIce_c{{}} : nullptr;
}}
'''


def transform_ice(root, text):
    overlays = root / "extracted/overlays"
    actor = (overlays / "overlay_0073.bin").read_bytes()
    stage = (overlays / "overlay_0057.bin").read_bytes()
    rows = (
        (0x02123428, 0x02123448, 0x0211153c),
        (0x02123430, 0x02123450, 0x0211155c),
        (0x02123440, 0x02123438, 0x0211159c),
    )
    assert struct.unpack_from("<9I", actor, 0x021231bc - 0x0211f000) == sum(rows, ())
    for _, _, address in rows:
        assert struct.unpack_from("<4sHH", stage, address - 0x021111a0) == (b"CLPS", 8, 1)
    callbacks = (
        (0x02123134, 0x0212239c), (0x0212312c, 0x021222ec),
        (0x0212311c, 0x021222c8), (0x0212310c, 0x02122200),
        (0x02123114, 0x021221e0), (0x02123124, 0x021220c0),
        (0x02123144, 0x02122034), (0x0212313c, 0x0212202c),
    )
    for slot, target in callbacks:
        assert struct.unpack_from("<2I", actor, slot - 0x0211f000) == (target, 0)

    def replace(old, new):
        nonlocal text
        assert text.count(old) == 1, old
        text = text.replace(old, new, 1)

    start = text.index("/* Column of the 0xc-stride file table.")
    end = text.index("enum {", start)
    surfaces = "".join(f'extern "C" unsigned char {name}[];\n' for name in ICE_SURFACES)
    text = text[:start] + surfaces + '''
struct daObjEwbIce_State {
    int (*enter)(daObjEwbIce_c *);
    int (*update)(daObjEwbIce_c *);
};
struct IceFiles {
    struct Handle : SharedFilePtr {
        unsigned fileId;
        void *file;
    } model, collision;
    unsigned modelId, collisionId;
    void *surface;
};
static IceFiles iceFiles[] = {
    {{}, {}, 1511, 1512, data_ov057_0211153c},
    {{}, {}, 1513, 1514, data_ov057_0211155c},
    {{}, {}, 1515, 1516, data_ov057_0211159c},
};
static unsigned char iceBigIndex, iceSmallIndex;
extern "C" daObjEwbIce_State data_ov073_021234b0;

''' + text[end:]
    for suffix in ("02123420", "02123424"):
        replace(f"extern u8 data_ov073_{suffix}[];\n", "")
    for suffix in ("021231bc", "021231c0", "021231c4"):
        replace(f"extern char data_ov073_{suffix}[];\n", "")
    replace("    return (ice->*(ice->mState->enter))();", "    return ice->mState->enter(ice);")
    replace("        (this->*(mState->update))();", "        mState->update(this);")
    for suffix, name in (("02123420", "iceSmallIndex"), ("02123424", "iceBigIndex")):
        text = text.replace(f"data_ov073_{suffix}[0]", name)
    replace("    case ACTOR_EWB_ICE_A:\n        mTimer = kReportFrames;",
            "    case ACTOR_EWB_ICE_A:\n        iceBigIndex = iceSmallIndex = 0;\n        mTimer = kReportFrames;")
    replace('''    ((EwbIceFileSlot *)data_ov073_021231bc)[mVariant].file->Release();
    ((EwbIceFileSlot *)data_ov073_021231c0)[mVariant].file->Release();''', '''    if (mState) {
        iceFiles[mVariant].model.Release();
        iceFiles[mVariant].collision.Release();
        mState = nullptr;
    }''')
    replace('''    bmd = (BMD_File *)Model::LoadFile(*((EwbIceFileSlot *)data_ov073_021231bc)[mVariant].file);
    mModel.SetFile(bmd, 1, -1);''', '''    static const bool initialized = [] {
        for (auto &files : iceFiles) {
            files.model.Construct(files.modelId);
            files.collision.Construct(files.collisionId);
        }
        return true;
    }();
    (void)initialized;
    auto &files = iceFiles[mVariant];
    bmd = static_cast<BMD_File *>(Model::LoadFile(files.model));
    if (!bmd)
        return 0;
    if (!mModel.SetFile(bmd, 1, -1)) {
        files.model.Release();
        return 0;
    }''')
    start = text.index("    /* mVariant reloaded after LoadFile")
    end = text.index("    func_020393c4(", start)
    text = text[:start] + '''    kcl = reinterpret_cast<KCL_File *>(dBgW_Kc::LoadFile(files.collision));
    if (!kcl) {
        files.model.Release();
        return 0;
    }
    _ZN10dBgW_KcMbg7SetFileEP8KCL_FileRK9Matrix4x35Fix12IiEsR10CLPS_Block(
        &mMeshCollider, kcl, &mClsnMat, 0x1000, mAngleY, files.surface);
    mMeshCollider.beforeClsnCallback = [](dBgW *mesh, dActor_c *actor,
            dBgPi *result, Vector3 *position, Vector3_16 *motion, Vector3_16 *angles) {
        dBgW::UpdatePosWithTransform(*mesh, actor, *result, *position, motion, angles);
    };
''' + text[end:]
    text = text[:text.index("/* Static-init globals")] + '''
extern "C" daObjEwbIce_State data_ov073_021234b0 = {
    [](daObjEwbIce_c *) { return func_ov073_0212239c(); }, func_ov073_021222ec
};
extern "C" daObjEwbIce_State data_ov073_021234c0 = {
    func_ov073_021222c8, func_ov073_02122200
};
extern "C" daObjEwbIce_State data_ov073_021234d0 = {
    func_ov073_021221e0, func_ov073_021220c0
};
extern "C" daObjEwbIce_State data_ov073_021234a0 = {
    [](daObjEwbIce_c *ice) { return func_ov073_02122034(reinterpret_cast<char *>(ice)); },
    [](daObjEwbIce_c *) { return func_ov073_0212202c(); }
};
'''
    return observe(text, "daObjEwbIce_c", "variant,index,timer",
                   ["int(mVariant)", "int(mSpawnIndex)", "int(mTimer)"])


def transform_boss(root, text):
    actor = (root / "extracted/overlays/overlay_0073.bin").read_bytes()
    profile = struct.pack("<IHHIiiii", 0x02121ec8, 218, 62, 0x10000000,
                          0x1f4000, 0x1f4000, 0x1000000, 0x1000000)
    assert actor.count(profile) == 1

    def replace(old, new):
        nonlocal text
        assert text.count(old) == 1, old
        text = text.replace(old, new, 1)

    replace('''struct C;
typedef int (C::*PMF)();
struct C { char pad[0x37c]; PMF *pp; };''', '''using C = daKing_Donketu_c;
using PMF = daKing_Donketu_c::State;''')
    replace("extern struct BCA_File *data_ov002_0210da30[2];",
            "extern SharedFilePtr data_ov002_0210da30;")
    replace('extern "C" int ChiefChilly_ChangeState(C *c, PMF *p) { c->pp = p; PMF *q = c->pp; if (*q == 0) return 1; return (c->**q)(); }',
            '''extern "C" int ChiefChilly_ChangeState(C *c, PMF *p)
{
    c->mState = p;
    return p->enter ? p->enter(*c) : 1;
}''')
    replace('''    if (*(void **)((char *)c->pp + 8) != 0) {
        PMF *p = c->pp + 1;
        (c->**p)();
    }''', '''    auto *state = static_cast<State *>(mState);
    if (state && state->update)
        state->update(*this);''')
    text = text.replace("(char *)c->pp", "(char *)mState")
    replace("  ((Base *)&mBlendModelAnim)->M((char*)&mScaleX);", """    pc_render_probe::Scope probe(this, 0, &mBlendModelAnim.mat4x3);
    mBlendModelAnim.Render(reinterpret_cast<const Vector3 *>(&mScaleX));""")
    for handle in ("KingDonketuModelFileHandle", "KingDonketuAnimResFileHandle"):
        replace(f"{handle}(unsigned int fileID) {{ pc_sharedfile_handle_construct(this, fileID); }}",
                f"{handle}(unsigned int id) : fileId(id), file(nullptr) {{}}")
        replace(f"~{handle}() {{ pc_sharedfile_handle_destroy(this); }}", f"~{handle}() {{}}")

    start = text.index("    LoadKeyModels(4);", text.index("int daKing_Donketu_c::InitResources()"))
    end = text.index("    mShadowModel.InitCylinder();", start)
    text = text[:start] + '''    SharedFilePtr *animations[] = {
        &data_ov073_02123280, &data_ov073_021232a0, &data_ov073_02123288,
        &data_ov073_021232a8, &data_ov073_02123290, &data_ov073_021232b0,
        &data_ov073_021232b8
    };
    static const bool initialized = [] {
        data_ov073_02123280.Construct(0x355);
        data_ov073_021232a0.Construct(0x356);
        data_ov073_02123288.Construct(0x35b);
        data_ov073_021232a8.Construct(0x354);
        data_ov073_02123290.Construct(0x357);
        data_ov073_021232b0.Construct(0x358);
        data_ov073_021232b8.Construct(0x35a);
        data_ov073_02123298.Construct(0x359);
        if (*reinterpret_cast<const u32 *>(&data_ov002_0210da30) == 0)
            data_ov002_0210da30.Construct(0x49f);
        return true;
    }();
    (void)initialized;
    LoadKeyModels(4);
    mNativeLoadedFiles = 1u << 9;
    for (unsigned slot = 0; slot < 7; ++slot) {
        if (!_ZN15dExtFrameCtrl_c8LoadFileER13SharedFilePtr(animations[slot]))
            return 0;
        mNativeLoadedFiles |= 1u << slot;
    }
    if (!_ZN5Model8LoadFileER13SharedFilePtr(&data_ov002_0210da30))
        return 0;
    mNativeLoadedFiles |= 1u << 7;
    f = _ZN5Model8LoadFileER13SharedFilePtr(&data_ov073_02123298);
    if (!f)
        return 0;
    mNativeLoadedFiles |= 1u << 8;
    if (!mBlendModelAnim.SetFile(f, 1, -1))
        return 0;
''' + text[end:]
    start = text.index("  UnloadKeyModels(4);")
    end = text.index("\n  return 1;", start)
    text = text[:start] + '''    SharedFilePtr *files[] = {
        &data_ov073_02123280, &data_ov073_021232a0, &data_ov073_02123288,
        &data_ov073_021232a8, &data_ov073_02123290, &data_ov073_021232b0,
        &data_ov073_021232b8, &data_ov002_0210da30, &data_ov073_02123298
    };
    if (mNativeLoadedFiles & (1u << 9))
        UnloadKeyModels(4);
    for (unsigned slot = 0; slot < 9; ++slot) {
        if (mNativeLoadedFiles & (1u << slot))
            files[slot]->Release();
    }
    mNativeLoadedFiles = 0;''' + text[end:]
    start = text.index('extern "C" daKing_Donketu_c *daKing_Donketu_c_classInit()\n{')
    end = text.index("\n/* Construction order", start)
    text = text[:start] + '''static_assert(offsetof(daKing_Donketu_c, mState) == 0x37c);
static_assert(offsetof(daKing_Donketu_c, mWaypointsA) == 0x3e8);
static_assert(offsetof(daKing_Donketu_c, mWaypointsB) == 0x448);
static_assert(offsetof(daKing_Donketu_c, mWaypointCursor) == 0x4c4);
static_assert(offsetof(daKing_Donketu_c, mNativeLoadedFiles) == 0x504);
extern "C" daKing_Donketu_c *daKing_Donketu_c_classInit()
{
    void *storage = _ZN7fBase_cnwEj(sizeof(daKing_Donketu_c));
    return storage ? ::new (storage) daKing_Donketu_c{} : nullptr;
}
''' + text[end:]
    pattern = (r'daKing_Donketu_c::State (data_ov073_02123[\da-f]{3}) = \{ '
               r'(?:\(int \(daKing_Donketu_c::\*\)\(\)\))?&daKing_Donketu_c::(func_ov073_[\da-f]+),\s+'
               r'&daKing_Donketu_c::(func_ov073_[\da-f]+) \};')
    states = re.findall(pattern, text)
    assert len(states) == 16
    native_targets = {int(fn.rsplit("_",1)[1], 16) for _, enter, update in states for fn in (enter, update)}
    slots = range(0x02122f40 - 0x0211f000, 0x02123040 - 0x0211f000, 8)
    original = [struct.unpack_from("<2I", actor, off) for off in slots]
    assert all(adjust == 0 for _, adjust in original)
    assert native_targets == {address for address, _ in original}
    text = re.sub(pattern, lambda m: f'''daKing_Donketu_c::State {m[1]} = {{
    [](daKing_Donketu_c &boss) -> int {{ return boss.{m[2]}(); }},
    [](daKing_Donketu_c &boss) -> int {{ return boss.{m[3]}(); }}
}};''', text)
    trace = '''    if (std::getenv("SM64DS_PC_BATTLEFIELD_PLATFORM_TRACE")) {
        static const State *last = nullptr;
        if (last != mState) {
            last = static_cast<State *>(mState);
            const char *stateName = "unknown";
'''
    trace += "".join(f'            if (last == &{name}) stateName = "{name[-4:]}";\n'
                     for name, _, _ in states)
    trace += '''            std::fprintf(stderr, "[native Chief Chilly] tick=%llu uid=%u state=%s hits=%u\\n",
                static_cast<unsigned long long>(ntr::rt_frame()), unsigned(uniqueID),
                stateName, unsigned(mHitsRemaining));
        }
    }
'''
    replace("    _ZN14BlendModelAnim7AdvanceEv(&mBlendModelAnim);",
            trace + "    _ZN14BlendModelAnim7AdvanceEv(&mBlendModelAnim);")
    replace(
        "        if (!line.DetectClsn()) {",
        '''        const bool groundAhead = line.DetectClsn();
        if (mState == &data_ov073_021233a0 &&
            std::getenv("SM64DS_PC_BATTLEFIELD_PLATFORM_TRACE")) {
            std::fprintf(stderr, "[native Chief Chilly rim] tick=%llu uid=%u hits=%u "
                "start=%d,%d,%d end=%d,%d,%d yaw=%d speed=%d ground=%u\\n",
                static_cast<unsigned long long>(ntr::rt_frame()), unsigned(uniqueID),
                unsigned(mHitsRemaining), rp.start.x, rp.start.y, rp.start.z,
                rp.end.x, rp.end.y, rp.end.z, int(mPrevAngleY), int(mHorzSpeed),
                unsigned(groundAhead));
        }
        if (!groundAhead) {''',
    )
    return '#include <cstddef>\n#include <new>\n#include <cstdio>\n#include <cstdlib>\n#include "hal/actor_render_probe.h"\n#include "ntr/rt.h"\n' + text
