"""Host bindings for the original Bullies and their shared behavior."""
import struct

from battlefield_platform_host import lazy_handles


def transform_small(root, text):
    rom = (root / 'extracted/overlays/overlay_0064.bin').read_bytes()
    base = 0x02115ee0
    profile = struct.unpack_from('<IHHIiiii', rom, 0x0211b818 - base)
    assert profile == (0x02117444, 215, 59, 0x10000006, 0x64000, 0x64000, 0x1000000, 0x1000000)
    addresses = (0x0211c668, 0x0211c670, 0x0211c660, 0x0211c678, 0x0211c680)
    row = struct.unpack_from('<5I7iI', rom, 0x0211b834 - base)
    assert row[:5] == addresses
    text = lazy_handles(text, ('DonketuModelFilePtr', 'DonketuAnimationFileHandle'))
    for address in addresses:
        handle = f'data_ov064_{address:08x}'
        prefix = 'DonketuModelFilePtr' if address == addresses[0] else 'DonketuAnimationFileHandle'
        text = text.replace(f'{prefix} {handle}(', f'extern "C" {prefix} {handle}(', 1)
    anchor = 'int daDonketu_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, 'extern "C" int pc_small_bully_resources();\n' + anchor, 1)
    text = text.replace('mFileTable = (int)&data_ov064_0211b834;\n    InitResourcesCommon();',
                        'mFileTable = pc_small_bully_resources();\n    return InitResourcesCommon();', 1)
    counter = 'bigBully->mNumBulliesKilled++;'
    assert text.count(counter) == 2
    text = text.replace(counter, counter + r'''
                if (std::getenv("SM64DS_PC_BULLY_TRACE"))
                    std::fprintf(stderr, "[native Big Bully prerequisite] tick=%llu boss=%u child=%u count=%u\n",
                                 static_cast<unsigned long long>(ntr::rt_frame()), unsigned(bigBully->uniqueID),
                                 unsigned(uniqueID), unsigned(bigBully->mNumBulliesKilled));''')
    assert text.count('if (coin) {') == 2
    text = text.replace('if (coin) {', r'''if (coin) {
                if (std::getenv("SM64DS_PC_BULLY_TRACE"))
                    std::fprintf(stderr, "[native Bully coin] tick=%llu bully=%u coin=%u actor=%u param=%u pos=%d,%d,%d\n",
                                 static_cast<unsigned long long>(ntr::rt_frame()), unsigned(uniqueID),
                                 unsigned(coin->uniqueID), unsigned(coin->actorID), unsigned(coin->param1),
                                 coin->mPosX, coin->mPosY, coin->mPosZ);''')
    text += '''
extern "C" void pc_construct_small_bully_handles() {
    static bool ready = false;
    if (!ready) {
''' + ''.join(f'        data_ov064_{address:08x}.Construct({0x2df+i});\n'
              for i, address in enumerate(addresses)) + '''        ready = true;
    }
}
'''
    return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n' + text


def transform_big(root, text):
    rom = (root / 'extracted/overlays/overlay_0064.bin').read_bytes()
    base = 0x02115ee0
    profile = struct.unpack_from('<IHHIiiii', rom, 0x0211b920 - base)
    assert profile == (0x0211791c, 216, 60, 0x10000006, 0xc8000, 0xc8000, 0x1000000, 0x1000000)
    resources = ((0x0211c6cc, 0x2de), (0x0211c6d4, 0x2e0), (0x0211c6c4, 0x2e1),
                 (0x0211c6dc, 0x2e2), (0x0211c6e4, 0x2e3))
    assert struct.unpack_from('<5I', rom, 0x0211b93c - base) == tuple(a for a, _ in resources)
    text = lazy_handles(text, ('DonketuModelFilePtr', 'DonketuAnimFileHandle'))
    for index, (address, _) in enumerate(resources):
        kind = 'DonketuModelFilePtr' if index == 0 else 'DonketuAnimFileHandle'
        definition = f'{kind} data_ov064_{address:08x}('
        assert text.count(definition) == 1
        text = text.replace(definition, 'extern "C" ' + definition, 1)
    anchor = 'int daBDonketu_c::InitResources()'
    assert text.count(anchor) == 1
    text = text.replace(anchor, 'extern "C" int pc_big_bully_resources();\n' + anchor, 1)
    text = text.replace('mFileTable = (s32)&data_ov064_0211b93c;',
                        'mFileTable = pc_big_bully_resources();', 1)
    text = text.replace('result = InitResourcesCommon();',
                        'result = InitResourcesCommon();\n    if (!result) return 0;', 1)
    text = text.replace('Vector3 spawnPos;', 'Vector3 spawnPos = {0, mPosY, 0};', 1)
    constructor = '\n'.join(f'        data_ov064_{a:08x}.Construct({file_id});'
                            for a, file_id in resources)
    return text + (
        '\nextern "C" void pc_construct_big_bully_handles() {\n'
        '    static bool ready = false;\n'
        '    if (!ready) {\n' + constructor + '\n'
        '        ready = true;\n    }\n}\n'
    )


def transform_base(root, text):
    rom = (root / 'extracted/overlays/overlay_0064.bin').read_bytes()
    resource = '\n'.join(resource_config(rom, kind, address)
                         for kind, address in (('small', 0x0211b834), ('big', 0x0211b93c)))
    anchor = '#define CONFIG(obj)'
    text = text.replace(anchor, resource + '\n' + anchor, 1)
    text = text.replace(
        'IsPlayerInRangeE5Fix12IiES1_S1_i(void*,int,int,int,int);',
        'IsPlayerInRangeE5Fix12IiES1_S1_i(dActor_c*,Fix12i,Fix12i,Fix12i,int);')
    text = text.replace('int daOts_c::UpdateRunState()\n{\n}', 'int daOts_c::UpdateRunState()\n{\n    return 0;\n}')
    anchor = '    func_ov064_02116754();'
    assert text.count(anchor) == 1
    text = text.replace(anchor, '''
    const unsigned contact = mdCcAc_c.otherOwner;
    const unsigned hit = mdCcAc_c.hitFlags;
''' + anchor + r'''
    if (const char *path = std::getenv("SM64DS_PC_BULLY_TRACE")) {
        static FILE *file = std::fopen(path, "w");
        if (file && (contact || ntr::rt_frame() % 5 == 0)) {
            std::fprintf(file, "%llu,%u,%u,%d,%d,%d,%d,%d,%d,%u,%u\n",
                static_cast<unsigned long long>(ntr::rt_frame()), unsigned(uniqueID),
                unsigned(actorID), mPosX, mPosY, mPosZ, four, tail().mState,
                mHorzSpeed, contact, hit);
            std::fflush(file);
        }
    }
''', 1)
    return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n' + text


def resource_config(rom, kind, address):
    table = struct.unpack_from('<5I7iI', rom, address - 0x02115ee0)
    handles = ', '.join(f'&data_ov064_{pointer:08x}' for pointer in table[:5])
    values = ', '.join(str(value) for value in table[5:])
    declarations = '\n'.join(f'extern "C" SharedFilePtr data_ov064_{pointer:08x};'
                             for pointer in table[:5])
    return declarations + f"""
extern "C" BullyResourceConfig pc_{kind}_bully_config = {{{{{handles}}}, {values}}};
extern "C" void pc_construct_{kind}_bully_handles();
extern "C" int pc_{kind}_bully_resources() {{
    pc_construct_{kind}_bully_handles();
    return reinterpret_cast<int>(&pc_{kind}_bully_config);
}}
"""
