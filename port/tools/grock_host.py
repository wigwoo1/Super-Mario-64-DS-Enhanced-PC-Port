"""Connect the original Hazy Maze Cave rock to native resource storage."""
import struct


def transform(root, text):
    overlay = (root / 'extracted/overlays/overlay_0021.bin').read_bytes()
    assert struct.unpack_from('<IHHIiiii', overlay, 0x02114824 - 0x021111a0) == (
        0x02112d64, 221, 70, 2, 0, 1228800, 24969216, 24969216)

    declaration = '''    GrockModelFilePtr(unsigned int fileID) { pc_sharedfile_handle_construct(this, fileID); }
    ~GrockModelFilePtr() { pc_sharedfile_handle_destroy(this); }
'''
    assert text.count(declaration) == 1
    text = text.replace(declaration, '')
    definition = 'GrockModelFilePtr data_ov021_02114a50(1492);'
    assert text.count(definition) == 1
    text = text.replace(definition, 'extern "C" { GrockModelFilePtr data_ov021_02114a50 = {}; }')

    anchor = 'int daGrock_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, '''extern "C" void pc_sharedfile_handle_construct(void *, unsigned int);
static_assert(sizeof(GrockModelFilePtr) == 8, "native rock resource handle");
static_assert(offsetof(daGrock_c, pad_188) == 0x188, "native rock shadow matrix");
static_assert(offsetof(daGrock_c, pad_3b8) == 0x3b8, "native rock sound handle");
static_assert(offsetof(daGrock_c, pad_3c3) == 0x3c3, "native rock particle handle");
''' + anchor + '''
    static const bool ready = [] {
        pc_sharedfile_handle_construct(&data_ov021_02114a50, 1492);
        return true;
    }();
    (void)ready;''')
    return '#include <cstddef>\n' + text

