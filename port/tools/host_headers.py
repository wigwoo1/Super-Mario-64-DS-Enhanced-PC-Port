#!/usr/bin/env python3
"""Emit build-tree header overlays for host compiler ABI syntax differences."""

import pathlib
import re
import sys


PATTERN = re.compile(r"operator\s+new\s*\(\s*unsigned\s+long\b")
BASE_DTOR = re.compile(r"^[ \t]*extern\s+void\s+_ZN11dScMgBase_cD2Ev\(void\s*\*\);[ \t]*$", re.MULTILINE)


def main() -> int:
    if len(sys.argv) != 4:
        sys.exit("usage: host_headers.py DECOMP_ROOT OUTPUT_DIR STAMP")
    root, output, stamp = map(pathlib.Path, sys.argv[1:])
    include = root / "include"
    count = 0
    headers = {p: p.read_text(encoding="utf-8", errors="replace") for p in include.rglob("*.h")}
    # Quoted nested includes search beside their parent first. Mirror all
    # consumers of the adapted state header so none bypass the host overlay.
    native = {'dMgState_c.h', 'daObj_Mip_Key_c.h', 'daObjCannonShutter_c.h',
              'daObjHatenaBlock_c.h'}
    while True:
        expanded = native | {p.name for p, text in headers.items() if any(
            '#include "' + name + '"' in text for name in native)}
        if expanded == native: break
        native = expanded
    for source, text in headers.items():
        if not PATTERN.search(text) and not BASE_DTOR.search(text) and source.name not in native:
            continue
        transformed, rewrites = PATTERN.subn("operator new(size_t", text)
        transformed, dtor_rewrites = BASE_DTOR.subn("", transformed)
        if source.name in ('daObjCannonShutter_c.h', 'daObjHatenaBlock_c.h'):
            # MSVC does not reuse the two bytes of dBgActor tail padding.
            transformed = transformed.replace('    u8  pad_31e[0x2];', '')
        if source.name == 'daObj_Mip_Key_c.h':
            transformed = transformed.replace('typedef int (daObj_Mip_Key_c::*MipKeyPMF)();',
                'typedef int (*MipKeyPMF)(daObj_Mip_Key_c *);')
        if source.name == 'dMgState_c.h':
            anchor = 'typedef void (dMgState_c::*Callback)();'
            if transformed.count(anchor) != 1: raise ValueError('Minigame callback typedef changed')
            transformed = '#include "ntr/minigame_callback.h"\n' + transformed.replace(anchor, 'typedef PcMinigameCallback<> Callback;')
        if rewrites or dtor_rewrites or source.name in native:
            transformed = "#include <stddef.h>\n" + transformed
            target = output / source.relative_to(include)
            target.parent.mkdir(parents=True, exist_ok=True)
            if not target.exists() or target.read_text(encoding="utf-8") != transformed:
                target.write_text(transformed, encoding="utf-8")
            count += 1
    stamp.parent.mkdir(parents=True, exist_ok=True)
    stamp.write_text(f"generated {count} host header overlays\n", encoding="ascii")
    print(f"host headers: {count} build-only ABI header overlays generated")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

