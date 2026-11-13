"""Bind the original DOKAN model, collision mesh, and surface table on PC."""

import re
import struct

from battlefield_platform_host import lazy_handles, observe


def transform(root, text):
    overlay = root / "config/arm9/overlays/ov102/delinks.txt"
    base = int(re.search(r"\.text\s+start:0x([\da-f]+)", overlay.read_text())[1], 16)
    rom = (root / "extracted/overlays/overlay_0102.bin").read_bytes()
    profile = struct.unpack_from("<IHHIiiii", rom, 0x0214E134 - base)
    assert profile == (0x02148EFC, 298, 187, 2, 0, 0x200000, 0x1000000, 0)
    resources = struct.unpack_from("<III", rom, 0x0214E188 + 3 * 12 - base)
    assert resources == (0x0214E718, 0x0214E6F8, 0x0210D814)

    text = lazy_handles(text, ("SimpleBgModelFilePtr", "SimpleBgClsnFilePtr"))
    old = "extern FileRow data_ov102_0214e188[];\nextern FileRow data_ov102_0214e18c[];"
    assert text.count(old) == 1
    text = text.replace(old, "extern unsigned char data_ov002_0210d814[];")
    # DOKAN is the only registered variant. Its two canonical handles live
    # alongside the other original handle definitions below the methods.
    anchor = "int daObjSimpleBg_c::CleanupResources()"
    text = text.replace(anchor, "extern SimpleBgModelFilePtr data_ov102_0214e718;\n"
                        "extern SimpleBgClsnFilePtr data_ov102_0214e6f8;\n\n" + anchor, 1)
    for column, handle, count in (("188", "718", 2), ("18c", "6f8", 3)):
        field = rf"data_ov102_0214e{column}\[(?:idx|mVariant)\]\.file"
        text, replacements = re.subn(field, f"(&data_ov102_0214e{handle})", text)
        assert replacements == count
    old = "((ClpsRow *)data_ov102_0214e190)[idx].block"
    assert text.count(old) == 2
    text = text.replace(old, "reinterpret_cast<CLPS_Block *>(data_ov002_0210d814)")
    anchor = "int daObjSimpleBg_c::InitResources()\n{"
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + """
    assert(actorID == 298);
    static bool files_ready = false;
    if (!files_ready) {
        data_ov102_0214e718.Construct(0x461);
        data_ov102_0214e6f8.Construct(0x462);
        files_ready = true;
    }
""", 1)
    return observe(text, "daObjSimpleBg_c", "variant", ["int(mVariant)"])
