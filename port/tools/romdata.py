#!/usr/bin/env python3
"""Emit the ROM data constants the gate-4b slice references.

These are read-only tables in the arm9 image (sine table, identity matrices,
texture-matrix constants). They are Nintendo's bytes, so they are never
committed: this reads the local decompressed arm9 (extracted/arm9_dec.bin,
flat image based at 0x02000000) and writes a C file under the build tree.
"""

import pathlib
import struct
import sys

# extracted/arm9_dec.bin: the DECOMPRESSED arm9, flat at base 0x02004000
# (verified by two code anchors: Copy36Bytes @0x0205a548, DecompressLZ16
# @0x0205a754). extracted/dsd/arm9/arm9.bin is the COMPRESSED payload
# (arm9.yaml: compressed true) and must not be read directly. Addresses at
# or past bss_start are runtime-initialized and belong in the HAL.
BASE = 0x02004000
BSS_START = 0x02094640

# (symbol address, byte length, C element type). BSS addresses do not belong
# here -- they are runtime-initialized and live in the HAL as storage.
TABLES = [
    # A fourth column names ROM symbols the decomp spells by NAME rather than
    # by address. Model's migrated constructor assigns IDENTITY_MATRIX4X3, not
    # data_02082128, so the alias is emitted here beside the definition rather
    # than in a HAL file: a HAL alias would point at a symbol port_refcheck
    # cannot find in src/ or include/, because this datum is generated and
    # deliberately kept out of git.
    (0x02082128, 48, "int", ["IDENTITY_MATRIX4X3"]),  # Model ctor default Matrix4x3
    (0x02082190, 48, "int"),        # matrix Render uses for a NULL argument
    (0x02082214, 0x4000, "short"),  # s16 trig table the material bind indexes
    (0x020755A0, 12, "unsigned char"),  # OBJ width table (shape x size)
    (0x020755AC, 12, "unsigned char"),  # OBJ height table
    (0x020994E0, 0x804, "unsigned short"),  # fixed atan lookup table; next symbol at 0x02099ce4
    # ROM pointer-to-virtual-member records used by fBase_c::Process. The
    # first word is the vtable byte offset; the second word is the ARM ABI
    # virtual-member tag. The host bridge translates these into MSVC vtable
    # slots rather than trying to execute ARM member pointers.
    (0x02099E74, 8, "int"),
    (0x02099E7C, 8, "int"),
    (0x02099E84, 8, "int"),
    (0x02099E8C, 8, "int"),
    (0x02099E9C, 8, "int"),
    (0x02099EA4, 8, "int"),
    (0x02099EAC, 8, "int"),
    (0x02099EB4, 8, "int"),
    (0x02099ECC, 8, "int"),
    (0x02099E94, 8, "int"),
    (0x02099EBC, 8, "int"),
    (0x02099EC4, 8, "int"),
    # Actor manager lifecycle callbacks initialized by __sinit_02075154.
    # These are ARM virtual-member records consumed by the PC scheduler shim.
    (0x02099F48, 8, "int"),
    (0x02099F50, 8, "int"),
    (0x02099F60, 8, "int"),
    (0x02099F68, 8, "int"),
    (0x02099F70, 8, "int"),
    (0x02075230, 8, "unsigned short"),  # actor IDs for the four coin popups
    (0x02075238, 12, "int"),  # coin-kind score values
    (0x02075244, 12, "int"),  # WDW water heights; next symbol at 0x02075250
    (0x02092144, 8, "short"),  # per-player health initialized by SetPlayerGlobals
    # Course mapping tables used by level-part and sublevel helpers. The first
    # ends exactly where the second begins; the latter covers the 0x2e indexed
    # sublevels referenced by the game (including sublevel 0x2e).
    (0x02075264, 0x34, "signed char", ["LEVEL_PART_TABLE"]),  # ends at the next symbol
    (0x02075298, 0x3c, "signed char", ["SUBLEVEL_LEVEL_TABLE"]),  # ends at 0x020752d4
    # GX VRAM window tables. The source names four interior/overlapping views
    # into the same 3-column table; each view is emitted independently so the
    # host linker can satisfy its historic symbol spelling.
    (0x02086314, 16, "unsigned short"),
    (0x02086324, 96, "unsigned short"),
    (0x02086326, 96, "unsigned short"),
    (0x02086328, 96, "unsigned short"),
]

