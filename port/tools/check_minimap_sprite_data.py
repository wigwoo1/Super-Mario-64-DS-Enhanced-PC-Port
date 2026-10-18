#!/usr/bin/env python3
"""Check compiled minimap sprite pointers and sprite bytes against the ROM."""
import argparse
import pathlib
import re
import struct

from check_player_animation_data import pe_bytes
from link_data import records, source_bytes

ROOT = pathlib.Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--map', type=pathlib.Path, default=ROOT / 'build/port/sm64ds_pc.map')
    parser.add_argument('--exe', type=pathlib.Path, default=ROOT / 'build/port/sm64ds_pc.exe')
    args = parser.parse_args()
    symbols = {}
    for line in args.map.read_text().splitlines():
        match = re.match(r'\s+\d{4}:[\da-f]+\s+(\S+)\s+([\da-f]{8})\s', line)
        if match:
            symbols[match[1]] = int(match[2], 16)

    def address(name):
        return symbols['_' + name]

    available = records(ROOT)
    checked = 0
    for name, count, target_module in (
            ('_ZN3OAM15MM_PLAYER_ICONSE', 17, 'ov001'),
            ('_ZN3OAM15MM_STAR_MARKERSE', 8, 'ov002'),
            ('_ZN3OAM18MM_VS_PLAYER_ICONSE', 16, 'ov002'),
            ('_ZN3OAM20MM_VS_PLAYER_ICONS_SE', 16, 'ov002'),
            ('data_ov002_0210c748', 3, 'ov002'),
            ('data_ov002_0210cac8', 3, 'ov002'),
            ('_ZN3OAM12MM_STAR_KEYSE', 2, 'ov002')):
        row = next(row for row in available if row['name'] == name)
        original = source_bytes(ROOT, row)[:count * 4]
        native = pe_bytes(args.exe, address(name), count * 4)
        for index, (guest, pointer) in enumerate(zip(struct.unpack(f'<{count}I', original),
                                                    struct.unpack(f'<{count}I', native))):
            targets = [row for row in available
                       if row['module'] == target_module and row['address'] == guest]
            if len(targets) != 1:
                raise ValueError(f'{name}[{index}]: ambiguous ROM sprite {guest:08x}')
            target = targets[0]
            if pointer == guest:
                raise ValueError(f'{name}[{index}]: unrelocated DS pointer {guest:08x}')
            if pointer != address(target['name']):
                raise ValueError(f'{name}[{index}]: not bound to canonical native sprite')
            expected = source_bytes(ROOT, target)
            if pe_bytes(args.exe, pointer, len(expected)) != expected:
                raise ValueError(f'{name}[{index}]: sprite bytes differ from ROM')
            checked += 1
    bank_base = address('data_020a6088')
    for guest in range(0x020a6088, 0x020a60a2, 2):
        if address(f'data_{guest:08x}') != bank_base + guest - 0x020a6088:
            raise ValueError(f'VRAM bank field {guest:08x}: not at its aggregate offset')
    print(f'minimap sprites: seven tables, {checked} canonical native pointers and ROM sprite bytes; '
          '13 shared VRAM bank field offsets pass')


if __name__ == '__main__':
    main()
