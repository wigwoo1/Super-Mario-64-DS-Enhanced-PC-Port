#!/usr/bin/env python3
"""Generate overlay 8's data-only image and self-relocations from the ROM extraction."""

import argparse
import pathlib
import re
import sys


ROOT = pathlib.Path(__file__).resolve().parents[2]
OVERLAY_ID = 8


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", required=True, type=pathlib.Path)
    args = parser.parse_args()

    image_path = ROOT / "extracted/overlays/overlay_0008.bin"
    metadata = ROOT / "config/arm9/overlays/ov008"
    if not image_path.is_file():
        sys.exit("missing overlay 8 extraction")
    for name in ("delinks.txt", "relocs.txt", "symbols.txt"):
        if not (metadata / name).is_file():
            sys.exit(f"missing overlay 8 {name}")

    delinks = (metadata / "delinks.txt").read_text(encoding="utf-8")
    sections = [int(value, 16) for value in re.findall(
        r"^\s*\.[^\s]+\s+start:0x([0-9a-fA-F]+)", delinks, re.MULTILINE)]
    bss = re.search(
        r"\.bss\s+start:0x([0-9a-fA-F]+)\s+end:0x([0-9a-fA-F]+)",
        delinks)
    if not sections or not bss:
        sys.exit("overlay 8 image/BSS bounds missing")
    base = min(sections)
    bss_start, overlay_end = (int(value, 16) for value in bss.groups())
    file_image = image_path.read_bytes()
    file_end = base + len(file_image)
    if file_end != bss_start or overlay_end != bss_start:
        sys.exit(
            f"overlay 8 is not data-only or bounds do not join: "
            f"image ends at 0x{file_end:08x}, BSS starts at 0x{bss_start:08x}, "
            f"overlay ends at 0x{overlay_end:08x}")

    reloc_offsets = []
    pattern = re.compile(
        r"from:0x([0-9a-fA-F]+) kind:load to:0x([0-9a-fA-F]+) "
        r"module:overlay\(8\)")
    for line in (metadata / "relocs.txt").read_text(encoding="utf-8").splitlines():
        match = pattern.search(line)
        if not match:
            # Overlay 8 must not need an ARM9 or other-overlay address fixup.
            if "module:" in line:
                sys.exit(f"overlay 8 has an external relocation the host image cannot map: {line}")
            continue
        source, target = (int(value, 16) for value in match.groups())
        if not base <= source <= file_end - 4 or not base <= target < overlay_end:
            sys.exit(f"overlay 8 relocation lies outside the image: {line}")
        reloc_offsets.append(source - base)

    symbols = []
    symbol_pattern = re.compile(
        r"^(\S+) kind:data\([^)]*\) addr:0x([0-9a-fA-F]+)(?:\s|$)")
    for line in (metadata / "symbols.txt").read_text(encoding="utf-8").splitlines():
        match = symbol_pattern.match(line)
        if match:
            name, address = match.groups()
            address = int(address, 16)
            if base <= address < overlay_end:
                symbols.append((name, address))

    output = [
        "/* Generated locally from overlay 8; do not commit ROM-derived bytes. */",
        "#include <cstddef>",
        "#include <cstdint>",
        "namespace {",
        f"constexpr uint32_t kOverlay8Base = 0x{base:08x}u;",
        f"alignas(32) unsigned char overlay8_image[{len(file_image)}] = {{",
    ]
    for start in range(0, len(file_image), 16):
        output.append("    " + ", ".join(
            f"0x{value:02x}" for value in file_image[start:start + 16]) + ",")
    output.extend(["};", "const uint32_t overlay8_relocations[] = {"])
    for start in range(0, len(reloc_offsets), 8):
        output.append("    " + ", ".join(
            f"0x{offset:x}u" for offset in reloc_offsets[start:start + 8]) + ",")
    output.extend([
        "};",
        "bool overlay8_relocated = false;",
        "void relocate_overlay8() {",
        "    if (overlay8_relocated) return;",
        "    overlay8_relocated = true;",
        "    const intptr_t delta = reinterpret_cast<intptr_t>(overlay8_image) - kOverlay8Base;",
        "    for (size_t i = 0; i < sizeof(overlay8_relocations) / sizeof(overlay8_relocations[0]); ++i) {",
        "        uint32_t *word = reinterpret_cast<uint32_t *>(overlay8_image + overlay8_relocations[i]);",
        "        *word = static_cast<uint32_t>(static_cast<intptr_t>(*word) + delta);",
        "    }",
        "}",
        "bool validate_overlay8() {",
        "    const uintptr_t begin = reinterpret_cast<uintptr_t>(overlay8_image);",
        "    const uintptr_t end = begin + sizeof(overlay8_image);",
        "    for (size_t i = 0; i < sizeof(overlay8_relocations) / sizeof(overlay8_relocations[0]); ++i) {",
        "        const uint32_t *word = reinterpret_cast<const uint32_t *>(overlay8_image + overlay8_relocations[i]);",
        "        const uintptr_t target = *word;",
        "        if (target < begin || target >= end) return false;",
        "    }",
        "    return true;",
        "}",
        "}",
        'extern "C" void pc_load_overlay8_image() { relocate_overlay8(); }',
        'extern "C" bool pc_validate_overlay8_image() {',
        "    relocate_overlay8();",
        "    return validate_overlay8();",
        "}",
        'extern "C" void *pc_overlay8_address(uint32_t address) {',
        "    relocate_overlay8();",
        f"    if (address < 0x{base:08x}u || address >= 0x{overlay_end:08x}u) return nullptr;",
        "    return overlay8_image + (address - kOverlay8Base);",
        "}",
        'extern "C" size_t pc_overlay8_relocation_count() {',
        "    return sizeof(overlay8_relocations) / sizeof(overlay8_relocations[0]);",
        "}",
    ])
    # Keep the recovered symbol/address roster available to maintainers and
    # fail generation if the map ever stops describing the complete data image.
    if not symbols or max(address for _, address in symbols) >= overlay_end:
        sys.exit("overlay 8 symbol map does not cover its data image")
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text("\n".join(output) + "\n", encoding="ascii")
    print(f"overlay 8 data image -> {args.out} ({len(file_image)} bytes, "
          f"{len(reloc_offsets)} self-relocations, {len(symbols)} mapped data symbols)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
