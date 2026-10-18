#!/usr/bin/env python3
"""Generate exact native Player state method mappings from public symbols."""
import argparse
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[2]


def expected_include():
    header = (ROOT / 'include/Player.h').read_text()
    rows = []
    raw_workers = {'St_SwingPlayer_Main': '_ZN6Player19St_SwingPlayer_MainEv'}
    for line in (ROOT / 'config/arm9/overlays/ov002/symbols.txt').read_text().splitlines():
        match = re.match(r'_ZN6Player\d+(St_\w+)Ev kind:function\([^)]*\) addr:0x([\da-f]+)', line)
        if match:
            method, address = match.groups()
            if method in raw_workers:
                rows.append((int(address, 16), 'WORKER', raw_workers[method]))
                continue
            if not re.search(r'\b(?:int|s32)\s+' + re.escape(method) + r'\(\s*(?:void\s*)?\)', header):
                raise ValueError(f'{method}: native no-argument int method is not declared')
            rows.append((int(address, 16), 'METHOD', method))
    if not rows:
        raise ValueError('no Player state methods found in the public manifest')
    return '// Exact public ov002 code addresses mapped to native Player methods.\n' + ''.join(
        f'PC_PLAYER_STATE_{kind}(0x{address:08x}u, {method})\n' for address, kind, method in sorted(rows))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write-include', action='store_true')
    parser.add_argument('--map', type=pathlib.Path, help='also verify every mapped callback links natively')
    args = parser.parse_args()
    target = ROOT / 'port/hal/player_state_methods.inc'
    expected = expected_include()
    if args.write_include:
        target.write_text(expected)
    if target.read_text() != expected:
        raise ValueError('native state include differs from the public symbol manifest')
    if args.map:
        names = set()
        for line in args.map.read_text().splitlines():
            match = re.match(r'\s+\d{4}:[\da-f]+\s+(\S+)\s+[\da-f]{8}\s', line)
            if match:
                names.add(match[1])
        for kind, symbol in re.findall(r'PC_PLAYER_STATE_(METHOD|WORKER)\(0x[\da-f]+u, (\w+)\)', expected):
            linked = ('?' + symbol + '@Player@@QAEHXZ') if kind == 'METHOD' else '_' + symbol
            if linked not in names:
                raise ValueError(f'{symbol}: mapped native callback is absent from the executable')
    print(f'PASS: {expected.count("PC_PLAYER_STATE_")} exact Player state callbacks mapped')


if __name__ == '__main__':
    main()
