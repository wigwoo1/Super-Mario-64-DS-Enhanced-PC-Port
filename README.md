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
- Current source fixes Yoshi's false collision hits and verifies native Buddy
  dialogue, cannon-shutter opening, camera return and restored controls.
  Repeat conversations and the open cannon flag also survive the original
  Save/Quit flow and a fresh-process reload.
  Current source also integrates the first Battlefield cannon: Yoshi can enter,
  aim horizontally/vertically, fire along the chosen direction and regain control.
  Two consecutive rides and a third after the original course exit/re-entry
  have also been checked.
  A second Battlefield cannon has since passed entry, aiming, firing and landing.
  Its approach exposed a first Mario-cap intro crash, now repaired in the current
  source. An original mission-1 water-bomb encounter now verifies cannon launches,
  repeated drops, bouncing, body/fragment damage and safe recovery. Other placements
  and the sound emitter/audio backend
  remain unfinished or unverified.
  An original mission-1 exit/re-entry replay also verifies removal of active
  water-bomb actors, painting reload, fresh cannon/dropper actors and renewed firing.
  The `dev-2026-10-19` kit contains the earlier Buddy checkpoint and predates
  cannon riding. The initial `dev-2026-10-18` kit retains its documented collision bug.
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
and 8/8 Buddy adapter checks. The later Buddy checkpoint also passed a
6,000-frame grounded conversation replay and a 7,500-frame mission-2 cannon
opening replay. See `port/docs/native-buddy-cannon-verified-2026-10-18.txt`.
The persistence checkpoint additionally verified repeat conversations and
Save/Quit/reload across 7,800- and 7,500-frame replays; see
`port/docs/native-buddy-persistence-verified-2026-10-18.txt`.
These checks do not establish complete gameplay compatibility.
The cannon checkpoint additionally passed 53/53 CTests, four cannon adapter
checks and a 7,500-frame original-menu/Buddy opening/cannon aim-and-fire replay.
See `port/docs/native-cannon-integration-2026-10-19.txt` for exact scope and limits.

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
