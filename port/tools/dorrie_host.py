"""Bind Dorrie's recovered movement and collision to the Win32 port."""
import re
import struct

ANIMATIONS = (0x0211d768, 0x0211d748, 0x0211d770)
COLLISIONS = (0x0211d740, 0x0211d758, 0x0211d730, 0x0211d738,
              0x0211d750, 0x0211d728, 0x0211d760)
SURFACES = (0x0211cf64, 0x0211cf44, 0x0211cf24, 0x0211cf04,
            0x0211cee4, 0x0211cec4, 0x0211cf84)


def transform(root, text):
    overlay = (root / 'extracted/overlays/overlay_0065.bin').read_bytes()
    for address, values in ((0x0211c080, ANIMATIONS), (0x0211c08c, COLLISIONS),
                            (0x0211cd68, SURFACES)):
        assert struct.unpack_from(f'<{len(values)}I', overlay, address - 0x02115ee0) == values

    start = text.index('struct DossyModelFilePtr')
    end = text.index('extern "C" {', start)
    handles = re.findall(r'^Dossy\w+ (data_ov065_\w+)\((0x[0-9a-f]+)\);$', text, re.M)
    assert len(handles) == 11
    storage = '''struct DossyFileHandle : SharedFilePtr {
    unsigned words[2];
};
static_assert(sizeof(DossyFileHandle) == 8, "native Dorrie resource handle");
static_assert(offsetof(daDossy_c, mModelAnim) == 0xec, "native Dorrie model");
static_assert(offsetof(daDossy_c, mPlatforms) == 0x150, "native Dorrie platforms");
static_assert(sizeof(DorriePlatform) == 0x200, "native Dorrie platform stride");
'''
    storage += ''.join(f'static DossyFileHandle {name};\n' for name, _ in handles)
    storage += 'extern "C" {\n'
    storage += ''.join(f'extern unsigned char data_ov065_{a:08x}[];\n' for a in SURFACES)
    storage += '}\n'
    for name, items, kind in (('0211c080', ANIMATIONS, 'SharedFilePtr *'),
                              ('0211c08c', COLLISIONS, 'SharedFilePtr *'),
                              ('0211cd68', SURFACES, 'void *')):
        prefix = '&' if kind == 'SharedFilePtr *' else ''
        entries = ', '.join(f'{prefix}data_ov065_{address:08x}' for address in items)
        storage += f'static {kind} const data_ov065_{name}[] = {{{entries}}};\n'
    text = text[:start] + storage + text[end:]
    text = re.sub(r'^extern (?:SharedFilePtr \*|void \*)data_ov065_(?:0211c080|0211c08c|0211cd68)\[\];\n',
                  '', text, flags=re.M)
    text = re.sub(r'^Dossy\w+ data_ov065_\w+\(0x[0-9a-f]+\);$', '', text, flags=re.M)
    text = re.sub(r'(data_ov065_\w+)\.ptr', r'LOADED_FILE(\1)', text)
    text = re.sub(r'return \(\(int \(\*\)\(void \*, void \*\)\)_ZN9daDossy_c19(func_ov065_\w+)EP8dActor_c\)\(b, c\);',
                  r'static_cast<daDossy_c *>(b)->\1(static_cast<dActor_c *>(c));\n    return 0;', text)

    anchor = 'int daDossy_c::InitResources()\n{'
    assert text.count(anchor) == 1
    init = ''.join(f'        {name}.Construct({file_id});\n' for name, file_id in handles)
    text = text.replace(anchor, anchor + '\n    static const bool ready = [] {\n' +
                        init + '        return true;\n    }();\n    (void)ready;')
    text = text.replace('*(int*)(((int)((char*)this) + 0x5c)) += 0x7d0000;', 'mPosX += 0x7d0000;')
    return '#include <cstddef>\n' + text
