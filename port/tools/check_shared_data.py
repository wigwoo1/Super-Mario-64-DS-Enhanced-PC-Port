"""Reject separate C/C++ storage for the same cartridge global in an MSVC map."""
import pathlib
import re
import sys


def main():
    if len(sys.argv) != 2:
        sys.exit("usage: check_shared_data.py GAME_MAP")
    symbols = {}
    for line in pathlib.Path(sys.argv[1]).read_text(errors="replace").splitlines():
        row = re.match(r"\s+\S+\s+(\S+)\s+([0-9a-fA-F]{8})\s+(.+)$", line)
        if row:
            name, address, owner = row.groups()
            symbols[name] = (int(address, 16), owner)
    checked = 0
    failures = []
    # These are two names for the same original ROM address, even though the
    # semantic name does not match the data_<address> spelling regex below.
    for name in ('_data_020a0eac', '_data_020a0eac_c'):
        if name in symbols and '_GAME_HEAP_PTR' in symbols:
            checked += 1
            if symbols[name][0] != symbols['_GAME_HEAP_PTR'][0]:
                failures.append((name, symbols[name], symbols['_GAME_HEAP_PTR']))
    for name, value in symbols.items():
        match = re.match(r"\?(data_(?:ov\d+_)?[0-9a-fA-F]{8})@@", name)
        if not match:
            continue
        c_name = "_" + match.group(1)
        if c_name in symbols:
            checked += 1
            if symbols[c_name][0] != value[0]:
                failures.append((name, value, symbols[c_name]))
    for name, cpp, c in failures:
        print(f"FAIL {name}: C++ {cpp[0]:08x} ({cpp[1]}) != C {c[0]:08x} ({c[1]})")
    if not checked:
        sys.exit("FAIL: no shared globals found; the map is missing or incomplete")
    print(f"Shared data: {checked} C/C++ bindings checked, {len(failures)} divergent addresses")
    return bool(failures)


if __name__ == "__main__":
    sys.exit(main())
