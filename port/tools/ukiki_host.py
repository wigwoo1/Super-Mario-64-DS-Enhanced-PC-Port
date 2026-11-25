"""Adapt Ukiki's existing file references to native shared-file handles."""
import re


# File references in the original initializer and animation load table.
ANIMATIONS = (
    ("02115ce0", 939),
    ("02115d08", 941),
    ("02115d10", 940),
    ("02115ce8", 944),
    ("02115d18", 942),
    ("02115d20", 943),
    ("02115cf0", 945),
    ("02115cd0", 946),
    ("02115cf8", 947),
    ("02115cd8", 948),
)


def replace(text, old, new):
    if text.count(old) != 1:
        raise ValueError(f"unexpected Ukiki source around {old!r}")
    return text.replace(old, new, 1)


def transform(text):
    index = {address: i for i, (address, _) in enumerate(ANIMATIONS)}
    idle_order = ("02115cf0", "02115cd8", "02115cf0", "02115cd0", "02115cd0",
                  "02115cd0", "02115cd0", "02115cf8", "02115cd8", "02115cf8", "02115cd8")
    resource_ids = ", ".join(str(file_id) for _, file_id in ANIMATIONS)
    idle_indices = ", ".join(str(index[address]) for address in idle_order)
    text = replace(text, '#include "dBgPi.h"', '#include "dBgPi.h"\n#include "daObjMarioCap_c.h"\n'
                   '#include "dClipper.h"\n#include <cstdint>\n#include <cstring>\n#include <new>\n' + f"""
namespace {{
struct UkikiFile : SharedFilePtr {{
    u16 fileId;
    u8 refs;
    u8 padding;
    void *asset;
}};
static_assert(sizeof(UkikiFile) == 8);

UkikiFile ukikiModel;
UkikiFile ukikiAnimations[{len(ANIMATIONS)}];
constexpr u16 ukikiAnimationIds[] = {{{resource_ids}}};
constexpr u8 ukikiIdleOrder[] = {{{idle_indices}}};
const Vector3 ukikiCarryOffsets[] = {{
    {{0x1c000, -0x42000, 0x18000}}, {{0x1c000, -0x28000, 0x28000}},
    {{0x2b000, -0x42000, 0x18000}}, {{0x30000, -0x1e000, 0x1e000}},
}};

// The native cap actor reads this pointer from the same dActor storage.
void setUkikiCapMatrix(dActor_c &cap, const void *matrix)
{{
    static_assert(offsetof(dActor_c, pad_0c5) == 0xc5 && sizeof(matrix) == 4);
    std::memcpy(cap.pad_0c5 + 3, &matrix, sizeof(matrix));
}}

void initializeUkikiFiles()
{{
    static const bool initialized = [] {{
        ukikiModel.Construct(938);
        for (unsigned i = 0; i < sizeof ukikiAnimationIds / sizeof *ukikiAnimationIds; ++i)
            ukikiAnimations[i].Construct(ukikiAnimationIds[i]);
        return true;
    }}();
    (void)initialized;
}}
}}

extern "C" void *_ZN7fBase_cnwEj(unsigned int);
extern "C" dClipper data_0209f43c;
extern "C" Matrix4x3 data_0209b3ec;
""")
    # Each recovered declaration describes a different view of the same handle.
    # Normalize all the views before the transformed file reaches MSVC.
    for address, _ in ANIMATIONS:
        symbol = "data_ov030_" + address
        handle = f"ukikiAnimations[{index[address]}].asset"
        text = re.sub(rf"^\s*extern [^\n;]+\b{symbol}(?:\[\])?;\n", "\n", text, flags=re.M)
        for form in (f"((void **)&{symbol})[1]", f"*(void **)({symbol} + 4)",
                     f"{symbol}.w[1]", f"{symbol}.b", f"{symbol}[1]"):
            text = text.replace(form, handle)
        if re.search(rf"\b{symbol}\b", text):
            raise ValueError(f"unconverted Ukiki animation reference: {symbol}")
    text = replace(text, '    extern void** data_ov030_02115bc8[];\n', '')
    text = replace(text, 'data_ov030_02115bc8[this->mAnimIdx][1]',
                   'ukikiAnimations[ukikiIdleOrder[this->mAnimIdx]].asset')
    text = text.replace('(int)this->mModelAnim.file', 'this->mModelAnim.file')
    text = text.replace('int v = this->mModelAnim.file;', 'const auto *v = this->mModelAnim.file;')
    text = replace(text, 'extern SharedFilePtr  data_ov030_02115d00;\n', '')
    text = replace(text, 'extern SharedFilePtr *data_ov030_02114824[10];\n', '')
    text = replace(text, '    data_ov030_02115d00.Release();\n'
                   '    for (i = 0; i < 10; i++)\n        data_ov030_02114824[i]->Release();',
                   '    ukikiModel.Release();\n    for (auto &animation : ukikiAnimations)\n        animation.Release();')
    text = replace(text, '    int i;\n    data_ov002_0210da40.Release();',
                   '    data_ov002_0210da40.Release();')
    text = replace(text, '    int i;\n    int b;\n    u16 h;',
                   '    int b;\n    u16 h;\n    initializeUkikiFiles();')
    text = replace(text, '    mModelAnim.SetFile((BMD_File *)Model::LoadFile(data_ov030_02115d00), 1, 1);\n'
                   '    for (i = 0; i < 10; i++)\n        dExtFrameCtrl_c::LoadFile(*data_ov030_02114824[i]);',
                   '''    auto *model = static_cast<BMD_File *>(Model::LoadFile(ukikiModel));
    bool animationsReady = true;
    for (auto &animation : ukikiAnimations) {
        if (!dExtFrameCtrl_c::LoadFile(animation))
            animationsReady = false;
    }
    if (!model || !animationsReady)
        return 0;
    mModelAnim.SetFile(model, 1, 1);''')
    text = replace(text, '    extern char data_0209f43c;\n    extern char data_0209b3ec;\n', '')
    text = replace(text, '        && *(int *)((char *)(Player *)this->unk_3a8 + 0xc8)) {',
                   '        && reinterpret_cast<dActor_c *>(static_cast<Player *>(unk_3a8)->mHeldObj) == this) {')
    text = replace(text, '    extern char data_ov030_02115ddc[];\n', '')
    text = replace(text, '*(Vector3 *)(data_ov030_02115ddc + idx * 0xc)',
                   'ukikiCarryOffsets[idx]')
    text = replace(text, '    obj = dActor_c::FindWithID(id);',
                   '    obj = dActor_c::FindWithID(id);\n    if (!obj || obj->actorID != 0x10d) return;')
    text = replace(text, '    *(int *)((char *)obj + 0xc8) = (int)(this->mCapMtx);',
                   '    setUkikiCapMatrix(*obj, mCapMtx);')
    text = replace(text, '                *(void **)((char *)a + 0xd0) = this->unk_3a8;\n'
                   '                a->mFlags |= 0x40000;', '''                if (a && a->actorID == 0x10d) {
                    static_cast<daObjMarioCap_c *>(a)->mEatingPlayer = reinterpret_cast<s32>(unk_3a8);
                    a->mFlags |= 0x40000;
                }''')
    text = replace(text, '                *(int *)((char *)act + 0xc8) = z;\n'
                   '                {', '''                if (act && act->actorID == 0x10d) {
                    setUkikiCapMatrix(*act, nullptr);''')
    text = re.sub(r'^    extern void\s*\*\s*data_02099368\[\];\n', '', text, flags=re.M)
    begin = text.index('    typedef void* (*Vfn)();', text.index('int daMky_c::func_ov030_02112400()'))
    end = text.index('    func_ov030_02111a00();', begin)
    text = text[:begin] + text[end:]
    begin = text.index('            char* r = (char *)func_0203567c')
    end = text.index('            _ZN5dBgPiD1Ev(&res);', begin) + len('            _ZN5dBgPiD1Ev(&res);')
    text = text[:begin] + '''            const auto &hit = static_cast<const dBgPi &>(mWithMeshClsn.mSphereClsn);
            if (hit.GetClsnID() == 0xffffffffu || !hit.owner)
                func_ov030_021141a8(0);''' + text[end:]
    begin = text.index('    typedef struct P2', text.index('int daMky_c::func_ov030_02112a84()'))
    end = text.index('    dBgCh_Actr_UpdateContinuous_Veneer', begin)
    text = text[:begin] + text[end:]
    begin = text.index('            char *r = (char *)func_0203567c')
    end = text.index('            _ZN5dBgPiD1Ev(&res);', begin) + len('            _ZN5dBgPiD1Ev(&res);')
    text = text[:begin] + '''            const auto &hit = static_cast<const dBgPi &>(mWithMeshClsn.mSphereClsn);
            if (hit.GetClsnID() != 0xffffffffu)
                func_ov030_021141a8(9);
            else
                func_ov030_021141a8(mPrevState);''' + text[end:]
    text = replace(text, '    dActor_c *cage = dActor_c::FindWithActorID(0x67, 0);\n'
                   '    Player *player = this->ClosestPlayer();',
                   '''    dActor_c *cage = dActor_c::FindWithActorID(0x67, 0);
    Player *player = this->ClosestPlayer();
    if (!cage || !player) {
        MarkForDestruction();
        return 1;
    }''')
    allocation = '''    void *storage = _ZN7fBase_cnwEj(sizeof(daMky_c));
    return storage ? ::new (storage) daMky_c{} : nullptr;'''
    if text.count('    return new daMky_c();') != 2:
        raise ValueError("expected both original Ukiki factories")
    return text.replace('    return new daMky_c();', allocation)
