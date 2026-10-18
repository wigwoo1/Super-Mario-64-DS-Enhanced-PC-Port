# Super Mario 64 DS Enhanced PC Port

An **unfinished, experimental native Windows PC port**, based on
[tangosdev/sm64ds-decomp](https://github.com/tangosdev/sm64ds-decomp).
This repository contains the current development source. It is not a complete
game release, and planned features are not promises of current functionality.

The native game uses recovered game code and a Windows hardware layer. It
requires your own **unmodified European Rev 0 cartridge dump**. No ROM, extracted
game assets, personal saves, or proprietary compiler is included here.

## Current state

- Native original title/file menus, course entry and selected gameplay actors.
- Native rendering with resolution settings up to 3840x2160, controller input,
  a minimap, and PC settings work already present in the source.
- Wanted minigame and pause/settings work; the remaining game is incomplete.
- The newest Buddy/cannon-shutter integration is diagnostic: **Yoshi can still
  rise above the ground incorrectly**. The cannon-opening sequence is unverified.
- Browser Download Play, complete multiplayer, and the requested AI texture
  packs remain development plans. See the detailed plans in `port/docs/`.

## Downloading a development build

Use the repository's **Releases** page. Build kits contain precompiled native
code, with ROM-derived data objects omitted. They are developer previews, not
ready-to-run game executables. A local assembly step recreates those objects
from your own ROM and links the native game on your computer.

Requirements: Windows, Python 3, `ndspy==4.2.0`, and Visual Studio Build Tools
with **Desktop development with C++** (x86 tools and Windows SDK). The release
archive contains the matching source; extract it to a short writable path.

1. Install those requirements. Install ndspy with `py -3 -m pip install ndspy==4.2.0`.
2. Drag your ROM onto `ASSEMBLE-PC.cmd` in the extracted build-kit folder.
3. Once local assembly succeeds, double-click `Play.cmd`.

The ROM must have SHA256
`ce5829aaa79f06c67140ed1f45cbca738b0a9d0180538c8c1498472d859db032`.
Keep your normal saves separate when experimenting with previews.

## Source and verification

The original decompilation source lives in `src/` and `include/`; host changes
live in `port/` and generated local copies. `SOURCE-MANIFEST.json` records the
published source hashes. Each build kit records its exact source commit,
precompiled-object hashes, and generated-data hashes.

The October 18 development checkpoint compiled and passed 52/52 CTest checks
and 8/8 Buddy adapter checks. Runtime checks found the collision bug above;
passing component tests does not establish complete gameplay compatibility.

For a full source build, prepare your local ROM extraction and asset catalog,
then use `port/build-port.cmd` for the component checks and build the explicit
`sm64ds_pc` CMake target for the game. ROM-free symbol lists in `port/build-support/`
supply the current indirect data link closure on a fresh checkout. The native
build does not need the proprietary original DS compiler.

## Credits and license

Original decompilation: Tango and the contributors listed in [CREDITS.md](CREDITS.md)
and the attribution files. The upstream [README](README.upstream.md) is retained.
Source is distributed under the original [MIT license](LICENSE). Nintendo game
assets are not covered by that license and are supplied locally by the user.
This is an independent fan development project and is not affiliated with Nintendo.
