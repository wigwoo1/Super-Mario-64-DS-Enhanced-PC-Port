#!/usr/bin/env python3
"""Emit selected overlay data symbols from the user's local ROM extraction.

File-backed bytes come from the extracted overlay image. BSS symbols receive
zero-initialized host storage sized to the next symbol boundary. Output stays
under build/ and is never committed as ROM-derived content.
"""

import argparse
import pathlib
import re
import struct
import sys
from link_data import NATIVE_CPP_DATA


ROOT = pathlib.Path(__file__).resolve().parents[2]
OV_BASE = {1: 0x020AA420, 2: 0x020AD660}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", required=True, type=pathlib.Path)
    ap.add_argument("--overlay", type=int, choices=(1, 2), default=2)
    ap.add_argument("--link-log", type=pathlib.Path,
                    help="include unresolved data symbols named by a prior link")
    ap.add_argument("symbols", nargs="+")
    args = ap.parse_args()

    symbol_file = ROOT / f"config/arm9/overlays/ov{args.overlay:03d}/symbols.txt"
    image_file = ROOT / f"extracted/overlays/overlay_{args.overlay:04d}.bin"
    if not symbol_file.is_file() or not image_file.is_file():
        sys.exit(f"missing ov{args.overlay:03d} symbols or extracted overlay image")

    records = {}
    all_records = []
    for line in symbol_file.read_text(encoding="utf-8").splitlines():
        match = re.match(r"(\S+) kind:(\S+) addr:0x([0-9a-fA-F]+)", line)
        if match:
            name, kind, raw_address = match.groups()
            record = (name, kind, int(raw_address, 16))
            records[name] = record
            all_records.append(record)
    all_records.sort(key=lambda row: row[2])
    image = image_file.read_bytes()
    ov_base = OV_BASE[args.overlay]
    bss_start = ov_base + len(image)

    requested = list(dict.fromkeys(args.symbols))
    decorated_aliases = {}
    if args.link_log is not None and args.link_log.is_file():
        c_symbol = re.compile(r"_data_ov%03d_([0-9a-fA-F]{8})" % args.overlay)
        cpp_symbol = re.compile(
            r"\?(data_ov%03d_[0-9a-fA-F]{8}@@3[^\s\"]+)" % args.overlay)
        for line in args.link_log.read_text(encoding="utf-8", errors="replace").splitlines():
            if "unresolved external symbol" not in line:
                continue
            for match in c_symbol.finditer(line):
                name = f"data_ov{args.overlay:03d}_{match.group(1).lower()}"
                if name in records and name not in requested:
                    requested.append(name)
            for match in cpp_symbol.finditer(line):
                decorated_aliases.setdefault(match.group(1).split("@@3", 1)[0], set()).add(
                    match.group(1))

    lines = [f"/* Generated locally from ov{args.overlay:03d}; do not commit ROM-derived bytes. */",
             'extern "C" {']
    for name in requested:
        if name in NATIVE_CPP_DATA:
            lines.append(f'#pragma comment(linker, "/alternatename:_{name}={NATIVE_CPP_DATA[name]}")')
            continue
        if name not in records:
            sys.exit(f"ov{args.overlay:03d} symbol not found: {name}")
        _, kind, address = records[name]
        next_address = next((row[2] for row in all_records if row[2] > address),
                            address + 4)
        size = max(4, next_address - address)
        # Player::SetAnim/IsAnim index by animation ID * 4 plus character.
        # The next named table at 0x02100000 ends this 736-entry pointer run;
        # 0x020ff790 is an interior label, not the end of the animation array.
        if args.overlay == 2 and name == "data_ov002_020ff480":
            size = 736 * 4
        # SOUND_OBJ indexes seven 12-byte rows. The labels at +4/+8/+10
        # name members inside the first row, not the end of the array.
        # Stopping at the next label emitted only the first sound ID and
        # made every later field/row read unrelated host globals.
        if args.overlay == 2 and name == "data_ov002_0210c080":
            size = 7 * 12
        if kind == "bss" or address >= bss_start:
            lines.append(f"__declspec(align(4)) unsigned char {name}[{size}] = {{0}};")
            continue
        offset = address - ov_base
        if offset < 0 or offset + size > len(image):
            sys.exit(f"{name} span {address:#x}+{size:#x} is outside ov{args.overlay:03d} file data")
        values = image[offset:offset + size]
        if args.overlay == 2 and name == "data_ov002_0210a084":
            # Yoshi's tongue/retract pair points at SharedFilePtr handles,
            # whose live native objects are not at their cartridge addresses.
            targets = (0x0210e6c0, 0x0210eb98)
            if values != struct.pack('<2I', *targets):
                sys.exit('Yoshi animation handle table changed')
            for target in targets:
                lines.append(f'extern unsigned char data_ov002_{target:08x}[];')
            lines.append(f'void *{name}[2] = {{' + ', '.join(
                f'data_ov002_{target:08x}' for target in targets) + '};')
            continue
        lines.append(f"__declspec(align(4)) unsigned char {name}[{size}] = {{")
        for start in range(0, size, 16):
            lines.append("    " + ", ".join(f"0x{x:02x}" for x in values[start:start + 16]) + ",")
        lines.append("};")
    lines.append("}")
    emitted = set(requested)
    for base_name, aliases in sorted(decorated_aliases.items()):
        if base_name in NATIVE_CPP_DATA:
            for decorated in sorted(aliases):
                target = NATIVE_CPP_DATA[base_name]
                if decorated != target:
                    lines.append(f'#pragma comment(linker, "/alternatename:{decorated}={target}")')
            continue
        if base_name not in emitted:
            continue
        for decorated in sorted(aliases):
            lines.append(
                f'#pragma comment(linker, "/alternatename:{decorated}=' 
                f'_{base_name}")')
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text("\n".join(lines) + "\n", encoding="ascii")
    print(f"ov{args.overlay:03d} data -> {args.out} ({len(args.symbols)} symbols)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
