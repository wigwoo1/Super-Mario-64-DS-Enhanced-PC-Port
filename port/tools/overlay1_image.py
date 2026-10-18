#!/usr/bin/env python3
"""Generate overlay 1 bytes and relocations from the local ROM extraction."""

import argparse
import pathlib
import re
import struct
import sys


ROOT = pathlib.Path(__file__).resolve().parents[2]
BASE = 0x020AA420


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", required=True, type=pathlib.Path)
    parser.add_argument("--symbols", action="store_true",
                        help="also export symbols located in the overlay image")
    args = parser.parse_args()

    image_path = ROOT / "extracted/overlays/overlay_0001.bin"
    reloc_path = ROOT / "config/arm9/overlays/ov001/relocs.txt"
    if not image_path.is_file() or not reloc_path.is_file():
        sys.exit("missing overlay 1 extraction or relocation table")

    file_image = image_path.read_bytes()
    image_end = BASE + len(file_image)
    bss = re.search(
        r"\.bss\s+start:0x([0-9a-fA-F]+)\s+end:0x([0-9a-fA-F]+)",
        (ROOT / "config/arm9/overlays/ov001/delinks.txt").read_text(encoding="utf-8"),
    )
    if not bss:
        sys.exit("overlay 1 BSS bounds missing")
    bss_start, overlay_end = (int(value, 16) for value in bss.groups())
    if image_end != bss_start or overlay_end < bss_start:
        sys.exit("overlay 1 file/BSS bounds do not join")
    image = file_image + bytes(overlay_end - image_end)
    reloc_offsets = []
    pattern = re.compile(
        r"from:0x([0-9a-fA-F]+) kind:load to:0x([0-9a-fA-F]+) module:overlay\(1\)"
    )
    for line in reloc_path.read_text(encoding="utf-8").splitlines():
        match = pattern.search(line)
        if not match:
            continue
        source, target = (int(value, 16) for value in match.groups())
        if BASE <= source <= image_end - 4:
            if not BASE <= target < BASE + len(image):
                sys.exit(f"overlay 1 relocation target outside image: {line}")
            reloc_offsets.append(source - BASE)

    output = [
        "/* Generated locally from overlay 1; do not commit ROM-derived bytes. */",
        "#include <stddef.h>",
        "#include <stdint.h>",
        'extern "C" {',
    ]
    if args.symbols:
        symbol_path = ROOT / "config/arm9/overlays/ov001/symbols.txt"
        if not symbol_path.is_file():
            sys.exit("missing overlay 1 symbol table")
        pattern = re.compile(r"(\S+) kind:data\([^)]*\) addr:0x([0-9a-fA-F]+)")
        symbol_rows = []
        for line in symbol_path.read_text(encoding="utf-8").splitlines():
            match = pattern.match(line)
            if not match:
                continue
            name, raw_address = match.groups()
            address = int(raw_address, 16)
            symbol_rows.append((address, name))
        for address, name in symbol_rows:
            if name.startswith("_ZN3OAM") and BASE <= address < BASE + len(image):
                if name == '_ZN3OAM7NUMBERSE':
                    # This symbol is a 20-entry pointer table, not a pointer
                    # variable. Course-clear RenderNumber indexes it directly.
                    end = min(a for a, _ in symbol_rows if a > address)
                    if end-address != 80:raise ValueError('OAM numeral table extent changed')
                    targets=struct.unpack_from('<20I',image,address-BASE)
                    for digit,target in enumerate(targets):
                        if not BASE<=target<=BASE+len(image)-8:
                            raise ValueError('OAM numeral descriptor outside overlay1')
                        data=image[target-BASE:target-BASE+8]
                        output.append(f'__declspec(selectany) unsigned char pc_oam_number_{digit}[8] = {{'+
                                      ', '.join(f'0x{x:02x}' for x in data)+'};')
                    output.append(f'__declspec(selectany) unsigned char *{name}[20] = {{'+
                                  ', '.join(f'pc_oam_number_{i}' for i in range(20))+'};')
                elif 0x020ab800 <= address < 0x020ab938:
                    # Player/cap icon symbols name OAM descriptor arrays,
                    # not pointer variables. Minimap tables take their address.
                    end = min(a for a, _ in symbol_rows if a > address)
                    data = image[address - BASE:end - BASE]
                    output.append(f'__declspec(selectany) unsigned char {name}[{len(data)}] = {{')
                    output.append('    ' + ', '.join(f'0x{value:02x}' for value in data))
                    output.append('};')
                else:
                    output.append(f'__declspec(selectany) unsigned char *{name} = '
                                  f'(unsigned char *)0x{address:08x};')
    output.extend([
        "}",
        "namespace {",
        f"alignas(32) unsigned char overlay1_image[{len(image)}] = {{",
    ])
    for start in range(0, len(image), 16):
        output.append("    " + ", ".join(f"0x{value:02x}" for value in image[start:start + 16]) + ",")
    output.extend(["};", "const uint32_t overlay1_relocations[] = {"])
    for start in range(0, len(reloc_offsets), 8):
        output.append("    " + ", ".join(f"0x{offset:x}u" for offset in reloc_offsets[start:start + 8]) + ",")
    output.extend([
        "};",
        "bool overlay1_relocated;",
        "void relocate_overlay1() {",
        "    if (overlay1_relocated) return;",
        "    overlay1_relocated = true;",
        "    const intptr_t delta = reinterpret_cast<intptr_t>(overlay1_image) - 0x020aa420;",
        "    for (size_t i = 0; i < sizeof(overlay1_relocations) / sizeof(overlay1_relocations[0]); ++i) {",
        "        uint32_t *word = reinterpret_cast<uint32_t *>(overlay1_image + overlay1_relocations[i]);",
        "        *word = static_cast<uint32_t>(static_cast<intptr_t>(*word) + delta);",
        "    }",
        "}",
        "}",
        'extern "C" void *port_overlay1_address(uint32_t address) {',
        "    relocate_overlay1();",
        f"    if (address < 0x020aa420u || address >= 0x{BASE + len(image):08x}u) return nullptr;",
        "    return overlay1_image + (address - 0x020aa420u);",
        "}",
    ])
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text("\n".join(output) + "\n", encoding="ascii")
    print(f"overlay 1 image -> {args.out} ({len(image)} bytes, {len(reloc_offsets)} relocs)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
