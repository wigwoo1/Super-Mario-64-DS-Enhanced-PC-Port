#!/usr/bin/env python3
"""Emit course-overlay bounds and self-reference offsets, without ROM bytes."""
import argparse
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    arm9 = (ROOT / 'extracted/arm9_dec.bin').read_bytes()
    ids = struct.unpack_from('<52I', arm9, 0x020758c8 - 0x02004000)
    records = struct.unpack_from('<52I', arm9, 0x02092208 - 0x02004000)
    output = ['// Generated course data metadata; no ROM image bytes.',
              '#include "hal/level_overlay.h"', 'namespace {']
    descriptors = []
    total = 0
    for level, (ov_id, record) in enumerate(zip(ids, records)):
        meta = ROOT / f'config/arm9/overlays/ov{ov_id:03d}'
        text = (meta / 'delinks.txt').read_text()
        base = min(int(s, 16) for s in re.findall(
            r'^\s*\.[^\s]+\s+start:0x([0-9a-fA-F]+)', text, re.M))
        bss = re.search(r'^\s*\.bss\s+start:0x([0-9a-fA-F]+)\s+end:0x([0-9a-fA-F]+)', text, re.M)
        if not bss:
            raise ValueError(f'overlay {ov_id}: missing BSS bounds')
        bss_start, end = (int(x, 16) for x in bss.groups())
        image = (ROOT / f'extracted/overlays/overlay_{ov_id:04d}.bin').read_bytes()
        if base + len(image) != bss_start or end < bss_start or not base <= record <= bss_start - 16:
            raise ValueError(f'overlay {ov_id}: invalid image or course record bounds')
        offsets = {}
        for line in (meta / 'relocs.txt').read_text().splitlines():
            match = re.fullmatch(r'from:0x([\da-fA-F]+) kind:load to:0x([\da-fA-F]+) module:overlay(?:s)?\(([^)]+)\)', line.strip())
            if not match or ov_id not in [int(x) for x in match[3].split(',')]:
                continue
            source, target = int(match[1], 16), int(match[2], 16)
            offset = source - base
            if not 0 <= offset <= len(image) - 4 or not base <= target < end:
                raise ValueError(f'overlay {ov_id}: self-reference outside image: {line}')
            if struct.unpack_from('<I', image, offset)[0] != target:
                raise ValueError(f'overlay {ov_id}: relocation disagrees with ROM: {line}')
            offsets[offset] = target
        offsets = sorted(offsets)
        total += len(offsets)
        name = f'level_{level}_relocations'
        output.append(f'const uint32_t {name}[] = {{')
        output += ['    ' + ', '.join(f'0x{x:x}u' for x in offsets[i:i+8]) + ','
                   for i in range(0, len(offsets), 8)]
        if not offsets:
            output.append('    0,')
        output.append('};')
        descriptors.append(f'    {{{ov_id}, 0x{base:x}u, {len(image)}, {end-bss_start}, 0x{record:x}u, {name}, {len(offsets)}}},')
    output += ['}', 'const PcLevelOverlaySpec pc_level_overlay_specs[PC_LEVEL_COUNT] = {',
               *descriptors, '};']
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text('\n'.join(output) + '\n', encoding='ascii')
    print(f'Course metadata: {len(descriptors)} overlays, {total} validated self-references')


if __name__ == '__main__':
    main()
