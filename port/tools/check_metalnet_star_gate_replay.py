"""Verify the isolated zero-star and 150-star castle-cannon gate replays."""

import argparse
import csv
import pathlib
import re
import struct
import zlib


def read_case(path: pathlib.Path):
    assert (path / "exit-code.txt").read_text().strip() == "0", f"{path}: game did not exit cleanly"
    log = (path / "stderr.log").read_text(errors="replace")
    states = list(csv.DictReader((path / "state.csv").open(newline="")))
    assert states, f"{path}: missing runtime state capture"
    assert (path / "ground-12000.bmp").is_file(), f"{path}: missing cannon-area screenshot"
    return log, states


def census(log: str, tick: int):
    prefix = f"[native actor census] tick={tick} "
    return [line for line in log.splitlines() if line.startswith(prefix)]


def valid_150_star_fixture(path: pathlib.Path):
    raw = bytearray(path.read_bytes())
    assert len(raw) == 92 and raw[:8] == b"SM64PC01", "bad test-save envelope"
    slot, size, checksum, reserved = struct.unpack_from("<4I", raw, 8)
    assert (slot, size, reserved) == (0, 68, 0), "wrong test-save slot or size"
    struct.pack_into("<I", raw, 16, 0)
    assert zlib.crc32(raw) == checksum, "test-save checksum mismatch"
    payload = raw[24:]
    assert payload[:4] == b"8000" and payload[0x41] == 3, "test save is not Yoshi File A"
    stars = sum(value.bit_count() for value in payload[0x14:0x32])
    assert stars == 150, f"test save has {stars} stars, expected 150"


def verify(low: pathlib.Path, high: pathlib.Path):
    low_log, low_states = read_case(low)
    high_log, high_states = read_case(high)
    low_census = census(low_log, 10000)
    high_census = census(high_log, 12000)
    assert any("id=207 " in row for row in low_census), "zero-star run did not spawn the castle grounds cannon"
    assert any("id=339 " in row and "param=0000ff01 " in row for row in low_census), \
        "zero-star run did not spawn the gated metal net"
    assert any("id=207 " in row for row in high_census), "150-star run did not spawn the cannon"
    assert not any("id=339 " in row and "param=0000ff01 " in row for row in high_census), \
        "150-star run still spawned the gated metal net"
    assert any(int(row["power_star_total"]) == 0 for row in low_states), "low-star run was not a zero-star save"
    assert any(int(row["power_star_total"]) == 150 for row in high_states), "150-star save was not loaded"
    assert any(row["player_state"] == "St_Wait_Main" and row["level"] == "1" for row in low_states), \
        "zero-star player did not remain grounded in the castle grounds"
    assert any(row["player_state"] == "St_Cannon_Main" and row["level"] == "1" for row in high_states), \
        "150-star player did not enter the castle grounds cannon"
    valid_150_star_fixture(high / "saves" / "slot1.sav")
    return {
        "status": "PASS",
        "zero_star": "metal net present; Yoshi remains grounded outside cannon",
        "150_stars": "gated metal net absent; Yoshi enters cannon state",
        "screenshots": [str(low / "ground-12000.bmp"), str(high / "ground-12000.bmp")],
        "user_saves_used": False,
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zero_star_case", type=pathlib.Path)
    parser.add_argument("star150_case", type=pathlib.Path)
    args = parser.parse_args()
    import json
    print(json.dumps(verify(args.zero_star_case, args.star150_case), indent=2))