# The ARM9 image has file-backed read-only islands after its first BSS range.
# This table is independently named in config/arm9/symbols.txt and its bytes
# form the game's monotone 0..0x2000 atan lookup sequence. Keep it explicit so
# ordinary runtime BSS addresses remain rejected below.
FILE_BACKED_AFTER_BSS = {
    (0x02099E74, 8),
    (0x02099E7C, 8),
    (0x02099E84, 8),
    (0x02099E8C, 8),
    (0x02099E9C, 8),
    (0x02099EA4, 8),
    (0x02099EAC, 8),
    (0x02099EB4, 8),
    (0x02099ECC, 8),
    (0x020994E0, 0x804),
    (0x02099E94, 8),
    (0x02099EBC, 8),
    (0x02099EC4, 8),
    (0x02099F48, 8),
    (0x02099F50, 8),
    (0x02099F60, 8),
    (0x02099F68, 8),
    (0x02099F70, 8),
}


def main():
    root = pathlib.Path(__file__).resolve().parents[2]
    img = root / "extracted/arm9_dec.bin"
    if not img.exists():
        sys.exit(f"missing {img} -- extract the ROM first")
    anchor = img.read_bytes()
    a = 0x0205A548 - BASE
    if anchor[a:a + 4] != b"\x0c\x10\xb0\xe8":
        sys.exit("arm9_dec.bin failed the Copy36Bytes anchor -- wrong image/base")
    for addr, length, _, *_rest in TABLES:
        if addr + length > BSS_START and (addr, length) not in FILE_BACKED_AFTER_BSS:
            sys.exit(f"{addr:#x}+{length:#x} crosses into BSS -- not file-backed")
    data = img.read_bytes()
    out_path = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else (
        root / "build/port/host-src/romdata.c")
    out_path.parent.mkdir(parents=True, exist_ok=True)

    lines = ["/* GENERATED by port/tools/romdata.py from extracted/arm9_dec.bin.",
             " * ROM constants stay out of git; regenerate locally. */", ""]
    for addr, length, ctype, *rest in TABLES:
        off = addr - BASE
        blob = data[off:off + length]
        if len(blob) != length:
            sys.exit(f"short read at {addr:#x}")
        width = 4 if ctype == "int" else (1 if ctype in ("unsigned char", "signed char") else 2)
        fmt = "<i" if ctype == "int" else ("<B" if ctype == "unsigned char" else ("<b" if ctype == "signed char" else ("<H" if ctype == "unsigned short" else "<h")))
        vals = [struct.unpack_from(fmt, blob, i)[0] for i in range(0, length, width)]
        name = f"data_{addr:08x}"
        body = ", ".join(str(v) for v in vals)
        lines.append(f"{ctype} {name}[{len(vals)}] = {{ {body} }};")
        # MSVC decorates a C symbol with a leading underscore and a C++ one with
        # its type, and callers of both spellings have to reach the same bytes,
        # so each alias is emitted in both decorations.
        # The datum's own name needs the C++ decoration too: a TU that declares
        # it as a Matrix4x3 rather than an int array decorates the reference by
        # type, and has to reach these same bytes.
        type_decorations = {
            "int": "H",
            "short": "F",
            "unsigned short": "G",
            "unsigned char": "E",
            "signed char": "D",
        }
        for alias in [name] + list(rest[0] if rest else []):
            decs = [f"_{alias}"]
            if alias == name:
                decs.append(f"?{alias}@@3PA{type_decorations[ctype]}A")
            else:
                # Named matrix aliases retain their host-side record type.
                decs.append(f"?{alias}@@3UMatrix4x3@@A")
            for dec in decs:
                lines.append(f'#pragma comment(linker, "/alternatename:{dec}=_{name}")')
        lines.append("")
    out_path.write_text("\n".join(lines), encoding="ascii")
    print(f"romdata -> {out_path}")


if __name__ == "__main__":
    main()
    (0x02099E74, 8, "int"),
    (0x02099E7C, 8, "int"),
    (0x02099E84, 8, "int"),
    (0x02099E8C, 8, "int"),
    (0x02099E9C, 8, "int"),
    (0x02099EA4, 8, "int"),
    (0x02099EAC, 8, "int"),
    (0x02099EB4, 8, "int"),
    (0x02099ECC, 8, "int"),
