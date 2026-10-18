#!/usr/bin/env python3
"""Generate host storage for unresolved ROM data symbols in the boot closure.

File-backed ARM9/overlay bytes are copied from this checkout's extracted ROM;
BSS starts zeroed. The generated C++ and manifest stay in the build tree.
"""

import json
import pathlib
import re
import struct
import sys


SYMBOL = re.compile(r"^(\S+) kind:(?:data\([^)]*\)|bss) addr:0x([0-9a-fA-F]+)")
ERROR = re.compile(r"unresolved external symbol (.+?)(?: referenced in function|$)")
ARM_BASE = 0x02004000
ARM_BSS = 0x02094640
FILE_BACKED_AFTER_BSS = ((0x020994D4, 0x02099CE4),
                         (0x02099E94, 0x02099E9C),
                         (0x02099EBC, 0x02099EC4),
                         (0x02099EC4, 0x02099ECC))

# These globals already have canonical C-linkage storage in the HAL. The
# production linker map showed C++ references landing in separate generated
# ROM/BSS copies. Bind every recovered spelling to the live HAL object.
HAL_OWNED_DATA = {
    "data_ov084_02130da4", "data_ov084_02130d9c", "data_ov084_02130dc4",
    "data_ov002_0210e12c", "data_ov002_0210e124",
    "data_0209b3ec",  # shared camera/view matrix, also read by native paintings
    "data_02082214",  # sine table already has canonical romdata.c storage
    "gTouchHeld", "gTouchEdge", "gTouchX", "gTouchY",
    "data_0209f61c",  # constructed native scene-wipe singleton
    "data_0209e674", "data_0209e67c", "data_0209e694", "data_0209ea74",
    "data_020a0e58", "data_020a0e5a", "data_020a0e5c", "data_020a0e5e",
    "data_0209f3c4",  # complete native minimap graphics callback/state
    "data_ov085_0213074c", "data_ov085_02130744", "data_ov085_0213073c",
    # DEMO handles and callback arrays have canonical storage in ov002_data.cpp.
    "data_ov002_02110b10",
    "data_ov002_02110b18",
    "data_ov002_02110b20",
    "data_ov002_02110b28",
    "data_ov002_02110b30",
    "data_ov002_02110b38",
    "data_ov002_02110b40",
    "data_ov002_02110b48",
    "data_ov002_02110b58",
    "data_ov002_02110b60",
    "data_ov002_02110b68",
    "data_ov002_02110b80",
    "data_ov002_02110b88",
    "data_ov002_02110b90",
    "data_ov002_02110ba0",
    "data_ov002_02110ba8",
    "data_ov002_02110bb0",
    "data_ov002_02110bb8",
    "data_ov002_02110bc0",
    "data_ov002_02110bd0",
    "data_ov002_02110bd8",
    "data_ov002_02110be0",
    "data_ov002_02110be8",
    "data_ov002_02110bf0",
    "data_ov002_02110bf8",
    "data_ov002_02110c00",
    "data_ov002_02110c08",
    "data_ov002_02110c10",
    "data_ov002_02110e4c",
    "data_ov002_02110ebc",
    "data_ov002_02110f34",
    "data_020758c8", "data_0208e378", "data_0209b45c", "data_0209b460",
    "data_0209b468", "data_0209caa0", "data_0209e650", "data_0209ee74",
    "data_0209ee90", "data_0209f20c", "data_0209f21c", "data_0209f220",
    "data_0209f294", "data_0209f2c4", "data_0209f2d8", "data_0209f2f8", "data_0209f318",
    "data_0209f394", "data_0209f40c", "data_0209f43c", "data_0209f5c0",
    "data_0209fc68", "data_020a0e5a", "data_020a0e68", "data_020a0ea0",
    "data_020a4bb8",
    "data_ov084_02130cc8", "data_ov084_02130cf8",
    "data_ov002_0210d9e8", "data_ov002_0210da08", "data_ov002_02110094",
    "data_ov002_0211013c", "data_ov002_02110244", "data_ov002_02110274",
    "data_ov002_0211028c", "data_ov002_02110364", "data_ov002_02110394",
    "data_ov002_02110574", "data_ov002_021105bc",
    "data_ov002_0211007c", "data_ov002_021103f4", "data_ov002_0211040c",
}

