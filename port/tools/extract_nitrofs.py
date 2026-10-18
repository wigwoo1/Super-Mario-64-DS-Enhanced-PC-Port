#!/usr/bin/env python3
"""Extract the supplied ROM's named NitroFS files for the native PC host.

The files are local build inputs under extracted/ (gitignored); this tool does
not copy ROM data into the source tree or modify the cartridge image.
"""

import argparse
import pathlib
import sys


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=pathlib.Path, help="your own .nds dump")
    parser.add_argument(
        "--out", type=pathlib.Path,
        default=pathlib.Path(__file__).resolve().parents[2]
        / "extracted" / "dsd" / "files",
        help="output directory (default: repository extracted/dsd/files)",
    )
    args = parser.parse_args()
    if not args.rom.is_file():
        parser.error(f"ROM not found: {args.rom}")

    try:
        import ndspy.rom
    except ImportError:
        sys.exit("ndspy not installed. Run: pip install ndspy")

    rom = ndspy.rom.NintendoDSRom.fromFile(str(args.rom))
    args.out.mkdir(parents=True, exist_ok=True)
    total = 0
    count = 0
    for file_id, payload in enumerate(rom.files):
        name = rom.filenames.filenameOf(file_id)
        if name is None:
            continue
        relative = pathlib.PurePosixPath(name.replace("\\", "/"))
        if relative.is_absolute() or ".." in relative.parts:
            raise ValueError(f"unsafe NitroFS path for FAT id {file_id}: {name!r}")
        target = args.out.joinpath(*relative.parts)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(payload)
        total += len(payload)
        count += 1

    print(f"Extracted {count:,} NitroFS files ({total:,} bytes) to {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
