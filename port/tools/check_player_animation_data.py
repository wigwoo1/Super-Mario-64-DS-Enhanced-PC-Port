#!/usr/bin/env python3
"""Check the native animation table and exact handle symbols against local ROM data."""
import argparse
import pathlib
import re
import struct

ROOT = pathlib.Path(__file__).resolve().parents[2]
START, END, ENTRIES = 0x0210E168, 0x0210ED68, 736
# This reserved record has no constructor/reference in the recovered overlay.
UNUSED = {0x0210E7B8}


def handle_records():
    records = {}
    for line in (ROOT / "config/arm9/overlays/ov002/symbols.txt").read_text().splitlines():
        match = re.match(r"(data_ov002_[\da-f]+) kind:\S+ addr:0x([\da-f]+)", line)
        if match:
            address = int(match[2], 16)
            if START <= address < END and address not in UNUSED:
                records[address] = match[1]
    if len(records) != 383:
        raise ValueError("overlay handle manifest changed; review reserved records")
    return records


def include_text(records):
    return "// Exact public ov002 symbols for linked native handle storage.\n" + "".join(
        f"PC_PLAYER_ANIM_HANDLE(0x{address:08x}u, {name})\n"
        for address, name in sorted(records.items()))


def pe_bytes(executable, address, length, allow_zero_tail=False):
    data = executable.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe:pe+4] != b"PE\0\0":
        raise ValueError("not a PE executable")
    section_count = struct.unpack_from("<H", data, pe + 6)[0]
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    optional = pe + 24
    if struct.unpack_from("<H", data, optional)[0] != 0x10B:
        raise ValueError("expected the native x86 PE32 build")
    image_base = struct.unpack_from("<I", data, optional + 28)[0]
    rva = address - image_base
    for index in range(section_count):
        header = optional + optional_size + index * 40
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from("<4I", data, header + 8)
        if virtual_address <= rva and rva + length <= virtual_address + raw_size:
            offset = raw_offset + rva - virtual_address
            return data[offset:offset+length]
        if (allow_zero_tail and virtual_address <= rva
                and rva + length <= virtual_address + virtual_size):
            # PE loader supplies zeros for an explicitly declared BSS tail.
            available = max(0, min(length, virtual_address + raw_size - rva))
            offset = raw_offset + rva - virtual_address
            return data[offset:offset+available] + bytes(length - available)
    raise ValueError("compiled animation array is not fully file-backed")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write-include", action="store_true")
    parser.add_argument("--map", type=pathlib.Path, default=ROOT / "build/port/sm64ds_pc.map")
    parser.add_argument("--exe", type=pathlib.Path, default=ROOT / "build/port/sm64ds_pc.exe")
    args = parser.parse_args()
    constructor=(ROOT/'src/unnamed/ov002/__sinit_ov002_021019d0.c').read_text()
    original_ids={suffix:int(file_id,16) for suffix,file_id in re.findall(
        r'_ZN13SharedFilePtr9ConstructEj\(&?data_ov002_0210([\da-f]+), (0x[\da-f]+)\)',constructor)}
    native=(ROOT/'port/hal/actor_vtables.cpp').read_text()
    overrides=re.findall(r'PC_INIT_PLAYER_ANIM_HANDLE\(([\da-f]+), (0x[\da-f]+)\)',native)
    for suffix,file_id in overrides:
        if original_ids.get(suffix)!=int(file_id,16):
            raise ValueError(f'native handle {suffix} overrides its original file ID')
    if original_ids.get('e168')!=0x8099:
        raise ValueError('original shared slide animation changed; review its archive asset')
    records = handle_records()
    include = ROOT / "port/hal/player_anim_handles.inc"
    expected = include_text(records)
    if args.write_include:
        if not include.exists() or include.read_text() != expected:
            include.write_text(expected)
    if include.read_text() != expected:
        raise ValueError("native handle include differs from the public symbol manifest")
    symbols = {}
    for line in args.map.read_text().splitlines():
        match = re.match(r"\s+\d{4}:[\da-f]+\s+(\S+)\s+([\da-f]{8})\s", line)
        if match:
            symbols[match[1]] = int(match[2], 16)
    missing = [name for name in records.values() if "_" + name not in symbols]
    if missing:
        raise ValueError(f"missing native handle symbols: {missing}")
    overlay = (ROOT / "extracted/overlays/overlay_0002.bin").read_bytes()
    original = overlay[0x020FF480 - 0x020AD660:0x02100000 - 0x020AD660]
    if len(original) != ENTRIES * 4:
        raise ValueError("local animation pointer table has the wrong extent")
    for index, (address,) in enumerate(struct.iter_unpack("<I", original)):
        if address not in records:
            raise ValueError(f"animation entry {index} has no exact native handle mapping")
    compiled = pe_bytes(args.exe, symbols["_data_ov002_020ff480"], ENTRIES * 4)
    if compiled != original:
        raise ValueError("compiled table does not contain all original 736 entries")
    print("PASS: 736 compiled animation entries match local ROM; 383 exact native handle symbols resolve; native handle overrides preserve original IDs including slide archive0x8099")


if __name__ == "__main__":
    main()
