#!/usr/bin/env python3
"""Generate overlay 9's host image and apply its self-relocations."""

import argparse
import pathlib
import re
import sys


ROOT = pathlib.Path(__file__).resolve().parents[2]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", required=True, type=pathlib.Path)
    args = parser.parse_args()

    image_path = ROOT / "extracted/overlays/overlay_0009.bin"
    metadata = ROOT / "config/arm9/overlays/ov009"
    if not image_path.is_file():
        sys.exit("missing overlay 9 extraction")
    for name in ("delinks.txt", "relocs.txt"):
        if not (metadata / name).is_file():
            sys.exit(f"missing overlay 9 {name}")

    delinks = (metadata / "delinks.txt").read_text(encoding="utf-8")
    sections = [int(value, 16) for value in re.findall(
        r"^\s*\.[^\s]+\s+start:0x([0-9a-fA-F]+)", delinks, re.MULTILINE)]
    bss = re.search(
        r"\.bss\s+start:0x([0-9a-fA-F]+)\s+end:0x([0-9a-fA-F]+)", delinks)
    if not sections or not bss:
        sys.exit("overlay 9 image/BSS bounds missing")
    base = min(sections)
    bss_start, overlay_end = (int(value, 16) for value in bss.groups())
    file_image = image_path.read_bytes()
    file_end = base + len(file_image)
    if file_end != bss_start or overlay_end < bss_start:
        sys.exit("overlay 9 image and BSS bounds do not join")
    image = file_image + bytes(overlay_end - bss_start)

    reloc_offsets = []
    pattern = re.compile(
        r"from:0x([0-9a-fA-F]+) kind:load to:0x([0-9a-fA-F]+) "
        r"module:overlay\(9\)")
    for line in (metadata / "relocs.txt").read_text(encoding="utf-8").splitlines():
        match = pattern.search(line)
        if not match:
            continue
        source, target = (int(value, 16) for value in match.groups())
        if not base <= source <= file_end - 4 or not base <= target < overlay_end:
            sys.exit(f"overlay 9 self-relocation lies outside the image: {line}")
        reloc_offsets.append(source - base)

    output = [
        "/* Generated locally from overlay 9; do not commit ROM-derived bytes. */",
        "#include <cstddef>", "#include <cstdint>", "namespace {",
        f"constexpr uint32_t kOverlay9Base = 0x{base:08x}u;",
        f"alignas(32) unsigned char overlay9_image[{len(image)}] = {{",
    ]
    for start in range(0, len(image), 16):
        output.append("    " + ", ".join(
            f"0x{value:02x}" for value in image[start:start + 16]) + ",")
    output.extend(["};", "const uint32_t overlay9_relocations[] = {"])
    for start in range(0, len(reloc_offsets), 8):
        output.append("    " + ", ".join(
            f"0x{offset:x}u" for offset in reloc_offsets[start:start + 8]) + ",")
    output.extend([
        "};", "bool overlay9_relocated = false;",
        "void relocate_overlay9() {",
        "    if (overlay9_relocated) return;",
        "    overlay9_relocated = true;",
        "    const intptr_t delta = reinterpret_cast<intptr_t>(overlay9_image) - kOverlay9Base;",
        "    for (size_t i = 0; i < sizeof(overlay9_relocations) / sizeof(overlay9_relocations[0]); ++i) {",
        "        uint32_t *word = reinterpret_cast<uint32_t *>(overlay9_image + overlay9_relocations[i]);",
        "        *word = static_cast<uint32_t>(static_cast<intptr_t>(*word) + delta);",
        "    }", "}", "}",
        'extern "C" void pc_load_overlay9_image() { relocate_overlay9(); }',
        'extern "C" void *pc_overlay9_address(uint32_t address) {',
        "    relocate_overlay9();",
        f"    if (address < 0x{base:08x}u || address >= 0x{overlay_end:08x}u) return nullptr;",
        "    return overlay9_image + (address - kOverlay9Base);", "}",
    ])
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text("\n".join(output) + "\n", encoding="ascii")
    print(f"overlay 9 host image -> {args.out} ({len(image)} bytes, "
          f"{len(reloc_offsets)} self-relocations)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
