"""Adapt the original Goomboss factories and state dispatch for native x86."""
import re
import struct

from battlefield_platform_host import lazy_handles


def transform_factory(text):
    factory, = re.findall(r'void\* (daKuriKing_c_classInit_KURIKING(?:_VANISH)?)\(void\)', text)
    return (
        '#include "daKuriKing_c.h"\n'
        '#include <stddef.h>\n'
        'static_assert(sizeof(daKuriKing_c) == 0x610, "Goomboss object size");\n'
        'static_assert(offsetof(daKuriKing_c, mState) == 0x5cc, "Goomboss state offset");\n'
        f'extern "C" void *{factory}() {{ return new daKuriKing_c(); }}\n'
    )


def transform_actor(text):
    start = text.index('struct KuriKingPmf;')
    end = text.index('#pragma defer_codegen off', start)
    original = text[start:end]
    members = re.findall(r'entries\[(\d+)\].pmf\[(\d+)\] = pc_make_kuriking_pmf\((.*?)\);', original)
    stubs = dict(re.findall(r'KuriKingFreeState (\w+) = \{ \(void \(\*\)\(\)\)(\w+) \};', text))
    assert len(members) == 18 and len(stubs) == 3
    callbacks = []
    for index, (state, slot, target) in enumerate(members):
        assert (int(state), int(slot)) == divmod(index, 2)
        if target.startswith('&daKuriKing_c::'):
            call = 'actor.' + target.split('::', 1)[1]
        else:
            call = stubs[target.removesuffix('.pmf')]
        callbacks.append(f'    [](daKuriKing_c &actor) {{ {call}(); }}')
    table = (
        'using PcGoombossCallback = void (*)(daKuriKing_c &);\n'
        'static const PcGoombossCallback pc_goomboss_states[9][2] = {\n'
        + ',\n'.join('  {\n' + ',\n'.join(callbacks[i:i+2]) + '\n  }'
                       for i in range(0, len(callbacks), 2))
        + '\n};\n\n'
    )
    text = text[:start] + table + text[end:]
    text, count = re.subn(
        r'    KuriKingPmf \*c = \(KuriKingPmf \*\)this;\n(?:    int j = mState;\n)?'
        r'    \(c->\*data_ov074_021230f8.entries\[j\].pmf\[(0|1)\]\)\(\);',
        lambda m: f'    pc_goomboss_states[mState][{m[1]}](*this);'
        + ('\n    return 1;' if m[1] == '0' else ''), text)
    assert count == 2
    text = text.replace('    int j = mState;\n    pc_goomboss_states', '    pc_goomboss_states')
    text = text.replace('KuriKingStateTable data_ov074_021230f8;', '')
    text = lazy_handles(text, ('KuriKingModelFilePtr', 'KuriKingTexSeqFilePtr', 'KuriKingAnimationFilePtr'))
    handles = re.findall(r'^(KuriKing\w+FilePtr) (data_ov074_[0-9a-f]+)\((0x[0-9a-f]+)\);', text, re.M)
    assert len(handles) == 14
    for kind, name, _ in handles:
        text = text.replace(f'extern {kind} {name};', f'extern "C" {kind} {name};')
        text = re.sub(rf'^{kind} {name}\(', f'extern "C" {kind} {name}(', text, flags=re.M)
    text += '\nextern "C" void pc_goomboss_init_handles() {\n'
    text += '\n'.join(f'    {name}.Construct({file_id});' for _, name, file_id in handles)
    text += '\n}\n'
    for declaration in ('SharedFilePtr *data_ov074_0212292c[]', 'SharedFilePtr *data_ov074_02122948[]',
                        'KuriKingAnimEntry data_ov074_02122f34[]'):
        assert text.count('extern '+declaration) == 1
        text = text.replace('extern '+declaration, 'extern "C" '+declaration)
    text = text.replace('extern BMA_File data_ov010_021123f4;', 'extern "C" void *pc_goomboss_bma();')
    text = text.replace('extern BTA_File data_ov010_021124ac;', 'extern "C" void *pc_goomboss_default_bta();')
    text = text.replace('&data_ov010_021123f4', 'pc_goomboss_bma()')
    text = text.replace('&data_ov010_021124ac', 'pc_goomboss_default_bta()')
    text = text.replace('    int off = idx * 0xc;\n'
                        '    void* file1 = *(void**)(*(char**)((char*)data_ov074_02122f34 + off) + 4);\n'
                        '    int arg2 = *(int*)(data_ov074_02122f3c + off);',
                        '    const KuriKingAnimEntry &clip = data_ov074_02122f34[idx];\n'
                        '    void *file1 = clip.anim->filePtr;\n'
                        '    int arg2 = clip.animArg;')
    text = text.replace('*(void**)(data_ov074_02122f38 + off)', 'clip.btaFile')
    return text


