# Super Mario 64 DS Enhanced PC Port

An **unfinished, experimental native Windows PC port**, based on
[tangosdev/sm64ds-decomp](https://github.com/tangosdev/sm64ds-decomp).
This repository contains the current development source. It is not a complete
game release, and planned features are not promises of current functionality.

The native game uses recovered game code and a Windows hardware layer. It
requires your own **unmodified European Rev 0 cartridge dump**. No ROM, extracted
game assets, personal saves, or proprietary compiler is included here.

After assembling a native kit with your ROM, use `PlayWithAudio.cmd` to enable
the experimental native audio, or `Play.cmd` for the existing launch behavior.
Audio listening accuracy and complete gameplay remain unverified.

## Current state

- Native attacking books and hidden book generators now use their original
  profiles. Selected muted tests cover all four corridor generators firing,
  one attacking book's activation/damage and Mario defeating it for a blue coin
  worth five coins. Original course exit/re-entry restores fresh book instances;
  the selected book can be defeated and collected again. Other placements,
  characters and repeated corridor firing remain unverified.
- Native Boo, Big Boo, bookshelf and book-switch factories load their original
  resources. Selected muted tests verify the five-Boo prerequisite sequence,
  Big Boo defeat and Star spawn, and a separate three-book puzzle opening.
  The original courtyard entrance-Boo/cage route and course re-entry now pass
  with an isolated prerequisite save. The bookshelf resets closed with all
  three switches restored. These are selected diagnostic checks, not complete
  campaign acceptance.
- Native Mad Piano and haunted chairs now spawn and render. Selected muted
  tests verify piano attacks, damage, linked-chair tipping, safe room-door
  escape, course re-entry with fresh actors and a second piano encounter.
  An unlinked chair jumps, pursues and breaks with original wood/dust effects.
  That stationary chair replay later dies; safe escape and respawning the
  destroyed chair remain unverified. Other placements/courses remain open.
- Native falling blocks and the original spinning floor now have selected
  Mario checks in Luigi's ghost-house maze. The block receives real player
  contact, shakes, drops and disables collision; normal ledge input restores
  Mario to the bank. The spinning floor renders, rotates and supports him.
  Yoshi, Mario, Luigi and Wario each cross the selected seven-tile row in both
  directions with grounded contact on every tile. The group resets after outward
  crossing; each character returns safely. Each also passes the original room
  reset door and a full scene reload, then crosses again on newly created tiles.
  This uses prepared diagnostic saves, not earned campaign entry. Other courses
  and placements remain unverified;
  see `port/docs/native-falling-block-and-spinning-floor-2026-10-27.txt`.
- The original level46 tilting trapdoor (profile160) now receives a real native
  long-jump landing, tilts, settles naturally, and allows a safe jump back to
  solid ground in a muted Wario replay. This is an isolated diagnostic route;
  earned course entry and other variants/placements remain unverified.
  The recovered staircase65 factory creates all three original steps in
  level12 missions 1 and 2. Muted tests verify all three render, child links,
  contact on all three steps and a jump onto the upper landing. Boss-triggered rise has separate selected runtime evidence. Full lifecycle
  remains open. Bookshelf158 and switch213 are connected; see
  `port/docs/native-kaidan65-integration-2026-10-28.txt` and
  `port/docs/native-trapdoor160-integration-2026-10-28.txt`.
- The original course/Star selector now uses one composed screen, with its
  character face and controls below the score. Selected Mario tests verify
  controller-style Mission 1 navigation in 4:3, inverse-mapped face-click
  confirmation for Mission 2 in 16:9, and original course re-entry. Eleven
  focused tests pass. Physical controller hardware, other character strips,
  alternate renderer runtime and arbitrary window resizing remain unverified;
  see `port/docs/native-star-select-single-screen-verified-2026-10-27.txt`.
- Both native arrow-sign types now render and block Mario in selected
  Battlefield tests. Original Exit Course/castle/painting re-entry destroys
  five old signs and loads five fresh instances at matching placements;
  repeat player collision and live mesh queries pass for each direction.
  Other courses, characters and destruction attacks remain unverified;
  see `port/docs/native-arrow-signs-verified-2026-10-27.txt`.
- Native recovery hearts now render, spin and heal in a selected Battlefield
  test. Original Exit Course, castle return and painting re-entry reload a
  fresh heart, which again restores health after real enemy damage. Testing
  exposed a native model-cache buffer sized for 64 entries instead of the
  original 100; the original capacity is restored. A muted course cycle safely
  registers 68 models, clears and reloads the cache, and preserves isolated
  saves. Earlier development builds may crash during this course exit. Other
  heart placements, characters and complete campaign coverage remain unverified;
  see `port/docs/native-healing-heart-reentry-2026-10-27.txt`.
- Native ambient emitters have selected castle playback, range, room reload
  and camera-underwater suppression/resumption checks. The original level8
  mission/collected-Star gate passes a three-case comparison with isolated
  prerequisites. Native sound-distance defaults are initialized through the
  original preset function. Nearby level8 playback, other gates, listening
  accuracy and spatial mixing remain unverified; see the October27 ambient
  reports in `port/docs/`. These are muted component checks, not full campaign
  acceptance.
- Selected Battlefield question blocks have native underside-hit and reward
  checks for Yoshi's fire, Mario's wings, Luigi's vanish and Wario's metal
  abilities, including course exit/re-entry. Mario's active-wing refresh also
  passes after fixing an incomplete native music-call signature. The original
  question-block activation switch now presses, animates, saves and reloads;
  its earned save enables a real Battlefield block reward. Original castle
  inspection entry now passes with a prepared 14-Star Mario save, while a
  13-Star save correctly stays in the castle. The original lobby light-beam
  model and action-button hint now have native checks; it is hidden below
  14 Stars or on reloading an activated save. One original front-door round trip
  reloads the beam and repeats its hint successfully. Earned prerequisite
  progression and broader block contents/placements remain unverified. These tests used isolated saves
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
  The volcano ring launches visible flame particles. A Mission 1 replay
  verifies that a falling flame hits Luigi on the ring, starts his burn
  animation, and removes one health wedge before he touches the lava.
  The flame actor retires after the hit. A follow-up replay steers Luigi
  along the ring during the burn and verifies that he regains normal
  movement and stops safely on the walkway.
  Another Luigi replay jumps from the stone ring to the crater and enters
  the volcano interior through the original transition. Luigi lands on the
  interior entry platform and crosses the lower stepping stones. One
  replay rides the first lift to the end of its rising path. Another
  backflips off the lift earlier, lands safely on the slamming slab's
  ledge, and triggers its full tip, hold, rise and reset cycle. The slab
  draws with its original collider enabled. Additional replays verify
  side collision and Luigi landing on the deployed slab. Walking off
  and attempting a jump back both led to a downhill slide. A safe
  return from the deployed slab remains unverified. A separate route
  waits for the slab to rise, crosses on fixed ground and backflips
  from the lower ledge to the raised west shelf. Luigi finishes standing
  safely on the shelf with normal control and full health. A second
  replay follows the shelf to the first coin island along the west wall.
  A longer replay makes all five island landings and collects each
  coin in order. Luigi finishes safely at full health after landing
  farther west on the fifth island. A separate replay shows the first
  interior jet hitting Luigi there, followed by his normal controls
  returning on the stone ledge. A timed replay waits for each jet to
  retract, walks across both paths safely, and jumps to the next raised
  ledge. Luigi finishes grounded with full health. The north-ramp
  replay long-jumps over the Bully, recovers from one grounded knockback
  on the far side, then crosses the ramp gap and reaches the first
  climbing pole at full health. Luigi grabs and climbs that pole, jumps
  across to the next pole's platform, and stops safely on the stone.
  A longer replay climbs the second pole, lands on the walkway, and
  collects the Hot-Foot-It into the Volcano Star. The original Save &
  Continue screen writes the Star to an isolated save and returns
  control in the castle. A fresh launch restores the earned Star and
  the total of 23. Luigi enters through the original painting and
  volcano entrance, collects the same Star again, and uses Save &
  Continue. The total stays at 23 and both isolated save files remain
  byte-identical. A separate replay on the route to the other volcano
  Star verifies Luigi landing on the second moving lift from the
  first, riding it, and jumping to the original pole. Luigi climbs
  and jumps off the first pole, lands on the ledge at the base of the
  taller pole, and regains normal control. A longer replay climbs
  the taller pole and jumps onto the shelf above it. Luigi clears
  the last two gaps, collects Inside the Volcano during the second
  jump, and lands on the Star shelf. Save & Continue writes the
  distinct Star to the isolated file and returns control in the
  castle. A fresh launch restores Inside the Volcano and 23 Stars.
  Luigi collects it again and saves with the same total and
  byte-identical isolated save files. A knockback-free passage by
  the interior Bully still needs verification.
  The original course menu opens Flaming Silver Stars from an isolated
  save prepared with the first six course Stars. All five Silver Stars,
  the reward and its camera, and the shell block spawn at their original
  placements. The same build's Mission 1 replay has none of those
  mission-specific placements. Luigi hits the item block from
  underneath, the original green shell appears, and contact puts him
  in the native riding state. The muted replay rides across the lava at
  full health before eventually taking damage on the unattended route.
  A controlled replay steers the shell through open lava and hops
  onto the eastern Silver Star. The mission counter advances to one,
  and Luigi remains mounted. The other four pickups, final award,
  and a continuous unlock playthrough remain unverified.
  The native lava surface effect produces bubbling particles in the
  exterior and inside the volcano.
  The two original lava-bubble fountains in the volcano interior each
  completed four launch, bounce, and retirement cycles during a Luigi
  replay. Both fire and smoke effects rendered. A second replay verifies
  that a jumping bubble hits Luigi on the east stone ledge and starts the
  original burn and health loss. Luigi recovers normal control on the ledge.
  The original flamethrowers now run natively. A selected Luigi replay
  shows the outdoor grate firing and verifies that the three interior
  nozzles extend and retract on their 60-update phases. The interior
  particle trace records both flame effects during the entrance sweep,
  where the star transition masks the view of the jets. On the outdoor
  grate, a second Luigi replay verifies contact, the original burn and
  health loss, and returning to normal walking on the stone path. Other
  flamethrower placements and variants still need contact coverage.
  Both original Lethal Lava Land mesh lifts spawn. A selected Luigi replay
  clears the guarded island, boards the east lift, rides its entire route
  and jumps onto the far bank. The west lift cycles through both original
  endpoints. Luigi then balances along the rolling log, jumps onto the
  raised stone bank and walks farther across it. After Exit Course and
  re-entry through the castle painting, both mesh lifts and the log spawn
  fresh. Luigi repeats the full east lift ride and safe log dismount.
  The November 19 Windows preview has separate Luigi replays for the three
  small Bullies, Big Bully, and the Power Star. The first pickup increases
  File A from 22 to 23 Stars. A repeat pickup keeps the total at 23 through
  the original Save & Continue flow and a fresh launch.
  The November 20 development replay also completes the fight and saves
  with 23 Stars and unchanged course Star flags. Its test pilot reacts to
  a charging Bully even when a stunned Bully is closer to Luigi.
  Selected Luigi replays verify Lethal Lava Land puzzle tile contact, all
  14 bonus coins spawning, and a route that collects all eight red coins.
  Luigi collects the revealed Power Star, saves, and reloads with the new
  credit in an isolated file. A second Luigi pickup leaves the total and
  saved credit unchanged. Separate Mario, Yoshi, and Wario fixtures also pass
  the eight-coin route, Star pickup, Save & Continue, and a fresh reload. The
  Yoshi and Wario replays time the jump using the moving tiles' live state.
  With Gameplay's Stay After Star setting enabled, a local Luigi replay
  also verifies the in-course save prompt, continued movement in the
  course, and retained Star credit after a fresh launch. A separate replay
  chooses Not now, continues in the course with the new Star for that session,
  and reloads the prior total from an unchanged save.
  The original gold choice cursor remains visible over the PC dialogue card.
  The native save prompt's cursor follows directional input in tested D3D11
  and Mesa llvmpipe OpenGL software sessions.
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
  pass with muted output; other character/language coverage remains incomplete.
  Local SUPIR and AI Generated 4K pack loading is available for supported
  materials. Production packs, broader asset coverage, online VS and browser
  Download Play remain open.

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

