"""Bind the original Skeeter profile resources and state callbacks on PC."""

import struct

from battlefield_platform_host import lazy_handles


def transform(root, text):
    rom = (root / 'extracted/overlays/overlay_0090.bin').read_bytes()
    base = 0x02130f00
    assert struct.unpack_from('<IHHIiiii', rom, 0x02134144 - base) == (
        0x02132654, 230, 85, 0x10000002, 0xc8000, 0xc8000, 0x1000000, 0x1000000)
    offset = struct.unpack_from('<3i', rom, 0x0213412c - base)
    assert offset == (0, 0x1e000, 0)
    text = lazy_handles(text, ('MenboModelFilePtr', 'MenboAnimationFilePtr'))
    text = text.replace('extern Vector3       data_ov090_0213412c;',
                        'static Vector3 data_ov090_0213412c{0, 0x1e000, 0};')
    text = text.replace('struct MenboState { char pad[0x370]; MenboStateFn *mState; };', '')
    text = text.replace('MenboState *c', 'daMenbo_c *c')
    text = text.replace('(MenboState *)', '')
    text = text.replace('MenboStateFn *q = c->mState;',
                        'auto *q = static_cast<MenboStateFn *>(c->mState);')
    text = text.replace('((daMenbo_c *)c->**q)()', '(c->**q)()')
    text = text.replace('*(int *)((int)this + 0x390)', 'MENBO_RIPPLES(this)->loops')
    text = text.replace('(s32 *)((int)this + 0x390)', '&MENBO_RIPPLES(this)->loops')
    text = text.replace('(s32 *)((int)self + 0x390)', '&MENBO_RIPPLES(self)->loops')
    text = text.replace('(short *)((unsigned int)this + 0x39a)', 'reinterpret_cast<short *>(&unk_39a)')
    text = text.replace('(short *)((char *)this + 0x39a)', 'reinterpret_cast<short *>(&unk_39a)')
    text = text.replace('void  func_02012694(int id,', 'void  func_02012694(unsigned int id,')
    anchor = 'int daMenbo_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '''
    static const bool handlesReady = [] {
        data_ov090_021344a0.Construct(0x39f);
        data_ov090_02134488.Construct(0x3a0);
        data_ov090_02134480.Construct(0x3a1);
        data_ov090_02134490.Construct(0x3a2);
        data_ov090_02134498.Construct(0x3a3);
        return true;
    }();
    (void)handlesReady;
''')
    text = text.replace('int daMenbo_c::Render()\n{', '''int daMenbo_c::Render()
{
    pc_render_probe::Scope probe(this, 0, &mModelAnim.mat4x3);''')
    return '#include "hal/actor_render_probe.h"\n' + text
