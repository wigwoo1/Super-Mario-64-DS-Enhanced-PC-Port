"""Adapt the original Dire Dire Docks submarine resource handles for PC."""

import re
import struct

from battlefield_platform_host import lazy_handles


def transform(root, text):
    cfg = (root / "config/arm9/overlays/ov026/delinks.txt").read_text()
    base = int(re.search(r"\.text\s+start:0x([\da-f]+)", cfg)[1], 16)
    rom = (root / "extracted/overlays/overlay_0026.bin").read_bytes()
    assert struct.unpack_from("<IHHIiiii", rom, 0x02113C48 - base) == (
        0x02111A70, 90, 251, 0, 0, 0x1000000, 0, 0
    )
    assert struct.unpack_from("<4sHH", rom, 0x02112FD4 - base) == (b"CLPS", 8, 4)
    text = lazy_handles(text, ("SubmarineModelFilePtr", "SubmarineCollisionFilePtr"))
    anchor = "s32 daObjWlSubmarine_c::InitResources() {"
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + """
  static bool files_ready = false;
  if (!files_ready) {
    Submarine_ModelFile.Construct(0x6dc);
    Submarine_ClsnFile.Construct(0x6dd);
    files_ready = true;
  }
""", 1)
    anchor = "  mMeshCollider.Enable(this);"
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + """
  if (std::getenv("SM64DS_PC_BATTLEFIELD_PLATFORM_TRACE")) {
    std::fprintf(stderr, "[native Docks submarine] tick=%llu uid=%u pos=%d,%d,%d collision=%d\\n",
        static_cast<unsigned long long>(ntr::rt_frame()), unsigned(uniqueID),
        mPosX, mPosY, mPosZ, int(mMeshCollider.IsEnabled()));
  }
""", 1)
    return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n' + text