def local_resource_cpp(root):
    """Rebase the original arena animation pointers in the local link-data object."""
    folder = root / 'extracted/overlays'
    stage = (folder / 'overlay_0053.bin').read_bytes()
    boss = (folder / 'overlay_0074.bin').read_bytes()
    stage_base, stage_end, boss_base = 0x021111a0, 0x021124dc, 0x0211f000
    materials = (0x021123f4, 0x02112404, 0x0211244c, 0x02112494,
                 0x021124c4, 0x02112464, 0x02112434, 0x021124ac)

    def words(address, count):
        return struct.unpack_from(f'<{count}I', boss, address-boss_base)

    fixups = {}
    relocs = (root / 'config/arm9/overlays/ov053/relocs.txt').read_text()
    for a, b in re.findall(r'from:0x([0-9a-f]+) kind:load to:0x([0-9a-f]+) module:overlay\(53\)', relocs):
        address, target = int(a,16), int(b,16)
        in_animation = address < 0x0211236c or any(
            header <= address < header + (16 if header == materials[0] else 24)
            for header in materials)
        if stage_base <= address < stage_end and in_animation:
            assert stage_base <= target < stage_end
            assert struct.unpack_from('<I', stage, address-stage_base)[0] == target
            fixups[address] = target
    assert len(fixups) == 46
    stage_name = 'pc_goomboss_stage_animations'

    def arena_pointer(address):
        return f'(reinterpret_cast<unsigned char *>({stage_name}) + 0x{address-stage_base:x})'

    scalars = struct.unpack_from(f'<{(stage_end-stage_base)//4}I', stage)
    generated = [f'static void *{stage_name}[] = {{']
    for n, value in enumerate(scalars):
        address = stage_base + n*4
        generated.append(f'    {arena_pointer(value) if address in fixups else f"(void *)0x{value:08x}u"},')
    generated.append('};')
    small = words(0x0212292c, 7)
    big = words(0x02122948, 12)
    clips = list(struct.iter_unpack('<III', boss[0x02122f34-boss_base:0x02122fd0-boss_base]))
    transforms = words(0x02122e5c, 7)
    assert len(clips) == 13 and list(transforms) == list(materials[1:])
    assert set(animation for animation, _, _ in clips) == set(big)
    for address in sorted(small):
        generated.append(f'extern unsigned char data_ov084_{address:08x}[];')
    for address in sorted(big):
        generated.append(f'extern unsigned char data_ov074_{address:08x}[];')
    generated.append('void *data_ov074_0212292c[] = {' + ', '.join(f'data_ov084_{a:08x}' for a in small) + '};')
    generated.append('void *data_ov074_02122948[] = {' + ', '.join(f'data_ov074_{a:08x}' for a in big) + '};')
    generated.append('void *data_ov074_02122e5c[] = {' + ', '.join(arena_pointer(a) for a in transforms) + '};')
    generated.append('struct PcGoombossClip { void *animation, *bta; int flags; };')
    generated.append('PcGoombossClip data_ov074_02122f34[] = {')
    for animation, transform, flags in clips:
        assert transform in transforms and flags in (0, 0x40000000)
        generated.append(f'    {{data_ov074_{animation:08x}, {arena_pointer(transform)}, 0x{flags:x}}},')
    generated.append('};')
    generated.append(f'void *pc_goomboss_bma() {{ return {arena_pointer(materials[0])}; }}')
    generated.append(f'void *pc_goomboss_default_bta() {{ return {arena_pointer(0x021124ac)}; }}')
    return generated
