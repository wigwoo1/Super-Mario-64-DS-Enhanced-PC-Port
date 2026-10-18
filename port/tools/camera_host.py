"""Recover the camera's startup records and preserve their native layout."""
import re
import struct

MODE_START, MODE_END = 0x02086fcc, 0x0208751c


def bindings(root):
    initializer = (root / 'src/unnamed/arm9/0207/__sinit_02073a24.c').read_text()
    symbols = (root / 'config/arm9/symbols.txt').read_text()
    methods = {int(address, 16): name for name, address in re.findall(
        r'^(\S+) kind:function\([^\n]*?addr:0x([\da-fA-F]+)', symbols, re.M)}
    arm = (root / 'extracted/arm9_dec.bin').read_bytes()
    pairs = {}
    for record, slot, constant in re.findall(
            r'(data_[\da-f]+)\.p([01]) = (data_[\da-f]+);', initializer):
        target, adjustment = struct.unpack_from('<II', arm, int(constant[5:], 16) - 0x02004000)
        if adjustment != 0 or target not in methods:
            raise ValueError(f'Unsupported camera descriptor {constant}')
        pairs.setdefault(record, {})[int(slot)] = methods[target]
    if len(pairs) != 19 or any(set(slots) != {0, 1} for slots in pairs.values()):
        raise ValueError('Camera startup record roster changed')
    return pairs


def owned_names(root):
    symbols = (root / 'config/arm9/symbols.txt').read_text()
    modes = {name: int(address, 16) for name, address in re.findall(
        r'^(data_[\da-f]+) kind:data\([^\n]*?addr:0x([\da-fA-F]+)', symbols, re.M)
        if MODE_START <= int(address, 16) < MODE_END}
    return bindings(root), modes


def write_sources(root, directory):
    pairs, modes = owned_names(root)
    # Three recovered workers have void return types. Their original callers
    # ignore the result, but typed wrappers avoid calling them as int functions.
    void_workers = {'func_0200987c', 'func_020094c0', 'func_02005000'}
    cpp = ['#include <cstddef>', '#include "dCamera_c.h"', 'extern "C" {']
    for name in sorted(void_workers):
        cpp += [f'void {name}(void *);',
                f'int pc_camera_{name}(void *self) {{ {name}(self); return 1; }}']
    cpp += [f'extern unsigned char {name}[];' for name in pairs]
    cpp += ['int pc_camera_state_id(const void *object) {',
            '    if (!object) return -1;',
            '    const auto *camera = static_cast<const dCamera_c *>(object);']
    for name in pairs:
        cpp.append(f'    if ((const void *)camera->mState == {name}) return 0x{name[5:]}u;')
    cpp += ['    return -2;', '}', '}',
            'static_assert(sizeof(dCamera_c) == 0x1a8, "native camera layout");',
            'static_assert(offsetof(dCamera_c, mState) == 0x138, "native camera state");',
            'static_assert(offsetof(dCamera_c, mFlags) == 0x154, "native camera flags");']
    asm = ['; Generated from local camera startup descriptors and mode bytes.',
           '.386', '.model flat', '.data', 'ALIGN 4']
    targets = {name for slots in pairs.values() for name in slots.values()}
    for name in sorted(targets):
        target = 'pc_camera_' + name if name in void_workers else name
        asm.append(f'EXTERN _{target}:PROC')
    for record, slots in pairs.items():
        asm += [f'PUBLIC _{record}', f'_{record} LABEL BYTE']
        for slot in range(2):
            name = slots[slot]
            target = 'pc_camera_' + name if name in void_workers else name
            asm += [f'DWORD OFFSET _{target}', 'DWORD 0']
    # Several state initializers index this table by index*0x28, and the
    # workers compare mode addresses. Interior labels must share one allocation.
    arm = (root / 'extracted/arm9_dec.bin').read_bytes()
    asm.append('ALIGN 4')
    for address in range(MODE_START, MODE_END, 4):
        for name, label in modes.items():
            if label == address:
                asm += [f'PUBLIC _{name}', f'_{name} LABEL BYTE']
        value = struct.unpack_from('<I', arm, address - 0x02004000)[0]
        asm.append(f'DWORD 0{value:08x}h')
    asm.append('END')
    directory.mkdir(parents=True, exist_ok=True)
    outputs = []
    for name, lines in (('camera_native_data.cpp', cpp), ('camera_native_data.asm', asm)):
        output = directory / name
        text = '\n'.join(lines) + '\n'
        if not output.exists() or output.read_text() != text:
            output.write_text(text)
        outputs.append(output.as_posix())
    return outputs


def transform(text):
    old = 'typedef int (CamPmfHost::*CamPmf)();'
    new = ('struct CamPmf { int (__cdecl *function)(void *); int adjustment; };\n'
           'static_assert(sizeof(CamPmf) == 8, "camera callback stride");')
    if old not in text:
        raise ValueError('Camera dispatcher declaration changed')
    text = text.replace(old, new)
    for old, new in (
            ('return (c->**p)();', 'return p->function(c_);'),
            ('r5 = (ch->**pp)();', 'r5 = pp->function(self);')):
        if text.count(old) != 1:
            raise ValueError('Camera dispatcher body changed')
        text = text.replace(old, new)
    return text
