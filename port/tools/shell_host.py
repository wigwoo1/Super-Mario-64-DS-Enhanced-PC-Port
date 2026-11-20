"""Bind the recovered rideable shell actor to the native PC runtime."""
import struct


def transform(root, text):
    rom = (root / "extracted/overlays/overlay_0102.bin").read_bytes()
    base = 0x02148a80
    assert struct.unpack_from("<IHHIiiii", rom, 0x0214e62c - base) == (
        0x0214d6b4, 285, 151, 2, 0x64000, 0x64000, 0x1000000, 0x640000)
    assert struct.unpack_from("<II", rom, 0x0214d70c - base) == (0x0210da18, 0x0210da20)
    callbacks = {
        "0214ea48": (0x0214d0bc, 0x0214d044),
        "0214ea58": (0x0214d020, 0x0214cfe4),
        "0214ea68": (0x0214d1b8, 0x0214d1b0),
        "0214ea78": (0x0214d148, 0x0214d114),
    }
    cartridge_order = (0x0214cfe4, 0x0214d044, 0x0214d1b8, 0x0214d020,
                       0x0214d1b0, 0x0214d0bc, 0x0214d148, 0x0214d114)
    assert struct.unpack_from("<16I", rom, 0x0214e5d4 - base) == tuple(
        value for callback in cartridge_order for value in (callback, 0))

    text = text.replace("int (daShl_c::*ShlStateFn)()", "int (*ShlStateFn)(daShl_c *)")
    text = text.replace("(this->*st->tick)()", "st->tick(this)")
    text = text.replace("(this->*st->enter)()", "st->enter(this)")
    for pair in callbacks.values():
        for address in pair:
            method = f"func_ov102_{address:08x}"
            old = f"&daShl_c::{method}"
            assert text.count(old) == 1
            text = text.replace(old, f"+[](daShl_c *shell) {{ return shell->{method}(); }}")

    text = text.replace("char rg[0x50];", "dBgCh_Gnd rg;")
    text = text.replace("        _ZN9dBgCh_GndC1Ev(rg);\n", "")
    text = text.replace("_ZN5dBgCh19StartDetectingWaterEv(rg)", "rg.StartDetectingWater()")
    text = text.replace("_ZN9dBgCh_Gnd12SetObjAndPosERK7Vector3P8dActor_c(rg, (Vector3 *)&pv[3], this)",
                        "rg.SetObjAndPos(Vector3{pv[3], pv[4], pv[5]}, this)")
    text = text.replace("_ZN9dBgCh_Gnd10DetectClsnEv(rg)", "rg.DetectClsn()")
    text = text.replace("(int *)(rg + 0x14)", "reinterpret_cast<int *>(&rg.surface)")
    text = text.replace("(unsigned int *)(rg + 0x14)", "reinterpret_cast<unsigned int *>(&rg.surface)")
    text = text.replace("*(int *)(rg + 0x44)", "rg.clsnY")
    text = "\n".join(line for line in text.splitlines() if "_ZN9dBgCh_GndD1Ev(rg);" not in line) + "\n"
    text = text.replace("*((u8 *)this->mCarrier + 0x6de)", "static_cast<Player *>(this->mCarrier)->mIsAirborne")
    for index, axis in enumerate("xyz", 9):
        text = text.replace(f"mat4x3.m[{index}]", f"mat4x3.t.{axis}")
    text = text.replace("    mModel.Render(0);", "    pc_render_probe::Scope probe(this, 0, &mModel.mat4x3);\n"
                        "    mModel.Render(0);", 1)
    return ('#include <cstddef>\n#include "Player.h"\n#include "dBgCh_Gnd.h"\n'
            '#include "hal/actor_render_probe.h"\n' + text + """
extern "C" {
extern SharedFilePtr data_ov002_0210da18;
extern SharedFilePtr data_ov002_0210da20;
SharedFilePtr *data_ov102_0214d70c[] = {&data_ov002_0210da18, &data_ov002_0210da20};

void pc_init_shell_resources() {
    static const bool initialized = [] {
        data_ov002_0210da18.Construct(0x3c7);
        data_ov002_0210da20.Construct(0x3c8);
        return true;
    }();
    (void)initialized;
}
}
static_assert(sizeof(daShl_c) == 0x3e0);
static_assert(offsetof(daShl_c, mDespawnTimer) == 0x3c6);
static_assert(sizeof(ShlState) == 8);
""")
