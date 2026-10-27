# Super Mario 64 DS Enhanced PC Port

An **unfinished, experimental native Windows PC port**, based on
[tangosdev/sm64ds-decomp](https://github.com/tangosdev/sm64ds-decomp).
This repository contains the current development source. It is not a complete
game release, and planned features are not promises of current functionality.

The native game uses recovered game code and a Windows hardware layer. It
requires your own **unmodified European Rev 0 cartridge dump**. No ROM, extracted
game assets, personal saves, or proprietary compiler is included here.

## Current state

- Selected Battlefield question blocks have native underside-hit and reward
  checks for Yoshi's fire, Mario's wings, Luigi's vanish and Wario's metal
  abilities, including course exit/re-entry. Mario's active-wing refresh also
  passes after fixing an incomplete native music-call signature. The original
  question-block activation switch now presses, animates, saves and reloads;
  its earned save enables a real Battlefield block reward. Initial switch
  course entry used the diagnostic picker; original castle access and broader
  block contents/placements remain unverified. These tests used isolated saves
  and muted audio. See the October27 question-switch and character reports in
  `port/docs/` for exact scope.
- The Secret Slide's opaque black background replaces the incorrect green
  palette backdrop. Mario's original castle route through Peach's window,
  Star-box hit, Star pickup, Save & Continue, reload and repeat without duplicate
  credit have muted isolated checks. The timer now uses the game's colorful
  number sprites at the top center, and the original timer start/stop behavior
  has a selected runtime check. The checkerboard finish stripe and timed Star
  are visible in the current Secret Slide end replay. Yoshi's original box hit,
  Star collection, saving and repeat collection without duplicate credit also
  have isolated checks. Full course completion and timed-Star save/reload
  remain unverified.
  Rear-plane, fog and all-course rendering still need coverage.
- The October24 checkpoint corrects the native OS clock units: the original
  stopwatch conversion now agrees with real elapsed time in a focused Windows
  test. Timer rendering is verified, while pause, high-score and timed-Star
  flows still need gameplay acceptance. Battlefield lifts, the tilting bridge and switch
  gates have selected gameplay checks. Yoshi boards and dismounts both original
  orange lifts; one also passes original menu exit, painting re-entry and a
  repeated ride. In Mission2, Mario reaches the original switch from the rear,
  activates it, opens both shortcut gates and walks through. After original Exit
  Course and painting re-entry, the switch and both gates reset to their original
  off/closed states; Mario can press and pass through them again. Wider variants
  remain unverified. Koopa's original player win, winning dialogue, Star spawn,
  actual reward collection and original Save & Continue pass selected tests with
  a CRC-valid disk save. A fresh-process reload and repeat pickup also pass
  without duplicate Star credit.
  A selected Mission2 Mario crossing of the tilting bridge now passes with
  native contact, tilt, player carry and stable landing on the far bank.
  Yoshi now also crosses in both directions with native grounded contact and
  stable landings on both banks, including a selected original course re-entry.
  Other characters and broader course-reload coverage remain unverified.
- Native original title/file flow, selected courses and actors, Power Star
  rewards, course-clear behavior, Wanted, and portions of the Rec Room work.
- Original particle rendering has selected brick-debris and opening sleep-bubble
  pop checks. Mario breaks a red brick and collects its three coins; Yoshi's
  original character restriction is preserved. A fresh-file intro shows the
  original yellow pop and smaller bubbles, cleanup, and return of Yoshi's normal
  control. Wider effects, renderers and resolutions still need acceptance.
- The native wooden crate has a selected authored Hazy Maze Cave check: Yoshi
  ground-pounds it, original debris renders and all three released coins are
  collected. A same-session distance/off-screen respawn and second Yoshi
  ground-pound also pass with no extra coins. Yoshi can capture this crate,
  swallow it and create its original cuboid egg containing three coins. Mario
  collects all three coins from an ordinary throw. Luigi and Wario also carry,
  throw and break the original HMC crate in muted isolated checks. Yoshi's HMC
  crate resets after course re-entry. Long-distance egg targeting, other crate
  placements and broader course coverage remain unverified.
- Five original castle signposts spawn; Yoshi's selected sign read, dialogue
  closure and two ground pounds have muted runtime checks. Black sign text now
  appears on a light parchment card, while white Lakitu dialogue retains its
  dark translucent card. Dialogue appearance was checked on D3D11; software
  renderer appearance and wider dialogue coverage remain unverified.
- Controller navigation is verified for several original title, file, pause,
  Game Options, course-clear and minigame menus. Physical Xbox hardware and all
  menu/game combinations have not been accepted. Recent player feedback reports
  that A does not confirm selections in the in-game pause menu. The October22
  checkpoint adds a keyboard/controller Controls page through Controller Modes,
  configurable gameplay bindings and separate stick deadzones, with selected
  muted menu/gameplay checks. It does not establish every pause-menu action;
  physical Nintendo Switch Pro acceptance remains pending.
- The Windows renderer offers resolutions up to 3840x2160, a repositionable
  minimap, and other PC settings. Render-distance presets have a verified native
  4K distant-Goomba body/shadow case and a 720p Bob-omb body/shadow and red-coin
  body case. Internal resolution can change independently of window size and
  includes Native DS (256x192), 360p (640x360), and 480p (854x480). All three
  lower presets now pass muted castle runtime captures at their requested
  internal sizes; window preference remains fixed at 1280x720. Dedicated video,
  window-independence and minimap-HUD CTests pass.
  Coin-shadow pixels and broader actor coverage remain incomplete. Game logic remains at
  the original scene rate; higher presentation targets repeat frames, and
  interpolation is future work.
- Native audio can be enabled with `SM64DS_PC_NATIVE_AUDIO=1`. It plays original
  sequences and effects in selected tested scenes, but remains opt-in and
  diagnostic. Listening fidelity, spatial effects, shared voice/channel
  behavior and coverage across the game remain incomplete.
  `SM64DS_PC_AUDIO_MUTED=1` silences device output while keeping the engine
  running for tests. Castle footsteps/birds, coin sound completion, and two
  Battlefield cannon positions have bounded checks; this does not establish
  listening accuracy or complete sound compatibility. Recent player feedback
  reported missing title/start/selection and save-selection sounds. The October22
  checkpoint connects the original menu's SDK events and handle lifetimes;
  selected muted title/save/options checks now produce non-silent PCM. The stage
  sound-bank table also retains its correct shared layout. Lakitu cloud timbre,
  some mixed-scene clipping and broader audio fidelity remain open.
  An optional Gameplay setting plays Rec Room music while paused, retaining the
  current song's sample position for resume; selected castle/Battlefield/Wanted
  flows and an exact resumed-PCM comparison have muted/offline checks.
- Original title/course entry, selected cannon and water-bomb encounters,
  repeat Star credit, pause/re-entry and selected audio flows have isolated
  runtime checks. See the dated `port/docs/` checkpoints for exact coverage and
  limitations; no single checkpoint establishes a complete port.
- The original 20-, 40- and 60-Star Toad placements have selected isolated
  runtime checks. The 20-Star reward was collected, saved, reloaded and repeated
  without duplicate credit. The 60-Star reward was also collected, saved,
  reloaded and repeated without duplicate credit using a synthetic 60-Star
  prerequisite save. The companion/variant branch still needs separate checks.
- The original upstairs Star Gate now renders and follows its 50-Star/Mario
  rule. Yoshi's denial, Mario's eligible entry, return of normal player control,
  and the original 60-Star Toad Star's collection/save/reload/repeat path have
  muted isolated checks. This is selected coverage, not a complete castle or
  all-course gameplay acceptance.
- Native Chain Chomp, its stump/fence release and Star reward, plus castle and
  Battlefield tree visibility/reload, have selected isolated runtime checks.
  The October22 audio checkpoint restores Battlefield's bank33 for Chain Chomp
  bark/bounce; isolated cue renders are non-silent and gameplay events reach the
  correct bank. Listening accuracy is unverified. A diagnostic approach replay
  stalls at a bridge wall and does not establish full encounter acceptance.
  The Battlefield flower warp works in both directions, preserves its original
  destination/fade/cooldown behavior, and survives course exit and re-entry in
  a muted isolated replay. The original Lethal Lava Land painting can also be
  entered again after course exit; both moving lava bridges unload and respawn
  with working collision for repeat crossings. These results use isolated
  test saves and selected courses; other course and mountain warp points were
  not tested.
  A selected open-ground camera test confirms that stopping Yoshi no longer
  pulls the original camera closer. Free-camera movement has software replay
  checks; physical Nintendo Switch Pro behavior remains unverified.
  The October22 checkpoint adds25..300% free-camera sensitivity, independent X/Y
  inversion and Off/Gentle/Smooth acceleration. Selected menu/gameplay checks
  pass; physical Nintendo Switch Pro acceptance remains pending.
  Broader all-course acceptance remains incomplete. Star, coin, health and life counters
  use original game artwork beside the minimap, scaling with its size. The counter
  layer now renders at the selected internal resolution, including 4K; original
  DS icon detail and the minimap itself remain source-resolution. Selected
  English/Yoshi coin collection, contact damage, large-map and map-off checks
  pass with muted output; other character/language coverage remains incomplete. HD texture packs, online VS and browser Download Play
  remain planned; requested AI texture generation and replacement also remain
  future work.

## Downloading a development build

Use the repository's **Releases** page. Build kits contain precompiled native
code, with ROM-derived data objects omitted. They are developer previews, not
ready-to-run game executables. A local assembly step recreates those objects
from your own ROM and links the native game on your computer.

**Packaging correction, October22:** use only a corrected format-2 kit. An audit
found ROM-derived camera configuration data in older precompiled kit objects;
those downloads did not meet the intended ROM-free rule. Format2 omits every reviewed
generated data object, including camera, minigame and stage-sound
tables. They are regenerated locally from your cartridge. A source checkout or
an older kit with only10 omitted objects is not equivalent to a corrected kit.

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