# These objects have real C++ definitions and resource constructors. Generated
# C spellings must reference those objects rather than create empty handles.
NATIVE_CPP_DATA = {
    "data_ov002_02110944": "?data_ov002_02110944@@3UStarAnimationFileHandle@@A",
    "data_ov002_02110aa4": "?data_ov002_02110aa4@@3UOneUpLogoModelFile@@A",
    "data_ov006_02140330": "?data_ov006_02140330@@3UHeyhoAdapterModelFilePtr@@A",
    "data_ov006_02140338": "?data_ov006_02140338@@3UHeyhoAdapterAnimationFileHandle@@A",
}
for address in (0x021305d8,0x021305e0):
    name=f'data_ov085_{address:08x}'
    NATIVE_CPP_DATA[name]=f'?{name}@@3UDaMipModelFile@@A'
for address in (0x021305b8,0x021305d0,0x021305b0,0x021305c8,0x021305c0):
    name=f'data_ov085_{address:08x}'
    NATIVE_CPP_DATA[name]=f'?{name}@@3UDaMipAnimationFilePtr@@A'
for address in range(0x0213066c,0x021306ec,16):
    name=f'data_ov085_{address:08x}'
    NATIVE_CPP_DATA[name]=f'?{name}@@3UMipStatePair@@A'
# daKrb_c's resource declarations have C linkage and own the live handles.
HAL_OWNED_DATA.update(f'data_ov084_{address:08x}' for address in
                      range(0x02130cc0, 0x02130d08, 8))
# Canonical native reward resources/callbacks live in the recovered C++ TUs.
HAL_OWNED_DATA.update(f'data_ov002_{address:08x}' for address in (
    *range(0x02110924, 0x0211096c, 8), 0x021109d8,
    *range(0x0210de00, 0x0210de68, 8),
    *range(0x0210951c, 0x021095ac, 8),
    *range(0x0210df04, 0x0210df94, 16)))

# Canonical boss handles and native callback records belong to its host TU.
HAL_OWNED_DATA.update(f'data_ov078_{address:08x}' for address in (
    *range(0x02126ee0,0x02126f48,8), *range(0x02126ffc,0x0212711c,16),
    0x0212711c))

# These halfword labels are interior views of one GX bank-state structure.
# MASM supplies real COFF labels at their original offsets in one allocation;
# independent C arrays would insert padding and separate aggregate/field uses.
HAL_OWNED_DATA.update(f'data_{address:08x}' for address in range(0x020a6088, 0x020a60a2, 2))

# Four processed-controller records share the retail stride of 24 bytes.
# Separate minimum-four-byte BSS arrays made mag overlap the pressed-key label.
HAL_OWNED_DATA.update(f'data_{address:08x}' for address in (
    0x0209f498,0x0209f49c,0x0209f49e,0x0209f4a0,0x0209f4a2,0x0209f4a4,
    0x0209f4a6,0x0209f4a8,0x0209f4a9,0x0209f4ab,0x0209f4ac,0x0209f4ae))


def message_text_callbacks(root: pathlib.Path):
    """The three original cdecl/no-argument message formatters."""
    arm = (root / 'extracted/arm9_dec.bin').read_bytes()
    targets = struct.unpack_from('<3I', arm, 0x0208ee80 - ARM_BASE)
    if targets != (0x0201aca4, 0x0201ab6c, 0x0201adac):
        raise ValueError('Message text callback manifest changed')
    return tuple(f'func_{target:08x}' for target in targets)


def module_records(path: pathlib.Path, module: str, segment_start: int, segment_end: int):
    rows = []
    if not path.is_file():
        return rows
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        match = SYMBOL.match(line)
        if match:
            name, address = match.groups()
            rows.append({"module": module, "name": name, "address": int(address, 16),
                         "segment_start": segment_start, "segment_end": segment_end})
    return rows


