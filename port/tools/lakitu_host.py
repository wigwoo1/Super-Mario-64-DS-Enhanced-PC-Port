"""Adapt Lakitu's recovered state/resource data to the native compiler ABI."""
import re
import struct


def callback_bindings(root):
    source = (root / 'src/actors/daC_Jugem_c.cpp').read_text()
    metadata = root / 'config/arm9/overlays/ov085'
    bounds = (metadata / 'delinks.txt').read_text()
    base = min(int(value, 16) for value in re.findall(
        r'^\s*\.[^\s]+\s+start:0x([\da-fA-F]+)', bounds, re.M))
    rom = (root / 'extracted/overlays/overlay_0085.bin').read_bytes()
    methods = {}
    for name, method, address in re.findall(
            r'^(_ZN11daC_Jugem_c\d+(State\w+)Ev) kind:function\([^\n]*?addr:0x([\da-fA-F]+)',
            (metadata / 'symbols.txt').read_text(), re.M):
        methods[int(address, 16)] = method
    names = re.findall(r'extern daC_Jugem_c::StateFn (data_ov085_[\da-f]+);', source)
    bindings = {}
    for name in names:
        address = int(name[-8:], 16)
        target, adjustment = struct.unpack_from('<II', rom, address - base)
        if adjustment != 0 or target not in methods:
            raise ValueError(f'Lakitu callback {name}: unsupported target/adjustment')
        bindings[name] = (methods[target], target)
    pairs = re.findall(
        r'daC_Jugem_c::StateFn (data_ov085_[\da-f]+)\[2\] = '\
        r'\{(data_ov085_[\da-f]+), (data_ov085_[\da-f]+)\};', source)
    if len(bindings) != 22 or len(pairs) != 11 or {p for _, a, b in pairs for p in (a, b)} != set(bindings):
        raise ValueError('Lakitu state record roster changed')
    return bindings, pairs


def transform(root, text):
    bindings, pairs = callback_bindings(root)
    # The recovery's local wrapper names refer to the original eight-byte
    # file handles. DEMO and Lakitu share canonical storage in the HAL.
    for name in ('JugemModelFile', 'JugemAnimationFilePtr', 'JugemTexSequenceFilePtr'):
        text, count = re.subn(r'struct ' + name + r' : SharedFilePtr \{.*?\};',
                             f'using {name} = SharedFilePtr;', text, count=1, flags=re.S)
        if count != 1:
            raise ValueError(f'Lakitu resource wrapper not found: {name}')
        text, count = re.subn(r'^' + name + r' data_ov085_[\da-f]+\(0x[\da-f]+\);\s*$',
                             '', text, count=1, flags=re.M)
        if count != 1:
            raise ValueError(f'Lakitu duplicate resource definition not found: {name}')
    text = text.replace('~JugemVec3();', '~JugemVec3() {}')
    text = re.sub(r'^extern daC_Jugem_c::StateFn data_ov085_[\da-f]+;\s*$', '', text, flags=re.M)
    for record, init, main in pairs:
        pattern = (r'daC_Jugem_c::StateFn ' + record + r'\[2\] = \{[^}]+\};')
        replacement = (f'daC_Jugem_c::StateFn {record}[2] = '
                       f'{{&daC_Jugem_c::{bindings[init][0]}, &daC_Jugem_c::{bindings[main][0]}}};')
        text, count = re.subn(pattern, lambda _: replacement, text, count=1)
        if count != 1:
            raise ValueError(f'Lakitu state definition not found: {record}')
    text = '#include <cstddef>\n' + text
    text += '\nstatic_assert(sizeof(daC_Jugem_c) == 0x2e8, "native Lakitu layout");\n'
    text += 'static_assert(offsetof(daC_Jugem_c, mState) == 0x1ec, "native Lakitu state offset");\n'
    text += 'static_assert(offsetof(daC_Jugem_c, mTimer) == 0x2c8, "native Lakitu timer offset");\n'
    text += 'static_assert(sizeof(daC_Jugem_c::StateFn) == 4, "native Lakitu callback width");\n'
    records = ', '.join(name for name, _, _ in pairs)
    ids = ', '.join(f'0x{name[-8:]}u' for name, _, _ in pairs)
    text += f'''
extern "C" int pc_lakitu_state_id(const void *object) {{
    if (!object) return -1;
    auto *actor = static_cast<const daC_Jugem_c *>(object);
    static daC_Jugem_c::StateFn *const records[] = {{{records}}};
    static const unsigned ids[] = {{{ids}}};
    for (unsigned i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i)
        if (actor->mState == records[i]) return static_cast<int>(ids[i]);
    return -2;
}}
extern "C" int pc_lakitu_timer(const void *object) {{
    return object ? static_cast<const daC_Jugem_c *>(object)->mTimer : 0;
}}
'''
    return text

