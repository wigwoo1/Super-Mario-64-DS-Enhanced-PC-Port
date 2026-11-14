"""Bind Flame Chomp and its fireball to the native actor runtime."""
import struct
from battlefield_platform_host import lazy_handles


def transform(root, text, projectile):
    rom = (root / 'extracted/overlays/overlay_0070.bin').read_bytes()
    base = 0x0211f000
    if projectile:
        assert struct.unpack_from('<IHHIiiii', rom, 0x02123424 - base) == (
            0x021221fc, 271, 130, 6, 0x1e000, 0x3c000, 0x1000000, 0x1000000)
        for slot, callback in (
            (0x021233ec, 0x02121fb0), (0x021233fc, 0x02121f18),
            (0x021233f4, 0x02121ef8), (0x02123404, 0x02121eb0),
        ):
            assert struct.unpack_from('<II', rom, slot - base) == (callback, 0)
            name = f'data_ov070_{slot:08x}'
            text = text.replace(f'extern "C" daKpFrStateMethod {name};', '')
            text = text.replace(name, f'reinterpret_cast<daKpFrStateMethod>(&daKpFr_c::func_ov070_{callback:08x})')
        text = text.replace('func_02012694(int id, void *pos)', 'func_02012694(unsigned int id, const Vector3 *pos)')
        return text.replace('func_02012694(0x166, &mCamSpacePosX)', 'func_02012694(0x166, reinterpret_cast<const Vector3 *>(&mCamSpacePosX))')

    assert struct.unpack_from('<IHHIiiii', rom, 0x0212334c - base) == (
        0x02121af8, 270, 129, 3, 0, 0x2d000, 0x1000000, 0xed8000)
    text = lazy_handles(text, ('KrpaModelFilePtr',))
    anchor = 'int daKrpa_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '''
    static const bool ready = [] {
        data_ov070_02123698.Construct(0x351);
        return true;
    }();
    (void)ready;''')
    render = 'int daKrpa_c::Render()\n{'
    return '#include "hal/actor_render_probe.h"\n' + text.replace(
        render, render + '\n    const bool inMouth = (mFlags & 0x40000) != 0;'
        '\n    pc_render_probe::Scope probe(this, inMouth, &mModelAnim.mat4x3);'
        '\n    if (inMouth) return 1;')
