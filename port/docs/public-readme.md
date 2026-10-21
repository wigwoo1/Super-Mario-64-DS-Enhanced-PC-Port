# Super Mario 64 DS Enhanced PC Port

An **unfinished, experimental native Windows PC port**, based on
[tangosdev/sm64ds-decomp](https://github.com/tangosdev/sm64ds-decomp).
This repository contains the current development source. It is not a complete
game release, and planned features are not promises of current functionality.

The native game uses recovered game code and a Windows hardware layer. It
requires your own **unmodified European Rev 0 cartridge dump**. No ROM, extracted
game assets, personal saves, or proprietary compiler is included here.

## Current state

- Native original title/file flow, selected courses and actors, Power Star
  rewards, course-clear behavior, Wanted, and portions of the Rec Room work.
- Controller navigation is verified for several original title, file, pause,
  Game Options, course-clear and minigame menus. Physical Xbox hardware and all
  menu/game combinations have not been accepted.
- The Windows renderer offers resolutions up to 3840x2160, a repositionable
  minimap, and other PC settings. Render-distance presets have a verified native
  4K distant-Goomba body/shadow case and a 720p Bob-omb body/shadow and red-coin
  body case. Coin-shadow pixels, Chain Chomp/fence integration and broader actor
  coverage remain incomplete. Game logic remains at
  the original scene rate; higher presentation targets repeat frames, and
  interpolation is future work.
- Native audio can be enabled with `SM64DS_PC_NATIVE_AUDIO=1`. It plays original
  sequences and effects in selected tested scenes, but remains opt-in and
  diagnostic. Listening fidelity, spatial effects, shared voice/channel
  behavior and coverage across the game remain incomplete.
- Original title/course entry, selected cannon and water-bomb encounters,
  repeat Star credit, pause/re-entry and selected audio flows have isolated
  runtime checks. See the dated `port/docs/` checkpoints for exact coverage and
  limitations; no single checkpoint establishes a complete port.
- Native Chain Chomp, its stump/fence release and Star reward, plus castle and
  Battlefield tree visibility/reload, have selected isolated runtime checks.
  The stay-after-Star option and free camera also have bounded tests, but broader
  all-course acceptance remains incomplete. HD texture packs, online VS and
  browser Download Play remain planned; requested AI texture generation and
  replacement also remain future work.

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
