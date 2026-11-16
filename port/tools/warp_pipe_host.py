"""Bind the original warp pipe and fortress tower resources on PC."""

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
    tower_profile = struct.unpack_from("<IHHIiiii", rom, 0x0214E118 - base)
    assert tower_profile == (0x02148F2C, 49, 202, 0, 0x300000, 0x800000, 0x1000000, 0)
    tower_resources = struct.unpack_from("<III", rom, 0x0214E188 + 2 * 12 - base)
    assert tower_resources == (0x0214E700, 0x0214E6E0, 0x021136F4)
    fortress = (root / "extracted/overlays/overlay_0015.bin").read_bytes()
    assert struct.unpack_from("<4sHH", fortress, 0x021136F4 - 0x021111A0) == (b"CLPS", 8, 1)

    text = lazy_handles(text, ("SimpleBgModelFilePtr", "SimpleBgClsnFilePtr"))
    old = "extern FileRow data_ov102_0214e188[];\nextern FileRow data_ov102_0214e18c[];"
    assert text.count(old) == 1
    text = text.replace(old, "extern unsigned char data_ov002_0210d814[];")
    # The canonical handles live below the methods in retail order.
    anchor = "int daObjSimpleBg_c::CleanupResources()"
    text = text.replace(anchor, "extern SimpleBgModelFilePtr data_ov102_0214e718;\n"
                        "extern SimpleBgClsnFilePtr data_ov102_0214e6f8;\n"
                        "extern SimpleBgModelFilePtr data_ov102_0214e700;\n"
                        "extern SimpleBgClsnFilePtr data_ov102_0214e6e0;\n\n" + anchor, 1)
    for column, pipe, tower, count in (("188", "718", "700", 2), ("18c", "6f8", "6e0", 3)):
        field = rf"data_ov102_0214e{column}\[(?:idx|mVariant)\]\.file"
        handle = f"(mVariant == 2 ? &data_ov102_0214e{tower} : &data_ov102_0214e{pipe})"
        text, replacements = re.subn(field, handle, text)
        assert replacements == count
    old = "((ClpsRow *)data_ov102_0214e190)[idx].block"
    assert text.count(old) == 2
    text = text.replace(old, "surface")
    anchor = "int daObjSimpleBg_c::InitResources()\n{"
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + """
    assert(actorID == 49 || actorID == 298);
    auto *surface = reinterpret_cast<CLPS_Block *>(data_ov002_0210d814);
    if (actorID == 49) {
        surface = static_cast<CLPS_Block *>(pc_level_overlay_address(7, 0x021136f4));
        if (!surface) return 0;
        static const bool ready = [] {
            data_ov102_0214e700.Construct(0x59d);
            data_ov102_0214e6e0.Construct(0x59e);
            return true;
        }();
        (void)ready;
    } else {
        static const bool ready = [] {
            data_ov102_0214e718.Construct(0x461);
            data_ov102_0214e6f8.Construct(0x462);
            return true;
        }();
        (void)ready;
    }
""", 1)
    return '#include "hal/level_overlay.h"\n' + observe(
        text, "daObjSimpleBg_c", "variant", ["int(mVariant)"])
