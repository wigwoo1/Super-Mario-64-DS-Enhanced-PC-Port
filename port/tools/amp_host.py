"""Native resource and callback bindings for the original Amp actor."""

import struct


def transform(root, text):
    overlay = (root / 'extracted/overlays/overlay_0070.bin').read_bytes()
    base = 0x0211f000

    def read(fmt, address):
        return struct.unpack_from('<' + fmt, overlay, address - base)

    assert read('IHHIiiii', 0x02123254) == (
        0x021210ac, 266, 125, 3, 0, 0x28000, 0x1000000, 0xa28000)
    assert read('12I', 0x0212320c) == (
        0x02120ce4, 0, 0x02120bf8, 0, 0x02120910, 0,
        0x02120cac, 0, 0x021208a4, 0, 0x021209e4, 0)
    assert read('6I', 0x021231f4) == (
        60, 0x02122244, 0x02122268, 0x02122248, 1, 0x0212224c)
    assert read('2I', 0x021222e0) == (0x021235f4, 0x0212360c)
    material_id, name_pointer, *channels = read('2I10H', 0x0212224c)
    assert name_pointer == 0x021231e4
    name = overlay[name_pointer - base:].split(b'\0', 1)[0].decode('ascii')
    assert name == 'mat_bilikyu2'
    rotation = ', '.join(map(str, read('60H', 0x02122268)))
    channel_values = ', '.join(map(str, channels))
    scale, = read('i', 0x02122244)
    translation, = read('i', 0x02122248)

    def replace(old, new):
        nonlocal text
        assert text.count(old) == 1, old
        text = text.replace(old, new)

    text = text.replace('extern SharedFilePtr *data_ov070_021222e0[];', '')
    text = text.replace('extern SharedFilePtr *data_ov070_021222e0[2];', '')
    text = text.replace('extern BTA_File data_ov070_021231f4;', '')
    text = text.replace('extern int data_ov070_0212360c[];', '')
    text = text.replace('extern Vector3 data_ov070_0212365c;', '')
    text = text.replace('extern BrqAnimationResource data_ov070_021235f4;', '')
    text = text.replace('extern daBrq_c::State data_ov070_02123668[];', '')
    text = text.replace('data_ov070_021231f4', 'pc_amp_bta.file')
    text = text.replace('(BCA_File *)data_ov070_0212360c[1]', 'pc_amp_idle.file')
    text = text.replace('data_ov070_021235f4.file', 'pc_amp_active.file')
    text = text.replace('data_ov070_0212365c', 'pc_amp_cylinder_offset')
    text = text.replace('data_ov070_021222e0', 'pc_amp_animations')
    replace('extern char IDENTITY_MATRIX4X3;', 'extern int IDENTITY_MATRIX4X3[];')
    replace('u32 PlayLong(u32 handle, u32 bank, u32 soundId,',
            'int PlayLong(u32 handle, u32 bank, u32 soundId,')
    replace('void ApproachLinear(s32 &value, s32 target, s32 step);',
            'int ApproachLinear(s32 &value, s32 target, s32 step);')
    replace('    mStateHandlers = &data_ov070_02123668[state];', '''
    static State handlers[] = {
        {&daBrq_c::EnterCooldownState, &daBrq_c::UpdateCooldownState},
        {&daBrq_c::EnterActiveState, &daBrq_c::UpdateActiveState},
        {&daBrq_c::EnterDefeatedState, &daBrq_c::UpdateDefeatedState},
    };
    mStateHandlers = &handlers[state];'''.lstrip('\n'))
    replace('struct BrqAnimationResource { int state; BCA_File *file; };\n', '')

    resources = f'''
#include "SharedFilePtr.h"
namespace {{
struct AmpAnimationHandle {{ int id; BCA_File *file; }};
AmpAnimationHandle pc_amp_active{{}}, pc_amp_idle{{}};
SharedFilePtr *const pc_amp_animations[] = {{
    reinterpret_cast<SharedFilePtr *>(&pc_amp_active),
    reinterpret_cast<SharedFilePtr *>(&pc_amp_idle),
}};
Vector3 pc_amp_cylinder_offset = {{0, -0x28000, 0}};

struct AmpTextureMaterial {{
    u32 id;
    const char *name;
    u16 channels[10];
}};
struct AmpTextureAnimation {{
    BTA_File file;
    u16 padding;
    int *scale;
    u16 *rotation;
    int *translation;
    u32 materialCount;
    AmpTextureMaterial *materials;
}};
static_assert(sizeof(AmpTextureMaterial) == 28 && sizeof(AmpTextureAnimation) == 24);
int ampScale = {scale};
int ampTranslation = {translation};
u16 ampRotation[] = {{{rotation}}};
AmpTextureMaterial ampMaterial = {{{material_id}, "{name}", {{{channel_values}}}}};
AmpTextureAnimation pc_amp_bta = {{{{60}}, 0, &ampScale, ampRotation, &ampTranslation, 1, &ampMaterial}};
}}
'''
    replace('struct BrqMatrixWords { int words[12]; };',
            resources + '\nstruct BrqMatrixWords { int words[12]; };')
    replace('int daBrq_c::InitResources()\n{', '''int daBrq_c::InitResources()
{
    static const bool handlesReady = [] {
        data_ov070_021235fc.Construct(0x2b1);
        data_ov070_02123604.Construct(0x2b3);
        pc_amp_animations[0]->Construct(0x2b2);
        pc_amp_animations[1]->Construct(0x2b5);
        data_ov070_021235ec.Construct(0x2b4);
        return true;
    }();
    (void)handlesReady;''')
    replace('    UpdateState();\n    mCylinderOffset.y', r'''    const auto previousState = mState;
    const auto contact = mdCcAcPos_c.otherOwner;
    const auto hit = mdCcAcPos_c.hitFlags;
    UpdateState();
    if (const char *path = std::getenv("SM64DS_PC_AMP_TRACE")) {
        static FILE *trace = std::fopen(path, "w");
        const auto tick = ntr::rt_frame();
        if (trace && (contact || previousState != mState || tick % 5 == 0)) {
            std::fprintf(trace, "%llu,%u,%u,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u\n",
                         static_cast<unsigned long long>(tick), unsigned(uniqueID),
                         unsigned(param1), mPosX, mPosY, mPosZ, mOrbitCenter.x,
                         mOrbitCenter.y, mOrbitCenter.z, mState, unsigned(mStateTimer),
                         unsigned(contact), unsigned(hit));
            std::fflush(trace);
        }
    }
    mCylinderOffset.y''')
    return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n' + text
