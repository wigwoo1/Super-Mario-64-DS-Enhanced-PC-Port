"""Bind the original Whomp's shared resource handles and native state methods."""

import re
import struct


def transform(root, text):
    rom = (root / 'extracted/overlays/overlay_0079.bin').read_bytes()
    base = 0x02123740
    assert struct.unpack_from('<IHHIiiii', rom, 0x02127c5c - base) == (
        0x021265e8, 164, 74, 0x10000002, 0, 0x320000, 0x1000000, 0x960000)

    text, count = re.subn(r'^struct Btn(?:ModelFilePtr|TextureSeqFileHandle|AnimationFileHandle|CollisionFilePtr) : SharedFilePtr \{.*?^\};\n',
                          '', text, flags=re.M | re.S)
    assert count == 4
    handles = dict(re.findall(r'^Btn\w+\s+(data_ov079_\w+)\((0x\w+)\);', text, re.M))
    assert len(handles) == 16
    text = re.sub(r'^Btn\w+\s+(data_ov079_\w+)\((0x\w+)\);',
                  r'static BtnHandle \1;', text, flags=re.M)
    text = text.replace('struct BtnFileRef { int unk0; void *file; };', '''struct BtnHandle : SharedFilePtr {
    unsigned int idAndRefs;
    void *file;
};
static_assert(sizeof(BtnHandle) == 8);
static_assert(offsetof(BtnHandle, file) == 4);''')
    text = text.replace('BtnFileRef', 'BtnHandle')

    tables = []
    for name, size in (('021275ec', 11), ('02127bf0', 2)):
        pointers = struct.unpack_from(f'<{size}I', rom, int(name, 16) - base)
        entries = [f'&data_ov079_{pointer:08x}' for pointer in pointers]
        assert all(entry[1:] in handles for entry in entries)
        tables.append(f'static BtnHandle *const data_ov079_{name}[] = {{\n    ' +
                      ',\n    '.join(entries) + '\n};')
    tables.append('static auto *const data_ov079_02127600 = data_ov079_021275ec + 5;')
    text = re.sub(r'^extern void \*data_ov079_021(?:275ec|27600|27bf0)\[\];\n',
                  '', text, flags=re.M)

    start = text.index('extern "C" {\nextern daBtn_c::StateFunc')
    end = text.index('/* OnAimedAtWithEggReturnVec', start)
    records = re.findall(r'extern daBtn_c::StateFunc data_ov079_(\w+);\s*/\* \d+ (func_ov079_\w+)', text[start:end])
    assert len(records) == 12
    workers = []
    for template, method in records:
        address, adjustment = struct.unpack_from('<II', rom, int(template, 16) - base)
        assert adjustment == 0 and method == f'func_ov079_{address:08x}'
        workers.append('&daBtn_c::' + method)
    text = text[:start] + '\n'.join(tables) + '\n\n' + (
        'static const daBtn_c::StateFunc data_ov079_02128280[] = {\n    ' +
        ',\n    '.join(workers) + '\n};\n\n') + text[end:]
    text = text.replace('void **tab;', 'BtnHandle *const *tab;')
    text = text.replace('StateFunc *step =', 'const StateFunc *step =')
    text = text.replace('namespace vecret { extern "C" Vector3 func_ov079_02123d4c(void *thiz); }', '')
    text = text.replace('return vecret::func_ov079_02123d4c(this);',
                        'int point[3];\n    func_ov079_02123d4c(point, this);\n'
                        '    Vector3 result;\n    result.x = point[0];\n    result.y = point[1];\n'
                        '    result.z = point[2];\n    return result;')
    text = text.replace('void func_02012694(int a,', 'void func_02012694(unsigned int a,')
    anchor = 'int daBtn_c::InitResources()\n{'
    assert text.count(anchor) == 1
    calls = '\n'.join(f'        {name}.Construct({file_id});' for name, file_id in handles.items())
    text = text.replace(anchor, anchor + f'''
    static const bool handlesReady = [] {{
{calls}
        return true;
    }}();
    (void)handlesReady;
''')
    text = text.replace('int daBtn_c::Render()\n{', '''int daBtn_c::Render()
{
    pc_render_probe::Scope probe(this, mState, &mModelAnim.mat4x3);''')
    anchor = '    func_ov079_02124188();\n\n    if (func_ov079_021243e0(0)'
    assert text.count(anchor) == 1
    text = text.replace(anchor, '''    if (mIsKing && std::getenv("SM64DS_PC_TEST_KING_WHOMP")) {
        static unsigned previous = 256;
        if (mSubState != previous || ntr::rt_frame() % 120 == 0) {
            previous = mSubState;
            std::fprintf(stderr,
                "[native King Whomp actor] tick=%llu state=%d phase=%u flags=%08x timer=%u nearest=%d target=%d\\n",
                static_cast<unsigned long long>(ntr::rt_frame()), mState,
                unsigned(mSubState), unsigned(mFlags), unsigned(mStateTimer),
                mNearestDist, int(mTargetAngle));
        }
    }

''' + anchor)
    return ('#include <cstddef>\n#include <cstdio>\n#include <cstdlib>\n'
            '#include "hal/actor_render_probe.h"\n#include "ntr/rt.h"\n' + text)
