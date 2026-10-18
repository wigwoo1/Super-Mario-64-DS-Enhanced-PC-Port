#!/usr/bin/env python3
"""Generate the native-pointer course-label table for the title overlay."""

import pathlib
import struct
import sys


BASE = 0x020AD660
TABLE = 0x020B117C
COUNT = 0x36


def main() -> int:
    if len(sys.argv) != 3:
        sys.exit("usage: title_overlay_data.py DECOMP_ROOT OUTPUT_CPP")
    root, output = map(pathlib.Path, sys.argv[1:])
    image = (root / "extracted/overlays/overlay_0003.bin").read_bytes()
    table_offset = TABLE - BASE
    labels = []
    ordinals = []
    for index in range(COUNT):
        entry = table_offset + index * 8
        if entry + 8 > len(image):
            sys.exit(f"title course table entry {index} is outside overlay 3")
        address, ordinal = struct.unpack_from("<II", image, entry)
        offset = address - BASE
        if not 0 <= offset < len(image):
            sys.exit(f"title course label {index} points outside overlay 3: {address:#x}")
        end = image.find(b"\0", offset)
        if end < 0:
            sys.exit(f"title course label {index} has no terminator")
        labels.append(image[offset:end + 1])
        ordinals.append(ordinal)

    lines = [
        "// Generated from the local European Rev 0 overlay 3 image.",
        "// ROM bytes are kept in the build tree; pointers are native host pointers.",
        "struct PcTitleCourseText { volatile signed char *ptr; int ordinal; };",
    ]
    for index, data in enumerate(labels):
        values = ", ".join(f"static_cast<signed char>(0x{x:02x})" for x in data)
        lines.append(f"static volatile signed char pc_title_label_{index}[{len(data)}] = {{{values}}};")
    lines.append('extern "C" {')
    lines.append("PcTitleCourseText data_ov003_020b117c[0x36] = {")
    for index, ordinal in enumerate(ordinals):
        lines.append(f"    {{pc_title_label_{index}, {ordinal}}},")
    lines.extend(["};"])
    # The scene code aliases bytes 0/1 of each 8-byte course entry as two
    # stride-8 arrays: signed course ID and sublevel. Emit those views in host
    # storage too, retaining the special -2/-1 selectors at the final rows.
    course_ids = [0] * (COUNT * 8)
    sublevels = [0] * (COUNT * 8)
    for index, ordinal in enumerate(ordinals):
        course_ids[index * 8] = ordinal & 0xFF
        sublevels[index * 8] = (ordinal >> 8) & 0xFF
    for name, values in (("data_ov003_020b1180", course_ids),
                         ("data_ov003_020b1181", sublevels)):
        lines.append(f"signed char {name}[{len(values)}] = {{")
        for start in range(0, len(values), 24):
            lines.append("    " + ", ".join(str(value if value < 128 else value - 256)
                                              for value in values[start:start + 24]) + ",")
        lines.append("};")
    lines.append("}")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(lines) + "\n", encoding="ascii")
    print(f"title overlay 3 course labels -> {output} ({COUNT} entries)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