def records(root: pathlib.Path):
    result = module_records(root / "config/arm9/symbols.txt", "arm9",
                            ARM_BASE, 0x020A0000)
    for directory in sorted((root / "config/arm9/overlays").glob("ov[0-9][0-9][0-9]")):
        module = directory.name
        delinks = directory / "delinks.txt"
        text = delinks.read_text(encoding="utf-8", errors="replace") if delinks.is_file() else ""
        text_bounds = re.search(r"\.text\s+start:0x([0-9a-fA-F]+)", text)
        bss_bounds = re.findall(r"\.bss\s+start:0x([0-9a-fA-F]+)\s+end:0x([0-9a-fA-F]+)", text)
        start = int(text_bounds.group(1), 16) if text_bounds else 0x020AD660
        end = int(bss_bounds[-1][1], 16) if bss_bounds else start
        result += module_records(directory / "symbols.txt", module, start, end)
    dtcm_end = 0x023C0020
    result += module_records(root / "config/arm9/dtcm/symbols.txt", "dtcm",
                            0x023C0000, dtcm_end)
    for module in {row["module"] for row in result}:
        group = sorted((row for row in result if row["module"] == module),
                       key=lambda row: row["address"])
        for index, row in enumerate(group):
            following = next((item["address"] for item in group[index + 1:]
                              if item["address"] > row["address"]), row["segment_end"])
            row["size"] = max(4, min(following - row["address"], 0x100000))
            if module == "ov084" and row["address"] == 0x02130208:
                # Four Goomba-type cylinder heights. Ambiguous interior labels
                # at +3/+10 are not array boundaries; callers index all 4 ints.
                row["size"] = 4 * 4
            if module == "arm9" and row["address"] == 0x02075f14:
                # The embedded SPA archive contains internal symbol labels.
                # Its complete resource span comes from its own header, not
                # the next label. The parser walks definitions and textures.
                arm = (root / "extracted/arm9_dec.bin").read_bytes()
                offset = row["address"] - ARM_BASE
                if arm[offset:offset + 8] != bytes.fromhex("2041505331305f31"):
                    raise ValueError("unexpected embedded particle archive header")
                particles, textures = struct.unpack_from("<II", arm, offset + 0x10)
                row["size"] = 0x20 + particles + textures
                if offset + row["size"] > len(arm):
                    raise ValueError("embedded particle archive exceeds extracted ARM9")
    return result


def parse_external(line: str):
    match = ERROR.search(line)
    if not match:
        return None
    raw = match.group(1).strip()
    decorated = re.search(r"\((\?[^()]*)\)$", raw)
    if decorated:
        linker_name = decorated.group(1)
        display = raw[:decorated.start()].strip().strip('"')
    else:
        linker_name = raw
        display = raw
    # COFF adds one underscore to the decomp's Itanium global-data spelling.
    # Keep the remaining underscore: overlay symbol manifests store `_ZN...`,
    # so lstrip("_") would make every such ROM-backed asset miss its record.
    if linker_name.startswith("__ZN") or linker_name.startswith("__Z"):
        return (linker_name, linker_name[1:])
    # Prefer the actual ROM global embedded in the human-readable type. For
    # MSVC member-pointer globals, words such as `__thiscall` appear earlier
    # in the display string than `data_ovNNN_address`; the generic underscore
    # fallback must not capture that calling-convention token instead.
    name = re.search(r"data_(?:ov\d+_)?[0-9a-fA-F]{8}", display)
    if not name:
        name = re.search(r"_[A-Za-z][A-Za-z0-9_]*", display)
    if not name:
        name = re.search(r"data_(?:ov\d+_)?[0-9a-fA-F]{8}", linker_name)
    if not name:
        name = re.search(r"[A-Za-z][A-Za-z0-9_]*", linker_name)
    return (linker_name, name.group(0).lstrip("_") if name else None)


def source_bytes(root: pathlib.Path, row: dict):
    address, size, module = row["address"], row["size"], row["module"]
    if module == "arm9":
        image = root / "extracted/arm9_dec.bin"
        if (ARM_BASE <= address and address + size <= ARM_BSS and image.is_file()):
            data = image.read_bytes()
            offset = address - ARM_BASE
            if offset + size <= len(data):
                return data[offset:offset + size]
        if any(start <= address and address + size <= end
               for start, end in FILE_BACKED_AFTER_BSS):
            image = root / "extracted/arm9_dec.bin"
            data = image.read_bytes()
            offset = address - ARM_BASE
            if 0 <= offset and offset + size <= len(data):
                return data[offset:offset + size]
    elif module.startswith("ov"):
        index = int(module[2:])
        image = root / f"extracted/overlays/overlay_{index:04d}.bin"
        base = row["segment_start"]
        if image.is_file() and address >= base:
            data = image.read_bytes()
            offset = address - base
            if offset + size <= len(data):
                return data[offset:offset + size]
    return bytes(size)


