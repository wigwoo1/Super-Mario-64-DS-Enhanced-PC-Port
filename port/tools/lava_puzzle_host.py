"""Bind the recovered lava puzzle to native resources and typed callbacks."""
import re
import struct


OVERLAY = 64
BASE = 0x02115EE0
MODEL_TABLE = 0x0211ADC8
SCRIPT_TABLE = 0x0211C198
COLLISION_SURFACE = 0x0211BAAC
STATE_METHODS = (
    "func_ov064_02118da0", "func_ov064_02118d3c", "func_ov064_02118d20",
    "func_ov064_02118d08", "func_ov064_02118cec", "func_ov064_02118cd4",
)


def tables(root):
    image = (root / "extracted/overlays/overlay_0064.bin").read_bytes()
    models = struct.unpack_from("<14I", image, MODEL_TABLE - BASE)
    scripts = struct.unpack_from("<14I", image, SCRIPT_TABLE - BASE)
    assert len(set(models)) == len(models) and len(set(scripts)) == len(scripts)
    for address in scripts:
        offset = address - BASE
        row = image[offset:offset+27]
        assert len(row) == 27 and row[-1] == 0xff and all(1 <= value <= 5 for value in row[:-1])
    assert struct.unpack_from("<4sHH", image, COLLISION_SURFACE - BASE) == (b"CLPS", 8, 1)
    return models, scripts


def data_requests(root):
    _, scripts = tables(root)
    return tuple(f"data_ov064_{address:08x}" for address in (*scripts, COLLISION_SURFACE))


def replace_once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


def transform(root, text):
    models, scripts = tables(root)
    # hostgen already binds the constructors and destructors for these handles.
    handles = dict(re.findall(r"FlCoinModelFilePtr (data_ov064_[\da-f]+)\((0x[\da-f]+)\);", text))
    assert set(handles) == {f"data_ov064_{address:08x}" for address in models}
    declarations = ",\n    ".join(f"&data_ov064_{address:08x}" for address in models)
    script_names = [f"data_ov064_{address:08x}" for address in scripts]
    text = replace_once(text, "extern int data_ov064_0211c198[];",
                        "extern const s8 *const data_ov064_0211c198[14];")

    # Retain the original six state methods. DS member pointers cannot be
    # loaded into native code that uses MSVC member-pointer encoding.
    state_anchor = 'extern "C" {\nextern Entry data_ov064_0211bf68;'
    assert text.count(state_anchor) == 1
    text = text[:text.index(state_anchor)] + """Entry data_ov064_0211c904[6] = {
""" + "".join(f"    {{&daObjFl_Puzzle_c::{method}}},\n" for method in STATE_METHODS) + """};
"""
    text += '\nextern "C" SharedFilePtr *data_ov064_0211adc8[14] = {\n    ' + declarations + '\n};\n'
    text += 'extern "C" {\n' + "".join(f"extern const s8 {name}[];\n" for name in script_names)
    text += 'extern const s8 *const data_ov064_0211c198[14] = {\n    '+",\n    ".join(script_names)+'\n};\n}\n'
    text += '''
static_assert(sizeof(FlCoinModelFilePtr)==8 && sizeof(FlCoinCollisionFilePtr)==8);
static_assert(offsetof(daObjFl_Coin_c,mLiveCoinCount)==0xd6);
static_assert(offsetof(daObjFl_Puzzle_c,mState)==0x336);
'''
    loader = """    _ZN9ModelBase7SetFileEP8BMD_Fileii(&mModel,
        _ZN5Model8LoadFileER13SharedFilePtr(*data_ov064_0211adc8[mType]), 1, -1);"""
    text = replace_once(text, loader, """    if (mType >= 14) return 0;
    mCoinMgrId = 0;
    mShakeOffsetY = 0;
    auto &model_handle = *data_ov064_0211adc8[mType];
    auto *model = static_cast<BMD_File *>(Model::LoadFile(model_handle));
    if (!model || !mModel.SetFile(model, 1, -1)) {
        if (model) model_handle.Release();
        mType = 14;
        return 0;
    }
    auto *collision = _ZN7dBgW_Kc8LoadFileER13SharedFilePtr(data_ov064_0211c800);
    if (!collision) {
        model_handle.Release();
        mType = 14;
        return 0;
    }""")
    text = replace_once(text, "        _ZN7dBgW_Kc8LoadFileER13SharedFilePtr(data_ov064_0211c800),",
                        "        collision,")
    text = replace_once(text, "_ZN10dBgW_KcMbg9TransformERK9Matrix4x3s(&mMeshCollider, mClsnMat, mAngleY)",
                        "mMeshCollider.Transform(mClsnMat, mAngleY)")
    text = replace_once(text, "    mStepScript = data_ov064_0211c198[mType];",
                        "    mStepScript = reinterpret_cast<s32>(data_ov064_0211c198[mType]);")
    text = replace_once(text, "    data_ov064_0211adc8[idx]->Release();\n    data_ov064_0211c800.Release();", """    if (idx < 14) {
        data_ov064_0211adc8[idx]->Release();
        data_ov064_0211c800.Release();
    }""")
    text = replace_once(text, "    *(unsigned char*)(((int)this + 0x336)) = *(unsigned char*)(((int)this + 0x336)) + 1;   /* mState++ */",
                        "    ++mState;")
    text = replace_once(text, "    u8 isPlayer = other->actorID == 0xbf;",
                        "    u8 isPlayer = other && other->actorID == 0xbf;")
    text = replace_once(text, "int daObjFl_Puzzle_c::Render()\n{", """int daObjFl_Puzzle_c::Render()
{
    pc_render_probe::Scope probe(this,mType,&mModel.mat4x3);""")
    piece_anchor = """    if (_ZN10dBgActor_c13IsClsnInRangeE5Fix12IiES1_(this, 0, 0) != 0)
        func_ov064_02118fa4();"""
    text = replace_once(text, piece_anchor, piece_anchor + """
    if (const char *trace=std::getenv("SM64DS_PC_LAVA_PUZZLE_TRACE")) {
        if (trace[0]=='1' && ntr::rt_frame()%10==0)
            std::fprintf(stderr,"[native lava puzzle tile] tick=%llu uid=%u type=%u manager=%u x=%d y=%d z=%d state=%u step=%u timer=%u shake=%d touch=%u collider=%u\n",
                static_cast<unsigned long long>(ntr::rt_frame()),uniqueID,unsigned(mType),mCoinMgrId,
                mPosX,mPosY,mPosZ,unsigned(mState),unsigned(mStepIndex),unsigned(mStepTimer),mShakeOffsetY,
                unsigned(mHadClsn),unsigned(mMeshCollider.IsEnabled()));
    }""".replace("\n\"", "\\n\""))
    text = replace_once(text, "    found->mLiveCoinCount += 1;",
                        """    found->mLiveCoinCount += 1;
    if (const char *trace=std::getenv("SM64DS_PC_LAVA_PUZZLE_TRACE")) {
        if (trace[0]=='1')
            std::fprintf(stderr,"[native lava puzzle coin] tick=%llu tile=%u coin=%u manager=%u live=%u\n",
                static_cast<unsigned long long>(ntr::rt_frame()),uniqueID,spawned->uniqueID,
                found->uniqueID,unsigned(found->mLiveCoinCount));
    }""".replace("\n\"", "\\n\""))
    return ('#include <cstddef>\n#include <cstdio>\n#include <cstdlib>\n'
            '#include "hal/actor_render_probe.h"\n#include "ntr/rt.h"\n'+text)
