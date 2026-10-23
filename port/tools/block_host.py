"""Adapt the original big-brick block's file handles for native startup."""
import re


def transform(text):
    for kind in ("BlockModelFile", "daObjBlockLClsnFile"):
        pattern = rf'{kind}\(unsigned int (\w+)\) \{{ pc_sharedfile_handle_construct\(this, \1\); \}}'
        text, count = re.subn(
            pattern,
            lambda match: f'{kind}(unsigned int {match[1]}) {{ words[0]={match[1]};words[1]=0; }}',
            text)
        assert count == 1, kind + " constructor changed"
        old = f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old) == 1, kind + " destructor changed"
        text = text.replace(old, f'~{kind}() {{}}', 1)

    # The original tables are three parallel 0xc-stride columns inside the
    # ov002 image. Host data generation emits each named column as a separate
    # object, so indexing the raw guest-address column as a C++ row array
    # crosses unrelated host allocations. Build native rows with real host
    # handles and collision-material pointers instead.
    text = text.replace(
        'extern BigBrickBlockFileRow data_ov002_02108ab0[];\n'
        'extern BigBrickBlockFileRow data_ov002_02108ab4[];\n',
        'static BigBrickBlockFileRow pc_block_model_rows[3];\n'
        'static BigBrickBlockFileRow pc_block_collision_rows[3];\n')
    text = text.replace(
        'extern ClpsRow data_ov002_02108ab8[];         /* CLPS blocks,       0xc stride */',
        'static ClpsRow pc_block_clps_rows[3];')
    assert 'data_ov002_02108ab0[]' not in text
    assert 'data_ov002_02108ab4[]' not in text
    assert 'data_ov002_02108ab8[]' not in text
    text = text.replace('data_ov002_02108ab0[mVariant]', 'pc_block_model_rows[mVariant]')
    text = text.replace('data_ov002_02108ab4[idx]', 'pc_block_collision_rows[idx]')
    text = text.replace('data_ov002_02108ab8[idx]', 'pc_block_clps_rows[idx]')
    text = text.replace('data_ov002_02108ab4[mVariant]', 'pc_block_collision_rows[mVariant]')

    # Collision material blocks live at two ov002 addresses and one address
    # in the adjacent object overlay. The generators preserve only these
    # local ROM bytes; they are never shipped in source or build kits.
    text = text.replace(
        'extern "C" BlockModelFile data_ov002_0210dcc0(1108);',
        'extern "C" unsigned char data_ov002_0210d794[];\n'
        'extern "C" unsigned char data_ov002_0210d8f4[];\n'
        'extern "C" unsigned char data_ov015_021135f4[];\n'
        'extern "C" BlockModelFile data_ov002_0210dcc0(1108);',
        1)

    # The recovered int-returning attack callbacks intentionally leave r0
    # untouched, matching their original instructions. Their callers discard
    # the result; provide a deterministic value in the host-only copy so MSVC
    # accepts the original valueless return paths.
    for method in ("OnAttacked1", "OnAttacked2"):
        signature = f"int daObjBlockL_c::{method}(dActor_c &other)"
        start = text.index(signature)
        end = text.find("// @symbol", start + len(signature))
        if end < 0:
            end = len(text)
        chunk = text[start:end]
        close = chunk.rfind("}")
        assert close >= 0 and "return;" in chunk[:close], method
        body = chunk[:close].replace("return;", "return 0;")
        text = text[:start] + body + "    return 0;\n" + chunk[close:] + text[end:]

    # SharedFilePtr handles need the runtime file table, which is unavailable
    # during process startup. Initialize them on the first actual block actor
    # resource load instead of from a global/profile-table initializer.
    signature = "int daObjBlockL_c::InitResources()\n{"
    assert text.count(signature) == 1, "InitResources signature changed"
    text = text.replace(
        signature,
        'extern "C" void pc_block_init_resources();\n\n'
        + signature + "\n    pc_block_init_resources();",
        1)

    text += '''
// The cartridge initializes these six global handles in overlay static init.
// Native startup waits until the file table is ready before registering them.
extern "C" void pc_block_init_resources() {
    static bool ready=false;if(ready)return;
    data_ov002_0210dcc0.Construct(1108);
    data_ov002_0210dcb8.Construct(1109);
    data_ov002_0210dce0.Construct(1110);
    data_ov002_0210dcc8.Construct(1111);
    data_ov002_0210dcd0.Construct(1425);
    data_ov002_0210dcd8.Construct(1426);
    pc_block_model_rows[0].file=&data_ov002_0210dcc0;
    pc_block_model_rows[1].file=&data_ov002_0210dce0;
    pc_block_model_rows[2].file=&data_ov002_0210dcd0;
    pc_block_collision_rows[0].file=&data_ov002_0210dcb8;
    pc_block_collision_rows[1].file=&data_ov002_0210dcc8;
    pc_block_collision_rows[2].file=&data_ov002_0210dcd8;
    pc_block_clps_rows[0].block=(CLPS_Block *)data_ov002_0210d794;
    pc_block_clps_rows[1].block=(CLPS_Block *)data_ov002_0210d8f4;
    pc_block_clps_rows[2].block=(CLPS_Block *)data_ov015_021135f4;
    ready=true;
}
'''
    return text