def main() -> int:
    if len(sys.argv) != 4:
        sys.exit("usage: link_data.py DECOMP_ROOT LINK_LOG OUTPUT_CPP")
    root, log_path, output = map(pathlib.Path, sys.argv[1:])
    root = root.resolve()
    output = output.resolve()
    manifest_path = output.with_suffix(".json")
    available = records(root)
    from dsmt_host import owned_names
    HAL_OWNED_DATA.update(n for n,_ in owned_names(root))
    from minigame_menu_data import owned_names as minigame_names
    HAL_OWNED_DATA.update(n for n,_ in minigame_names(root))
    from camera_host import owned_names
    camera_states, camera_modes = owned_names(root)
    HAL_OWNED_DATA.update(camera_states)
    HAL_OWNED_DATA.update(camera_modes)
    from minigame_host import owned_names as callback_names
    HAL_OWNED_DATA.update(callback_names(root))
    from minigame_data import owned_names as resource_names
    HAL_OWNED_DATA.update(resource_names(root))
    by_name = {}
    for row in available:
        by_name.setdefault(row["name"], []).append(row)

    # The indirect message workers are seeded explicitly, so include their
    # font/control data even when the historical link log predates them.
    external_names = {'_' + name: name for name in (
        'data_0208ee64', 'data_0208ee6c', 'data_0209d688', 'data_0209d6e4')}
    external_names.update({'_'+name:name for name in NATIVE_CPP_DATA})
    # Native cap factories now make these indirect resource tables reachable.
    external_names.update({'_' + name: name for name in (
        'data_ov002_020ff0a0', 'data_ov002_020ff0ac',
        'data_ov002_020ff0b8', 'data_ov002_020ff0c4')})
    if manifest_path.is_file():
        try:
            external_names.update(json.loads(manifest_path.read_text(encoding="utf-8")))
        except (OSError, json.JSONDecodeError):
            pass
    if log_path.is_file():
        for line in log_path.read_text(encoding="utf-8", errors="replace").splitlines():
            parsed = parse_external(line)
            if parsed:
                linker_name, data_name = parsed
                if data_name in by_name and len(by_name[data_name]) == 1:
                    external_names[linker_name] = data_name

        # Some recovered actor TUs retain the original flat-int declaration
        # for the 12-word identity matrix, while typed model code declares the
        # exact same ROM storage as Matrix4x3. Bridge that known layout variant
        # to the ROM bytes rather than emitting a duplicate host object.
        for line in log_path.read_text(encoding="utf-8", errors="replace").splitlines():
            if "?IDENTITY_MATRIX4X3@@3PAHA" in line:
                rows = by_name.get("IDENTITY_MATRIX4X3", [])
                if len(rows) == 1:
                    external_names["?IDENTITY_MATRIX4X3@@3PAHA"] = "IDENTITY_MATRIX4X3"

    selected = {}
    missing = []
    host_aliases = []
    cpp_aliases = []
    for linker_name, name in sorted(external_names.items()):
        if name in NATIVE_CPP_DATA:
            target = NATIVE_CPP_DATA[name]
            if linker_name != target:
                cpp_aliases.append((linker_name, target))
            continue
        if name in HAL_OWNED_DATA:
            if linker_name != "_" + name:
                host_aliases.append((linker_name, name))
            continue
        # The title actor consumes this overlay table as native pointers. Its
        # specialized generated definition rebases all 54 text pointers from
        # the local overlay image; raw copied ROM addresses are not usable in
        # the host process.
        if name in {
            "data_ov003_020b117c",
            "data_ov003_020b1180",
            "data_ov003_020b1181",
            # Per-player active-high key input table is live state, not ROM data.
            "data_020a1162",
            # Player pointers are owned by cxxname_bridge.cpp and filled by
            # the entrance loader. Every language spelling must share that
            # live table; a separate ROM/BSS copy leaves the camera empty.
            "data_0209f394",
        }:
            continue
        candidates = by_name.get(name, [])
        if len(candidates) != 1:
            missing.append({"symbol": name, "linker_name": linker_name})
            continue
        row = candidates[0]
        key = (row["module"], row["address"])
        if key not in selected or row["size"] > selected[key]["row"]["size"]:
            selected[key] = {"row": row, "linker_names": []}
        selected[key]["linker_names"].append(linker_name)

    # Preserve each pointed-to record's ROM bytes, but bind tables to native
    # storage. Minimap player sprites belong to ov001: ov000 reuses their DS
    # addresses for unrelated data, so the owning overlay must be explicit.
    pointer_tables = {}
    for sequence_key, count, target_module in (
            # Power Star camera scripts. func_0200ee8c indexes these six
            # pointers; raw DS addresses fault in ProcessKuppaScript on x86.
            (('arm9', 0x020876e4), 6, 'arm9'),
            (('ov084', 0x02130278), 7, 'ov084'),
            (('ov002', 0x020ff028), 6, 'ov002'),
            (('ov002', 0x020ff0a0), 3, 'ov002'),
            (('ov002', 0x020ff0ac), 3, 'ov002'),  # cap model handles
            (('ov002', 0x020ff0b8), 3, 'ov002'),  # cap animation handles
            (('ov002', 0x020ff0c4), 3, 'ov002'),
            (('ov002', 0x021000a0), 2, 'ov002'),  # Yoshi egg model handles
            (('ov002', 0x020ff060), 3, 'ov002'),
            (('ov002', 0x020ff06c), 3, 'ov002'),
            # Yoshi tongue/retract animation handles: ROM addresses cannot
            # be dereferenced by the native animation controller.
            (('ov002', 0x0210a084), 2, 'ov002'),
            (('ov002', 0x0210a7a0), 18, 'ov002'),
            (('ov002', 0x0210be1c), 4, 'ov002'),
            (('ov002', 0x0210b998), 4, 'ov002'),
            (('ov002', 0x0210c3e0), 2, 'ov002'),
            (('arm9', 0x020756f0), 12, 'ov002'),
            (('arm9', 0x0209214c), 3, 'arm9'),
            (('ov002', 0x0210c174), 17, 'ov001'),
            (('ov002', 0x0210c5b8), 8, 'ov002'),
            (('ov002', 0x0210c894), 16, 'ov002'),
            (('ov002', 0x0210c9b4), 16, 'ov002'),
            (('ov002', 0x0210c748), 3, 'ov002'),
            (('ov002', 0x0210cac8), 3, 'ov002'),
            (('ov002', 0x0210cae4), 2, 'ov002')):
        if sequence_key not in selected:
            continue
        sequence_targets = []
        table = source_bytes(root, selected[sequence_key]['row'])
        if sequence_key == ('ov002', 0x0210c174):
            # The following vtable's two-word ABI prefix precedes its symbol
            # at 0210c1c0. It is outside the 17-entry player-icon table.
            if table[count * 4:] != struct.pack('<II', 0, 0x0210c168):
                raise ValueError('Minimap player-icon table/vtable boundary changed')
            table = table[:count * 4]
        if len(table) != count * 4:
            raise ValueError(f'Native pointer table {sequence_key} extent changed')
        for address in struct.unpack(f'<{count}I', table):
            if address == 0:
                sequence_targets.append(None)
                continue
            matches = [row for row in available
                       if row['module'] == target_module and row['address'] == address]
            if len(matches) != 1:
                raise ValueError(f'Native pointer table target {address:08x} is ambiguous')
            row = matches[0]
            key = (target_module, address)
            selected.setdefault(key, {'row': row, 'linker_names': []})
            # A strong definition may already own initialized live storage.
            # Bind through its public C symbol; provide generated bytes only
            # as the fallback when no translation unit owns that object.
            if row['name'] in NATIVE_CPP_DATA:
                cpp_aliases.append(('_' + row['name'], NATIVE_CPP_DATA[row['name']]))
            else:
                selected[key]['linker_names'].append('_' + row['name'])
            sequence_targets.append(key)
        pointer_tables[sequence_key] = sequence_targets

    # Embedded Goomba BMA files contain absolute pointers, unlike disk BMA
    # files whose loader fixes offsets. Preserve their words and rebase only
    # the six source-proven pointer fields (header values/entries and names).
    material_relocations = {}
    if ('ov084', 0x0213089c) in selected:
        expected = {0x0213088c: {4: 0x02130178, 12: 0x0213018c},
                    0x0213089c: {4: 0x02130174, 12: 0x021301c8},
                    0x0213018c: {4: 0x02130880},
                    0x021301c8: {4: 0x02130880}}
        for address in set(expected) | {p for fields in expected.values() for p in fields.values()}:
            row = next(r for r in available if r['module']=='ov084' and r['address']==address)
            key=('ov084', address)
            selected.setdefault(key, {'row': row, 'linker_names': []})
            selected[key]['linker_names'].append('_'+row['name'])
        for address, fields in expected.items():
            key=('ov084', address)
            data=source_bytes(root, selected[key]['row'])
            for offset, target in fields.items():
                if struct.unpack_from('<I', data, offset)[0]!=target:
                    raise ValueError('Goomba embedded material pointer changed')
            material_relocations[key]=fields

    lines = ["// Generated from local ROM symbols and unresolved linker names.",
             "// Do not edit or commit; source bytes remain in the user's ROM.",
             '#include <stddef.h>', 'extern "C" {']
    aliases = []
    storage_names = {key: f'pc_rom_data_{index}'
                     for index, key in enumerate(sorted(selected))}
    for index, (key, item) in enumerate(sorted(selected.items())):
        row = item["row"]
        data = source_bytes(root, row)
        storage = f"pc_rom_data_{index}"
        if key in material_relocations:
            fields=material_relocations[key]
            for target in set(fields.values()):
                lines.append(f'extern unsigned char data_ov084_{target:08x}[];')
            words=[]
            for offset in range(0,len(data),4):
                if offset in fields: words.append(f'data_ov084_{fields[offset]:08x}')
                else: words.append(f'(void*)0x{struct.unpack_from("<I",data,offset)[0]:08x}u')
            lines.append(f'void *{storage}[{len(words)}] = {{'+', '.join(words)+'};')
            aliases.extend((external,storage) for external in sorted(set(item['linker_names'])))
            continue
        if row['name'] == 'MSG_GEN_TEXT_FUNCS':
            targets = message_text_callbacks(root)
            if len(data) != len(targets) * 4:
                raise ValueError('Message text callback table extent changed')
            lines.extend(f'void {target}(void);' for target in targets)
            lines.append(f'void (*{storage}[{len(targets)}])(void) = {{' +
                         ', '.join(targets) + '};')
            aliases.extend((external, storage) for external in sorted(set(item['linker_names'])))
            continue
        if key in pointer_tables:
            sequence_targets = pointer_tables[key]
            # Targets can follow the table in address order; forward declare
            # the exact generated allocation before using its native address.
            for target in sorted(target for target in set(sequence_targets) if target is not None):
                lines.append(f'extern unsigned char {selected[target]["row"]["name"]}[];')
            lines.append(f'unsigned char *{storage}[{len(sequence_targets)}] = {{')
            lines.extend(f'    {selected[target]["row"]["name"]},' if target is not None else '    nullptr,'
                         for target in sequence_targets)
            lines.append('};')
            aliases.extend((external, storage) for external in sorted(set(item['linker_names'])))
            continue
        lines.append(f"__declspec(align(4)) unsigned char {storage}[{len(data)}] = {{")
        if any(data):
            for start in range(0, len(data), 16):
                lines.append("    " + ", ".join(f"0x{x:02x}" for x in data[start:start + 16]) + ",")
        else:
            lines.append("    0,")
        lines.append("};")
        aliases.extend((external, storage) for external in sorted(set(item["linker_names"])))
    lines.append("}")
    for external, storage in aliases:
        lines.append(f'#pragma comment(linker, "/alternatename:{external}=_'+storage+'")')
    for external, storage in host_aliases:
        lines.append(f'#pragma comment(linker, "/alternatename:{external}=_{storage}")')
    for external, target in cpp_aliases:
        lines.append(f'#pragma comment(linker, "/alternatename:{external}={target}")')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(lines) + "\n", encoding="ascii")
    manifest_path.write_text(json.dumps(external_names, indent=2, sort_keys=True) + "\n",
                             encoding="utf-8")
    print(f"link data: {len(selected)} data regions, {len(aliases)} decorated references, {len(missing)} unknown symbols")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

