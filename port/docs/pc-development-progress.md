## Current native build installed and launched from F: (2026-10-16)

- The current Release build was up to date; `cmake --build build/port --target sm64ds_pc --parallel 4` completed with no work required. CTest passed45/45 (25.01s).
- F: previously contained the October15 executable. Installed the October16 executable and map with SHA-256 `BBF216EC9AF7901F4050C7B8777DE5B71DBC5AD80973646DDA45F431C9D55410` and `B595BD514B385134F61E01069F85DE7B8F062898C38D0B276CD4E4DF7BAD0D94`. Previous copies are preserved at `F:\AI Games\Mario 64 DS\native-checkpoints\native-checkpoint-2026-10-16-current-build\previous-installed`. Copy hashes match. The absent user INI remains absent; ROM, user saves and launch script were left untouched.
- Launched the installed executable from F: with its matching supplied ROM and isolated QA save folder on C:. The automated title/file-select replay reached tick1900 and reports exit0. It captures the title at tick1180 and File A/B/C at tick1780 in the default1280x720 mode. Screenshot review confirms readable title and file labels; the castle background still needs visual-fidelity work. These captures verify installed startup/menu rendering only, not complete gameplay or exact DS artwork.
- Follow-up installed File A replay completed6000 ticks and exited0. The final state reports level1, intro complete, `St_Wait_Main`, and save/selected/player character IDs all3 (Yoshi); the tick5900 screenshot visibly shows Yoshi and the overlaid minimap. This verifies the installed File A new-game route through the opening to idle gameplay at1280x720. It does not verify movement, all courses, physical Xbox input, or exact rendering fidelity. QA saves remain on C:.
- Follow-up File A stick replay on the installed executable completed6000 ticks and exited0. At tick5480 Yoshi is idle at X=-4915200; partial stick magnitude1989/4096 produces `St_Walk_Main` and X=-2247260 by tick5750. After release, input magnitude returns to0, the player reaches X=965064, and returns to `St_Wait_Main` by tick5990. Character remains ID3 (Yoshi). Tick5750 screenshot shows Yoshi walking with the minimap. This verifies simulated analog input on the installed build; physical Xbox testing remains outstanding.
- Evidence: `build/installed-current-menu.stdout.log`, `build/installed-current-menu.stderr.log`, `build/port/installed-current-menu-state.csv`, and `build/port/installed-current-menu-{1180,1780}.png`. The CTest record is `build/port/Testing/Temporary/LastTest.log`. F: installation was updated, but standalone packaging and the full port remain incomplete.
- File A evidence: `build/installed-file-a.exit-code.txt`, `build/port/installed-file-a-state.csv`, and `build/port/installed-file-a-5900.png` (bottom-screen capture alongside it).
- File A stick evidence: `build/installed-file-a-stick.exit-code.txt`, `build/port/installed-file-a-stick-state.csv`, and `build/port/installed-file-a-stick-5750.png` (bottom-screen capture alongside it).

## Minimap settings regression checked with restored gameplay sprites (2026-10-16)

- Same verified executable BBF216EC9AF7901F4050C7B8777DE5B71DBC5AD80973646DDA45F431C9D55410; no source/build changes this check. A 7200-tick File B test-fixture replay exits0 and traverses Display, Visual, and Map inside the original pause Options flow, then resumes Yoshi gameplay.
- Correct hexadecimal scripted inputs select Rotate with player and toggle Hide Touch to OFF. State log confirms rotation0->1 and hide1->0; the isolated runtime INI persists both settings. Captures show readable menu pages without gameplay OBJ overlap and a restored minimap/gameplay view after resume. This verifies the settings path and persistence; physical Xbox navigation and same-heading hidden/visible minimap comparison remain pending.
- Runner input masks are hexadecimal after the colon (e.g. `100` = R, `200` = L, `10` = Right, `20` = Left). The first replay used decimal-looking masks and exited settings early; it was discarded, then the corrected replay above completed normally. Test saves are isolated; F: untouched.

## Current title and file-select artwork replay (2026-10-16)

- Same executable BBF216EC9AF7901F4050C7B8777DE5B71DBC5AD80973646DDA45F431C9D55410 completes a 1900-tick title/file-select replay with exit0. Fresh captures: `native-file-menu-original-artwork-current-check-1180.png` and `...-1780.png`.
- At1180 the title has one Mario hat emblem, the repeated blue castle artwork, and no white side borders. At1780 the original File A/B/C artwork and recovered castle background geometry are present. This is a visual regression check, not proof of exact DS color/animation fidelity. No justified new art edit was identified; F: remains unchanged.

## Original confirmation text restored (2026-10-16)

- Fixed 2D background composition over 3D according to main-engine priorities; the original erase prompt now renders at 4K.
- Build and 45/45 tests pass, including foreground priority/transparency/pause checks. Runtime prompt and No/cancel verified; both isolated test saves unchanged.
- Yes/erase, overwrite, gameplay composition regression and final F: packaging remain pending. No claim of complete DS blending/window/OBJ priority fidelity.

## Original File Options copy crash repaired (2026-10-16)

- Restored the complete original message ID table with canonical interior labels; truncated generated storage caused the dialog crash.
- Build, 45/45 tests and compiled-data audit pass. Original-menu 4K replay copies test File B to A, exits cleanly, and shows both files.
- Disk validation confirms correct slot headers, valid CRCs, equal payloads and unchanged source. Test saves only; copied-file reload, overwrite/cancel and erase remain unverified. F: installation unchanged.

## File B save and reload verified (2026-10-16)

- Fixed loaded-save menu crash: recovered constructor020c2b9c omitted its original ARM pointer return. Port-only generation restores it; original source unchanged. Five adapter tests and45/45 compiled tests pass22.74s.
- Original File B -> native pause save writes only slot2.sav plus shared minigames; both checksums pass. Fresh-process load recognizes B with zero stars, leaves A/C NEW, enters castle grounds as Yoshi and completes6000 ticks with exit0. Screenshots inspected/shown. Collected-star progression and post-load movement remain untested.
- Loaded saves skip the empty-save initial touch gate. Replay runner supports AutoConfirmPoll0 and an explicit SaveDirectory to preserve separate startup logs.
- Current SHA2568C34E93C08A8FBD59F7C28C87D3AA3589AE1C997E301AC51F780616A7A9C39DC. Full menu file management/minimap sprite cleanup/gameplay/audio/HD pack/highFPS/standalone packaging remain incomplete. F unchanged.

## Custom settings sprite overlap fixed (2026-10-16)

- Bottom-screen map sprites are excluded only while the custom PC Settings screen is active. Native background tiles and menu artwork remain; gameplay/original pause sprites restore without changing OAM data.
- Build and all45 tests pass31.25s. Same-binary original6700 replay opens settings, changes textures to75%, resumes and exits0. Screenshots6360/6680 confirm clear rows/footer and restored minimap sprites.
- Current executable SHA256 A234142043C7EC1C08879C91BB3BF4830463275D5120057A7F1EB5F409CA0C11. Evidence: pause-sprite-filter logs/captures and detailed handoff. F installation unchanged; full goal remains incomplete.

## Current checkpoint (2026-10-16)

- Latest verified executable: B8B65B0081A24DC40217ACE4940F429D725237911738E91D07B08455B160E680. Build and all 45 tests pass. Original-menu File A replay reaches idle Yoshi at 3840x2160 and exits normally.
- Menu repairs now include palette uploads, quad UVs, projection, castle grid geometry, clamp/repeat/mirror, toon/highlight shading and intensity precision. The duplicate hat emblem and observed pale polygon patches are resolved in captured frames. Saved official manual imagery supports the current castle orientation; exact animation/color fidelity remains unverified.
- Original-walk-jump replay completed 6600 ticks, exit 0: partial analog input produces St_Walk_Main, B enters airborne St_Jump_Main, then landing, walking and idle after release. Screenshots show walking after landing and final idle; airborne interval is verified by state logs. Evidence: build/original-walk-jump-summary.json and original replay captures. Session 45077 is terminal.
- SUPIR remains a plan for all textures, including characters. No production HD pack exists. Full gameplay, audio, physical controller verification, true 120/240 FPS and standalone F: packaging remain incomplete. F: installation has not received these menu fixes.
- Detailed evidence and continuation instructions: `native-menu-artwork-handoff-2026-10-16.txt`. Older sections below are chronological history and may describe issues fixed later.

## Texture clamp/repeat/mirror restored (2026-10-16)

- Preserved original per-axis texture addressing in software and GPU geometry. This removes the duplicate title-hat emblem and visible repeated road edges. Castle menu orientation/tint remains unfinished.
- Final build and45/45 CTests pass, including128 address cases checked against expected pixels in both backends. Initial byte-hash failures were traced to new structure padding and repaired without weakening tests.
-6000-frame File A replay reaches Yoshi; final padding-only binary also passes1800-frame menu replay. Actual screenshots inspected/shown. F: unchanged. Exact evidence and remaining work: native-menu-artwork-handoff-2026-10-16.txt. Keep Astra Low.
## File-selection castle geometry restored (2026-10-16)

- Nintendo's official manual confirms blue castle artwork behind the file buttons. The original grid emitter used five volatile-typedef register macros that bypassed the PC renderer. The source adapter now routes those exact macros through the existing MMIO layer; retail source is unchanged.
- Build, four focused adapter tests, actual generated grid-emitter test, all45 CTests and DSMT data audit pass. A6000-frame native4K replay selects File A, completes the intro and reaches idle Yoshi. Castle geometry increases file-menu triangles from736 to1476 and restores the missing artwork.
- Appearance remains partially verified: background orientation/tint and duplicated hat emblem need investigation. Gameplay comparison is not pixel-identical; final captured state differs only in idle timer by2 ticks. F: unchanged. See native-menu-artwork-handoff-2026-10-16.txt. Keep Astra Low.
## Original menu artwork investigation (2026-10-16)

- Palette and third-corner UV writes now reach the renderer; menu projection preserves the full title background. Original source stays untouched. Earlier 6000-frame File A replay reaches idle Yoshi with unchanged gameplay pixels.
- Latest diagnostic build passes all 45 tests and an 1800-frame native 4K file-selection replay. Castle art exists on engine B BG2; main BG1 still displays white. Added bounded original BG-list traces to investigate the actual tilemap/upload path. Duplicated hat emblem remains unresolved.
- SUPIR is planned for ALL textures in HD/Upscaled, superseding the old character exclusion. Installation and production pack remain pending.
- Evidence, current executable hash and exact next investigation: native-menu-artwork-handoff-2026-10-16.txt. F: unchanged. Keep Astra Low; full goal remains incomplete.
## Native disk progress saves and pause save action (2026-10-15)

- Replaced cartridge backup I/O with checked native records for three game files/minigames, temporary write/flush/validation, atomic replacement, valid previous-save backup and exclusive per-file locks. Corrupt unrecoverable records are preserved. Native PC records are not emulator .sav imports; game/minigame writes are sequential, not a cross-file transaction.
- Course-picker startup stages FILE 1/minigame loads before live-state changes, uses retail missing-file defaults, applies the retail mask and selects the saved character. Valid live progress remains intact; errors cancel launch. Full file selection/copy/erase UI remains unfinished.
- Native pause PC Settings now has five pages. Page5 saves FILE 1 progress with clear success/failure feedback and no held-A repeat. Original star/quit failure guards are source-adapted but full course sequences untested. Existing native touch coordinates supported; physical mouse/Xbox untested.
- Release/default build,37/37 CTest (31.91s), native-data/render-bridge/consumer audits pass. First system-temp fixture hit sandbox rename denial; identical code passes in writable build fixture, now configured for CTest. Tests cover actual retail SaveCurrentFile, all slots/minigames, child-process reload, Unicode paths, CRC/backup repair, contention and blocked writes.
- Final same-binary native4K runs:4900-tick save,2200-tick fresh reload,2200-tick damaged-primary recovery,4900-tick blocked save and1200-tick unrecoverable-read cancellation all exit0. Saved intro flags/Yoshi survive restart, backup recovers the same progress, failed writes show failure and resume, unrecoverable data stays byte-identical with no course/player spawned. Actual screenshots inspected/shown. Full gameplay/art/audio/highFPS/packaging remain incomplete.
- Notes: native-save-checkpoint-2026-10-15.txt; format: native-save-format-2026-10-15.txt; evidence: build/native-save-* and build/verify-native-save.py. Installed F: executable/ROM completes4900-tick menu save and2200-tick reload, exit0 at1280x720; original Yoshi/intro flags and valid native records survive restart. Installed audits/ROM hashes pass; exact executable hash matches, user INI absence/save files preserved. QA records are isolated in the checkpoint; installed screenshots inspected/shown. Keep Sol High; goal stays active.

## Correct Yoshi startup, restored head and draw-context texture guard (2026-10-15)

- Installed executable/F: ROM completed4200 Castle ticks, exit0, at1280x720. Saved/selected/active model remain Yoshi3; correct head/minimap screenshot inspected. Installed native-data/direct-render bridge audits and ROM hashes pass. Executable matches SHA2568822ECA2C0A6B343D80E18DE1F494491AFE0BAFA9C17B2D972EC0095AC16647C; user INI absence preserved.
- User correction exposed that earlier images showed Mario during the Yoshi opening. Pixel equality alone missed the wrong starting character. The PC course picker bypassed file selection/new-file initialization and left zero/Mario save/selection bytes. A generated title adaptation now uses retail defaults only for pristine live data and copies the valid saved character to the spawn selection. Existing valid character choices/progress are preserved; full disk save/load/file selection is still unfinished.
- Correctly spawning Yoshi exposed a missing head. The Model::Render C bridge incorrectly invoked virtual animation work after the player prepared its head transform. Explicit base dispatch restores the head. Compiled-code audit rejects the old slot5 call and verifies the new direct base target. Same-state4K comparison regains56360 green head pixels; original src/texture data are untouched.
- Per-draw Character/Object scopes support nine explicit shared coin/cap paths without changing their Unknown file roles. Native Player/Coin scopes are linked and tested; enemy/cap source transforms are checked but their gameplay remains unverified. Nested/unknown/character-protection tests pass, as do user-ROM coin-table/source audits. Native TestMap coin draw/load uses Object context and uploads the fixture once.
- Release/default builds and36/36 CTest pass (16.78s final suite), including actual retail defaults/all four valid saved-character cases. Native-data, consumer and direct-render bridge audits pass. Final4200-tick Original/fixture native4K runs retain original Yoshi art despite three matching cyan PNGs. Final6000-tick stick replay verifies walking, movement, stopping and no phantom presses; attached head/standing/walking screenshots inspected and shown. Physical Xbox remains untested.
- Full notes: native-yoshi-start-and-render-checkpoint-2026-10-15.txt; build/yoshi-session-* records first initialization/head evidence, build/yoshi-render-* records final rendering/behavior. Complete gameplay/art/save/audio/highFPS/packaging remains incomplete. Keep Sol High; goal stays active.

## Source-verified archive texture roles (2026-10-15)

- Reclassified29 archive models through constructors, source consumers and user-ROM tables:15 Character,13 Object,1 Level. The12 player body/component slots match ROM and host rebasing; player wings and Chain Chomp body/links are protected Character assets. Doors/platforms/collectibles and entry-room scenery are identified explicitly. Shared coin/enemy body and cap/player cosmetic draws stay Unknown pending consumer context.
- Added texture-model-source-evidence.json and check_texture_source_roles.py. The audit independently BLZ-decodes user-ROM overlay2, verifies the full extraction and12 model pointers, archive ranges, constructors, consumers, host slots and file SHA provenance. Retail src, textures, renderer and timing are unchanged.
- Release build,35/35 CTest (37.60s), source audit and native-data audit pass. Updated inventory recognizes505 anonymous full-model identities with1796 unchanged initial bindings/1574 texture records. Model counts:120 Character,285 Object,66 Level,34 Unknown.854 referenced eligible keys/185 character keys have zero overlap;121 unbound records (83 Character,24 Object,14 Unknown) and complete runtime/animated coverage remain unfinished.
- Two4200-tick native4K Original/private-fixture runs exit0. RGB and bottom captures match previous streaming build exactly, Mario stays original despite a cyan replacement, and trace explicitly labels archive body Character. Screenshots inspected and shown. No production AI art exists yet.
- Installed on F: with verified hashes and rollback backups; user INI absence preserved. Installed executable with F: ROM completes1800 TestMap ticks, exit0, one live coin at1750,1280x720 hardware GPU; screenshot inspected. Installed native-data audit/ROM hashes pass.
- Full notes: native-archive-roles-checkpoint-2026-10-15.txt; evidence build/archive-role-*. Keep Sol High for remaining shared draw ownership/reverse engineering. Goal remains active; no switch recommended.

## Bounded native replacement texture streaming (2026-10-15)

- NativeGpu retains original CPU pixels and small asset/role references. Eligible PNGs load only for submitted geometry, with one temporary64MiB RGBA vector and a GPU LRU. Character/Unknown exclusions are enforced at draw time. Missing/invalid files fall back and are memoized; changing quality/backend reloads replacements. Software runtime Upscaled retains originals; offline eager tooling remains supported.
- Groups retain descriptors, so LRU eviction is safe across opaque/partial-alpha passes. First stress exposed D3D11 queued resource retention despite map eviction; GPU command retirement now bounds those references too. A later test initialization conflict was fixed by reserving DS addresses before GPU allocation. Failed logs are retained. Over-budget working sets can stall/redecode; no complete-pack FPS claim.
- Release build, native-data audit and35/35 CTest pass (25.97s). New32-asset4096 stress streams2GiB total image payload through128MiB cache:33 uploads,31 evictions,64MiB temporary peak, correct alpha after eviction. Missing/new/edited files, size guards, character/unknown exclusions and small original CPU decode pass. A separate whole-WARP-test observation reports363978752-byte peak working set and472002560-byte peak virtual; payload counters are not total memory.
- Final4200-tick Original and diagnostic replacement native4K runs exit0. Both match prior RGB and bottom images exactly, with unchanged Mario/road ROI. GX retained CPU payload peak629760 bytes in both. Replacement loads once; warm ticks have no PNG lookups or material uploads. Screenshots were inspected and shown. Production AI art is still absent.
- Installed on F: with SHA-verified copies and rollback backups; user INI absence preserved. Actual installed executable/F: ROM completes1800 TestMap ticks, exit0, one live coin,1280x720 hardware GPU and zero texture rescans. Installed native-data audit and ROM hash agree; screenshot inspected.
- Full notes/evidence: native-texture-streaming-checkpoint-2026-10-15.txt and build/texture-streaming-*. Keep Sol High for remaining asset/source investigation. Goal remains active; no model switch recommended.

# PC frame pacing progress

## Cached decoded texture identities and CPU scan removal (2026-10-15)

- Decoded VRAM entries now retain RGBA identities/partial-alpha metadata; triangles carry it to D3D11. Immutable textures avoid rescanning pixels every presentation, while mutable bindings still detect updates. New content identities upload once. Added optional CPU submission/material-work profile counters. Retail src, artwork, roles and timing are unchanged.
- Release/default builds, native-data audit and34/34 CTest pass (21.80s). A strict geometry comparison caught six uninitialized metadata padding bytes; explicit zero reserved bytes fix them while retaining the original492-triangle memcmp gate. Initial failure/diagnostic logs remain.
- An isolated4096-texture/256x192-output WARP comparison scans335544320 texels over20 warm forced renders and zero with cached metadata; images match exactly, material uploads remain zero. CPU submission averages45.140ms/0.2572ms in that fixture, not complete-game FPS.
- Two4200-tick actual4K castle replays exit0. Original and synthetic-replacement captures each match the previous ownership checkpoint RGB exactly; bottom RGB also matches. Both profile runs show zero GPU material scan texels, and steady ticks4000..4180 have zero material uploads and no4K decode-metadata scan. Render CPU medians842.5us/776us exclude gameplay/GPU completion. Screenshots inspected and shown.
- Full replacement streaming/32-bit memory budgets, wholesale GPU cache eviction, missing-file/transient-binding costs,63 Unknown model roles and121 unbound texture records remain required before a full AI art pack. Complete gameplay/audio/save, independent highFPS, physical inputs and packaging remain unfinished.
- Installed on F: with verified hashes and rollback copies, preserving user INI absence. Installed executable/map audit and ROM hash comparison pass; a separate1800-tick TestMap startup exits0 with one live coin at1280x720 and zero GPU material scan texels. Actual screenshot inspected. Diagnostic replacements remain outside the active installation.
- Full notes: native-texture-performance-checkpoint-2026-10-15.txt. Evidence: build/texture-performance-*, build/verify-texture-performance.py. Keep Sol High for remaining rendering/streaming work. Goal remains active; no model switch is recommended.

## Texture ownership, archive identities and original-reference inventory (2026-10-15)

- Added505 explicit model roles and full-model identities:105 Character,65 Level,272 Object,63 Unknown. Native FS loads register provenance; generated ModelComponents::Render scopes bind ownership to the actual model. Individual/range frees and heap destruction remove stale origins. VRAM cache keys include role/quality/current content; history is pruned without invalidating submitted frames. Retail src remains unchanged.
- Canonical keys ignore unused palette bytes, and direct-color keys need no palette. Fixed missing COM initialization in the original PNG export writer. Release/tools build, native-data audit and33/33 CTest pass (22.34s), including shared-key exclusion across roles, nested scopes, lifetime removal, palette updates and bounded cache history.
- Inventory validates455 named and50 archive BMDs:1574 texture records,1796 decoded initial material bindings,1122 unique source PNGs, zero decode errors. Python full-ROM model identities independently agree, and all PNG dimensions/alpha counts are checked.121 unbound texture records remain listed for animation/selection investigation.835 referenced unique keys have Level/Object ownership; complete AI pack coverage is not established.
- Two4200-tick native4K replays exit0. Original tick4180 matches the prior checkpoint RGB exactly. A private magenta-grass/cyan-Mario diagnostic pack loads the grass at4096 while rejecting Mario's body image via Unknown ownership; actual Character bindings also retain original dimensions.2847762 pixels change, while the620x600 Mario/road region and raw bottom RGB are identical. Screenshots inspected and shown. Diagnostic colors are not production AI art.
- Required next: profile full-resolution replacement hashing/decoding and32-bit memory use; resolve63 Unknown roles (archive player members128..131 have source leads); inventory121 animation/selection variants; then generate/review/stream the full eligible AI pack and verify character exclusions and coverage. Full gameplay/audio/save, independent highFPS and physical inputs remain pending.
- Installed on F: with verified hashes, rollback copies and an archive containing all1125 inventory/reference files. User INI absence is preserved. Installed executable/map audit and ROM hash comparison pass. Separate1800-tick installed TestMap startup exits0 with one live coin at1280x720; screenshot inspected. Diagnostic replacements are not active in the installed game.
- Full notes: native-texture-ownership-checkpoint-2026-10-15.txt. Evidence: build/texture-ownership-*, build/texture-inventory, build/verify-texture-ownership.py. Goal stays active. Keep Sol High for the remaining rendering/ownership work; no model switch is recommended.

## Native texture-quality selector and reduced/untextured rendering (2026-10-15)

- Native Display settings now cycle Original/75%/50%/25%/No Textures/Upscaled; F1 T uses the same helper. Percentages reduce each dimension with deterministic center sampling and minimum one texel. No Textures retains every original alpha value and material/vertex lighting; decal mode uses modulation. Logical UV scaling prevents atlas/repeat displacement when texture dimensions change; cache keys separate modes.
- Added saved quality with old-toggle migration, key-order priority and invalid-value fallback. New quality checks cover exact samples/dimensions, partial alpha, live VRAM cache separation, UV normalization, decal/shadow attributes, GPU/scalar cutout and lighting parity. Release build, compiled-data audit and32/32 CTest pass (29.26s).
- A7900-tick actual4K castle replay exits0, selects every value in the native menu and resumes. All six menu captures match147456 raw-bottom RGB samples each; paused world positions remain fixed. Original tick4180 matches the previous shadow checkpoint RGB exactly. Static terrain changes130642/187691/256339 pixels at75/50/25%,450000 without textures, and0 in Upscaled fallback. Screenshots were inspected and shown.
- A separate5000-tick native menu replay saves25%, resumes and exits0; the same executable restarts from the unedited saved INI and completes1800 TestMap ticks with one live coin and quality3 throughout, exit0. The saved file remains byte-identical. Both native and restart screenshots were inspected.
- Upscaled explicitly says NO PACK. Character/Unknown descriptor roles block matching PNGs, verified in tests. All actual game materials remain Unknown until inventory and role propagation are completed. Complete level/object AI art, coverage and exclusion verification are still required; this is not a finished4K texture pack. Full gameplay/audio/save, independent120/240FPS and final standalone packaging remain pending.
- Installed on F: with verified copies and rollback backups; user INI absence is preserved. Installed native-data audit and ROM hash comparison pass. Separate1800-tick TestMap startup exits0 at1280x720 with Original textures and a live coin; the tick1750 screenshot was inspected.
- Evidence: build/texture-quality-*, build/verify-texture-quality.py; full notes: native-texture-quality-checkpoint-2026-10-15.txt. Keep Sol High for asset ownership/runtime roles and remaining core gameplay. Goal remains active; no model switch is recommended.

## Processed controller layout and actual analog walking (2026-10-15)

- Canonical four-record controller allocation restores all original 24-byte offsets. Separate padded labels made player code read stick strength as jump buttons and the Y axis as magnitude. The retail source and game rules are unchanged.
- Release build, compiled-data audit and31/31 CTest pass. New production CheckInput test covers layout, four-player key lanes, partial/signed/full/neutral input, release and intro gating. Diagnostic CSV now names the player state and records both controller-reader and writer values.
- Two6000-tick actual native4K castle replays exit0: partial/full stick enter St_Walk_Main, translate Mario, produce no phantom button presses/flips, and stop after release. Same flat-ground median speeds20852/122880 and strengths1989/4096 distinguish partial/full travel. Mario and translucent shadows remain clear in inspected screenshots shown to the user. Physical Xbox operation is still unverified.
- Earlier camera/movement evidence described position changes now identified as unintended flips. These bounded replays establish actual walking. Full courses/gameplay/audio/save, independent highFPS and requested texture modes/art remain pending.
- Installed on F: with verified hashes and rollback backups. A separate4800-tick castle run exits0, opens native Options/PC Settings and resumes; installed native-data audit passes. Both menu captures agree with an independent bilinear reference across2764800 RGB channels each (max1.414/255); gameplay returns with665 triangles. The initial verifier assumed crisp scaling and was corrected to the preserved smooth default. No production changes were needed.
- Full notes: native-controller-records-checkpoint-2026-10-15.txt. Evidence: build/controller-* and build/verify-controller-records.py. Keep Sol High for further core gameplay work; goal remains active.

## Shadow masks and restored Mario visibility (2026-10-15)

- The recorded castle geometry has 32 ID0 mask and 32 ID1 visible-shadow triangles, alpha 15. Both were treated as opaque surfaces. Added full-resolution stencil masks, depth-fail marking, caster polygon-ID exclusion, mask consumption/reset and translucent shading. Opaque pixels precede translucent submissions; alpha/depth flags now apply. The software reference independently implements the same rules. Retail source and textures are unchanged.
- Release build, compiled-data audit and 30/30 CTest pass. Expanded GPU tests cover shadow/receiver/caster/overlap/reset semantics, partial alpha and depth writes; the scalar reference agrees at interior samples. Native 4K, pause/map, clipping/UV/viewports remain covered.
- Matched before/after frames use byte-identical 1995-triangle submissions, unchanged player/camera state and unchanged bottom-screen RGB. Exact-black pixels drop 204534→0; 26718 pixels in Mario's region regain blue clothing, 132403 show green terrain through shadows. Actual 4K screenshots were inspected and shown.
- Final 5100-tick replay exits 0, retains visible Mario/shadows through additional pose/camera views, opens native PC Settings and resumes. Stick input changes heading/camera, but position telemetry stays fixed: walking translation remains unverified and needs separate investigation. The first verifier assumed translation; its assertion was corrected to observed behavior without speculative gameplay edits.
- Full checkpoint: `native-shadow-masks-checkpoint-2026-10-15.txt`; evidence `build/shadow-*`, `build/verify-shadow-repair.py`. Full port remains unfinished. Keep Sol High for the next movement/rendering investigation; goal stays active with no model-switch recommendation.
- Installed on F: with verified file hashes and a rollback checkpoint. A separate installed-folder startup completes 1800 Test Map ticks with one live coin, exit 0, using the hardware GPU at 1280x720. The actual tick 1750 screenshot was inspected; user INI settings are preserved.

## Native 4K GPU rasterization and texture-quality plan (2026-10-15)

- Added Windows D3D11 rasterization/presentation at the selected output resolution through 3840x2160. Game geometry decode, clipping, lighting and original textures remain shared with the software reference. Perspective views preserve proportions and widen the frustum; 2D menus/minimap stay separate. Screenshots read the actual GPU target. Software remains an explicit diagnostic fallback.
- Final Release build, compiled-data audit and 30/30 CTest pass. New GPU test verifies full-resolution edges (275 mixed logical DS blocks), reference parity, perspective UV/depth/cutouts, culling/clipping, viewports, completed frames, widescreen, pause/map and resizing. Screenshot review caught zero-alpha pause backdrop becoming black; base-layer RGB handling and its regression test fix it.
- Corrected hardware castle replay completes 4800 ticks, exit0, on RTX5070. Five captures verify native 4K gameplay, PC Options, a live change to1280x800 and back to4K, and resumed gameplay. Three menu captures match 147456 raw-bottom RGB samples; player X/Z stay fixed while paused. Final GPU captures contain 665/668 rendered triangles. Lossless screenshots were inspected and shown.
- Added the requested planned texture choices: Original,75%,50%,25%,No Textures,Upscaled. Full level/object 4K AI art with all characters excluded requires inventory/classification, generation, integration and complete coverage verification. See `texture-quality-plan-2026-10-15.txt`. New modes/pack are not implemented yet.
- Known remaining visuals: opaque black shadow shapes/character obscuration. Full rendering fidelity, performance profiling, independent high-FPS frames, gameplay coverage, audio/save and packaging remain unfinished. Keep Sol High for the next transparency/shadow investigation. User authorized resume; no model switch/pause recommended.
- Full notes/evidence: `native-4k-checkpoint-2026-10-15.txt`, `build/native-4k-verified*`, `build/native-4k-evidence.json`, `build/verify-native-4k.py`.
- Installed on F: with verified file hashes and rollback copies in `native-checkpoint-2026-10-15-native-4k`; user INI preserved. A separate installed-folder Test Map startup completes1800 ticks with one live coin, exit0, hardware GPU1280x720 and a tick1750 screenshot. Complete standalone gameplay packaging remains pending.

## Native pause-menu PC settings (2026-10-15)

- Original pause Options now opens PC SETTINGS / GAME OPTIONS / BACK. Four PC pages use the original font and golden panel art to expose all 12 existing saved settings, including upright/player-relative minimap and hide-touch-target. Original Sound/Backlight options remain accessible. PC hooks are generated; retail source is unchanged.
- Added D-pad/stick/keyboard pause navigation, A confirmation, B back, direction repeat and page shoulders. Opening confirm is consumed. Custom rows accept native touch coordinates; physical Xbox and mouse navigation are still unverified.
- Release and smoke builds pass; all 29 CTest tests pass. The new menu test covers all state transitions, bounds, reverse selection and native tile output. The compiled-data audit passes.
- A 6250-tick castle replay changes every option, opens the original Options screen, revisits PC Settings and resumes gameplay; exit 0. Actual settings/page CSV checks pass, player X/Z remain fixed while paused, and eight full menu captures match raw bottom-screen pixels. A separate 1800-tick Test Map run after restart reloads all 12 settings, retains one live coin and exits 0. Screenshots were inspected and shown.
- Full checkpoint notes: `native-pc-settings-checkpoint-2026-10-15.txt`; updated plan: `pc-settings-menu-plan-2026-10-15.txt`. Native 4K detail, independent 120/240 FPS, replacement texture content and the full playable port remain unfinished. Keep Sol High for remaining gameplay/rendering investigation; Luna Low is suitable for small wording/documentation tasks. Pause after installation at the user's explicit request.

## Clean original pause display (2026-10-15)

- Original pause background captures identify clean text/panels/map layers; the corrupt patches are leftover OBJ sprites. Found that the two-buffer OAM reset's buffer+8 and buffer+32 labels were separate native arrays, and memcpy lost the original ARM copies' forward-overlap behavior. Supplied one canonical assembly allocation for the original paired sprite buffers and restored the exact word/eight-word-block copy semantics. Retail OAM source is unchanged.
- Added a native pause display context: the bottom-screen menu becomes the main view while the actual pause flag is active; top-screen/minimap presentation returns on resume. The flag works without a minimap callback. Paused Stage enables menu stick directions; gameplay retains analog separation. Mouse touch input is suppressed while F1 is open.
- Release build, compiled-data audit and final 28/28 CTest recheck pass. Expanded OAM tests cover dirty-buffer reset in both-buffer mode, original halfword/padding behavior, runtime alias addresses and overlapping block loads. Initial fixture assumptions about padding/compile-time alias comparisons were corrected to the original source semantics; prior failed logs are preserved.
- Final castle replay completes 4,800 frames with exit 0, pauses via Start4200, resumes via Start4500. Nine paused samples show fullscreen context 1; resume samples show it cleared. The captured main pause view matches all 49,152 RGB pixels of the raw menu, and visual inspection confirms clean native buttons/text. Test Map completes 1,800 frames with exit 0 and one live coin; its regular gameplay/minimap view is retained.
- Evidence: `build/pause-display-final-*`, `build/verify-pause-display.py`, `build/port/pause-display-final*`; background diagnostics under `pause-layers-original*`. Full notes: `native-pause-display-checkpoint-2026-10-15.txt`.
- Next: verify Options/Back and add native PC Settings. Those settings remain in F1 until integrated; physical Xbox/mouse menu use remains unverified. Keep Sol High. Goal stays active.

## Original pause/resume input foundation (2026-10-15)

- Baseline Start replay completed without opening pause. The native held/pressed labels were eight bytes apart instead of two, and the base allocation held only two of the four records written by the recovered reader. Replaced those separate buffers with one 16-byte assembly allocation and interior labels. Retail input update/reset source is unchanged.
- Added opt-in button-sequence replay and pause-state CSV fields. A simulation-tick screenshot threshold ensures captures correspond to the actual paused state rather than an earlier presentation during the intro.
- New `smoke_input_state` compiles the recovered update/reset routines against the production assembly input state and checks all four records, held/edge/release behavior, label offsets and reset. Game and smoke builds succeed, compiled-data audit passes, and all 28 CTest tests pass.
- The repaired castle route completes 4,700 frames with exit 0. Start4200 enters the original pause menu; nine samples show active pause at ticks 4230..4470. Start4500 resumes; samples 4530/4680 show pause cleared. No pause globals were forced. Screenshots at simulation tick 4350 were inspected.
- Native Continue/Controller Mode/Options/Quit graphics now appear, but the bottom menu is still shrunk into the minimap corner and stray graphics obscure it. Readable pause presentation, clean rendering, Options navigation and native PC Settings remain pending. The next tasks are detailed in `native-pause-input-checkpoint-2026-10-15.txt`; keep Sol High and leave the goal active.

## Native camera tracking and minimap settings (2026-10-15)

- Verified the first camera state record was zero in the old executable. A new host generator reads the original startup initializer/ROM descriptors and supplies 19 native two-callback records, the original 16-byte stride, and all 34 contiguous camera mode records with original interior labels. Host dispatch uses the recovered workers' native calling convention; three void workers have adapters. Retail camera/minimap source remains unchanged.
- Added bridges to the real camera-save and PathPtr methods. The first movement replay exposed a raw DS pointer in a two-entry camera warning-icon table; rebound both icons and audited their full original OAM bytes.
- Final Release build and compiled-data audit pass, including 38 camera callbacks, all mode records/labels, both warning icons and prior checks. All smoke targets rebuild and CTest passes 27/27. Tests now cover angle wrap, settings persistence, and filtering visible BG2 without losing BG3 or changing registers.
- Final castle replay completes 5,400 frames with exit 0. Software analog movement changes player Z from 27852800 to 13290616 while camera Z follows from 31340994 to 16338390. Mario remains visible. The final Test Map regression completes 1,800 frames with exit 0 and one live yellow coin. This verifies these bounded routes; physical Xbox and complete gameplay remain unverified.
- Added saved F1 options: **O** switches Upright / Rotate with player; **V** hides/shows the touch target. Defaults are upright/hidden. The display angle applies to the map and all projected icons, while original intro-state angle progress is preserved internally. Eleven upright runtime samples and eleven post-intro rotating samples pass matrix/heading checks.
- The target is a separate BG2 layer. The overlay filters it only while a native minimap callback is active, retaining raw bottom-screen output and input. Final raw/filtered images differ at 1,260 pixels exclusively in the top-left 40x40 area; all other map pixels match. Screenshots were inspected.
- Full report: `camera-map-settings-checkpoint-2026-10-15.txt`. Evidence: `build/camera-map-final-*`, `build/port/camera-map-final-*`, and `build/verify-camera-map-evidence.py`. The earlier `camera-native-castle*` crash and exploratory `camera-map-rotate-castle*` run are preserved.
- Next requested milestone is native pause/options-menu integration using the recovered Stage `PS_*` flow; the separate plan below remains pending. Keep Sol High. The goal stays active.

## Requested minimap settings follow-up (2026-10-15)

- **Pause-menu integration milestone (user request):** Add a PC Settings entry to the game's own pause/options menu. Place resolution through 4K, 60/120/240 FPS, replacement textures, 2D scaling, and minimap on/off, orientation, opacity, size, position, and touch-target visibility there. Match the game's menu appearance, support Xbox controller and keyboard navigation, apply choices safely, and retain them across restarts. Verify opening the menu during play, editing each option, backing out/resuming without input leaking into gameplay, and reloading saved choices. F1 is the current interim interface; this in-game integration is pending and required for the requested final port.
- Saved orientation and touch-target settings are now implemented in the interim F1 interface and verified as described above. Integration into the game's own pause menu is pending; see `pc-settings-menu-plan-2026-10-15.txt` for the sequence and acceptance checks.

## Native Lakitu intro handoff and analog movement (2026-10-15)

- Enrolled `daC_Jugem_c_classInit` and registered actor 235 with the original profile priorities/ranges. A host-only source transform validates the 22 ROM member-pointer constants and supplies eleven native state pairs, preserving the recovered actor methods. Native object size and state/timer offsets are asserted at build time. Shared eight-byte model/animation/texture handles retain canonical storage with DEMO and initialize once, avoiding duplicate definitions or mid-lifetime reference-count resets.
- The first replay runs Lakitu's intro states and reaches the original intro-ready write. It then faults when input starts because the three control-mode pointer entries still contain ARM9 addresses (`0x02075650`/`70`/`90`). Rebound them to native storage and audited their 16-halfword contents against the ROM. The title's stick-to-digital navigation conversion is now disabled during Stage gameplay, preserving independent analog magnitude and digital D-pad input. New input fixtures check both contexts.
- Final Release build and compiled-data audit pass: ten profiles, all eleven Lakitu state pairs, three control-mode maps, and existing resource/callback checks. All 27 smoke tests pass after rebuilding the default targets.
- The final castle replay completes 6,000 frames with exit 0. The first sampled ready flag is at tick 3960. At tick 5040, raw stick X=20000 produces game magnitude 1989/4096. Player horizontal position changes from (-4915200,27852800) at tick 4920 to (-4887754,13290616) at tick 6000; X/Z settles after stick release. This verifies the software analog path through game movement. It does not verify physical Xbox hardware, complete direction fidelity, jumping/collection, or full gameplay.
- Test Map regression completes 1,800 frames with exit 0; the selected yellow coin remains alive. Both screen pairs were viewed, along with the original failed replay's images. Final castle images show the minimap tracking the moved player, while the main camera stays fixed and the character leaves view. Camera tracking remains an open gameplay issue.
- Next camera lead: `src/unnamed/arm9/0207/__sinit_02073a24.c` initializes the nineteen Camera state pairs at `data_0209b008` through `data_0209b128`; its function is absent from the current executable map. Investigate initialization and native callback binding in `src/engine/view/dCamera_c.cpp` before modifying tracking behavior. This is a lead, not a verified camera fix.
- Evidence: `build/lakitu-native-*` preserves the first input-pointer crash; `build/lakitu-input-*` contains final build/audit/test/runtime reports and `lakitu-input-evidence.json`; screenshots and state CSVs are under `build/port/`.

## Native coin profiles and gameplay input gate (2026-10-15)

- The native executable already contains the recovered yellow/red/blue coin class and factory/profile objects, but the PC actor registry never registered IDs `0x120`-`0x122`. Registered the canonical compiled profiles and rebound the six model-resource table pointers at ov002 `0x020ff060`/`0x020ff06c` to their initialized native SharedFilePtr objects. No substitute actor implementation was added.
- Extended the compiled-data audit to check all three coin factories, their original 28-byte profile metadata and the six canonical resource targets. The audit now passes nine native profiles. The Test Map data contains 128 yellow coin records across seven course filters: the current filter selects one, and the 1,800-frame runtime report confirms that selected coin reaches/remains in alive state 1. Exit 0; the top/bottom captures were inspected. Collection, red/blue runtime behavior, and every filter remain unverified.
- Added opt-in `SM64DS_PC_STATE_CAPTURE` CSV sampling of real actors/player positions and a standalone crash-trace switch. Timed synthetic stick start/end polls let a replay apply partial analog travel after the intro, rather than throughout it. The input smoke verifies neutral-before, partial-travel-during, neutral-after and no added digital directions.
- A full rebuild of the smoke binaries exposed the old piano preview fitting X/Y but leaving most of its geometry outside the now-clipped Z range. Corrected the test camera to fit all three axes, kept the original `drawn > 500` requirement, and added assertions that every fitted vertex lies inside the view volume. The probe covers 8,575 pixels with 250 triangles. All 27 tests pass after the fresh rebuild/correction.
- Castle movement replay: 6,000 simulation frames, partial raw stick X=20000 during polls 5000-5119, exit 0. The adapter receives the sample but player X/Z stays unchanged. A second 3,600-frame probe during polls 3360-3479 confirms `script_active=0`, `intro_ready=0`, `game_mode=0`, `game_stick_magnitude=0`, and unchanged player position. The game returns through its intro lock before executing its joystick injection. Both screen pairs were inspected; these runs do not verify movement or physical Xbox gameplay.
- The retail actor table maps `g_profile_C_JUGEM` (ov085 `0x02130320`) to actor 235 (`0xeb`), which is not registered natively. Its recovered `daC_Jugem_c::StateIntroMain` owns the write of `data_0209caa0[2] |= 0x80` that unlocks movement. Next work should port/register that actor and its real state callbacks/resource initialization, then repeat the input probe. Do not set the save flag as a shortcut to claim playable control.
- Evidence is in `build/coin-registry-*`, `build/coin-movement-*`, and `build/port/coin-*-state.csv`/screen captures. `coin-movement-ctest.log` preserves the initial piano-fixture failure; `coin-movement-final-ctest.log` is the final 27/27 result.

## Course overlay ownership and ROM data loading (2026-10-15)

- The failing Test Map route selected course 0/overlay 8, but Stage initialization unconditionally translated its `0x02111210` record through overlay 9. All 52 course overlays reuse the `0x021111a0` DS address range. Reading the castle image at that address yielded an invalid model ID `ff1e`; the original Test Map record contains `07d7`. The subsequent collision-file crash was therefore downstream of the wrong course-data owner, rather than proof that the object-overlay requests caused this particular fault.
- Added `port/hal/level_overlay.cpp` and a metadata generator covering the retail 52-course overlay lookup table. Course images are read/decompressed from the mounted ROM, retained in separate Windows-heap buffers, and adjusted using verified self-reference offsets. The generated metadata contains bounds/offsets, without embedded course-image bytes. Stage selects the owning course before mapping its record; the native LoadModel generator now consumes that canonical host pointer without reinterpreting it through overlay 9.
- The production LoadOverlay path loads data for overlays 8-59 through this layer. This is data support; ARM instructions are never executed. Existing native actor code remains separately compiled, and dynamic actor-code overlays 60-102 remain unfinished.
- Release build succeeds. The optional `smoke_level_overlay` audit reads the supplied European ROM and compares every loaded course image, zeroed BSS, and adjusted pointer with the local extraction: 52 images, 344,384 bytes and 4,689 self-references pass. It also verifies that identical DS addresses in two courses resolve to distinct buffers and checks invalid levels/address bounds. The existing configured CTest suite passes 27/27.
- Test Map replay: isolated confirm at poll 450, no Down or repeat, 1,800 simulation frames. Correct model ID `07d7` loads with valid model/collision data; the pipe scene renders and shutdown exits 0. Castle regression: isolated Down300/confirm450, 6,000 simulation frames, exit 0 after intro and Stage reload. Both pairs of screenshots were inspected. These are stationary startup checks; other courses, movement, transitions and hardware input are not verified by them.
- Evidence: `build/level-overlay-game-build.log`, `level-overlay-smoke-build.log`, `level-overlay-rom-audit.log`, `level-overlay-ctest.log`, `level-overlay-testmap*`, `level-overlay-castle*`, and screen images `build/port/level-overlay-{testmap,castle}*.bmp`.
- Installed to `F:\AI Games\Mario 64 DS` with matching executable/map/source hashes and rollback copies in `native-checkpoint-2026-10-15-course-overlay-data`. The F: ROM matches the tested ROM hash. An additional installed-folder Test Map startup run completes 1,800 frames and exits 0; its one-time screenshot at presentation 1,200 shows the Peach intro. That capture establishes installed-location rendering and is not a final-frame screenshot of Test Map.

## Minimap green-area coverage diagnosis (2026-10-15)

- Added a bounded, opt-in capture for the sub-screen background layers and a black-and-white coverage image. `SM64DS_PC_SUB_LAYERS_AFTER` delays the one-time capture until a chosen host simulation tick; without these diagnostic variables, rendering is unchanged.
- The first capture landed in the intro transition, so the diagnostic was moved to tick 5000 and the same 6000-tick route was repeated. The run exited 0 and saved `build/port/minimap-map-mask.bmp`, `minimap-map-mask-bottom.bmp`, the four `minimap-map-mask-bg*.bmp` layer images, and `minimap-map-mask-coverage.bmp`.
- The late coverage mask shows that the bright-green region is covered by BG3 pixels, not uncovered DS backdrop. `ppu_scanout(ENGINE_B, ..., true)` already preserves transparency for backdrop/transparent BG pixels. A blanket green color key would erase authored map pixels that use green shades, so no speculative color-key change was made.
- `sm64ds_pc`, `smoke_ppu_affine`, and `smoke_video` build successfully; the two relevant tests pass. The captures confirm the player marker remains colored. This resolves the backdrop-vs-map ambiguity but does not yet identify whether the green BG3 fill should be represented differently in a PC minimap overlay.

## PC minimap border transparency follow-up (2026-10-15)

- Added a host-only transparency mask for the PC minimap composite. It detects a dominant, uniform edge color and removes only exact-color pixels connected to the screen edge, while preserving isolated interior pixels of that same color. The retail bottom screen remains unchanged.
- `smoke_video` now checks the edge fill, an already transparent texel, and an enclosed interior pixel. The test passes; the full configured CTest suite passes 27/27.
- One replay with a different automated menu-input sequence selected an unsupported route and crashed during dynamic-overlay setup; that capture was not used as visual evidence. Repeating the repository's known isolated sequence (Down at input poll 300, confirm at 450) reaches castle gameplay and exits cleanly after 6,000 frames. On that route, the 23,740-pixel edge mask is active and the top capture shows the map floating over gameplay.
- Repeated the same route with a temporary 50%-screen, fully opaque minimap preview. The enlarged capture makes the edge cutout and retained map/marker visible; that run also exits 0. The original INI settings were restored. Captures and logs are saved as `build/port/minimap-current-stage*` and `build/port/minimap-large-stage*` and copied into the F: checkpoint.

## Minimap player marker asset loading (2026-10-15)

- The minimap OAM trace showed the player marker was using the correct 2D tile mapping, but its tile bytes and palette bank were empty. The recovered retail HUD (`dMeter_c::InitResources`) identifies the missing sub-screen resources: OBJ graphics file `0x229` at sub OBJ VRAM `0x06602000`, and palette file `0x22A` at palette offset `0x120` for `0xe0` bytes.
- The native build registers the minimap actor directly without creating the retail HUD actor. Added a PC-only resource load in `dMap_c::InitResources` for just those two shared minimap resources. DS builds and behavior are unchanged.
- Rebuilt `sm64ds_pc` with Visual Studio 18 x86 Build Tools. The full configured CTest suite passes 27/27. A 6,000-simulation-frame run using the supplied European ROM exits with code 0. The `GXS::LoadOBJPltt` trace confirms the palette copied to the expected destination.
- Captures: `build/port/minimap-hud-assets.bmp` (top, including the overlaid minimap) and `build/port/minimap-hud-assets-bottom.bmp` (bottom map with the restored red-and-white Mario marker). This verifies the player marker in this test scene only; other map icons and all gameplay scenes still need review.

## Native VBlank and minimap OAM follow-up (2026-10-15)

- The PC runtime now dispatches the registered game VBlank callback once per simulated host tick. A traced 6,000-tick run confirmed all 6,000 callbacks reached the game handler.
- Fixed the PC VBlank OAM update gate for Stage gameplay. The DS uses a frame delta of 2 in this scene, while the host already coalesces the corresponding VBlank cadence into one simulation tick. The PC-only check now treats that cadence as one tick without changing the game's timer delta or ARM/DS behavior.
- Verified the OAM upload path now continues through the entire 6,000-tick run and bottom-screen minimap sprite submissions reach the sub-engine buffer. Before the fix, the update gate stopped after 120 uploads and the sub-engine OAM remained empty.
- Built `sm64ds_pc`, `smoke_frames`, and `smoke_ppu_affine`; `smoke_ppu_affine`, `smoke_frames`, and `smoke_video` all pass. Two 6,000-tick native runs exited successfully. Captures show the bottom map and the minimap composited over the top screen.
- Remaining visual issue: some map markers look distorted or incomplete in the captured gameplay image. Their OAM now uploads; the remaining investigation is sprite tile/palette/scanout fidelity. This does not verify every marker or all scenes.
- Captures: `build/port/minimap-overlay-verification.bmp` (composited top) and `build/port/minimap-overlay-verification-bottom.bmp` (bottom screen).
- Follow-up diagnostic capture confirms the active player marker submits OAM attributes (`attr0=0070`, `attr1=8063`, `attr2=a980`) and the host scanout draws 614 nontransparent OBJ pixels in the sampled scene. This establishes that sprite data reaches the renderer, but does not yet confirm the tile/palette interpretation is visually accurate. The optional `SM64DS_PC_TRACE_OAM=1` trace is available for the next sprite-rendering investigation.
- Rebuilt after adding that trace and ran the full configured CTest suite: all 26 tests passed. The diagnostic 6,000-tick replay exited successfully and saved `build/port/minimap-tile-diagnostic.bmp` plus its bottom-screen capture.
- Compared the live sub-engine `DISPCNT` setting with DS OBJ mapping rules and found that this gameplay scene uses 2D character mapping (bit 4 clear), while the host renderer always laid multi-tile sprites out as a 1D strip. Updated OBJ scanout to use 32-tile-wide rows in 2D mode, including the even tile-number stride for 8bpp, while preserving the existing 1D boundary path.
- Added `smoke_ppu_obj` to verify four distinct quadrants for both 2D 4bpp and 2D 8bpp sprites, plus the prior 1D path. The full configured suite passes 27/27; `sm64ds_pc` builds and a 6,000-tick game run exits successfully.
- Captures after the mapping correction: `build/port/minimap-2d-fix.bmp` (composited top) and `build/port/minimap-2d-fix-bottom.bmp` (bottom). The top overlay is visible, but the player marker still looks dark/wedge-shaped in this captured scene, so visual fidelity is not yet fully resolved.

## Xbox analog movement integration (2026-10-13)

The Windows XInput adapter feeds the left stick into the game's existing virtual-joystick fields during `dScStage_c::CheckInput`: normalized direction, partial magnitude, and the game's fixed-point angle. This follows the Circle Pad patch's 0x69 joystick radius and preserves touchscreen movement whenever the stick is centered. The active `sm64ds_pc` target includes this controller update and links its native input function. The build also replaces signed left shifts in Q12 vector scaling with multiplication to avoid undefined behavior for left/down input. `smoke_input` passes its radial deadzone, cardinal direction, full range, diagonal normalization, and partial travel checks. Live in-game movement remains unverified because Stage currently reaches object overlay requests that are not yet natively implemented.

Updated the Windows host fiber runtime with `ntr::rt_set_target_fps()` and `ntr::rt_target_fps()` in `port/ntr/rt.h` / `port/ntr/rt.cpp`. Accepted values are 60, 120, and 240; unsupported values leave the current selection unchanged. `rt_run()` preserves the chosen value across runtime state initialization and uses QueryPerformanceCounter to pace completed host frames. Game logic still advances only once per emulated DS VBlank (nominal 60 Hz); 120/240 currently represent host pacing requests, not faster game simulation or independently rendered intermediate frames.

Added assertions in `port/tests/smoke_frames.cpp` for valid and invalid setting values. `ntr` library and smoke_frames translation unit compile. `smoke_frames.exe` does not link: the existing target fails with 18 unresolved model/GX symbols (and duplicate/missing bridge definitions), so those assertions could not be run. Rebuilt dependency-light smoke_heap/smoke_roots targets are current; smoke_heap passed. Full port remains non-playable and lacks a Windows window, runtime input, settings menu, minimap, high-resolution rendering, or texture upscale path.

Source root: `<USER_HOME>\Documents\Codex\2026-10-10\can-you-compile-this-mario-64\work\source-extracted\sm64ds-decomp-main`
## Smoke-link closure follow-up (2026-10-10)

Fixed two concrete stale build-graph/link issues while attempting to extend gates:
- Gate 6 now excludes `src/engine/oam/OAM.cpp` from the direct SLICE6 source list because `hostgen` of the promoted OAM::Load function emits the entire translation unit. This removes a second copy of all OAM definitions from `smoke_oam`.
- Replaced the obsolete duplicate `Model::LoadCompressedTextureToVram` method body in `port/hal/gx_upload_bridge.cpp` with a plain C ABI thunk to the recovered method in `src/engine/model/Model.cpp`.
- `smoke_oam.cpp` now includes `OAM.h` for the static class API instead of declaring `OAM::Reset` as a namespace free function.

Reconfigured successfully. `smoke_model` still fails to link on 17 unresolved model destructor and GX/upload dependencies. `smoke_oam` still fails to link on 30 unresolved symbols, now without the prior duplicate OAM translation-unit errors. Major groups are OAM data/function linkage aliases, missing GX helpers and model ABI slots. No larger smoke target passed in this attempt. Prior smoke_heap remains passing.

These changes are in the temporary extracted tree named below; it is the active development copy, not yet installed as a portable source distribution under F:.

## Host model/collision and frame-scheduler continuation (2026-10-10)

Closed the model/GX link gap by generating the ROM-owned texture bank lookup data from the extracted ARM9 image, routing the host to the fixed mapped VRAM windows, and linking the real DMA/halfword copy helpers. MSVC-only placeholder destructor slots satisfy the model vtable layout; the real resource-release destructors remain linked separately. The model, animation, frame, and catalog-soak gates now link.

Closed the collision gate with the reconstructed `dBgCh`, `dBgPi`, sphere/line geometry, and helper TUs. The custom host line-raycast duplicate was removed because `dBgW_Kc_itcm.cpp` now supplies that method. `smoke_clsn` links with the real reconstructed collision path.

Corrected `ntr::rt_run`: simulation ticks stay at 60 Hz while `FrameHook` presents at 60/120/240 Hz, repeating the most recent game state between simulation ticks. `rt_frame()` and `max_frames` count simulation ticks; the frame hook receives presentation-frame numbering. This code compiles and links through `smoke_frames`; no executable was run.

Full CMake/Ninja build status: 13 of 14 configured smoke targets link. `smoke_actor` is the remaining link failure with 111 unresolved symbols spanning player/actor lifecycle, audio, particle, stage overlays, and scene behaviors. A full playable game still needs those systems plus a native window/presentation/input path and requested settings/features. Current all-target log: `work\\build-all-postfps.log` relative to the Codex task directory.
## Native host frame presentation (2026-10-11)

- Added `port/ntr/video.h` and `video.cpp`: a Win32 resizable window, DS Engine A 2D/OBJ plus GX scanout as the main view, and Engine B 2D/OBJ composited as a translucent minimap. The minimap has runtime controls for visibility, opacity, size, and normalized position; output dimensions are clamped to 256..3840 by 192..2160.
- `rt_run` now presents through the native window when an application has opened it with `ntr::video_open()`. The API is opt-in, preserving headless smoke hooks. `video_set_settings()` updates the client resolution so a future settings menu can bind to it.
- Built `smoke_frames` successfully with the Visual Studio 18 x86 toolchain after adding the module to the `ntr` library. This was a compile/link check; no executable was run.
- This remains an integration seam, not a playable game: there is no standalone target or menu/startup path calling `video_open`, the DS scanout still has feature gaps, and controller/touch input and asset replacement are not wired.

## Host controls and stylus seam (2026-10-11)

- Added `port/ntr/input.h` and `input.cpp`. The host polls XInput-compatible pads dynamically (XInput 1.4/1.3/9.1), retains signed analogue stick and trigger values, maps the left stick to eight directional DS keypad inputs, and offers keyboard fallback controls.
- The DS `KEYINPUT` read now returns the active-low host state. The host maps the ARM9 DTCM input window and initializes the lid latch, and it writes mouse stylus samples in the packed 12-bit touch format read by the original touch callback. Mouse coordinates are taken from the host window and normalized to bottom-screen coordinates.
- `input_poll` runs before each emulated game tick; mouse position/button state is latched during window presentation for the next tick. `smoke_frames` rebuilt and linked after integrating this input module. It was not run.
- Limitations: no live standalone title exists to confirm its asynchronous touch callback or controls; the game still sees DS digital direction bits, while analogue values are exposed for future game-side movement integration. The Windows settings menu, touch-mode screen routing, and upscaled asset pipeline are outstanding.

## Actor gate link cleanup (2026-10-11)

- Removed duplicate actor/collision virtual trap bodies where the current production TUs already define those methods. `ActorBase.cpp` is now built only from its MMIO-transformed hostgen output in Gate 9, not both the raw source and generated copy.
- Removed two obsolete shadow/actor no-op aliases that collided with recovered source definitions, changed the smoke driver to the current `daObjYajirusi_c_classInit_YAJIRUSI_R` factory, and added Itanium-to-MSVC thunks for the existing SceneNode and collision methods.
- Latest full build: 13 smoke executables link; `smoke_actor` still fails with 103 unresolved externals and no duplicate definitions (down from 111 unresolved plus 65 duplicate reports in the pre-cleanup link). The remaining actor link closure spans actor/player, sound, heap, collision, particle, overlay, and game-data dependencies. Build/link only; no executable was run.

## Native analogue movement integration (2026-10-11)

- Adapted the Circle Pad patch's controller data path for the PC left stick. The reference patch reads a signed X/Y vector, clamps it to its 105-unit radius, and writes movement magnitude, normalized X/Y, angle, and the active-stick flag into the game's normal controller record.
- The PC input layer now radially dead-zones XInput axes and preserves analog travel. `dScStage_c::CheckInput()` uses those values to fill the same fields after the standard DS input calculation, so analog motion takes precedence when the stick is deflected while D-pad and touchscreen controls remain available.
- Added the compile-only CMake target `pc_analog_input` for this production controller update. It compiled successfully with the Visual Studio 18 x86 toolchain. No game executable exists yet and no gameplay runtime test was possible.
- Updated actor-link attempt remains at 100 unresolved externals, with no duplicate-definition errors. Overall project estimate: 6%; this counts the host renderer/input groundwork and smoke link progress, while the playable-game integration, settings/menu, 4K asset pipelines, and release packaging remain outstanding.
- Reference: [GBAtemp Circle Pad patch thread](https://gbatemp.net/threads/circle-pad-patches-for-super-mario-64-ds-and-other-games-in-twilightmenu-with-twpatcher-and-rtcom.623267/) and its [patch source](https://github.com/shocoman/Analog-Controls-for-NDS-Games-on-3DS/blob/master/arm9_controls_hook.s).


## Actor gate link cleanup continued (2026-10-11)

- Added the real `dBgCh_Lin` global used by the KCL collision path, restored the `dBgPi` data/vtable alias and ground-collider destructor alias, and added the `fLiNdBaPr_c` destructor ABI bridge with its backing global.
- Added the missing Heap `Sizeof`, `CreateSolidHeap`, and `Rescue` production methods and backing game-heap pointer globals. Attempting to add SolidHeap allocator and particle creation TUs pulled in additional vtables and manager dependencies, so those broad additions were reverted.
- The actor smoke link is still failing at 71 unresolved references (no duplicate-definition errors). The remaining set has widened into player/actor methods, sound, particles, star tracking, level globals, and game overlays; this target needs a deliberately broader game-runtime dependency stack before it can link.
- Validation was build/link only; no executable was run. Overall project estimate remains about 6%.

## Actor link dependency reduction (2026-10-11)

- Added the missing moving-collider production translation unit to the actor target, wired its raw ROM-style `SetFile` symbol to the host method, and aliased the actor's inherited poof/destruction helpers to the real base implementations.
- Kept the production shadow and transform helper sources already present in the target, and removed standalone SolidHeap/particle source additions whose extra dependencies increased unresolved symbols without helping the actor entry path.
- The latest actor link failure dropped from 71 to 64 unresolved references, with no compilation errors or duplicate definitions. Remaining errors still span sound, player/gameplay, particle runtime, game-state globals and overlays; it does not link a playable game.
- Validation was compile/link only; no executable was run. Overall project estimate remains about 6%.

- Follow-up ABI verification with `dumpbin /symbols` confirmed `dExtShadowModel_c::InitModel` exists under MSVC decoration while the actor TU calls the raw decomp spelling. Added a linker alias for this method. The latest link now reports 64 unresolved references (down from 66).



- The full Sound.cpp integration experiment was rejected: it raised the actor target to 160 unresolved symbols even with function sections and `/OPT:REF`, so that translation unit and those linker flags were removed again. A precise alias now connects the C++ collider helper call to the already-built production C helper; the actor target is at 64 unresolved references. No executable was run.

## Actor link/data integration update (2026-10-11)

- Extended `port/tools/romdata.py` to emit the ARM9 `LEVEL_PART_TABLE` at 0x02075264 and `SUBLEVEL_LEVEL_TABLE` at 0x02075298 from the local extracted ARM9 image. The generator now supports signed-byte values and emits host linkage aliases for the historical symbol spellings. This keeps copyrighted ROM bytes out of source control while providing the production course mapping data to the host build.
- Added the production `Heap::CreateSolidHeap` path together with `SolidHeapAllocator`, the full `SolidHeap` virtual surface, `Heap::Reallocate`, and constructor ABI bridges. This reduced the actor link to 49 unresolved references (down from 52 before the heap cluster). The ARM9 ROM data generator compiles with correctly bounded mapping tables and host symbol aliases, but the actor executable still does not link.
- Remaining unresolved dependencies include player methods/state, sound and particle systems, overlay state/functions, save-star flags, and the solid-heap factory required by `Heap::InitializeSolidHeapAsDefault`. No runtime/gameplay validation was possible.
- These changes are being synchronized to `F:\AI Games\Mario 64 DS\pc-port-source`; the ROM itself remains local and is only used to extract build data.
- Overall project estimate remains about 6%; standalone startup/gameplay, settings/menu, 4K output, texture upscaling, minimap controls, and release packaging are not complete.

## Production solid-heap and level-data integration (2026-10-11)

- Linked the complete recovered solid-heap allocator layer into `smoke_actor`: the allocator TU, `Heap::CreateSolidHeap`, `Heap::Reallocate`, `SolidHeap` constructor and virtual surface, plus C1 ABI forwarders that use placement construction. The new link reduced unresolved symbols from 67 to 50; correcting the table symbol aliases reduced it again to 49.
- Corrected ROM-data generation for `LEVEL_PART_TABLE` and `SUBLEVEL_LEVEL_TABLE`: emitted the actual host symbol aliases and bounded the sublevel table at 0x020752d4, where the next data structure begins. The generated C object compiled.
- `smoke_actor` still fails to link. Remaining unresolved groups include player/actor behaviors, sound and particles, stage overlays/data, save-star flags, and actor-world globals. No standalone game startup, menu, or gameplay test exists yet.
- An actor factory registration experiment was reverted: the call chain depends on fBase process-list setup and relocation-sensitive ROM pointer-to-member tables; adding only the recovered factory helpers widened the unresolved count to 55. No placeholder registry data was added.
- Rebuilt all configured targets after the heap bridge changes. Thirteen of fourteen smoke executables link; `smoke_actor` is the only failure at 49 unresolved references. A first placement of the solid-heap C1 bridges in shared `heap_globals.cpp` broke three unrelated smoke links, so the bridges were moved into the actor-only `actor_ctor_bridge.cpp`; the all-target rebuild confirms the other 13 links are restored.
- F: project files and this status record are synchronized to `F:\AI Games\Mario 64 DS\pc-port-source`. Overall project progress remains about 6%.

## Actor ABI bridge follow-up and full F: source mirror (2026-10-11)

- Added cdecl-to-thiscall forwarders for `dBgActor_c::KillByMegaChar` and `dActor_c::DistToCPlayer`, where recovered TUs call those real C++ methods through the original Itanium C symbol spellings. `smoke_actor` now fails at 47 unresolved externals (down from 49), with no compiler errors.
- Rebuilt all configured targets before this bridge follow-up: 13/14 linked, with `smoke_actor` the only failure. The two new forwarders have since been rebuilt in that actor target; the other targets do not compile `cxxname_bridge.cpp`.
- Started a non-destructive Robocopy of the full source checkout to `F:\AI Games\Mario 64 DS\pc-port-source`, excluding temporary workspace files and generated host build output. The transfer is still live and F: has responded slowly; completion and independent configure verification remain pending.
- Overall project progress remains about 6%; no standalone game is built.

## Circle Pad reference port and actor link follow-up (2026-10-11)

- Used the GBAtemp thread's linked Circle Pad hook as the reference for translating a signed X/Y stick vector into the DS controller fields: radial magnitude, normalized direction, angle and active-stick state. The native input adapter retains analog travel after a radial dead zone, and `dScStage_c::CheckInput()` feeds it into the game's normal input record so the game uses its existing movement path. Xbox controller acquisition uses XInput.
- Continued smoke actor link cleanup with the production `ModelAnim2::Func_020162C4` implementation and `dExtFrameCtrl_c` playback methods. The newest link returned to 29 unresolved references after confirming the optional `atan2` addition required more ROM math support and removing that addition.
- Fresh configuration/build in `build/port2` completed compilation and reached the actor link. `smoke_actor` remains the only known failed smoke executable; no standalone PC game is linked or playable yet.
- The full source mirror completed to `F:\AI Games\Mario 64 DS\pc-port-source` with Robocopy reporting zero failed files. A follow-up incremental synchronization copied the latest changes.
- Overall project estimate remains about 6%; native analog code exists, while in-game controller validation, game startup, settings/menu, high resolution/aspect ratio, upscaled textures/sprites, minimap, frame-rate options and final packaging remain outstanding.

## Actor link and host math progress (2026-10-11)

- Added real MSVC member-call bridges for the recovered `dActor_c::LandingDustAt` and `HugeLandingDustAt` entry points, plus the existing `fLiNdBa_c` destructor source. The actor link went from 29 to 26 unresolved symbols.
- Added the ROM-backed fixed arctangent table to the local-data generator and a host `cstd::atan2(Fix12, Fix12)` implementation that preserves the game's table lookup and quadrant logic while replacing its DS soft-float sign test with an integer sign check. The table was confirmed file-backed in `arm9_dec.bin` and monotone through 0x2000; the generator now explicitly permits this one named ROM data range past its initial BSS boundary. The actor link now reports 25 unresolved symbols.
- A trial adding `Player.cpp` failed under MSVC because that aggregate TU declares local classes as `extern`; it was removed. Separate attempts to add Player state/hurt and number-object TUs widened dependencies from 25 to 54, so those files were removed as well. No fake implementations were retained.
- Current validation: `smoke_actor` compiles and reaches link, then fails with 25 unresolved symbols in player, audio, particles, and overlay paths. Full 14-target build is being checked; no game executable or in-game analog test exists.
- Overall project estimate: 7%. Native input and additional math/actor seams now compile, but playable startup, all smoke-link closure, renderer/menu integration, requested settings/assets and packaging remain incomplete.

## Latest actor closure and full smoke build (2026-10-11)

- Added the recovered player body-model selection rule as a native method and Itanium-name bridge, the recovered score-popup owner attachment behavior, and the ground-pound camera jitter calculation. The latter reuses the existing game distance routine and host fixed-point divide. The fixed-angle lookup data continues to come from the local extracted ROM.
- The actor target compiles these sources and reaches link; unresolved symbols are down from 25 to 22. Remaining references include sound, particle manager, player state/health, actor factory, and ov002 data/code.
- Completed the fresh full CMake/Ninja build (838 steps). Thirteen smoke executables linked successfully; `smoke_actor` is the only failure. No playable standalone game or in-game controller test is available yet.
- The equivalent source updates and this progress note are being synchronized to `F:\AI Games\Mario 64 DS\pc-port-source`.
- Overall estimate remains about 7% of the full requested project; launch/runtime, display/settings menu, 4K/aspect ratios, asset upscaling, map overlay, frame-rate controls, and distribution are still outstanding.

## Particle creation path added to actor target (2026-10-11)

- Added production `Particle::System::NewSimple`, `Particle::Manager::AddSystem`, and their three recovered list/system initialization helpers. A native ABI bridge connects the decomp's Itanium manager call to the real C++ method; the actor HAL now provides the correct zero-initialized `data_0209ee74` tracker pointer storage, which the recovered `SysTracker` constructor will populate when the runtime initializes it.
- `smoke_actor` compiles and links to 21 unresolved symbols (down from 22). The particle creation call and its manager methods no longer appear among the unresolveds. The smoke driver does not initialize a live particle manager, so gameplay particle behavior is not runtime-validated.
- An incremental all-target build confirms the sole out-of-date/failing target is still `smoke_actor`; the previous complete fresh build linked the other 13/14 smoke runners. No standalone title exists yet.
- Overall project estimate remains about 7%; audio backend, initialized particle resources, player/overlay closure, boot/runtime, requested settings/assets and packaging remain incomplete.
- Overall project estimate is now about 8%: host analog and presentation paths, 13 linkable smoke executables, and a growing set of production actor dependencies are present. The full playable PC runtime and all requested options are still incomplete.

## Function-level smoke relink audit (2026-10-11)

- Rebuilt `smoke_actor` with MSVC `/Gy` function sections and `/OPT:REF` to let the linker remove actor methods not reachable from the smoke driver. This did not hide any references from the exercised smoke call graph: the same 21 unresolved references remain.
- Kept these options on the actor smoke target because they are valid release-size optimization flags and do not change the unresolved dependency set. The standalone game target must still link every reachable production behavior.
- The latest full-rebuild history remains 13/14 smoke binaries linked, with only `smoke_actor` failing. Latest targeted actor log: `work/build-actor-funcref.log` in the Codex workspace.

## ROM asset recovery and input latch seam (2026-10-11)

- Installed `ndspy` into the workspace-local `build/python-packages` directory because the bundled Python profile is not writable. Cataloged the supplied European ROM: 2,072 named NitroFS files and 2,058 runtime handles.
- Added `port/tools/extract_nitrofs.py` to extract the user's own named cartridge files into gitignored `extracted/dsd/files`; extracted 14,203,899 bytes locally. Generated `files.txt` using `port/tools/ntr_manifest.py`.
- Replaced unavailable fixed host mapping for `0x027fffa8..0x027fffac` with explicit host-side lid/touch latches in `ntr/io.cpp`; `input.cpp` uses the I/O seam, and `hostgen.py` now routes the game's direct lid read through `NTR_MMIO` without editing `src/`.
- Verified hostgen rewrites `func_02019ac4`'s lid read. `smoke_fs` passes raw and LZ77 reads/refcounts; `smoke_frames` passes 24 simulation frames with rendered geometry; the current 12-test CTest subset excluding actor and collision passes 12/12.
- Current full CTest: 11/14 passed before generating the directory manifest. After the manifest, 12/14 non-blocked targets pass; `smoke_clsn` has zero floor hits in its current probe grid, and `smoke_actor` still cannot link due to 21 unresolved Sound, Player, actor-factory and ov002 code/data symbols. No standalone playable executable exists.
- Build required `TEMP`/`TMP` redirected to workspace because Windows denied MSVC's default Temp intermediates. Overall project estimate: about 10%.

## Actor factory and process ABI follow-up (2026-10-11)

- Added the production `dBase_c` implementation and recovered ARM9 actor factory / process entry points to `smoke_actor`; the previous `dBase_c::Spawn` and actor factory link errors are gone. Their required ARM9 zero-initialized state storage is defined in the actor-only host bridge.
- Generated ov002 globals from the user's local ROM extraction, reducing the previous actor link failure from 21 unresolved symbols to 12. Adding the real spawn/process path exposed ARM virtual-member records; the host adapter now translates their verified 0/4/8-byte virtual slots into MSVC vtable dispatch and preserves the production before/action/after status rules. The records come from actual ROM data generated into the build tree.
- Added a host-only extraction of the exact recovered `Player::Heal` method, its real `GiveHealth` dependency, and ROM-backed health/state data. This reduced the actor link from 11 to 10 unresolved symbols.
- Added host slices for the recovered hundred-coin star reward, `daStar_c::AddStarMarker`, and the real Event bit helpers/state. The star spawn/marker path and its ARM9 BSS storage now resolve; the event global's C/C++ decorated names share the same host storage. The actor link is down to 9 unresolved symbols.
- Remaining actor-link failures include Sound backend calls, player methods, and two ov002 routines. A trial adding recovered Player::Hurt and Player::ChangeState widened the closure to 30 unresolved; adding the aggregate Sound TU widened it to 106. Both broad additions were reverted. The actor smoke still does not link.
- A trial compiling the aggregate `src/engine/sound/Sound.cpp` widened the actor link from 11 to 106 unresolved symbols across unrelated music/player setup. That aggregate inclusion was reverted; the production sound backend needs a deliberate native integration or smaller verified source seam, not a broad source-list addition.
- No standalone game executable or in-game analog test exists; overall progress remains about 10%.

## Coin reward actor dependency slice (2026-10-11)

- Linked the recovered hundred-coin reward routine from the `daCoin_c` aggregate as a host-specific source slice, together with the actual star-marker method, `Event.cpp`, the star-marker clear helper, and the existing star/level helpers. Added the shared ARM9 BSS slots those production functions use; no no-op coin or star behavior was introduced.
- Targeted actor build compiles all additions and now fails at link with 9 symbols (Sound and player/animation behavior). The other 13 tests pass 13/13 with `ctest -E smoke_actor`; `smoke_actor` remains unlinked, so the current full build is still 13/14.
- Updated sources and progress notes are being synchronized to F:. The full project remains about 10%; there is no standalone game executable yet.

## Collision smoke probe correction (2026-10-11)

- The previous collision smoke sampled the center of the KCL mask capacity, which lies outside the occupied castle mesh. Updated it to select real upward-facing KCL triangle vertices and cast short vertical probes around them.
- Built and ran `smoke_clsn.exe`: 28 probes produced 3 hits, 3 shortened rays, and 3 upward normals. `ctest -E smoke_actor` passes 13/13. A fresh full build links 13/14 targets; `smoke_actor` remains the only link failure with 11 unresolved Sound, Player, and overlay references.
- The process ABI bridge and collision-test source are synced to the F: source mirror; the refreshed smoke_clsn binary is copied to the F: project folder. Overall project estimate remains about 10%; a standalone playable game is still not available.

## Analog stick behavior from the Circle Pad patch (2026-10-11)

- Compared the linked GBAtemp patch with its `arm9_controls_hook.s`. Its key behavior is to map the stick to SM64DS's virtual touchscreen joystick record (magnitude, normalized direction, angle, active flag), rather than relying only on a D-pad emulation. The native `CheckInput` PC path already performs this injection with the game's `0x69` joystick radius and game fixed-point angle routine; XInput supplies a radially dead-zoned left stick while preserving partial travel.
- Extracted the raw-stick normalization into `ntr::normalize_stick`, still used by the XInput accessor, and added `smoke_input` coverage for center/deadzone noise, cardinal full deflection, diagonal normalization, and intermediate travel. `smoke_input` passes; the `pc_analog_input` object target is up to date and compiles.
- Targeted actor relink still fails with the known 9 Sound/Player/animation symbols. The analog check exercises the native stick vector math; a playable game and live controller-to-Mario runtime check await the unfinished standalone runtime.
- The repository source update and smoke executable are synced to the requested F: folder; overall project progress remains about 10%.

## Player mega-kill dependency slice (2026-10-11)

- Added a host extraction of the recovered `Player::IncMegaKillCount` routine, the actual `GiveLives` routine and its ARM9 life-count BSS slot, plus the exact 1-Up-logo owner attachment helper it calls. The real actor spawn path remains in use; no player or popup no-op was added.
- The actor target compiles these sources and its link failure is reduced from 9 to 8 unresolved symbols. The remaining failures are sound playback, `Player::Hurt`, and `Player::ChangeState`/animation.
- All 14 other CTest smoke runners pass 14/14, including the added input normalization test. The `pc_analog_input` production object target is up to date. The standalone game still does not link or launch; overall project estimate remains about 10%.
- Sync these sources and the updated status to the F: project directory.

## Resolution and aspect-ratio presets (2026-10-11)

- Expanded the native display resolution shortcut to cycle through 12 presets: 16:9 (1280x720 through 3840x2160), 16:10 (1280x800 through 3456x2160), and 21:9 (1280x549 through 3840x1646). The settings menu displays the detected aspect ratio. These are output/window modes; the game still has no standalone launch target.
- Added `smoke_video` to check all 12 ratios and bounds. It passes, as do all 15 currently linkable CTest targets (the 14 earlier non-actor tests plus video presets). `smoke_actor` remains the only target that does not link, with 8 Sound/Player-state references.
- Copy the resolution changes, smoke runner, and updated progress/status to the requested F: project directory. Overall project estimate remains about 10%.

## Pixel-crisp 2D/HUD presentation option (2026-10-11)

- Added a native menu toggle (`F1`, then `H`) selecting pixel-crisp `COLORONCOLOR` scaling or the existing smooth `HALFTONE` scaling for the DS framebuffer. This keeps DS sprite/HUD pixel edges crisp at PC output sizes. The current framebuffer scaler applies to the composited image, including its 3D pixels; this is a presentation scaler, not a set of artist-made HD replacement assets.
- Extended `smoke_video` to check the option round-trips through the native video settings API. All 15 linkable CTest targets pass; `smoke_actor` remains blocked at link by the same 8 Sound/Player-state symbols. Optional 4K texture packs and per-asset HD replacements remain future work. Overall project estimate remains about 10%.
- Sync the changed video source/header, smoke target, and status to F:.

## Hash-keyed PNG texture replacement packs (2026-10-11)

- Added a Windows Imaging Component PNG loader for hash-keyed replacement assets under `textures/4k/<key>.png`. Enable the existing F1 menu and press `T` to turn replacement lookup on; missing replacements fall back to the original DS texture and are exported under `textures/source/<key>.png` so an artist can upscale them. `textures/4k/manifest.tsv` records keys, native dimensions, and DS format.
- The replacement loader accepts images up to 8192x8192 (64 million pixels), supports alpha, and caches native and replacement texture dimensions separately in the GX texture cache. It does not include upscaled artwork or claim every texture is 4K; users must create replacements from the exported source PNGs.
- Full build compiles and links every target except `smoke_actor`, which still has 8 unresolved production dependencies (Sound, Player::Hurt/ChangeState, and overlay functions). The 15 linkable tests pass 15/15, including resolution/aspect, menu option, and native texture decoding checks.
- The standalone game launch path remains unfinished. Overall project progress remains about 10%.
- Sync replacement loader sources, video menu integration, smoke target, progress note, and refreshed `smoke_video.exe` to F:.

## Replacement PNG loader runtime check (2026-10-11)

- Strengthened `smoke_video` to create a temporary 4x4 alpha PNG replacement for a synthetic 1x1 DS texture, then decode it through the production WIC loader. It checks replacement dimensions, pixel channel order/alpha, and the higher-resolution GX texture contract; test assets are isolated in and removed from a temporary subdirectory of the test working directory.
- `smoke_video` builds and passes with the real replacement load path exercised. The full build still fails only at `smoke_actor` linking, with 8 unresolved Sound/Player/overlay dependencies; all other smoke targets link.
- The PC executable and actual boot-scene dispatch are still absent. Native display/input and optional 3D texture replacements are foundations, while sprite/HUD replacement assets and the actual game scene flow remain unfinished. Overall progress remains about 10%.
- Sync the strengthened smoke source, progress note, and refreshed `smoke_video.exe` to F:.
- Follow-up: initial fixture placement under the OS temp directory was denied by the sandbox. The fixture now uses a unique temporary subdirectory under the test working directory; the refreshed binary passes both directly and through CTest, and all 15 non-actor tests pass 15/15. The previous F: `smoke_video.exe` predates this runtime check and must be replaced by the refreshed binary.

## Actor-link and boot-path dependency audit (2026-10-11)

- Traced the remaining `smoke_actor` symbols to real code: Sound wrappers and mixer calls live in `src/engine/sound/Sound.cpp`; `Player::Hurt` and `Player::ChangeState` have separate recovered files; `func_ov002_020cd28c` and `Player::SetAnim` are in the large aggregated `src/actors/Player.cpp` translation unit.
- A direct host compile of the entire Player aggregate is not viable as-is: MSVC rejects its function-local classes used by `extern` declarations (C2624). Linking the full Sound aggregate expands the actor target from 8 missing symbols to 103 Sound-engine and ROM-state dependencies, so it is not a drop-in fix. The temporary CMake experiment was reverted; the actor target remains at its prior 8 unresolved dependencies without fake definitions.
- Boot startup calls `func_0201a054` before entering `func_020197b8`, whose infinite game loop invokes `dScene_c::SpawnIfNecessary`. The PC runtime has the fiber/VBlank and display seams, but not this complete startup/scene closure or an executable entrypoint.
- Next actor-closure step is host-side extraction of only the needed Player methods from the aggregate, followed by a real platform sound backend/dependency slice. The standalone executable and full boot scene are still absent; overall progress remains about 10%.
- Sync this dependency audit and status to F:.

## Native analog controls aligned with Circle Pad patch (2026-10-11)

- Compared the GBAtemp Circle Pad patch and its `arm9_controls_hook.s`: it writes continuous stick magnitude, normalized direction components, and angle into SM64DS's virtual touchscreen-joystick control record. The native PC `CheckInput` hook already follows that data path and keeps digital touch/D-pad handling available.
- Found and removed duplicate left-stick-to-D-pad mapping in the Windows host input layer. The Xbox left stick now drives continuous virtual-joystick movement only; physical D-pad and WASD continue to supply digital directions. This avoids combining analog movement with a second digital direction input.
- Xbox radial deadzone and scaling are retained in `normalize_stick`, with partial travel preserved and full deflection mapped to the game's `0x69` radius.
- Validation status: code path was compared with the upstream assembly; no live game/controller test is possible yet because the standalone game executable and boot path are not linked. Overall project progress remains about 10%.
- Sync `port/ntr/input.cpp` and this progress note to F:.

## Circle Pad patch source cross-check (2026-10-11)

- Re-read the GBAtemp thread and its linked `arm9_controls_hook.s`. The patch computes a capped vector, writes the fixed-point magnitude, normalized X/Y, angle and active flag into the game's virtual joystick record, and falls back to native touchscreen/D-pad input when the Circle Pad is neutral.
- The existing PC `dScStage_c::CheckInput()` analog path follows those same fields and uses the game's `atan2` routine. The XInput left stick preserves partial travel after a circular dead zone; left-stick directions are no longer duplicated as digital D-pad input. Physical D-pad, keyboard movement and mouse stylus remain active.
- Rebuilt the `smoke_input` and `pc_analog_input` targets through the installed Visual Studio 18 x86 environment; CMake/Ninja reported both targets up to date. No live gameplay check is possible until the standalone game links and launches. Project estimate remains about 12%.

## Circle Pad analog path follow-up (2026-10-11)

- Rechecked the GBAtemp patch behavior against the production `dScStage_c::CheckInput()` integration: analog values are injected into the game's normal virtual-joystick magnitude, normalized X/Y, angle, and active fields, while centered-stick input retains the original touchscreen/D-pad result. The Windows input layer keeps the Xbox left stick continuous and avoids duplicating it as D-pad input.
- The full Windows game target regenerated and compiled its source closure, including the production analog input path, then stopped at link with 1,036 unresolved symbols. This remains a boot/runtime closure issue; it did not expose a compile failure in the analog integration. In-game stick feel still cannot be checked until the executable links and runs.
- A standalone `pc_analog_input` rebuild was attempted outside the configured helper and failed because MSVC could not write its temporary file under the default AppData temp directory. The full build helper uses the bounded workspace temp directory and compiled the game closure successfully. Overall project estimate remains about 18%.

## Actor smoke link closure (2026-10-11)

- Added recovered overlay functions/data and host ABI bridges required by `smoke_actor`; `build-smoke-actor.cmd` now compiles and links the actor executable with BUILD_STATUS=0. `build-all-smokes.cmd` also completed the aggregate build phase with BUILD_STATUS=0; all 16 registered test executables exist.
- The smoke-only audio bridge satisfies legacy audio calls without adding fake audio to a game target. Production runtime work still needs a native audio backend and broader scene/boot closure.
- Runtime validation is not clean yet: CTest reports 15/16 pass and `smoke_actor` crashes during actor resource initialization (access violation). This is a harness/runtime failure to investigate separately from the now-clean link.
- Current actor dependency additions are in `port/CMakeLists.txt`, `port/hal/actor_runtime_bridge.cpp`, `port/hal/actor_bss.cpp`, and `port/hal/smoke_sound_bridge.cpp`. Overall project estimate remains about 10% because no standalone game executable or boot/scene loop exists.
- Sync these sources, the updated CMake file, and this progress note to F:.

## Actor smoke resource-init follow-up (2026-10-11)

- Fixed the smoke harness to install its documented flattened PC actor vtable and stage its actorID. The `0x12b` sign selects YAJIRUSI_L, so the staged SharedFilePtr handles are 1175/1176 (model/KCL).
- The harness now routes the sign model through `Model::LoadFile`, which applies the production pointer fixups. Shadow templates share addresses between overlays 000 and 001: overlay 000 contains unrelated path strings at these addresses, while live overlay 001 has the cuboid/cylinder BMD records. Added `port/tools/overlay1_image.py` to generate the extracted image and relocation data under `build/`; the PC shadow initializer resolves templates from this relocated image. No ROM bytes are checked into source.
- Validation: actor smoke initializes, runs four behavior ticks, renders 19 triangles, and passes. `build-all-smokes.cmd` reports BUILD_STATUS=0 and TEST_STATUS=0; CTest passes 16/16, including `smoke_actor`.
- The native settings overlay already cycles 16:9/16:10/21:9 presets through 4K, chooses 60/120/240 presentation Hz, toggles HD texture replacements and crisp 2D scaling, and composites the bottom screen with opacity and size controls. Added arrow-key minimap position adjustment; both axes clamp to the available space so the map remains on-screen. `smoke_video` verifies position round-tripping and scale-aware clamping.
- Rebuilt all smoke targets and reran CTest after the menu change: BUILD_STATUS=0, TEST_STATUS=0, 16/16 passed. The boot entrypoints `func_0201a054` and `func_020197b8` have a static call closure of roughly 810 source translation units before accounting for dynamic overlay actor dispatch and ROM-backed tables. There is still no standalone game executable or live menu/game integration.
- Overall project estimate remains about 11%. The boot/scene loop, 4K texture and 2D replacement asset sets, and live mouse stylus/game validation remain unfinished.
- Sync the overlay generator, CMake/source/test changes, this progress note, and rebuilt actor/video executables to F:.

## Smoke target link closure and current game link (2026-10-11)

- Repaired the missing host delete bridge on the ModelAnim, animation soak, animation, and actor smoke targets. The actor-only runtime bridge now provides the exact Player health/air-state field reads, collider-list unlink behavior, allocation entrypoint, and memory-fill wrapper needed by that harness. The production game target compiles the real recovered implementations instead; the harness-only definitions are excluded under `SM64DS_GAME_TARGET`.
- Added the recovered `func_0206e330` memory-fill source to the actor smoke target. The full native smoke build completed with `BUILD_STATUS=0`; CTest passed 16/16, including all four affected targets.
- Fixed the batch helper's `findstr` pattern so the percent sign in `100% tests` is escaped as `100%% tests`.
- Rebuilt the opt-in standalone executable target after the bridge changes. All 258/261 compile/link steps completed, but the linker still reports 960 unresolved externals. Representative groups are stage/scene methods, sound engine methods, collision methods, allocator/heap ABI entrypoints, overlay actor dispatch, and ROM-backed data. No playable `sm64ds_pc.exe` was produced.
- Existing 60/120/240 FPS selection paces host presentation while keeping game simulation at the original 60 Hz. The 3D texture replacement path is in place, but artist-made replacement assets and a true upscaled 2D/HUD/minigame asset path are not complete. The 2D pixel-crisp switch only changes final framebuffer scaling.
- The boot/scene closure is still not link-complete. Continue from `build/pc-game-build.log`; do not treat smoke success as a playable game. Overall project estimate: 21%.

## Cumulative PC link closure and host ABI bridges (2026-10-11)

- Fixed the standalone build's log handoff. `port/build-pc-game.cmd` now adds each completed link's unresolved-symbol records to `build/pc-game-link-history.log`; CMake uses that stable cumulative file for source closure and ROM-data discovery. This prevents a retry from dropping translation units discovered in an earlier link, and avoids reconfiguring against the live log while Ninja is writing it.
- Added game-target ABI bridges for raw Itanium-call sites that otherwise could not reach MSVC C++ definitions: actor spawning, vector cleanup, minigame base/particle constructors, movement/control helpers, collision constructors, GX blend/VRAM calls, and legacy `PlayBank2_2D` signatures. The game-only bridge set is excluded from `smoke_actor`.
- The latest production build compiles 2,501 source candidates and reaches link, but still fails with 897 unresolved symbols across scene/level startup, model and collision rendering, sound, overlay dispatch, and a small number of ROM-backed globals. No playable `sm64ds_pc.exe` exists yet.
- Rebuilt the full smoke suite after the bridge changes: `BUILD_STATUS=0`, `TEST_STATUS=0`, CTest passed 16/16. The production bridge translation unit also compiles in the standalone target.
- F: `BUILD-STATUS.txt` and `pc-port-source` need syncing with these current linker counts and bridge changes. Current overall project estimate: 22%.

## Native analog bridge and continued game-link closure (2026-10-11)

- Compared the XInput analog path against the GBAtemp Circle Pad patch data flow: magnitude, normalized X/Y, angle, and active state feed the game's virtual joystick record, with radial deadzone and continuous travel. This remains implemented in the production stage input hook.
- Extended boot source discovery to find class methods defined inside namespaces, and added a legacy MSVC ABI alias for `dExtFrameCtrl_c::WillHitFrame`. The standalone link now closes 2,581 reachable translation units / 2,504 source candidates and has 891 unresolved symbols (down from 894 immediately before this pass). The executable still does not link.
- Rebuilt smoke targets and reran CTest: `BUILD_STATUS=0`, `TEST_STATUS=0`, all 16/16 tests passed, including `smoke_input`, `smoke_actor`, and `smoke_video`.
- Current blockers remain broad scene/level boot calls, graphics/model/collision APIs, sound, overlay dispatch, and ROM-backed functions/data. The analog input path compiles but has no live play test until the native game target launches. Overall progress remains 22%.
- Sync this source-discovery fix, bridge, latest game link log/history, smoke evidence, and status report to F:.

## Host runtime ABI work and overlay closure audit (2026-10-11)

- Added host C++ IRQ disable/restore wrappers, C++ `WaitByLoop` spelling, and CP15 cache-maintenance wrappers to the NTR runtime; cache maintenance is a no-op on coherent desktop memory. The existing raw IRQ/CP15 implementations remain available for C-linkage calls.
- Verified an object-overlay loader dependency against the supplied ROM metadata. Enabling it directly adds unresolved overlay table globals and ROM overlay IDs; a general overlay runtime is still required before the game can boot a level. The repo contains 103 extracted overlay binaries and corresponding relocation tables, but only overlays 001/002 currently have dedicated host image/data generators.
- Tried typed OAM static-call bridges and a Particle alias. The OAM bridges compile but did not lower the unresolved set, and the Particle object already exports the expected MSVC method name; the ineffective Particle alternate was removed. The latest completed native link remains at 888 unresolved symbols and no executable was produced.
- Rebuilt the complete smoke suite after runtime edits: `BUILD_STATUS=0`, `TEST_STATUS=0`, all 16/16 CTest checks passed. Current important blocker is production startup/scene closure, including overlay loading/relocation, model/collision rendering, sound, and C++/Itanium ABI edges. Overall estimate stays 22%.
- Sync latest runtime, bridge, source-discovery, build logs, smoke evidence, and current status to F:.

## Native analog reference follow-up and overlay loader audit (2026-10-11)

- Reviewed the user-provided GBAtemp Circle Pad patch reference against the host input path. Xbox analog values are mapped through the game's virtual joystick representation with normalized axes, direction, magnitude, and a radial deadzone; the implementation compiles, but live game validation still awaits a bootable executable.
- Investigated the missing `LoadOrUnloadObjectOverlays` link dependency. Its recovered routine reads three ARM9 lookup tables and dispatches loads/unloads for seven object overlays per level, plus hard-coded overlays 060/098/102. The corresponding images exist locally, but the desktop target does not yet have a general overlay loader/relocator and runtime registry.
- A trial build that forced the overlay helper into the base closure confirmed the tables can be emitted from `extracted/arm9_dec.bin`, but pulled in overlay-only actors and expanded the unresolved set to 880. The source closure seed was reverted; no fake loader or executable was added. `port/tools/level_overlay_tables.py` and its opt-in CMake generation rule currently provide the real table-data groundwork.
- Latest base closure audit: 2,582 reachable TUs / 2,504 source candidates. Latest native link still fails with 879 unresolved externals; there is no playable `sm64ds_pc.exe`.
- The customary `port/build-port.cmd` verification helper could not run because this shell lacks `py.exe`/system Python. A direct smoke rebuild also failed before tests because the current managed shell could not access MSVC temporary/include files, so the prior 16/16 CTest result remains the latest successful verification.
- Overall project estimate remains 22%. Next major work is a host overlay manager that loads and relocates extracted module images, then resolves level/object overlay residency without pulling every mutually exclusive overlay into the base link.

## Native method-closure repair and game ABI thunks (2026-10-11)

- Fixed `boot_closure.py`'s qualified-method index. The old regex skipped valid reference-returning methods such as `Vector3 &dCcAcPos_c::GetPos()`. The replacement requires a function body while avoiding ambiguous repeated subexpressions; an isolated check recognizes the collider method in under 1 ms.
- Added game-only typed ABI thunks for recovered raw member spellings used by model loading, texture-transform updates, path construction, collision clear/update, scene fade/graphics initialization, and sound sequence limits. Added the host-side `dFdWipe_c` constructor needed by the game bridge. These forward to the production implementations or initialize the same production subobjects.
- The standalone target rebuilt and reached link. After the closure scanner change, unresolved symbols decreased from 870 to 554; expanded actor/overlay vtables now account for much of the remaining set. The target still does not link, so no playable executable is available.
- Reconfigured and rebuilt the smoke suite with bundled Python, Visual Studio x86 tools, and workspace-local MSVC temp storage. CTest passed 16/16, including input, actor, video, model, animation, and heap checks.
- Next work: classify the expanded unresolved methods by owning overlay and distinguish missing sources from real unimplemented ROM functions, then implement host overlay residency/relocation so selected level/object actors can link and run without making every mutually exclusive overlay resident. Overall progress remains 22%.
- Updated audit after the regex repair: current source closure is 1,830 reachable TUs / 1,752 host-compilable candidates / 77 assembly TUs, with 554 unresolved symbols at link. This replaces the previous 2,582/2,504/879 figures for the current build state.

## Circle Pad reference implementation sync (2026-10-11)

- Rechecked the linked GBAtemp patch against its upstream `arm9_controls_hook.s`. The native port follows its movement data path: signed stick axes become the SM64DS virtual joystick's fixed-point magnitude, normalized X/Y, angle, and active flag. Centered stick input leaves the game's touchscreen/D-pad result intact.
- The Windows input bridge reads the Xbox-compatible controller through XInput, applies a circular deadzone with rescaled remaining travel, and reserves the left stick for continuous movement. Physical D-pad, keyboard, and stylus input remain separate.
- Focused verification: Visual Studio's bundled CMake built `pc_analog_input` and `smoke_input`; Ninja reported both up to date. No live in-game analog check is possible while the standalone target still fails to link.
- Latest standalone link log in this workspace: 1,914 reachable translation units, 1,836 source candidates and 77 assembly units; 526 unresolved externals. No playable executable has been produced. Overall progress remains 22%.

## Standalone link-closure continuation (2026-10-11)

- Extended `boot_closure.py` to seed recovered free-function TUs from unresolved MSVC free-function names. This discovered the existing level object-overlay dispatcher and raised the closure from 1,914/1,836 to 1,915 reachable TUs / 1,837 host candidates (77 assembly TUs).
- Added production-backed host ABI bridges for texture transformer preparation, stage 2D graphics initialization, default-heap switching, scene scheduling, wipe-model file loading, fog-table setup, and sound operations. Corrected `dScStage_c::Behavior`'s local declarations for static `CheckInput` and the integer-returning scene selector, and annotated the recovered level-overlay functions so their original Itanium call sites bind to MSVC implementations.
- Three incremental standalone links moved from 526 to 527 (newly discovered dispatcher dependencies), then 520, 516, and 514 unresolved externals. Current build reaches link but still produces no executable. Remaining blockers include overlay ID/residency handling, ROM OAM sprite data, fader vtable slots, actor/audio methods, CP15/IRQ veneers, and missing ITCM routines.
- Focused analog targets were already up to date earlier in this turn; the last successful full CTest result remains 16/16. Smoke targets were not affected by the game-only bridge and closure edits.
- Current overall estimate remains 22%; playable startup, general overlay loading, and runtime validation are still incomplete.

## Model vtable ABI closure (2026-10-11)

- Added host-only `Destructor1`/`Destructor0` adapters for `BlendModelAnim` and `dExtCommonModel_c`, mapping their DS vtable slots to the recovered destructor and heap-deallocation behavior. These adapters are excluded from the ARM build.
- Rebuilt the standalone game target. The link moved from 514 to 510 unresolved externals; it still fails to produce `sm64ds_pc.exe`.
- Current unresolved groups include overlay marker IDs and residency, OAM sprite records, several particle/model/save-data methods, ARM9 ITCM routines, and hardware/IRQ veneers. Overall project estimate remains 22%.

## Cutscene model ABI adapters (2026-10-11)

- Added game-target adapters mapping the cutscene's RTTI-named `dExtSimpleModel_c` / `dExtAnmModel_c` methods to the existing production `Model` / `ModelAnim` code. The adapters preserve the separate model-file and transform-buffer destructor responsibilities, and provide the raw C2 constructor entry points used by the cutscene.
- The standalone target rebuilt and reached link; unresolved symbols fell from 510 to 498. It still does not produce an executable.
- Current high-level blockers include overlay ID mapping/residency, ROM-backed OAM data, particle and save-data dependencies, and hardware/runtime functions. The smoke suite's latest successful CTest result is still 16/16; smoke targets were not changed by these game-target adapters. Overall estimate remains 22%.

## Circle Pad input follow-up (2026-10-11)
- Rechecked the GBAtemp-linked ARM9 hook: it reads a signed stick vector, clamps to the game's 0x69 radius, computes magnitude/direction/angle, and marks the virtual joystick active; neutral input returns to the original touch/D-pad update.
- The PC adapter already uses that same controller record and preserves partial analog travel through XInput's radial deadzone. Corrected the fixed-point radius conversion to round signed components symmetrically before calculating magnitude, and only overrides the game input when the stick is actually deflected.
- Built the production pc_analog_input object and ran smoke_input; both completed successfully. A live gameplay controller check still awaits the standalone executable.


## ROM overlay table integration (2026-10-11)
- Attached the generated ROM-derived level and object overlay lookup tables to the standalone executable. This supplies the actual stage-to-overlay ID mappings used by LoadLevelOverlays from the provided ROM extraction.
- Added PC-only overlay IDs for the hard-coded object-overlay calls, preserving the original &overlay_N DS linker convention. Updated the boot closure generator to emit the adjusted host TU and CMake to watch/rebuild its generated sources.
- Fixed build configuration to use CMake's discovered bundled Python interpreter for source-path lookups instead of relying on a system python command. Reconfiguration now succeeds in the managed Windows environment.
- Standalone compile/link reaches the final link; current unresolved count is 496 versus 498 before the change. The remaining link includes broad rendering/actor/system dependencies, so this overlay table wiring does not yet implement dynamic overlay image loading or produce a playable executable.
- Focused input verification still passes (smoke_input 1/1; pc_analog_input builds). Overall project progress remains 22%.


## Host IRQ, OAM, particle and heap link repairs (2026-10-11)
- Replaced ARM CP15 interrupt/MPU instructions with PC host behavior and connected interrupt masking to the existing frame runtime. Corrected the C++ ABI for IRQ/CP15 callers and removed duplicate definitions already provided by 
tr/runtime.cpp.
- Added the recovered src/engine/oam/OAM.cpp manager to the standalone target so its real OAM staging, render and upload methods are linked; unresolved overlay sprite data remains.
- Added typed raw-name forwarders for Particle::SysTracker::Contents::FindData/Create and explicit heap receiver forwarders to Heap::Sizeof/Reallocate. These resolve to the existing production pool and allocator implementations.
- Latest standalone build reaches link with 476 unresolved externals (down from 480). No executable is produced. Focused controller verification passes: pc_analog_input target builds and smoke_input passes 1/1. Last complete CTest remains 16/16.
- Main unresolved areas are overlay/actor methods and data, ROM OAM sprite records, ITCM routines, fader virtual slots, and remaining system support. Overall estimate remains 22%.


## OAM data and wipe-fader host linkage (2026-10-11)
- Fixed static OAM method aliasing to restrict linker alternates to explicitly qualified `OAM::` definitions, avoiding malformed aliases for constructors/destructors.
- Added generated exports for OAM sprite pointer tables from the local overlay 1/2 extraction and connected the minimap/player icon data to the standalone executable.
- Included the recovered wipe-fader destructor and `AdvanceFade` body through the MMIO-transformed host source. Those virtual slots now link.
- The latest standalone link moved to 519 unresolved externals; after the fader inclusion it is 519 (the prior 521 included two fader slots). Build still does not produce an executable. The enlarged generated overlay closure accounts for more newly visible actor references.
- Current closure audit: 1,647 reachable TUs, 1,572 source candidates, 75 assembly. Analog focused verification remains the last verified live-independent check; no gameplay executable is available. Overall progress remains 22%.

## ROM OVT parsing and smoke-build recovery (2026-10-11)
- Added bounds-checked ARM9 overlay-table parsing to the NTR ROM filesystem. `OverlayInfo` now exposes the eight cartridge fields (overlay ID, RAM address/size, BSS size, static initializer bounds, FAT file ID, compressed size), and overlay payload reads resolve through the ROM FAT. Direct validation against the supplied EU Rev 0 ROM confirmed 103 OVT records and the expected overlay 1/2 addresses and file IDs.
- Corrected the table index and field layout against the actual ROM bytes before using the new API. The overlay manager is not yet wired into game startup; the host still needs decompression/relocation and lifecycle integration.
- Narrowed the fader host transformation to the recovered wipe-fader methods only, avoiding duplicate base fader implementations. Removed duplicate overlay 1 OAM exports (already emitted by the image generator).
- Removed a raw OAM source accidentally added to `smoke_model`; it duplicated the dedicated OAM gate's responsibility and failed to link in that model target.
- Rebuilt all smoke targets successfully and ran CTest: 16/16 passed. Focused input and video targets linked as part of the full build.
- Latest standalone target still fails at link with 518 unresolved externals; no playable executable exists. The main unresolved groups are ARM9 runtime/ITCM routines, system/member ABI methods, overlay actor methods, and static overlay function/data records. Overall project estimate: 23%.

## GBAtemp Circle Pad reference and PC input verification (2026-10-11)
- Applied the requested Circle Pad patch behavior to native Xbox analog: radial deadzone, preserved partial travel, fixed-point virtual joystick vector and angle, and neutral stick leaves the game's touch/D-pad input intact. This uses the already integrated `pc_analog_input` adapter in the production `dScStage_c::CheckInput` path.
- Built `pc_analog_input` and `smoke_input`; the native analog mapping smoke check passes. The full target still cannot be checked in gameplay because `sm64ds_pc.exe` has not linked.
- Continued standalone link closure with PC equivalents for ARM9 ITCM arithmetic/IRQ helpers and host ABI forwarders. Link diagnostics moved from 518 unresolved symbols to 492. A trial that added large recovered GX/GXS translation units did not reduce the unresolved count and introduced additional missing references, so that experiment was reverted.
- Current blockers remain broad actor/system method closure, DS graphics/menu services, sound, raw overlay residency/relocation, and ARM context/fiber routines. No executable exists yet; overall progress remains 23%.

## Native linker namespace ABI closure (2026-10-11)
- Extended the PC boot source generator to add MSVC alternate-name aliases for annotated free functions inside the recovered GX/GXS/G2/G2x, Sound, and CP15 namespaces. Instance methods remain excluded; their `thiscall` ABI still needs typed thunks.
- The standalone target reconfigured and reached link. Unresolved externals fell from 492 to 442, resolving recovered graphics and sound namespace symbols without no-op replacements. It still produces no executable.
- Feeding the latest link output back into the source-closure discovery did not reduce the 442-symbol result further. The latest link log is `build/pc-closure-reseed.txt`; next likely work is class-method ABI adapters and real overlay/system runtime, while some ARM-only context helpers still have no host implementation.
- Focused Xbox analog mapping remains verified by `smoke_input`; live play test remains unavailable. Overall progress remains 23%.

## Address-named C++ helper linkage (2026-10-11)
- Extended the annotated boot-source alias generator to cover global recovered helpers named `func_<address>` as well as the earlier SDK namespaces. It only aliases annotated free functions at global scope; class methods remain excluded because their MSVC `thiscall` needs typed adapters.
- The standalone build reached link and unresolved symbols fell from 442 to 429. This resolved recovered camera/helper bodies already present in C++ TUs; no executable is produced yet.
- Current high-level blockers remain 181 class method/vtable entries, overlay-backed actor code/data, missing host implementations for ARM context/runtime routines, and other game subsystem services. Overall project estimate remains 23%.

## Startup ABI and native Circle Pad follow-through (2026-10-11)
- Added typed host entry points for `Heap::InitializeRootHeap`, `Heap::SetNodeID`, `Heap::InitializeGameHeap`, and `dScene_c::SpawnIfNecessary`, forwarding to the recovered implementations. The standalone MSVC link moved from 376 to 372 unresolved symbols.
- Confirmed the next blocker is not a single missing entry: the original start closure brings in optional actor/scene translation units and their global overlay constructors. The executable still lacks a dynamic native overlay loader and the DS rendering/display services needed to run those paths.
- Native Xbox analog uses the GBAtemp Circle Pad patch's key behavior: compute/clamp radial magnitude, preserve partial travel, update the virtual joystick while deflected, and leave original touch/D-pad input intact at neutral. The focused input smoke check passes; in-game behavior awaits a bootable executable.
- Updated root `BUILD-STATUS.txt` with the current 372-symbol link result and clarified that display settings, minimap, and HD texture settings have not been integrated into a playable game. Overall progress remains 23%.

## ROM-backed host overlay images (2026-10-11)
- Extended `ntr::fs` with `fs_overlay_load`: it reads the ARM9 OVT/FAT payload, handles the overlay BLZ backward-LZ format, checks the output against `ram_size`, appends a zeroed BSS region, and returns the loaded image size. Raw `fs_overlay_read` remains available for inspecting the cartridge payload.
- Added the opt-in `smoke_overlay` target. Against the user's replacement Europe Rev 0 ROM, it decompressed 102 overlays and matched every byte of each available extracted reference image; it also verified each BSS range was zeroed.
- Rebuilt the standalone game target after regenerating the source closure from the preserved full link history. The generated manifest shrank from 385 KiB to 292 KiB, but the link remains at 372 unresolved symbols and produces no executable.
- The overlay API validates and materializes binary images only; it does not yet relocate these images into an executable module system or invoke ARM initializer code. Scene startup, graphics, menu integration, and live controller validation remain outstanding. Overall project estimate: 24%.

## Circle Pad reference follow-up (2026-10-11)
- Rechecked the user-provided GBAtemp Circle Pad patch reference. The existing PC path already routes the Xbox left stick through a radial deadzone and injects continuous magnitude, normalized direction, angle, and active state into the game's virtual joystick record; a neutral stick preserves touchscreen and D-pad input.

## Circle Pad speed calculation alignment (2026-10-11)
- Matched the reference hook's fixed-point speed order in the PC `CheckInput()` path: compute raw vector length, divide by the game's `0x69` virtual-stick radius, then clamp the 12-bit magnitude to `0x1000`. Direction components remain normalized against the same radius, and neutral-stick fallback is unchanged.
- Rebuilt the production `pc_analog_input` object successfully with the installed Visual Studio 18 x86 environment. The standalone executable is still unavailable, so in-game analog behavior cannot yet be checked.

## Host-generated particle renderer slice (2026-10-11)
- Added the recovered ARM9 particle render dispatch and helper functions to the PC executable through hostgen, including the relevant 3D register writes, which are redirected into the native GX adapter. The full target's link failure moved from 196 to 203 unresolveds while direct MMIO C files were temporarily linked, then back to 195 after switching to host-generated sources and adding the final texture-parameter helper.
- Updated CMake's hostgen custom command to invoke the configured `Python3_EXECUTABLE`, avoiding a dependency on a globally installed `python` command. Latest detailed link log: `build/pc-particle-hostgen-slice2.txt`; the executable still does not link.

## Sound caller ABI correction (2026-10-11)
- `ProcessKuppaScript.cpp` declared `Sound` as a class with static methods, while the recovered implementation and shared API use a namespace. Those forms produce different MSVC symbols; the caller now includes `Sound.h` and uses the repository's namespace declarations.
- The standalone link now reports 194 unresolved symbols, one fewer than before; log: `build/pc-sound-namespace-bridge.txt`. CMake hostgen now invokes the configured Python interpreter instead of requiring a global `python` command.

## Source refresh and native linker ABI follow-up (2026-10-11)
- Added source freshness tracking to the boot-closure CMake configuration. It now reruns MMIO host-source generation when any `src/*.c` or `src/*.cpp` input changes; before this, edited host-rewritten copies could remain stale because only the closure script and link-history log triggered regeneration.
- Aligned recovered call declarations for GX/GXS palette uploads and OAM rendering with their actual namespace/static-class implementations. Added a C ABI bridge from the crash-screen raw symbol to `dScene_c::ResetHardwareRegisters()`, and a bridge from the ARM9 player's raw `func_020bd828` call to recovered `Player::CanPause()`.
- Full target now reaches link with 189 unresolved symbols, down from 194. Latest log: `build/pc-dscene-reset-bridge.txt`. The executable still has not linked.

## ROM-backed overlay data and final link progress for this pass (2026-10-11)
- Fixed `link_data.py` parsing for COFF-prefixed Itanium globals: it now removes exactly the COFF prefix and retains the leading underscore needed to match `_ZN...` overlay data records. The previous `lstrip("_")` path missed these symbols.
- The next link-data generation emitted 3,480 ROM-backed data regions and 3,668 aliases with zero unknown symbols. This resolved map/minimap and VS OAM graphics assets from the extracted European ROM. Updated `dMap_c.cpp` to use the shared Event namespace, and `dBgActor_c.cpp` to call recovered `Sound::PlayBank0` through its shared declaration.
- Latest full link reports 157 unresolved symbols, down from 188 before overlay data aliasing. Log: `build/pc-sound-bank-namespace.txt`. Still no standalone executable; overall project estimate is 28%.
- Clarified in `dScStage_c::CheckInput()` that the native stick applies to the first active controller record, independent of the configured active-player count.
- Rebuilt the production `pc_analog_input` object with the installed Visual Studio toolchain and ran `smoke_input`: 1/1 passed. A live in-game controller test remains unavailable because the standalone executable still does not link. Overall project estimate remains 24%.

## Native game ABI link work (2026-10-11)
- Corrected the stale root build status by parsing the newest standalone link output: 403 unresolved entries represented 358 distinct linker names before this work; the refreshed build now has 379 entries representing 343 distinct missing symbols.
- Added typed bridges from CodeWarrior-style call sites to recovered host implementations for `Memory::Allocate(size, alignment)`, Heap allocation/deallocation and utility methods, `Model::UpdateFileOffsets`, polygon/material state, rendering, destruction, and the star marker's collect/link behavior. These use the actual recovered methods and reduce the distinct unresolved count from 358 to 341.
- Extended the ROM-backed link-data generator with a specific typed-name mapping for the flat `int[12]` spelling of `IDENTITY_MATRIX4X3`; that actor reference now resolves to the bytes at the unique ROM symbol. An attempted explicit duplicate storage definition did not help and was removed.
- The four explicit Heap source additions were redundant (their sources were already in the closure), and were removed; no duplicate allocator source remains in CMake. An initial wrapper-alias heuristic emitted no definitions and was removed. The standalone target still fails at link with 341 distinct unresolved names, primarily actor resource-handle constructors/destructors, actor/particle methods, and overlay/runtime integration. No executable is available for launch. Overall progress remains 24%.
- Continued typed ABI work for recovered model/render and gameplay APIs: added adapters for blend advance/C1/D1, material and texture-sequence preparation, shadow model initialization/render/destruction, frame-control flag reads, star tracking/untracking, camera flag updates, actor lookup by ID, and actor sound-object spawning. The full target moved from 358 distinct unresolved names to 324 (351 repeated linker entries); the real `sm64ds_pc.exe` still does not link or run. The latest linker report is `build/pc-closure-restore.txt`. An attempted closure change that retained `@symbol` comments expanded the unresolved set and was reverted. Overall progress remains 24%.


## ABI bridge follow-up (2026-10-11)
- Added typed forwarders from raw decomp symbols to recovered dActor, dCapEnemy, dBgCh_Actr, and Player methods. This resolved 16 distinct symbols in the standalone target.
- Added native ABI aliases for the recovered level overlay load/unload functions used by stage startup. The current full target reaches link and reports 306 distinct unresolved externals (332 unresolved linker entries); no executable is produced. Latest log: uild/pc-stage-overlay-thunks2.txt.
- Remaining high-frequency unresolved symbols include per-actor SharedFilePtr-derived static resource handle constructors/destructors, minigame vtables and callbacks, particle services, overlay image/relocation data, and unported DS graphics/system services. Overall project estimate remains 24%.


## Native link follow-up: actor handles and gameplay ABIs (2026-10-11)
- Added hostgen support for local SharedFilePtr-derived actor resource handles with declared-only constructors and destructors. Generated host copies now forward those methods to the recovered unc_02017e0c initialization helper and unc_02017e34 teardown helper. The full target's unresolved total fell from 306 to 210.
- Added further typed bridges for actor/enemy/collision/player and Jango methods. The latest full link reports 203 distinct unresolved externals in 227 linker entries; no sm64ds_pc.exe is produced yet.
- Removed a duplicate explicit SolidHeapAllocator.cpp source because the boot closure already included it. The current log has no LNK2005 duplicate-definition errors. Remaining blockers include particle callback implementations/vtables, local overlay vtables, ROM-backed overlay/image data, and systems/runtime code. Overall project progress remains 27%.

## Analog lookup ABI correction (2026-10-11)
- Corrected the recovered collision helper's C-linkage declaration of `dActor_c::FindWithID` to return `dActor_c*`, matching the implementation signature. This resolved one unresolved host symbol.
- Full target link now reports 155 unresolved externals, down from 156. Latest log: `build/pc-analog-abi-fix.txt`. No standalone executable is available yet, so Xbox gameplay validation and the requested runtime/menu features remain unverified.
- Overall project estimate remains 28%.

## Host ABI and recovered helper follow-up (2026-10-11)
- Changed the dBgW raw-name forwarders to C linkage and matched `Disable()`'s integer return. The standalone link resolved both `Disable` and `IsEnabled`; a typed forwarder also connected the C caller for `dBgW::UpdatePosAndAngs` to the recovered static method.
- Added the host `Copy32Bytes` primitive for the mapped DS I/O window and C-linkage bridges from raw SDK names to the recovered `cstd::strlen` and divider-result implementations.
- Corrected the G2S screen-map pointer return type and supplied an alias for legacy callers declaring the address as `unsigned int`. Aligned several `ApproachLinear` declarations with their recovered return types, resolving three more linker symbols.
- Added raw-name forwards to the recovered `CopyToViewMat` and `dCamera_c::ChangeState` methods. Latest full build: `build/pc-camera-matrix-bridge.txt`, 144 unresolved externals (down from 155 at start of this pass). No standalone game executable yet; overall estimate is 29%.

## Path runtime symbols (2026-10-11)
- Defined the path-table pointers/count as zero-initialized host globals, matching their ARM9 BSS lifetime and existing level-load setters.
- Added C ABI forwarders for the recovered `PathPtr::FromID`, `NumNodes`, `GetNode`, and `GetUnk004` methods so legacy raw-name callers reach the C++ implementations.
- Full target link now reports 137 unresolved externals, down from 144. Latest log: `build/pc-pathptr-bridges.txt`. No game executable yet; overall project estimate is 30%.

## Overlay index and ROM member-table link support (2026-10-12)
- Added `OVERLAY_ID()` so PC builds use the absolute overlay-table index encoded by DS linker symbols, while console builds retain `&overlay_N`. Corrected the include placement after the first build exposed a trailing-include issue.
- Fixed `link_data.py` to prioritize `data_ovNNN_address` names in MSVC member-pointer diagnostics rather than mistaking `__thiscall` for the data symbol. With the latest link log in its input history, link-data generation sourced overlay state-table bytes directly from the supplied ROM.
- Added C ABI bridges for recovered star/Kpa/jump-Mario destructors and trampoline minigame state handlers.
- Latest full build: `build/pc-overlay-index-retry.txt`, 65 unresolved externals. This is down from 155 at the start of this run. No executable has linked yet; overall project estimate is 31%.

## Host ABI bridge follow-up (2026-10-12)
- Added the MSVC vtable entry for `dWipe_c::SetBackwardTime()` and adapted the ROM's raw three-argument brightness-fader forwarder to the recovered one-argument member. This resolves both fade-related link errors while retaining the existing recovered logic.
- Added the signed-int `ApproachLinear2` raw symbol adapter and an MSVC linker alias for overlay call sites that discard `ModelBase::SetFile()`'s return value. The first rebuild caught a missing overload declaration; after adding it, the full target compiled and reached link.
- Latest full link: 58 unresolved externals, down from 62. Log: `build/pc-wipe-approach-bridges-retry.txt`. No standalone executable is produced yet; remaining blockers include particle callbacks, actor/minigame constructors and vtables, player/audio functions, and DS runtime services. Overall project estimate remains 31%.

## Stage vtable and additional host ABI bridges (2026-10-12)
- Added MSVC aliases from the recovered `dScStage_c` non-virtual method decorations to the stage vtable's virtual entries for `InitResources`, `Behavior`, and `Render`. Each alias targets an existing recovered body.
- The full target compiles and now reaches link with 55 unresolved externals, down from 58. Latest log: `build/pc-stage-vtable-aliases.txt`. No standalone executable exists yet; remaining failures include particle callbacks, minigame class constructors/vtables, player APIs, audio, and DS runtime functions. Overall project estimate remains 31%.

## First native executable link (2026-10-12)
- Closed the remaining standalone linker gaps through production particle callback bodies, MSVC/Itanium method and global aliases, a raw dClipper adapter, and host implementations for the DS soft-float and matrix rotation primitives needed by the Kuri King actor. Added a host vtable source for Trampoline Terror.
- The unresolved-symbol count moved from 27 to zero. `build/pc-runtime-and-vtable.txt` reached the final `Linking CXX executable sm64ds_pc.exe` step; the resulting executable is 669,184 bytes. A follow-up CMake build exited successfully and reported `ninja: no work to do`.
- Copied the executable and current build artifacts to the requested F: destination. The executable has not been launched, so boot and gameplay remain unverified. The GBAtemp Circle Pad behavior remains implemented in the production stage input path and awaits in-game controller validation.
- Resolution, aspect, minimap, texture-scale, and frame-pacing APIs are present, but the settings menu/game-flow integration remains open. No 4K replacement art pack is included. Overall estimate: 36%.

## Circle Pad patch implementation and startup check (2026-10-12)
- Cross-checked the user-provided GBAtemp reference and its linked Circle Pad hook against the native controller path. XInput supplies signed axes; the host applies a radial deadzone while preserving partial travel; the stage input update writes the 0x69-radius virtual joystick magnitude, normalized vector, angle, and active flag. Neutral input preserves the game's original touch/D-pad handling.
- Rebuilt `sm64ds_pc.exe` and `smoke_input.exe` successfully. The focused analog smoke executable links; its latest recorded test result in the project log is 1/1 passing.
- Launched the current executable with the supplied ROM argument; the process remained active for the 8-second startup observation and did not repeat the earlier archive-handle abort. Gameplay and physical Xbox movement remain unverified. Project estimate remains 36%.

## Native startup and GX state follow-up (2026-10-12)
- Fixed the PC GX bridge: the raw game symbols for texture and texture-palette bank selection had been mapped to empty host stubs. They now forward to the recovered GX functions, so retail startup's VRAM-state checks pass.
- Added PC startup paths around DS-only sound initialization, DS settings reads, and the ARM7 boot-mode mailbox. The native executable builds and advances through graphics and ARM9 initialization to the main scene loop entry.
- The process then exits with Windows access violation `0xC0000005` during the first main-scene frame. `build/pc-final-run.err` ends after the main scene entry, so the first scene's render/input update has not yet been verified.
- The production Xbox analog path is included in this successfully linked executable and follows the GBAtemp patch's virtual-joystick record mapping. The build cannot yet demonstrate Mario movement because the first scene frame faults before gameplay input is confirmed. Overall project estimate: 37%.

## BOOT scene lifecycle and Circle Pad follow-up (2026-10-12)
- Continued tracing the first scene frame. The host now resolves the BOOT actor profile, constructs `dScBoot_c`, and enters its native lifecycle through the MSVC virtual-method bridge. `InitResources` completes both screens' VRAM assignment, boot tiles, palettes, and maps.
- The current fault occurs when `InitResources` starts the DS cooperative resource-job scheduler (`func_02058048` / ARM context switch). The latest successful executable is `build/port/sm64ds_pc.exe`; `build/pc-resource-job-trace-run.err` records the furthest startup point. The BOOT menu has not appeared yet.
- Reconfirmed the GBAtemp Circle Pad behavior against the production path: XInput's radially dead-zoned left stick supplies continuous magnitude, normalized direction, angle, and active state to SM64DS's virtual joystick record, using its 0x69 radius. Centered input leaves touchscreen and digital movement intact; keyboard and physical Xbox D-pad remain digital controls.
- The full `sm64ds_pc` build succeeds after the trace additions. The current blocker is startup scheduler compatibility, so analog movement still awaits in-game verification. Overall project estimate: 39%.

## BOOT resources and host frame-yield follow-up (2026-10-12)
- Replaced BOOT's first DS worker callback with a synchronous host-fiber load, skipped the DS-only save/audio branches, and treated base/boot/message overlays (IDs 0-2) as resident because their recovered code and data are linked into the PC executable. Other dynamic overlay IDs still need a real host loader.
- Added a PC `LoadFont` path through the host ROM/NARC filesystem adapter. The run trace confirms both glyph and font-map assets load and the map decompresses into mapped VRAM.
- Adapted the frame-end DS thread sleep to `ntr_rt_vblank_wait()`, so control returns to the native host frame clock. The current build links successfully (`build/pc-host-vblank-rebuild.txt`), and runtime trace (`build/pc-host-vblank-run.err`) confirms BOOT resource setup, actor processing, and completion of simulation frame 1 followed by the VBlank yield.
- Windows capture is unavailable in this session (the computer-use surface inventory returned no native app surfaces). No visual confirmation of the menu, repeated-frame stability, or controller movement is claimed. Overall project estimate: 42%.

## Native BOOT frame loop and fader callback follow-up (2026-10-12)
- The complete `sm64ds_pc` target builds with Visual Studio 18 x86 and CMake.
- The supplied European Rev 0 ROM mounts; BOOT assets and font files load; the runtime progressed continuously through 2,520 simulation frames without crashing.
- On PC, fader callbacks dispatch directly to native methods because MSVC and the ARM ROM use different virtual destructor layouts. The PC color fader retains interpolation while omitting DS blend-register writes; presentation still needs renderer integration.
- Native app surfaces were not available for visual confirmation. Gameplay, controller movement, dynamic overlay loading, settings, 4K/ultrawide output, minimap, texture/sprite upscaling, minigame mouse input, and native audio remain incomplete or unverified. Completion estimate: 43%.

## Native BOOT loop follow-up (2026-10-12)
- Corrected ARM-vs-MSVC fader callback dispatch for the active color fader and safely skipped the optional null brightness-fader object.
- The supplied ROM now advances through at least 2,880 frames in a capped runtime session without crashing; fader callbacks return each observed frame. This verifies sustained BOOT-loop stability, not visual menu presentation or gameplay.
- Native 2D/GX rendering, gameplay and dynamic overlay loading remain the next major work areas. Completion estimate: 44%.

## Standalone folder and smoke link follow-up (2026-10-12)
- The native entrypoint now switches to the executable directory after mounting the explicit ROM path. This puts texture replacement/source-dump paths and other runtime files beside the distributed executable instead of anchoring them to the source checkout.
- Corrected the `smoke_frames` link by adding frame-target-only GX bank no-ops. Texture VRAM is a fixed host window for this frame smoke; the standalone game retains its real GX bank implementation and links without duplicates.
- Rebuilt `smoke_frames.exe` and `sm64ds_pc.exe`; the same build invocation also linked `smoke_video.exe` and `smoke_input.exe`. Launched the updated executable from `F:\AI Games\Mario 64 DS` with the ROM in that folder. Trace shows font loading and ongoing BOOT updates. The UI could not be visually captured in this session.
- The renderer still scales a DS-resolution scene to output dimensions, so 4K presets are output/window modes rather than native-resolution game rendering. Settings lack visual/gameplay verification; no custom 4K replacement art pack is supplied. Completion estimate: 46%.

## Native actor callback and Circle Pad input follow-up (2026-10-12)
- The GBAtemp Circle Pad patch thread's relevant behavior is virtual touchscreen-stick injection into the game's existing movement record. The PC build already routes Xbox XInput's left stick through the same `dScStage_c::CheckInput()` record fields (magnitude, normalized vector, and direction angle), using a radial dead zone and the DS stick radius of 0x69. Input polling and normalization are in `port/ntr/input.cpp`; the game injection is in `src/_ZN10dScStage_c10CheckInputEv.cpp`.
- Actor manager static callback records and the nested lifecycle virtual-member tables are now emitted from the local extracted ARM9 image by `port/tools/romdata.py`. Native startup calls the retail static initializer, and the PC actor scheduler maps recovered ROM callback addresses to their linked host implementations.
- `sm64ds_pc` relinks successfully. Traced startup now creates the BOOT actor, loads BOOT and font assets, and reaches the active actor lists. The short diagnostic run did not verify title-menu progression or playable analog movement; callback progression remains a startup issue. Completion estimate: 48%.

## 2026-10-12 - Native title startup

- Routed the PC offline BOOT transition to the retail TITLE actor profile (actor ID 2), registered the title factory, and linked the title actor and its native dependencies into sm64ds_pc.
- Added a build-time generator for the 54 title course labels so overlay 3 ROM addresses become valid native host pointers. Added the title overlay to the PC resident set and bypassed DS music-driver initialization while native audio remains unwired.
- The Visual Studio 18 x86 build links successfully. With the supplied Europe Rev 0 ROM, the trace reaches completed title resource initialization and begins title callbacks. A capped clean exit and visual/gameplay input check remain unverified; the 121/180-frame runtime checks exceeded the available 35-50 second observation window.


- Follow-up: the title menu also reads course ID and sublevel from stride-8 byte views inside the same overlay table. Updated the local-ROM generator to emit both native byte arrays, preserving the final special selectors -2 and -1.

## 2026-10-12 - Circle Pad input bridge and title input verification

- Compared the linked GBAtemp Circle Pad patch with the game's input flow. Its gameplay behavior writes virtual joystick magnitude, normalized direction and angle; the existing PC gameplay hook already follows that data path with XInput's radial-deadzoned left stick.
- Fixed the PC game key source: `data_020a1162` is a live four-player BSS input table, not ROM-backed bytes. The PC build now supplies zeroed native storage, reads active-high Xbox/keyboard buttons through the host input adapter, and merges analog directions before the ordinary game key-history conversion.
- The supplied European Rev 0 ROM run reached the title. With the left stick forced right, title state logged `held=0010`; an automated A press logged `held=0011, pressed=0001`. The runtime reached 240 simulation frames and returned cleanly. This verifies title/menu input delivery, not player movement in a gameplay level.
- The native target links successfully with Visual Studio 18 x86. Completion estimate: 56%.

## Native Stage factory and lifecycle progress (2026-10-13)
- Added a host-native `dScStage_c` constructor path, PC scene profile validation/dispatch for built-in scene IDs, and PC handling for statically linked overlays 0-7. The European Rev 0 ROM reaches the title; automated title confirmation selects course 0 and queues the transition.
- Verified scene ID 4 is accepted, the native Stage factory constructs and schedules the actor, and the scheduler's ROM slot 1 now reaches `dScene_c::BeforeInitResources` through a PC adapter vtable in ROM slot order.
- The current runtime trace stops inside the Stage actor's `InitResources` action before it returns. That routine uses DS MMIO, archive loading, and level overlay setup; native host support for this path remains the next major gate. No playable level or physical controller movement has yet been verified.
- The PC target linked successfully before the latest Stage slot adapter; the next build/run should resume with the attached `SM64DS_PC_TRACE` diagnostics. The in-tree build may intermittently be held open by a stale hidden game process. Project estimate remains 56%.

## Archive path resolver diagnosis (2026-10-13)
- Rebuilt the standalone target with the ROM-order Stage adapter and added PC-only trace checkpoints through Stage `InitResources` and `LoadArchive`.
- Fresh ROM run verifies scene ID 4, the native Stage factory, the Stage lifecycle adapter, scene graphics startup, 3D engine setup, level ID resolution, and all four archive-unload calls.
- The original blocker was `LoadArchive(6)` receiving raw DS address `0x0208EC88` as a path. The PC target now maps archive indices to catalog names and paths and reads NARC bytes through the host catalog/ROM filesystem before using the existing parser.
- A successful title-to-course diagnostic run with the supplied European Rev 0 ROM confirms that archives 6 (`c2d`), 8 (language text), and 0 (`arc0`) load and parse. Stage completes level graphics initialization and requests dynamic overlays 8, 81, 84, 92, 98, and 102. The run does not prove playable gameplay; native overlay behavior is the next gate.
- The Circle Pad patch parity update is integrated: radial-deadzoned Xbox left stick overrides the virtual touch joystick while deflected; neutral input leaves the original touchscreen/D-pad path intact. `smoke_input` and the production analog object compile successfully, but player movement in gameplay remains unverified. Project estimate: 60%.

## Circle Pad patch parity update (2026-10-13)
- Reviewed the user-provided GBAtemp thread and its linked `arm9_controls_hook.s`. The patch returns to the original touchscreen/D-pad path when the stick is neutral; while deflected, it computes vector length, clamps 12-bit magnitude to `0x1000`, calculates angle and per-axis vector components against radius `0x69`, marks the virtual joystick active, and skips the original touchscreen joystick update for that frame.
- Kept the native XInput adapter's radial deadzone and continuous travel. Reordered the PC-only injection within `dScStage_c::CheckInput()` so it overrides the game's touch-stick values on deflection, clearing the touch-stick delay/counter, while neutral input still runs the original touch/D-pad logic. This does not alter raw touchscreen records used by menus.
- Validation: rebuilt `sm64ds_pc` and `smoke_input`; the input smoke reports radial deadzone, direction, full range, and analog travel pass. Rebuilt the production `pc_analog_input` object with the local writable MSVC temp directory. No actual player movement test is possible yet because Stage initialization still blocks before gameplay.
- Project estimate remains 56%; analog semantics are integrated and compile-checked, with live level input awaiting the existing startup/archive blocker.

## Native target and archive-path follow-up (2026-10-13)
- Rebuilt the `sm64ds_pc` target successfully after the ROM-backed archive reader changes. It links to a Windows x86 executable with the supplied European Rev 0 ROM path supported at launch.
- Fixed the filesystem smoke target's link dependency on `ntr` and added the recovered solid-heap allocator TU to the actor smoke link closure. The actor smoke still fails to link: unresolved symbols remain across broad camera/path/minigame/player/actor code pulled in by that fixture. It is not evidence of a production game target link failure.
- A fresh bounded launch mounted the ROM and ran the BOOT update loop. Its 30-second cap stopped the run before the scripted menu transition. A separate successful automated course run earlier reached Stage, parsed archives 6/8/0, and exposed the six dynamic overlay requests recorded above.
- No native dynamic-overlay mapping exists yet, and the current Stage trace does not establish gameplay. The full menu, rendering, minimap, texture-upscale, frame-rate presentation, and live analog movement requirements remain unfinished. Overall estimate remains 60%.

## Data-only level overlay 8 (2026-10-13)
- Confirmed overlay 8 has no function symbols, its extracted file image joins exactly to the mapped BSS boundary, and all 25 load relocations target overlay 8 itself.
- Added `port/tools/overlay8_image.py` to generate a host-resident image from the local ROM bytes, apply its self-relocations once, translate original DS addresses to host pointers, and validate the mapped data-symbol roster. The native `LoadOverlay(8)` path now installs this data image instead of treating it as unavailable; ARM-code overlays remain unimplemented.
- Added `smoke_overlay8`, which verifies the 25 relocated pointers remain within the image and checks the address translation bounds. `smoke_overlay8.exe` passes. Rebuilt `sm64ds_pc.exe` successfully with the generated image linked in.
- A 1,000-frame run with scripted input remained in the BOOT loop and ended with Windows access-violation status; it did not reach Stage, so the new loader path has only focused data-level validation. The object overlays requested alongside ID 8 have hundreds of functions per map, and overlays 81 and 92 have no source files in this decomp checkout. Overall estimate: 61%.

## Native Stage runtime progress (2026-10-13)
- Corrected the PC startup path for message-bank access: the game can enter a Stage without the original boot scene, so `func_0201d590` now loads the language bank lazily when the message table has not been initialized.
- Wired `LoadFile`, `LoadCompressedFileAt`, and `LoadFileAt` to PC filesystem adapters. The host reads catalog files and NARC members directly, supports both the extracted `LZ77` wrapper and raw DS type-0x10 headers, and copies raw assets into the mapped palette/VRAM windows.
- Replaced the Windows-incompatible DS boot-info read with the native PC equivalent. The executable rebuilds and now advances through text, archives, fonts, and 2D graphics setup.
- Added PC translation for known overlay 8 data pointers. Remaining Stage startup stops at `LoadClsnAndObjects`: level overlay pointer is null. Twelve common model handles refer to ARM9 BSS addresses that are not mapped in this process, and dynamic overlays 81/84/92/98/102 have no native loader; the level overlay needed for collision and object tables is consequently unavailable.
- Xbox left-stick input remains integrated from the Circle Pad patch behavior, and title/menu held/pressed input has been observed in the native runtime. Physical gameplay movement, the full Stage, 4K options, 60/120/240 FPS options, minimap presentation, and optional texture replacement are not yet verified as a finished playable port.
- The production target builds successfully with the installed Visual Studio 18 x86 tools. Current estimate: 72%; native gameplay startup and the requested presentation settings remain incomplete.

- Trace correction: the latest fault address maps to dScStage_c::LoadClsnAndObjects at its first dereference of the missing level overlay, rather than to func_02016ff4. The direct model call change moved startup beyond that earlier failure.

## Production link closure and bounded native startup follow-up (2026-10-13)
- Rebuilt the production `sm64ds_pc` target successfully with Visual Studio 18 x86 Build Tools. Added overlay 2 data references discovered in prior linker output to the generated host closure and implemented the host `ModelAnim2` destructor slots; production link now completes.
- Refactored pre-ROM path discovery to bounded Win32 buffers. A traced run with the supplied European Rev 0 ROM confirmed discovery, a 16 MiB read and mount, ARM9 initialization, entry into the main scene loop, and 120 simulation frames.
- The diagnostic run previously crashed during `ntr::fs_unmount()` after the game allocator had replaced the CRT allocator. Native process shutdown now leaves those startup CRT buffers for process cleanup; the new executable links, but a clean-exit trace for this specific change has not yet been captured.
- Current `smoke_actor` still has 26 unresolved symbols; other smoke targets were not revalidated. This link milestone does not establish playable Stage startup. Dynamic overlays, Stage/gameplay, visual settings, minimap, upscaled art, and live movement remain open.
- Xbox left-stick analog injection continues to follow the GBAtemp virtual Circle Pad behavior and compiles into the production target. Actual movement in a playable level remains unverified. Overall estimate: 78%.

## Smoke actor link closure restored (2026-10-13)
- Updated the MSVC build wrapper to use a project-local temporary directory and discover the bundled Python runtime, removing environment failures that prevented a full smoke build.
- Changed `smoke_actor` to link the recovered actor/game sources from a static library. This keeps archive members lazy until referenced and prevents unrelated whole-program undefined references from contaminating the Arrow Sign lifecycle smoke.
- Added actor-smoke-only failure fixtures for unrelated scene-transition, camera/path, fader, star, and minigame methods. They abort with the symbol name if execution reaches those paths; production bridges retain their normal implementations.
- Built all smoke targets and ran the registered suite: CTest passed 17/17, including `smoke_actor`, input, video, filesystem, and overlay checks. The production `sm64ds_pc` target was also relinked successfully after the CMake change (`BUILD_STATUS=0`).
- Remaining port gates are still full playable Stage startup and live level movement; the menu resolution/aspect/FPS settings, adjustable minimap, optional upscaled art and minigame stylus settings are not complete. The recent 120-frame ROM trace remains evidence of main-loop execution only, not a playable level or clean exit of the newest binary. Overall estimate: 80%.

## Clean process teardown verified (2026-10-13)
- The previous 120-frame run still faulted inside `ExpandingHeapAllocator::Deallocate` after `main` returned, confirming that C++ static destruction continued freeing CRT-owned host buffers through the installed game allocator.
- Updated native shutdown to close the video window and call `ExitProcess(0)`, allowing Windows to reclaim process-owned resources without invoking those mismatched CRT destructors.
- Rebuilt the production target successfully (`BUILD_STATUS=0`) and launched the latest executable with the supplied European Rev 0 ROM. Trace mounted the ROM, ran 120 simulation frames, closed the window, and exited with process code 0; no unhandled exception was logged.
- This verifies bounded startup, frame-loop stability, and clean shutdown. It does not establish playable Stage/gameplay startup or movement. All 17 smoke tests passed on the preceding full smoke run.
- Remaining requested presentation options and full playable gameplay are still outstanding. Overall estimate: 81%.

## Inactive player slot crash isolated (2026-10-14)
- The prior course-confirm trace showed the active player initialized with valid models, then an inactive object decoded a host stack pointer as packed player parameters and selected model slot 6, causing an access violation in `Player::SetAnim`.
- Added a PC-only `Player::InitResources` guard that skips player slots at or above the configured single-player count. Rebuilt `sm64ds_pc` successfully (link completed at step 261/264).
- Re-ran the supplied European Rev 0 ROM for 1,200 simulation frames with automated course selection and a frame capture. The crash no longer reproduced and the executable exited with code 0. The screenshot still shows a flat green frame with scattered glyphs. `dScStage_c::InitResources` cycles repeatedly and readiness reports `mode=0 ready=0`, so stage initialization and playable rendering remain unresolved.
- Captured and showed the screenshot for this runtime check at `build/game-frame-resume45.bmp`.

## Single-player Stage retry latch and next runtime fault (2026-10-14)
- The Stage readiness trace showed `mode=0 ready=0` forever. The recovered `mWaitingForLoad` latch at object offset `0x9c4` only belongs to multiplayer transitions, but native `operator new` does not provide the original zeroed DS heap state. Initialized the latch during the PC Stage allocation and added a single-player guard to clear any stale retry value before waiting.
- Rebuilt `sm64ds_pc` successfully (link completed at step 260/263). The title remains readable in a stable 450-frame boot capture at `build/game-frame-resume47-title.bmp`.
- Automated course-0 confirmation now progresses beyond the readiness loop and requests dynamic overlays 81, 84, 92, 98, and 102. That test then crashes in `dCamera_c::IsUnderwater` before its configured frame capture. Native dynamic overlay loading and correct camera/player setup are still required for gameplay; no playable scene is established.
- The crash run had no screenshot because it faulted before the late-frame capture. Captured and displayed the stable course-select screen in a separate successful run for this test cycle.

## Archive path host-pointer fix and course-load checkpoint (2026-10-14)
- The PC archive loader previously passed a 64-bit host string through a 32-bit `int`, truncating paths before `pc_fs_archive_path_size`. Replaced it with a 1-based archive index token resolved by the PC loader. The NARC reader now parses the supplied ROM's BTAF/GMIF chunks into low-address buffers to match the game's 32-bit archive resource fields.
- Rebuilt `sm64ds_pc` successfully (link completed at step 262/265). A course-select run now captures `build/game-frame-resume56.bmp`; screenshot is still the readable course list, because the course transition faults before rendering a level.
- Latest exception is at module RVA `0x3d232`; symbol-map inspection places this in `dExtShadowModel_c::InitCylinder`/adjacent shadow setup, with EAX `0x88000000`. This shifts the next investigation to `port_overlay1_address(0x020ad560)` and native overlay residency/address translation. It does not show successful Stage startup.
- The requested settings integration, translucent minimap, optional 4K art replacements, higher frame-rate execution, controller gameplay verification, and packaging remain incomplete. Keep screenshot capture enabled for each runtime check.

## Stage model data belongs to a dynamic overlay (2026-10-14)
- Fixed the `LoadModel` boot-closure rewrite so the generated host TU retains the PC implementation; it now reads the pointer value stored in `data_0209f340`, rather than dereferencing the host global as if it were the level record.
- The native target rebuilds and launches with the supplied ROM. Course-confirm tracing proves stage index 1 selects DS address `0x02112bdc`. The overlay-8 mapper returns null because this address belongs to the address space reused by dynamic overlays, including overlay 9; dynamic overlay image mapping/residency remains the current blocker.
- The process faults after the null-stage-data guard, before gameplay. `build/game-frame-resume63.bmp` shows the course list and was displayed for this test. Keep capturing and showing a screenshot for each runtime test.
- Resolution choices through 4K, minimap integration, upscaled texture assets, 60/120/240 pacing, physical controller play, and copying a verified build into `F:\AI Games\Mario 64 DS` remain outstanding.

## Dynamic overlay 9 and actor spawn arguments (2026-10-14)
- Added `port/tools/overlay9_image.py` and linked its generated overlay image into the production target. The native image covers initialized data and zero-initialized BSS, rebases 176 overlay-9 self-relocations, and is loaded by `LoadOverlay(9)`.
- Course-confirm now maps the stage-1 data pointer `0x02112bdc` and reads model ID `0x0797`; Stage model initialization reaches player creation. Fixed the actor spawn bridge to forward both trailing spawn arguments through `dBase_c::Spawn` and `func_02042ffc`, so the player number no longer comes from an uninitialized stack value.
- The latest course-confirm run still crashes in player collision setup, at `RVA 0x0cd09` / preferred address `0x0040cd09` inside `dBgCh_Gnd::DetectClsn`. The captured and displayed `build/game-frame-resume66.bmp` is black because the fault occurs before the first useful rendered frame.
- Requested resolution/menu integration, minimap overlay, 4K texture pack, 60/120/240 pacing, controller gameplay validation, and packaging in `F:\AI Games\Mario 64 DS` remain unfinished.
## 2026-10-15 - Local original-menu investigation and live save aliases

- The expected new-game character is **Yoshi**. The user's correction is recorded; final native 4K default-route and reload checks use Yoshi (character 3).
- Original file-menu code is linked behind `SM64DS_PC_ORIGINAL_MENU=1`. Native factory, canonical data, source-declared self relocations, firmware adaptation, message-bank startup and exact ARM tail-jump adapters let the scene run 1200 ticks without crashing. Its graphics remain scrambled in both rendering paths, so the route stays disabled by default and is not installed on F:.
- Fixed live save interior aliases so real star and coin getters read the loaded parent record. A private six-star/77-coin fixture survives native pause save and a fresh process reload; checksums validate. These are seeded storage checks, not completed gameplay accomplishments.
- Local build passes 39/39 tests and the original-ROM/native-image audit. Same executable passes 4900-tick save, 2200-tick reload and 4200-tick fresh-start runs, all exit 0. Fresh native 4K screenshot is RGB-identical to the previous verified Yoshi checkpoint. Gameplay and save-confirmation captures inspected.
- See `native-file-menu-development-2026-10-15.txt` for exact executable hashes, evidence, limits and reproduction. F: remains at the preceding verified native-save checkpoint. Keep Sol High for menu reverse engineering; goal remains active. Full port is unfinished.

## 2026-10-15 - Menu DMA/helper repairs and GPU rendering handoff

- Corrected DMA fills and isolated incompatible file-local `inline_fn` helpers in generated PC copies. Actual menu traces now keep inactive panels hidden; stray sprites disappear. Retail sources remain unchanged.
- Final native build passes 41/41 CTest tests plus two Python transformation tests and native data/render audits. Fresh Yoshi gameplay (4200 ticks) and seeded progress reload (2200 ticks) exit 0 at native 4K. Fresh screenshot remains RGB-identical to the earlier verified Yoshi checkpoint.
- Original menu is still unfinished and disabled by default. Matching tick-300 dumps show identical sampled geometry in software and native runs, but only the GPU capture magnifies the title star. Stop further repairs after the two focused attempts; recommend Astra Low for this remaining GPU-path investigation.
- Full evidence, exact executable hash, reproduction and next actions are in `native-menu-render-handoff-2026-10-15.txt`. No F: files changed. Preserve the full goal; pause for the recommended model switch under the user's standing instruction.



## 2026-10-15: Original menu star rotation ABI

Fixed negative short arguments reaching native int rotation helpers as positive values (e.g. -113 became 65423). Same-tick software/GPU evidence disproved the earlier GPU-only diagnosis. Generated PC declarations now match the native primitive ABI; original source and GPU scale unchanged. Build, 42 native tests, four hostgen tests, ROM data audit and 1200-tick 4K menu capture pass. Original menu remains opt-in and incomplete. Details: `native-menu-star-fix-2026-10-15.txt`.


## 2026-10-15: Original menu BG0 selector

Main BG0 tile scanout now respects the 3D selector; stray font characters disappear in native4K menu capture. Build and all42 tests pass, with added main/sub BG0 selector regression coverage. Menu presentation and input remain incomplete; next input-index ABI boundary and touch-replay evidence are recorded in `native-menu-bg0-fix-2026-10-15.txt`.


## 2026-10-15: Original menu native input

Typed selected-record adapter and native player1 touch/buttons now advance the original star sequence to the Mario-head title. 43 tests and two transform tests pass. Adventure tap exposes a new constructor/initializer ABI crash; no full file-select claim. Exact reproduction and disassembly evidence: `native-menu-input-handoff-2026-10-15.txt`. Original menu stays opt-in; F unchanged.


## 2026-10-15: Adventure transition reaches original file selection

Generated constructor now passes its allocated object to the real initializer. Original-source regression reproduces the missing argument; corrected test covers16 count/flag cases. Build,44 native tests,6 hostgen tests and data audit pass. Identical formerly crashing1800-tick replay now exits0 and visibly reaches File A/B/C. Full file selection/game handoff and menu layout remain unfinished; F unchanged. See `native-menu-file-select-handoff-2026-10-15.txt`.




## Toon/highlight replay verified (2026-10-16)
- Original menu toon/highlight shading now reads the original palette in both CPU and GPU paths. Build and all 45 CTests pass (22.86 s).
- The same executable completed the 6000-tick File A replay with exit 0, reaching idle Yoshi in level 1 at native 3840x2160. File-select and gameplay screenshots inspected and shown.
- Castle blue/lavender shading is restored; pale polygon patches and reference appearance still need investigation. Orientation may represent an intentional reflection and is not verified as defective. Observed texture matrices were identity.
- Evidence: native-file-menu-original-toon-shading logs/state/captures; SHA256 4C33D2238C1D492872819E7B36F0440D6BE6A6F995DCDCFC7C873AD61B75A1CB. See native-menu-artwork-handoff-2026-10-16.txt. F installation unchanged; full port remains incomplete.

## Toon precision correction verified (2026-10-16)
- Added separate 5-bit red / 9-bit interpolated intensity for toon/highlight polygons in CPU and GPU rendering, including clipping. The entry18/19 gradient boundary test passes in both paths.
- Build and all45 CTests pass (13.32s, toon-precision-final-ctest.log). Initial test-only framebuffer stack failure resolved by reusing its existing buffer; failed log retained.
- Same binary B8B65B0081A24DC40217ACE4940F429D725237911738E91D07B08455B160E680 completed toon-precision6000 File A replay, exit0, idle Yoshi in level1. Menu1780 and gameplay5900 PNG screenshots inspected and shown. Previous pale menu patches no longer apparent.
- Exact hardware fixed-point lighting/coverage and menu reference fidelity remain unverified. F installation unchanged. Full port goal remains active.

## Original-menu pause integration (2026-10-16)
- File A -> native PC Settings -> texture75% -> gameplay succeeds at4K; preference saved and paused position stationary. Replay6700 exit0, screenshots inspected/shown.
- Presentation defect found: map sprites/clouds overlap custom settings text. This remains the next repair; functional success does not establish menu visual correctness.
- Save action label corrected to SAVE CURRENT FILE because original SaveCurrentFile uses the selected slot. Label assertion and all45 tests pass29.51s. Current label-only binary1B0A095AEB0BD70F0E0629D3998DA4949D1236EAB707E3B24E448285850018F5; replay used previousB8B65B build. F unchanged.
# Goal continuation progress (2026-10-16)

- Reconciled texture planning into two separate deliverables: SUPIR-upscaled
  content across the full game-texture inventory, including characters; and
  source-matching AI Generated 4K content for eligible level/object textures.
  Character textures are explicitly excluded from generated content and must
  resolve to the SUPIR-upscaled set in that mode. Updated both the PC settings
  plan and the detailed SUPIR/texture-quality plan; no runtime feature or art
  pack was added in this planning milestone.
- Verification: inspected both edited documents and searched for/removed the
  contradictory character-scope rules. No build/tests run; documentation only.
- Remaining concrete work includes native anisotropic filtering, render distance,
  FSR3/AA, motion blur, both texture modes and packs, course/actor/audio coverage,
  physical Xbox verification, standalone packaging, and complete port QA.

- New renderer request (2026-10-16): plan a selectable OpenGL path specifically validating software-driver behavior, distinct from the existing D3D11 and 256x192 software diagnostic renderer. Scope and feasibility are documented in pc-settings-menu-plan-2026-10-15.txt; not implemented.

- Implemented D3D11 anisotropic filtering controls on a new Graphics page:
  Off/2x/4x/8x/16x, persisted in the native INI and applied to 3D sampler states
  while preserving per-axis clamp/wrap/mirror behavior. Release build and all45
  CTests pass, including WARP sampler-level checks, menu cycling and INI reload.
  Native game launch/replay did not complete or produce a capture after startup;
  feature is not installed to F: and runtime visual improvement is unverified.
- Runtime follow-up (2026-10-16): after rebuilding sm64ds_pc.exe, two bounded
  native launches (6000-tick File A and 1800-tick title/file-select), plus a
  direct Start-Process capture attempt, all stop after the same startup messages
  ending at “sound group”. No state file, screenshot, or reliable exit code is
  produced. D3D11 initialization reports the NVIDIA RTX 5070 successfully; no
  F: files were touched. Automated GPU WARP and menu/persistence tests remain
  green. Do not install this build until the native startup stop is explained.
- Differential check (2026-10-16): the pre-anisotropic executable was launched
  with the same direct Start-Process capture and stopped at its normal GPU
  initialization output without creating a frame/state capture or returning to
  the PowerShell wrapper. This reproduces the observation on the prior binary,
  so current evidence does not isolate an anisotropic regression; the bounded
  Windows process-capture/runtime setup itself needs diagnosis by the next model.
- Touchscreen minigame display request (2026-10-16): plan selectable stacked
  Top-over-Bottom, Left/Right, Right/Left layouts, with a 75% top-screen and
  25% bottom touch-screen allocation. See pc-settings-menu-plan-2026-10-15.txt;
  implementation and touch-coordinate verification remain pending.

- Anisotropic filtering runtime follow-up (2026-10-16): traced 5-frame and 1800-frame
  native launches completed successfully; a 6000-frame File A replay also exited 0,
  entered level 1, and ran as Yoshi at 3840x2160. The 5900-frame screenshot was
  inspected: Yoshi and the castle-grounds map overlay are visible. This run used the
  default anisotropic setting, so it verifies startup/gameplay stability, not the
  visual effect of enabling anisotropic filtering. Captures and state are under
  build/port/native-file-menu-baseline-anisotropic-yoshi-*; screenshot PNG is
  native-file-menu-baseline-anisotropic-yoshi-5900.png. The earlier startup-stop
  observation was caused by the untraced capture route and is superseded by these
  successful traced runs. F installation remains unchanged.

- Anisotropic 16x runtime check (2026-10-16): staged a separate runtime with
  anisotropic_filtering=16 in its INI. The 6000-frame native D3D11 replay exited 0,
  reached Yoshi gameplay at 3840x2160, and captured frame 5900. This verifies the
  setting-loaded game remains playable; no before/after surface comparison was made,
  and the screenshot has an unusually low camera angle, so image-quality benefit is
  not established. The isolated runtime is build/anisotropic-16x-runtime and capture
  is build/port/native-file-menu-baseline-anisotropic16x-yoshi-5900.png. F install
  remains unchanged. Render-distance implementation investigation found no existing
  distance-culling/far-clip setting in port/ntr; a real control needs renderer/world
  visibility analysis rather than a display-menu-only change.

- F: installation updated (2026-10-16): backed up the prior executable and map to
  F:/AI Games/Mario 64 DS/native-checkpoints/native-checkpoint-2026-10-16-anisotropic,
  then installed build/port/sm64ds_pc.exe and its matching map. Installed executable
  SHA256 is DD65386CCA34B2A20694C4BF9C97C2ADB8BE5B849F421BCB39819222E91F2CEC,
  matching the built executable. The F: Europe ROM SHA256 matches the workspace ROM.
  A bounded 1800-frame launch from the installed folder exited 0 and initialized
  NVIDIA D3D11; capture shows the black-background star startup splash at default
  1280x720, so this verifies install/startup, not a full F: gameplay session. Test
  save output was redirected to the workspace; no F: INI was created. Earlier 6000
  frame 16x test-copy replay reached Yoshi gameplay at 3840x2160. The overall PC port
  remains incomplete; planned graphics/input/texture/package work remains.

- F: title progression follow-up (2026-10-16): the installed executable SHA256 still
  matches the workspace build. An 1800-frame launch with no scripted input stayed at
  the initial star scene. A separate 2400-frame run with original-menu mode and one
  synthetic A press at poll 450 also stayed there. A third 3000-frame run with
  synthetic A presses at polls 450, 900, 1350, 1800 and 2250 exited 0, but captures
  at ticks 1780 and 2980 still showed only the animated star (78 raster triangles);
  recorded game level remained 0. The title animation is running, so this is not a
  process crash. The same binary and ROM previously reached file select in the
  workspace replay. Test saves remained redirected to build/installed-f-drive-smoke-saves.
  No source changes were made from this symptom. Next diagnostic should compare input
  poll and title-state transitions for installed-folder launch against the known-good
  workspace runner, then verify real A/Start handling before changing title rendering.

- Installed-launch working-directory check (2026-10-16): repeated the 3000-frame
  synthetic-confirm test while the process current directory was F:\\AI Games\\Mario 64 DS,
  matching Play Mario 64 DS.cmd. It again exited 0, initialized D3D11, and remained at
  level 0 with 78 triangles at captures 1780/2980. The working-directory mismatch is
  ruled out as the cause. The installed executable and Europe ROM hashes match the
  successful workspace replay inputs. F: title input/progression remains unresolved;
  next step is a Sol Medium trace of input poll values and title actor state in the
  known-good and F: launch routes. No speculative source edits were made.

- Title replay reproducibility correction (2026-10-16): controlled 1280x720 and
  3840x2160 runs with the same executable/ROM and repeated A taps both remained on the
  animated star through 3000 frames. The exact original-menu runner was then rerun
  with its normal 4K INI, auto-confirm at input polls450-454, TRACE and DSMT trace;
  its log confirms the A pulse and it exited0, but screenshots at1180/1780 show the
  star, not file select. Therefore the older native-file-menu-original-anisotropic-
  traced-title-1780.png file-select capture is not reproducible and must not be used
  as proof that the workspace reaches file select. Current evidence points to a
  general title/input progression problem, not an F:-only issue or resolution issue.
  No source code was changed. A Sol Medium investigation should compare title actor
  callbacks, emulated key input consumption, and VBlank/scene updates around the A
  pulse, and verify all observed menu states before any further install replacement.

- F: title/menu correction (2026-10-16): the earlier keyboard-only title probes
  intentionally used the wrong control for this touchscreen-driven original menu.
  The previously successful file-select screenshot was valid; its touch schedule was
  documented in native-menu-file-select-handoff-2026-10-15.txt. Replayed the current
  installed executable DD65386CCA34B2A20694C4BF9C97C2ADB8BE5B849F421BCB39819222E91F2CEC
  from the actual F: working directory with AutoConfirmPoll=0 and touch taps
  450:128:96 (title), 1250:128:168 (Adventure), and 1850:64:70 (File A). The 6000-frame
  run exited0. At1780 it visibly shows File A/B/C; at6000 state is level1, idle Yoshi
  (character IDs3), 1280x720. Screenshot at5900 shows Yoshi and the minimap. The
  workspace QA save directory was used, preserving F: saves. No source changes were
  needed; this supersedes the earlier claim that the installed title input path was
  broken or that the successful file-select screenshot was not reproducible. Build
  and matching map hashes on F: are DD65386CCA34B2A20694C4BF9C97C2ADB8BE5B849F421BCB39819222E91F2CEC
  and 715935C0559E95EC3F67DB8257576D1C56F4F614388826FE211C7787084207C1. The last
  recorded CTest run for this exact executable passed45/45 at04:14; this replay did
  not rerun CTest. Evidence: build/installed-f-drive-touch-file-a.exit-code.txt,
  build/port/installed-f-drive-touch-file-a-state.csv, and captures
  build/port/installed-f-drive-touch-file-a-{1780,5900}.png.

- Touchscreen dual-screen layout architecture assessment (2026-10-16): the
  presenter currently shows one DS engine full-screen and the minimap as an overlay;
  bottom-screen touch coordinates map across the entire client area. Correct 75/25
  layouts therefore require one shared aspect-preserving geometry transform for
  D3D11/software composition and inverse touch mapping, plus a scene lifecycle signal
  to activate layouts only for touch minigames. Minigames use dScMg* scenes, but no
  scene-active signal is currently exposed to the video layer. Added the ordered
  implementation/verification requirements to pc-settings-menu-plan-2026-10-15.txt.
  No code or tests changed. This combined scene/GPU/input task is recommended for
  Sol Medium; avoid exposing settings until the layouts and touch mapping both work.

- Touchscreen layout development (2026-10-16): implemented shared 75/25 screen
  slots, aspect-preserving output and resize letterboxing, inverse bottom-screen
  touch mapping, D3D11/software composition, and POWCNT1 engine routing. Added
  persisted minigame_layout=0/1/2, native PC Settings page7/7 and F1/L selection.
  The first development checkpoint built successfully and all46 CTests passed;
  the new GPU checks compare each layout/engine assignment against CPU output.
  A4900-frame baseline replay reached the native settings page, changed the
  selection and saved it. A5400-frame original-menu File A replay exited0 in
  level1 with Yoshi IDs3. The checkpoint executable SHA256 is
  6C70470540F18BB4C4A29209045BD483D1E8107AECD75AE38643A8C120BA0019,
  saved with its map under build/minigame-layout-checkpoint.
- Actual Rec Room verification initially revealed missing native scene profile5.
  Added its recovered factory to the host closure/table, preserved overlay5's
  contiguous data/OAM labels with32 documented pointer relocations, and adapted
  two stale API declarations to the canonical host headers. The new build links;
  its linked-data audit checks3536 bytes,32 pointers,48 labels and profile5.
  A2400-frame original-title Rec Room replay now exits0 and captures both screens
  in stacked75/25 slots. This establishes Rec Room presentation, not playable
  minigames. Remaining work includes selection/gameplay, all three runtime
  orientations, software, touch/pause/return checks and installation. Details and
  evidence are in minigame-layout-checkpoint-2026-10-16.txt. F: has not been
  replaced by this development build; the full goal remains active.

- VS scope addition (2026-10-16): recorded native owner-to-owner P2P VS and a
  separate Download Play path in the original VS menu. A host launches a browser
  build and shares a URL with up to three guests; the host serves only the code
  and assets required for VS, so guests need no native copy or ROM. Networking,
  browser runtime, VS-only package, reachability, lobby and real four-player
  validation milestones are in vs-multiplayer-download-play-plan-2026-10-16.txt.
  This is planned scope, not implemented functionality; touch-input work continues.

- Wanted ABI investigation (2026-10-16): compiled and ran the actual MSVC x86
  layout probe. Three 4-byte native member callbacks shrink dMgState_c by12
  bytes versus the DS layout; the base and Wanted fields then shift12 bytes.
  Original state handlers still write8-byte callback records and shared base
  helpers still use fixed DS offsets. Enabling the missing factory alone is
  unsafe. No production changes, full build/tests or runtime replay in this
  investigation. Recommend Sol High for a coordinated host-only callback/layout
  repair; pause for the user's model switch. Exact evidence and continuation
  steps: wanted-native-abi-handoff-2026-10-16.txt.

- Minigame touch repair (2026-10-16): restored canonical16-byte touch history with
  held/edge/x/y interior labels and native local-player sampling. Real Rec Room
  testing then exposed a cartridge-vtable-slot mismatch: all six input/save gates
  now call the typed native IsAtStart method. Character-tab touches and Back to
  the original title are verified with screenshots. Corrected smoke_stage_graph
  to test inactive-scene pause isolation and active-Stage pause separately.
  Selection further exposed an unconstructed wipe object; it now has canonical
  native storage/construction (48 bytes, not the44-byte ROM layout). The final
  executable58D32BE5220602A2196C0CBDF1B79197A7CB83FB83A371826E270E31AB71123C
  builds, passes48/48 CTests and14 Python checks, and passes both data audits.
  A final2700-frame Wanted-selection replay exits0 without an exception and
  requests the missing native profile366. Actual minigame gameplay remains
  unimplemented/unverified. An independently observed Adventure action crash
  after a mistimed pause replay is retained for comparison; no cause/fix claim.
  See minigame-touch-fix-2026-10-16.txt for hashes, exact replay evidence and
  limits. Preview is in build/touch-fix-preview; F: install remains unchanged.

- Wanted callback/layout repair (2026-10-16): restored the shared minigame
  controller's8-byte native callback records and exact0x28/0x4660/0x545c
  controller/base/Wanted layouts through host-only header/source generation.
  Connected native profile366,72 callback constants,23 startup table slots,
  1522 resource labels,1157 pointer relocations and38 indirect graphics workers.
  Repaired shared DS virtual-slot calls, C-to-native graph dispatch, the raw
  path file-loading seam and LoadFile tail wrapper. Added layout/dispatch/data
  checks and PPU diagnostic/display/window-color regressions. Final EXE hash
  11704D5111877B5F5E79DEED297E88D493AE5966D1D116B8FBCB4D4D7591B876
  builds, passes49/49 CTests,14 existing hostgen plus5 new Python checks and all
  three linked-data audits. Real Wanted tests now enter play, count down, accept
  a correct Mario touch, add5 seconds, start another round and pause/resume.
  Normal-play poster visibility and pause controls remain incomplete because
  WIN0/WIN1/OBJ-window composition is not implemented; return-to-Rec-Room is NOT
  verified. Recommend Astra Low for that renderer work and pause for the model
  switch. See wanted-native-callback-checkpoint-2026-10-16.txt for exact replays,
  screenshots, limitations and next steps. Preview: build/wanted-callback-preview.
  F: installation remains unchanged; the overall port goal is incomplete.

- Final Wanted-build Stage regression (2026-10-16):4900-frame baseline replay
  exits0 in level1 as Yoshi; native PC Settings page8 is visible at4500 and
  gameplay/minimap resume at4880. Screenshots inspected. Exact Wanted checkpoint
  EXE unchanged; full final49/49 CTest pass takes21.63seconds. This does not
  diagnose the separate original-menu Adventure action crash.

- Wanted window masking follow-up (2026-10-16): previous Astra Low milestone
  repaired both2D window engines and per-line HBlank snapshots. Normal-play
  poster visibility verified in GPU/software,all3layouts and4K.50/50 tests pass.
  See wanted-window-masking-checkpoint-2026-10-16.txt for exact evidence.

- Wanted pause touch flow (2026-10-16,Sol Low): fixed the options function's
  byte-return declaration and the resume method's raw DS vtable dispatch.
  Pause buttons now animate onscreen,render,and accept Continue/Quit/Options.
  Canonical pause icon sizes are initialized from the17 original POD records,
  enabling normal button hit areas. Real tests verify off-center options/OK
  taps,Continue,and Quit to Rec Room from pause and results. Play Again has
  restarted Wanted in a pre-extents build; exact final-build retry check pending.
  Build passes50/50 CTests,6generator tests and3linked-data audits. See
  wanted-pause-flow-checkpoint-2026-10-16.txt for hashes/screenshots/limitations.

- Final Wanted pause/retry milestone VERIFIED: exact EXE
  2FC6827F1C6461B341005ECCC0ABF618C533D9431594A3420E448363FE16205F.
  GPU6300-frame replay confirms Play Again starts a new round, then pause Quit
  returns to Rec Room. Software7300-frame replay confirms off-center options/
  OK/Continue taps and results Quit. Both exit0. All50 CTests,6Python generator
  checks and3linked-data audits pass. Updated build/wanted-callback-preview.
  Physical input/audio effects and other minigames remain unverified. F: install
  unchanged. Recommend Luna Medium for bounded menu polish; pause for switch.

- Wanted PC Settings menu investigation (2026-10-16,Luna Medium): the existing
  settings state accepted Wanted pause input and changed the FPS value, but the
  shared BG UI was black in this minigame's separate screen setup. Presenter and
  register experiments were removed; safe original Options behavior is restored.
  Rebuilt successfully;50/50 CTests and6 generator checks pass. Not implemented.
  Recommend Sol Medium to inspect Wanted BG0/BG1 register, VRAM and palette state
  against the working Stage settings page. Goal paused for model switch. Details:
  wanted-pc-settings-rendering-handoff-2026-10-16.txt.

- Wanted PC Settings VERIFIED (2026-10-16,Sol Medium): original Options opens
  the native PC/Game Options chooser. A separate preserved sub-screen surface
  fixes the blank UI; custom input is handled before retail resume buttons.
  GPU4900-frame and software5000-frame replays pass with visible menu pages,
  saved changes,all7pages,original options,Back/Continue and Quit to Rec Room.
  Fresh3300-frame process loadsFPS120,shows3840x2160 settings and changes to
  2560x1440 live.51/51 CTests,7generator checks,3data audits pass. Preview
  updated;F: unchanged. Full goal incomplete. Luna Medium suitable for bounded
  UI polish;pause for that model switch. See wanted-pc-settings-checkpoint.

- Wanted PC Settings polish (2026-10-16,Luna Medium): replaced a few internal
  abbreviations with clearer wording and changed map size, opacity and position
  to percentages. The 3800-frame software replay exits0; edited pages fit the
  native256x192 surface. Build succeeds and51/51 CTests pass. Preview EXE/MAP
  hashes match the tested build. F: install unchanged; full goal remains active.
  See wanted-pc-settings-polish-2026-10-16.txt.

- F: installed build milestone (2026-10-16): copied current tested EXE/MAP to
  F:\AI Games\Mario 64 DS after preserving the prior executable, map and status
  files in a rollback checkpoint. The exact installed binary launches from F:\
  with the supplied ROM and completes a3600-frame software Wanted-settings run,
  visiting all seven pages. Build passes;51/51 CTests pass. ROM and user saves
  unchanged.4K/controller behavior is not claimed by the installed software
  replay. See f-installed-build-checkpoint-2026-10-16.txt. Overall goal remains
  active and incomplete.

- OpenGL CPU renderer VERIFIED (2026-10-16,Sol Medium): added a saved/live
  D3D11/OpenGL CPU choice to native Graphics page3/7. App-local Mesa26.2.4 is
  forced/verified llvmpipe; absent runtime restores D3D11 with a native notice.
  Dedicated8MiB renderer worker preserves the fixed DS address ranges; float
  targets retain exact DS blend rounding. Build succeeds;52/52 CTests and all3
  linked-data audits pass. Wanted5300-frame switching, Stage5000-frame live4K
  Yoshi/minimap, fresh restart2400 and missing-runtime3100 all exit0. Installed
  exact EXE/MAP/runtime in F: with rollback, then actual F: OpenGL Wanted2400
  replay exits0. Main launcher now opens original menu; development course
  picker retained. ROM/user saves preserved. CPU4K is slow; full game/port and
  other requested features remain unfinished. Stay Sol Medium; no model switch.
  See native-opengl-cpu-checkpoint-2026-10-16.txt.

- Yoshi Adventure attack crash VERIFIED (2026-10-16,Sol Medium): corrected
  the target-query native virtual method and rebased both tongue/retract
  animation handles. Original title -> Adventure -> File A replay completes
  three attacks and one jump,6300frames,exit0 with recovery/landing confirmed
  in stateCSV and screenshots. Build,52/52CTests,14generator checks and4data
  audits pass. Full gameplay/other actors remain unverified. Stay Sol Medium;
  goal active. See yoshi-action-checkpoint-2026-10-16.txt.

- Bob-omb tutorial-message fix and movement VERIFIED (2026-10-16,Sol Medium):
  all3message formatters now use native callbacks and are included in the build.
  Former tutorial page-change crash fixed. Local3600-frame course6 replay
  verifies movement/slope/wall,release,attack,jump/landing; exit0. Build passes,
  52/52CTests,14generator checks and4data audits pass. Installed exact EXE/MAP
  to F: with rollback; actual F:3600-frame replay also exits0 with movement,
  attack and jump/landing verified. ROM/user saves preserved; QA INI absence
  restored. Low-magnitude stick12000(game663) did not move the player; stronger
  stick25000(game2808) works. Low-range response/physical input and full course
  progression remain follow-ups. Goal active; keep Sol Medium. See
  bobomb-tutorial-movement-checkpoint-2026-10-16.txt.

- Low-magnitude native analog movement VERIFIED (2026-10-16,Sol Medium):
  mapped active Xbox travel above the retail touch stick stationary range,
  preserving zero/full travel and direction. Both Standard/Touch modes and
  tiny diagonals covered in production-controller checks.52/52CTests and4data
  audits pass. Final small-cardinal and near-deadzone-diagonal3000-frame game
  replays both exit0,move Yoshi and return to idle on release. Installed F: and
  preview hashes match exact tested final build; rollback saved. Physical
  controller input and complete gameplay still unverified. Keep Sol Medium;
  goal active. See native-analog-range-checkpoint-2026-10-16.txt.

- Next Goomba milestone diagnosed (2026-10-16,Sol Medium): native spawn table
  lacks profiles200/201/202. Recovered daKrb_c implementation uses five DS
  callback records/free-stub unions and mixed-overlay resource initialization;
  these need a deliberate native ABI/data integration. No new code changes
  or Goomba repair attempts yet. Known-good analog build retained. Recommend
  Sol High before this architectural/ABI milestone; pause for requested switch.
  See goomba-native-integration-handoff-2026-10-16.txt.

- Native Goombas (2026-10-16,Sol High): registered three recovered profiles,
  adapted callbacks/resources/material and cap pointers, repaired collision
  receiver forwarding and native Yoshi queries. Course6 has15native Goombas;
  rendering, walking/chasing and contact damage verified. Release build,
  52/52CTests,19generator checks and5data audits pass. Final local2400-frame
  and installed F:1800-frame replays exit0; screenshots retained. Installed
  exact EXE/MAP with rollback and separate Goomba Development Preview launcher.
  Swallowing remains unverified after two focused interaction routes; stomp,
  eggs/rewards,small/large runtime behavior and full progression remain open.
  Recommend Astra Low to trace tongue/contact/mouth records; pause for requested
  model switch. See native-goomba-checkpoint-2026-10-16.txt.

- Yoshi Goomba swallowing VERIFIED (2026-10-17,Astra Low): corrected two native
  bool/int return mismatches (state identity and animation frame events), three
  remaining swallow-path DS virtual calls, and enrolled the original egg actor
  with canonical model pointers/native target queries. Manual swallow and
  installed F: automatic swallow each complete2000frames,exit0: real tongue
  capture, held Goomba, consume/removal, egg creation, jump/landing and idle.
  Final52/52CTests,19generator checks and5data audits pass. Exact EXE/MAP installed
  with rollback; user saves/ROM/settings preserved. Egg throwing/payouts/stomping
  and full gameplay remain unverified. Recommend Sol Medium for those bounded
  checks; pause for requested model switch. See
  yoshi-goomba-swallow-checkpoint-2026-10-17.txt.

- Egg throwing and normal Goomba rewards VERIFIED (2026-10-17,Sol Medium):
  connected the recovered native target-position method and restored the full
  four-entry Goomba cylinder-height table (ambiguous interior labels had
  truncated it). Egg flight/hit/death and coin payout/collection pass locally
  and in the installed F:2400-frame replay,exit0; course coins0->1. Stomp,
  bounce,enemy removal and original coin drop also pass locally and in installed
  F:2200-frame replay,exit0. Swallowing regression passes. Build,52/52CTests,
  19generator checks and5data audits pass. Final EXE/MAP installed with rollback;
  screenshots/logs/source snapshot saved; ROM/saves/settings preserved.
  Special Goomba reward variants and full progression remain unverified.
  Keep Sol Medium for bounded gameplay checks; no new model switch recommended.
  See egg-goomba-rewards-checkpoint-2026-10-17.txt.

- Special Goomba rewards PARTIALLY VERIFIED (2026-10-17,Sol Medium): scanned
  all52 original course object tables and found silver/cap/large variants.
  Native actor registry lacks silver179,marker180 and cap269; integration
  requires callback/resource/layout work. Added observers for Goomba200..202
  and QA course0 selection. Native build,52/52CTests and Goomba data audit pass.
  Course0/course24 scouts exit0. Two large ground-pound routes exit0 but miss
  the top, so blue-coin payout/collection remains unverified; second route
  suffers repeated contact damage and exits the course. No gameplay fix or
  F: executable install this milestone. Screenshots/logs/CSV retained.
  Recommend Sol High for silver-star/cap integration and targeted reward tests;
  pause for requested switch. See special-goomba-rewards-handoff-2026-10-17.txt.

- Silver-star and cap rewards VERIFIED (2026-10-17,Sol High): registered native
  silver179,marker180 and cap269; adapted18cap callbacks,14star callbacks,
  resource pointers and20player no-control initialization descriptors. Fixed
  static-cap modulo-zero and pickup calls to DS addresses. Original src/include
  unchanged. Silver Goomba swallow/egg/release/touch/counter0->1, active Goomba
  Luigi-cap stripping/collection/disguise, and placed Mario-cap collection pass
  locally and in installed F:2800/2850/2400-frame replays,each exit0; control,
  later actions and idle restored. Normal egg/coin regression also passes.
  Build,52/52CTests,24generator checks and6data audits pass. Exact EXE/MAP
  installed with rollback,three watch-test launchers,isolated QA saves and
  screenshots. Redundant BMPs consolidated losslessly; duplicate binaries
  removed while preserving distinct versions/source/saves/logs/rollback.
  Full progression/conditional VS rewards/Power Star178/blue coins remain open.
  Recommend Sol Medium for bounded next reward checks; pause for requested
  model switch. See special-rewards-checkpoint-2026-10-17.txt.

## Large Goomba blue-coin reward verified (2026-10-17)

- Sustained Yoshi jump followed by a ground pound defeats large Goomba202, selects blue reward3 and spawns coin290. Actual pickup at1754 adds exactly five coins (0->5). A later jump succeeds; normal idle/control returns in course24; local and installed F: replays both complete2450frames with exit0. Earlier short jumps never cleared the target; no gameplay code fix was needed.
- Native build is up to date,52/52 CTests pass46.68s, eight new replay-validator tests pass, and Goomba/egg audit now also verifies coin actor IDs and values1/2/5. Existing executable9533D6D00C9D7CB2E5E1663A646635B00B18930AB6FB6844AFCC04757BE3C7A0 remains installed.
- Installed Watch Large Goomba Blue Coin Test.cmd uses separate test saves. Screenshots/logs/CSV evidence and source are archived in native-checkpoint-2026-10-17-blue-coin; details in port/docs/blue-coin-checkpoint-2026-10-17.txt. Full port remains incomplete. Luna Medium is reasonable for the next routine reward regression checks; new callback/actor integration warrants Sol High.

## Conditional Goomba silver-star release and slide-animation crash fixed (2026-10-17)

- Runtime trace confirms the conditional check uses active VS sequence ID0. The original unused test-map Goomba carries ID15 and releases no star after a confirmed ground pound. An explicit isolated fixture with Goomba ID0 releases exactly one silver star and marker with reciprocal links and marks the reward spent; fixture ID1 releases neither. All four final local/installed replays confirm the hit at1722, continue2350frames, accept a later jump, return to same-course idle/control and exit0. Pickup and sequence advancement remain separate checks.
- Removed a legacy native override that replaced original shared animation0x8099 with a Mario head model. Crash snapshots identified Yoshi animation263 reading that model as BCA data. The original constructor handle is now preserved, and native animation audits reject mismatched HAL constructor IDs.
- Native build passes;52/52 CTests pass20.95s, five Goomba generator tests and seven meaningful verifier rejection checks pass, as do Goomba/egg/coin, reward and player-animation data audits. Installed EXE71432172C751CA0E9158662290541A2BF2B5D634BE3E52A441DAF5FF64BC227E and MAP9718B07F6E96D321DDABA81375607B746726B7C0DF4BC5A878405E6FF6BA8D27 match the verified local build.
- Watch Conditional Silver-Star Test.cmd on F: demonstrates the matching fixture with separate QA saves. Verified build/source/evidence and rollback are stored under native-checkpoint-2026-10-17-conditional-reward. Details: port/docs/conditional-reward-checkpoint-2026-10-17.txt. Sol Medium is recommended for the next bounded kind3 pickup/VS-sequence check; new actor/callback integration warrants Sol High. Full port remains incomplete.

## Conditional silver-star pickup and live VS sequence verified (2026-10-17)

- Local and installed F: controller replays each complete2500frames with exit0. Confirmed Goomba hit1722, release1737, actual pickup1778, sequence index0->1 / active ID0->1, credit0->1 at1846, collected star removal, later jump and final normal control. Uses the explicitly labelled isolated matching fixture; full VS multiplayer and subsequent spawning remain unverified. No gameplay code fix needed.
- Build current;52/52 CTests pass20.35s,10 new verifier unit tests and7 negative checks using actual replay traces pass. Added strict pickup verifier and Pickup mode/new watch launcher with separate QA saves. EXE/MAP remain identical to the conditional-reward checkpoint. Details: conditional-pickup-checkpoint-2026-10-17.txt.
- Next bounded progression task is missing native Power Star actor178 and full collection/course completion. Sol High recommended for the new actor callbacks/resource integration. Pause for that model switch; overall port remains incomplete.

## Native Power Star course clear and character eyes verified (2026-10-17)

- Registered Power Star178 with original metadata and the existing native star class. Original Bob-omb param7 initializes normally. Explicit isolated QA placement of param1 is collected1792, advances story-course0 mask0->2 / total6->7, exits to castle1920, accepts touchscreen Save & Continue2500, commits saves, accepts jump2900 and returns to normal idle. Local and installed3300-frame replays exit0. Separate process reload confirms the saved mask2,total7.
- Fixed two exact native-data faults: six star-camera script pointers were raw DS addresses; course-clear OAM::NUMBERS was exported as a pointer variable instead of a20-entry pointer table. Native audits now verify both tables and original glyph bytes.
- Restored GX texture-coordinate generation modes0..3; missing mode1 texture transforms caused the cyclops eyes. Matching Yoshi/Mario screenshots compare old single pupils with corrected pairs; Luigi also verified. All three final installed character replays exit0 with normal idle/control. Original textures unchanged.
- Build passes;52/52 CTests32.42s,5 reward generator tests,9 actual-trace rejection checks and3 native data/resource audits pass. Final installed EXE757F74D22F0F2D2C26E6526197FAC5FA916DDA0A96B5DF236EE6D20736B2CA25; MAPCDD2EAFC29951A255B3E3691FD39BD310A4523DB54A76DBC10B960FD9FEEAFD4. Watch Power Star Course Clear Test.cmd uses separate QA saves. Source/evidence/rollback in native-checkpoint-2026-10-17-power-star-eyes; full details in power-star-eyes-checkpoint-2026-10-17.txt.
- Full port remains incomplete. Luna Medium is appropriate for the next bounded already-collected-star regression using the existing fixture. Pause for that recommended downgrade; new actor or complex progression work merits a stronger model.

## Repeat Power Star collection verified (2026-10-17)

- Corrected the previous duplicate-credit diagnosis: ID1 is mask2, ID7 mask128; the ID7 test collected a different reward. Removed the unnecessary speculative guard and restored the original param1 QA fixture. No gameplay fix or replacement executable is needed.
- Fresh installed-build3300-frame replay confirms real contact1791,pickup1792,course exit1920,Save & Continue2500,committed save,later jump and final normal control. Live and disk totals stay7;course0 mask stays2;all course progress unchanged. Strict verifier plus9 altered-trace rejection checks pass.
- Added Watch Repeat Power Star Test.cmd with isolated resettable QA saves. Exact EXE remains757F74D22F0F2D2C26E6526197FAC5FA916DDA0A96B5DF236EE6D20736B2CA25. Details in power-star-repeat-checkpoint-2026-10-17.txt. Full port unfinished;Luna Medium remains appropriate for bounded regression checks.

## Restored-source build and repeat-star regression verified (2026-10-17)

- Fresh normal build completes after102.2s configure/16.4s generation. Earlier perceived stall not reproduced; no generator workaround needed.52/52 CTests pass78.26s;reward and player-animation audits pass.
- Rebuilt EXE AC713BAB3F364D8C33D135D77ED185987587EBDDB48DE725AC58AC9D3188553D completes3300-frame repeat collection with saved/live7->7,mask2,course clear,save,later jump and normal control. Archived as development build;installed verified EXE unchanged.
- Next bounded regression: Save & Quit course-clear/title return. Luna Medium remains suitable. Full port unfinished. Details: restored-build-checkpoint-2026-10-17.txt.

## Save & Quit title return fixed and verified (2026-10-17)

- Original course-clear Save & Quit destroyed the stage then showed a green screen from course-picker sessions: native DSMT scene1 was registered only for original-menu startup. Register its existing factory for all game sessions;boot destination selection stays independent. Original cartridge sources unchanged.
- Local and installed3800-frame replays: real contact1791,pickup1792,credit6->7,Save & Quit2500,committed checksum-valid save,all other course masks preserved,title primary-save reload and animated original menu visible through final frame.52/52 CTests27.46s;8 verifier rejection checks including original blank-screen evidence pass.
- Installed EXE BFDAAC31DB0915374D229A2D6C810CE2A2A2C8ADC2A010786E7F9C6F52FB7D45;MAP461EEBED34915E66B6CAEA00F3C75C30FDEE4E0BC5B3DFF62F59323DF65605E5. Watch Power Star Save and Quit Test.cmd uses isolated QA saves. Rollback,source and evidence in native-checkpoint-2026-10-17-save-quit. Full port unfinished. Next bounded test: Continue without saving. Luna Medium remains appropriate.

## Continue without saving verified (2026-10-17)

- Added read-only live star-count/course telemetry. Local3300-frame collection/Continue/jump/idle replay keeps live7;four save files stay byte-identical at6. Separate1750-frame process reloads6,mask0,and no fixture. Build,52/52 CTests32.08s,and8 negative verifier checks pass. No gameplay repair needed.
- Diagnostic build/evidence/source archived in native-checkpoint-2026-10-17-continue-nosave;installed game EXE unchanged. Watch Power Star Continue Test.cmd installed (syntax checked;local equivalent route verified).
- Next full-progression task is native King Bob-omb actor189,confirmed in original course6 mission filters1/4 and absent native registry. Sol High recommended for boss state/resource/player-grab integration. Pause for model switch. Full port unfinished. See continue-nosave-checkpoint-2026-10-17.txt.

## Camera control modes requested (2026-10-17)

- Planned native pause setting: Preset Angles uses the original touchscreen camera increments with the right stick; Free Camera allows continuous orbit using the right stick or mouse. Include sensitivity, invert-Y, safe mouse capture, saved preference, and original cutscene/pause behavior. Full requirements are in pc-settings-menu-plan-2026-10-15.txt. No camera implementation claimed.


## Native King Bob-omb Yoshi fight and reward verified (2026-10-17)

- Registered original boss189 and Bob-omb206,18 native state pairs/36 callbacks,13 original resource handles and contiguous carry vectors;held-player receiver corrected. Restored ROM unit scale for billboard bones, fixing the invisible spherical body. Cartridge src/include unchanged.
- Local and installed F:4600-frame replays each exit0: original map boss99,paramff01;three actual caught/spat bombs hit2072/2430/2964,health3->2->1->0;defeat dialogue;original kind4 Star111 spawn3145,pickup3311,course exit3460;Save & Continue4000,checksum-valid disk credit6->7/mask2,later castle jump and normal idle/control. QA travel moves only Yoshi to summit once;no artificial reward or damage. Other character strategies remain unverified.
- Release build,52/52 CTests18.85s,5 boss generator tests,11 altered-evidence checks and3 native data audits pass. Installed EXE45DEEB8449303FD83564E1FB15A4800E8534BA148589FF07C6EE37C04A91C48F;MAP554A3E73A42C06D48E2DF4849A341AA9CB83205B94F6A0F96DDEB0C2955C8E5F. Watch King Bob-omb Test.cmd uses separate QA saves;rollback/source/evidence preserved.
- Camera Preset Angles/Free Camera (right stick/mouse) added to settings plan only. Full port unfinished. Repeat collection of the same King Bob-omb Star was also verified: same installed EXE and matching ROM;real boss fight and reward pickup;live and saved course-0 mask2,total7 before and after;process exit0. No gameplay fix/rebuild needed. See native-checkpoint-2026-10-17-bombking/repeat-test and bombking-checkpoint-2026-10-17.txt. Luna Medium remains suitable for the next bounded task.

## Original mission selection preview (2026-10-17)

VERIFIED for the bounded course-picker -> original mission selection -> Bob-omb Battlefield route. Overall PC port remains incomplete.

See star-selection-checkpoint-2026-10-17.txt. Native scene4, sprite bindings, fade dispatch, conditional message preload, dual-screen presentation and sub-OBJ extended palettes implemented. Build/52 tests/profile audit/415 shared bindings pass. Final touch selection enters mission1/level6, accepts a later jump and ends normal control;isolated save unchanged. Preview separate from installed main build pending original course-entry and QA launcher integration. Keep Astra Low; no switch or pause.


## Original course entry and native painting verified (2026-10-17)

ORIGINAL COURSE ENTRY VERIFIED - 2026-10-17

Original title -> Adventure -> File A -> castle key door -> lobby stairs ->
Bob-omb room door -> native painting -> original mission1 -> Yoshi gameplay.
Final candidate replay from F: passed6000ticks, later jump and idle/control;
seven-star QA save byte-identical. EXE SHA256 3A292142642E68CB2DCE0D8C2274D0A111CD85F42F54944BFB45D4863B98D9A9.
Build passed;52/52 CTests8.63sec;3 painting-transform tests;5 verifier
negative-evidence checks;11 native actor profiles/416 shared bindings pass.
Original painting actor307 and resources/texture cache integrated; original
cartridge src/include unchanged. Original mission selection is promoted.

First installed replay failed at room door. Root cause in input-only replay:
presentation hook could advance waypoint state twice per simulation tick.
Now course/boss replay controls update once per simulation tick. Failed
evidence retained in checkpoint/qa; corrected evidence in qa-tick-gated.
Normal player input is unchanged.

Watch Original Course Entry Test.cmd uses isolated QA saves (seven stars,
Yoshi, castle key already owned). User saves and settings untouched. Prior
reward Watch scripts use their preserved original EXE to retain verified
schedules. Rollback: native-checkpoint-2026-10-17-pre-course-entry.

Remaining: repeated course exit/re-entry, painting texture cleanup/reload,
other courses and fresh-save key acquisition are unverified. Full port is
incomplete. Next bounded regression: leave and re-enter this same course.
Luna Medium recommended for that check; pause for model switch after install.

## 2026-10-17: Course exit and re-entry verified

Completed an installed 10,800-frame replay of the original title, course
entry, pause-menu Exit Course, castle return, and entry into Bob-omb Battlefield
again. The trace confirms the painting texture cache resets after returning
to the castle and all six painting actors reload. The second course entry
reached normal Yoshi gameplay, the QA save stayed unchanged, and the process
exited cleanly. A follow-up capture at frame 7100 visibly shows the original
Bomb-omb painting artwork between that reload and course cleanup (908 rendered
triangles). Screenshots at frames 7100, 9800 and 10400 plus the state trace and
logs are saved in the F: course-entry checkpoint. Build and 52/52 CTests passed;
painting transform tests passed 3/3. Full project remains in development.


## 2026-10-19: Native cannon entry, aiming and firing verified

Registered the original cannon and water-bomb actors. Corrected canonical raw/input-history memory and the PC offline publication step, so the shot follows the selected camera heading. A7500-frame original-menu replay opens the shutter through Buddy, rides the cannon, aims in both axes, launches, lands and returns normal control without changing the isolated disk save. Release,53 CTests,4 cannon and8 Buddy adapter tests and native-data audit pass. Installed F: EXE BFB57A03... with previous1C4BB94C... retained for rollback; original ROM and saves hash-identical. See native-cannon-integration-2026-10-19.txt. Other cannons, repeat use and projectile combat remain unverified; full port and broader roadmap unfinished.

## 2026-10-19: Repeat cannon rides and course reentry verified

An input-only original-menu replay completes two rides in cannon91, exits through the original pause menu, returns through the castle painting and original mission2 selection, then rides recreated cannon256. All three aiming/launch/landing sequences pass; shutter stays open, old actor is removed, painting textures/actors reload and isolated saves stay unchanged.11000 frames exit0; Release build,53 CTests (22.80s), native-data audit and lifecycle checker pass. Five damaged-evidence probes are rejected. Fixed only the replay waypoint that walked into the Buddy; normal gameplay required no repair and the verified F: build remains BFB57A03.... Screenshots shown in chat. See native-cannon-repeat-2026-10-19.txt. Next bounded regression: another existing Battlefield cannon, suitable for Luna Medium; pause for that recommended downgrade. Other cannons/projectile combat and full port remain unfinished.

## 2026-10-19: Second Battlefield cannon and first Mario-cap crash verified

The input-only replay now uses a measured walking route to cannon92 and precise waypoint approaches, completing original FileA/course entry, cap pickup, a second cannon ride and grounded control return. Found and fixed a real cap-intro crash: the PC sound-spawn bridge discarded its actor return, then the original sequence dereferenced an unavailable emitter. The bridge returns Spawn's result and the generated Player code guards the release when no emitter exists; original sources and normal cap/camera behavior are preserved. Native sound actor/audio remain unfinished.9500frames exit0; aim7513,launch7634,idle7793,final idle9500.53/53 CTests26.93s,5 reward adapters, data audit and cap/cannon verifier pass; isolated save bytes unchanged. Screenshots shown, including Yoshi in Mario-cap form. Details native-second-cannon-2026-10-19.txt. Next bounded regression: water-bomb combat from the existing native cannon/projectile actors; Luna Medium is suitable to start, escalate on a new difficult failure. Full port and broader roadmap remain unfinished.

## 2026-10-19: Original water-bomb combat and recovery verified

Original mission1 native cannon launches, repeated drops, gravity, homing bounce, body and fragment damage, five-fragment bursts and safe grounded recovery pass a9500-frame original-menu replay, exit0. No gameplay repair needed; added opt-in input-only QA controls, telemetry, evidence checker and independent ROM comparison of five fragment angles.53/53 CTests27.18s, native-data audit, unchanged isolated saves and seven negative verifier probes pass. Screenshots shown. Corrected handoff: mission1 variant0 fires; mission2 variant1 is static. Normal F: game remains691DD302... because it already contains this gameplay; QA candidate1F64B982... stays local. See native-waterbomb-combat-2026-10-19.txt. Next bounded check is projectile cleanup across original course exit/reentry, suitable for Luna Medium. Full port remains unfinished and goal active.

## 2026-10-19: Water-bomb course lifecycle partially verified

Two diagnostic input-only runs investigated course exit with active projectiles. First paused during Hurt and reached the original full-screen map. Replay now checks Player::CanPause before Start; second run exits through original selection3 with bomb217 active, returns to castle6795 with old enemies absent, reloads paintings and reenters Battlefield7508. However original selection confirms mission2, so the strict mission1 lifecycle checker FAILS.11000frames exit0, final grounded control, isolated save bytes unchanged. Release/53CTest22.84s/native-data audit pass; no gameplay repair or stable promotion. Screenshots shown. Details native-waterbomb-reentry-2026-10-19.txt. Recommend Sol Medium for reliable original scene/input routing before another run; pause for this switch under standing authorization. Prior accepted combat remains valid; full port unfinished.

## 2026-10-19: Mission1 selection and water-bomb lifecycle verified

Fixed QA replay by observing the original STAR_SELECT highlight/navigation/fade state before player/dialogue branches. Normal Left inputs activate navigation and select mission1; A confirms only highlight0/phase2. Original game fields/progress remain authoritative.11000-frame replay exits with falling bomb217 active, cleans old actors in castle6795, confirms mission1 again, recreates droppers374/375 and cannons376/380, fires new projectiles and restores grounded Yoshi control7402. Strict lifecycle checker and eight negative probes pass; saves byte-identical. Release,53CTest22.69s,3 new star-select transform tests and native-data audit pass. Screenshots shown. See native-waterbomb-reentry-verified-2026-10-19.txt. Normal F:691DD302 unchanged; QA candidateAD881D92 remains private. Prior mission2 diagnostics stay rejected. Full port unfinished. Recommend Luna Medium for refreshing the ROM-free kit with established fresh reconstruction/runtime checks; pause for this downgrade.
## 2026-10-21: Castle-grounds cannon grate and 150-star gate verified

- The original ov009 `daObjMc_Metalnet_c` source already implements the retail
  gate: with a normal authored parameter it removes itself at 150 stars; with
  parameter `0xff` it follows save unlock bit `0x80000`. It explicitly remains
  in mode 1. The PC actor table had no profile 339, so this castle-grounds
  grate was unavailable to native scene spawning.
- Registered reconstructed profile 339 and rooted its class factory in the PC
  boot closure. The actor's CLPS block is at ov009 `0x02112bf8`; linked the
  existing generated overlay-9 image and resolve that data through its
  relocation-aware address helper rather than copying raw DS pointers.
- Runtime verification used the local extracted European Rev 0 ROM and two
  isolated folders, each with its own save directory. The zero-star Yoshi replay
  completes 13,000 ticks and exits 0. Actor census at the outside cannon shows
  cannon actor207 and metal net actor339 `param=0000ff01`; Yoshi stays grounded
  at the grate position and never enters `St_Cannon_Main`.
- A separate checksummed QA save has exactly 150 star bits and Yoshi selected.
  The matching 13,000-tick replay exits 0, retains actor207, removes the
  `param=0000ff01` metal net, and reaches `St_Cannon_Main` at the same cannon.
  Other actor339 instances with `param=0000ffff` remain, matching their
  separate save-bit gate. These runs verify the intended zero/150 boundary;
  they do not claim a full save-progress playthrough.
- Captures: `work/native-metalnet-extended-20261021/ground-12000.bmp`,
  `work/native-metalnet-150stars-20261021/ground-11000.bmp`, and
  `work/native-metalnet-150stars-20261021/ground-12000.bmp`. The zero-star
  image shows Yoshi over the grate; the 150-star approach shows the opened
  cannon mouth and its later image is captured in cannon mode.
  User saves were not used or changed. After both checks passed, the tested
  EXE and matching map were installed to `F:\AI Games\Mario 64 DS`; SHA-256
  hashes match the tested outputs (`427C43A6...965CC5` and
  `5C7BF331...9A8E4A39`). The prior EXE/map/status are preserved in
  `F:\AI Games\Mario 64 DS\native-checkpoints\native-checkpoint-2026-10-21-metalnet`.
  The installed INI, ROM, saves, SDL3 dependency, and launchers were preserved.
- Release build succeeded on 2026-10-21 with Visual Studio 18 x86/Ninja:
  `cmake --build build/port --target sm64ds_pc -j 4`. The map includes the
  metalnet factory/profile and CLPS source. This is compile/link evidence only;
  collision, visible grate, cannon blocking, and 150-star disappearance have
  not yet been verified in the running game. Overall port remains unfinished.
- Added `smoke_overlay9`, which checks all 176 self-relocations remain inside
  the host image, validates the overlay address limits, and checks the exact
  CLPS record and model handle addresses used by the grate. Fresh Release game
  and smoke builds succeeded; the smoke executable and CTest check both pass
  (`smoke_overlay9: 176 self-relocations and metal-net CLPS/model bounds pass`).
  This covers data-image integrity, not live collision or star-count behavior.
- `port/tools/check_metalnet_star_gate_replay.py` passes on both isolated runtime
  folders, checking actor census, star counts, grounded-vs-cannon player states,
  screenshots, and the 150-star QA save checksum.

## 2026-10-22: Zero-star cannon entry attempt replay

- Replayed the installed `427C43A6...965CC5` executable from an empty isolated
  save folder through the original game menu and an input-only route to the
  castle-grounds cannon. The runtime trace reports zero collected stars.
- Repeated A presses at the cannon area did not enter `St_Cannon_Main`; Yoshi
  returned to `St_Wait_Main`. Process exit was 0. The 150-star replay above
  still enters the cannon, so the two save thresholds behave differently in
  the tested runtime.
- Captures are in `work/native-metalnet-entry-attempt-4-20261022/` at ticks
  11000, 13000, 14000, 15000, and 16000. The 14000 image shows Yoshi standing
  in the cannon opening area. The visible grate is not clear enough in this
  image to certify its appearance or prove the collision boundary by itself.
- No code or installed files changed during this check; the ROM and user saves
  were untouched. Since the user's star count at the time of entry is unknown,
  their reported cannon ride is not yet reconciled with the zero/150-star test.

## 2026-10-22: Castle water actor integration handoff

- The exterior moat and grounds-cannon basin use recovered profile 338,
  `CASTLE_WATER` (`daObjMcWater_c`, overlay 9). The actor source exists at
  `src/game/actors/d_a_obj_mc_water.cpp`, but the PC actor registry does not
  currently construct it. The basement-water profile is a separate actor and
  does not address the missing exterior water.
- Two constrained build attempts were reverted: registering the profile alone
  left the actor class-init symbols unresolved; adding the actor and overlay
  initializer sources then exposed 50 unresolved overlay-9 data symbols. The
  baseline `sm64ds_pc` target successfully rebuilt after reverting both
  attempts. No incomplete water changes were retained or installed.
- Next investigation should adapt this actor's overlay-9 globals through the
  existing `pc_overlay9_address()` image bridge, including the shared model /
  collision handles and splash-position table initialized by
  `__sinit_ov009_02112524`, then register profile 338. Verify the resulting
  build and live water rendering at both the moat and castle-grounds cannon
  before installing it. Screenshot evidence of the current absence is in
  `work/native-metalnet-entry-attempt-4-20261022/gate-14000.bmp`.
- Follow-up source inspection confirmed the bridge is already linked as
  `ov009_image.cpp`, and the metal-net actor already reads overlay data through
  it. The water TU should resolve handles/data by address rather than declaring
  unresolved `data_ov009_*` globals: model handle `0x02113c68` (file `0x693`),
  collision handle `0x02113c70` (file `0x694`), BTA `0x02112bc4`, and CLPS
  `0x02112c38`. Its splash points are three contiguous seven-`Vector3` tables
  beginning at `0x02113d8c`, `0x02113de0`, and `0x02113e34`; values and
  fixed-point units are specified in `src/unnamed/ov009/__sinit_ov009_02112524.c`.
  A once-only PC initializer can construct the two shared handles and populate
  those BSS tables before the water actor first loads. Profile 338's recovered
  descriptor is at `0x021139f4`, with factory `0x02111d8c`, priorities 338,
  flags 0, clip offset 0, radius `0x00320000`, distance `0x01f40000`, and far
  distance `0x00050000` (`symbols/profile_reconstruction_registry.json`).
- Implemented the bridge-backed Castle Water actor and registered profile 338
  in the native actor table. The actor now initializes model/collision handles
  and the recovered splash-point BSS tables using `pc_overlay9_address()`;
  optional `SM64DS_PC_WATER_TRACE=1` logs the actor's initialization position
  for diagnostic runs. The baseline production link succeeds, and focused
  `smoke_overlay9` passes (1/1).
- An isolated 16,000-frame run completed with exit code 0 and used a fresh QA
  save folder. A follow-up 6,500-frame traced run also exited 0 and confirmed
  `InitResources` ran, reporting position `(0,0,0)`, area 0. Screenshots and
  logs are in `work/native-castle-water-20261022/` and
  `work/native-castle-water-trace-20261022/`. The captures do not clearly show
  the moat or cannon basin, so visible water rendering is not yet verified and
  this diagnostic build has not been installed on F:.

- Follow-up replay navigated through the castle door to the moat cannon basin
  and captured frames at ticks 12,000, 14,000, 16,000, and 18,000. Water is
  visible around the basin and land edges in all four captures; the 16,000 and
  18,000 views retain the same framing while Yoshi is stationary. Evidence is
  in `work/native-castle-water-basin-20261022/`. This confirms water rendering
  at this basin, but does not reproduce the user's reported small idle zoom.
- Added a saved Gameplay setting `CAMERA PRESET/FREE`. Free mode uses continuous
  right-stick yaw/pitch orbit with the selected distance-zoom scale and the
  existing camera-solid ray clip. The right-stick click remains the zoom-cycle
  binding. The Windows production build and six focused CTest cases pass, and
  `test_camera_host.py` passes 2/2. A live right-stick camera replay has not
  yet been completed, so the free-camera behavior remains diagnostic and is
  not installed on F:.


## 2026-10-22: In-game free-camera toggle

- Added Xbox-style left-stick click (L3) as a press-edge toggle between the saved
  Free Camera and Preset/original camera behavior. The click is ignored during
  menu navigation, plays the existing camera-mode cue, and saves the new mode.
  Right-stick movement affects the view only while Free Camera is active.
- Refreshed CMake generation so the produced camera adapter includes this new
  handler, then rebuilt `sm64ds_pc.exe` successfully. Focused CTest results:
  `smoke_input`, `smoke_pause_settings`, `smoke_input_state`,
  `smoke_input_history`, and `smoke_video` all pass (5/5).
- A fresh-save 14,200-frame runtime replay exited 0. The trace records Free at
  tick 10,000, right-stick orbit movement, then Preset at tick 12,000; no orbit
  updates follow the return to Preset while right-stick input continues. Both
  mode changes saved successfully. Evidence: `work/native-free-camera-mode-toggle-verified-qa-20261022/`.
- This validates the toggle and free-camera input path. The separate small zoom
  reported when Yoshi stops moving is still not reproduced or fixed; it remains
  open for the next camera investigation. Nothing was installed on F:.

## 2026-10-22: Second Lakitu talk and idle camera trace

- Replayed the user's wake-up, locked-door approach, second Lakitu talk, and
  walk-away sequence using the refreshed production executable, 12,500 frames,
  and isolated fresh saves. Runtime exited 0. Camera trace, state capture, and
  screenshots are in `work/native-fresh-door-idle-zoom-20261022/`.
- The view intentionally moves closer while Yoshi is in `St_Talk_Main` (about
  865 to 377 world units in this replay). After the talk, it returns to normal
  tracking as Yoshi walks away (about 857 units at tick 11,000). Once Yoshi
  stops at the final route point, distance grows from about 785 to 810 units
  across the initial settling frames, then remains far framed. This does not
  reproduce the reported zoom-in on an ordinary stop; it confirms directed
  dialogue framing followed by an outward camera settle on this route.
- No camera code or user installation changed in this check. Keep the reported
  general idle zoom as an open issue until a runtime case reproduces the inward
  movement. Do not remove the dialogue framing based on this trace.

## 2026-10-22: F: camera test copy

- Created `F:\AI Games\Mario 64 DS\camera-test-current-20261022` as a separate diagnostic folder; the installed game, root ROM, main settings, and personal saves were not changed. Its `sm64ds_pc.exe` SHA256 is `A1DF0142A7C8838C413B3C76AA1860D3D471F3C503364A1CBF99ED401AA5E944`, identical to the C: executable that passed the 14,200-frame Free/Preset runtime replay.
- The folder launcher uses the user ROM on F:, the current source/assets root on C:, and creates saves only under `play-saves`. F: ROM hash matches the verified local ROM copy.
- F: runtime startup smoke passed 900 frames (exit 0), mounted the supplied ROM, and captured 1280x720 frames at ticks 300/600/900. Screenshot: `f-drive-smoke-900.bmp`.
- An additional F: long camera replay was stopped at tick 8,760 because verbose trace output grew beyond 82 MB on the slow volume before reaching the scheduled mode inputs. It is not counted as a Free Camera pass. The equivalent exact executable hash was previously verified on C: for Free toggle/orbit/Preset return.
- The slight idle zoom remains unresolved; the exact second-Lakitu route still settles outward, so no camera behavior was changed.

## 2026-10-23: Free-camera movement, speed, and idle-distance repair

- Free Camera now rotates at 0.052 radians per full right-stick unit per render
  update horizontally and 0.035 vertically, up from 0.012/0.008. A scripted
  20,000-unit right-stick turn changed the camera angle by about 8,276 angle
  units (45 degrees) during the test interval.
- Player movement now receives the angle difference between the free camera's
  world-facing angle and the native camera angle through `GetAngleToCamera`.
  Runtime state captured Yoshi's forward heading at 24,492 while the free
  camera angle was -8,276, the expected 180-degree actor-forward convention.
  Yoshi did move under the rotated view; the scripted route led into a steep
  hill and entered the existing stomach-slide state, so this verifies angle
  alignment but is not a clean open-ground traversal acceptance test.
- The render hook now remembers the largest native gameplay camera distance
  up to 900 world units per camera and collision-checks outward placement. This
  filters large transition spikes. In the Free replay, the rendered distance
  remained at 900 while Yoshi stood still instead of expanding to 4,115 units.
  A separate Preset replay showed no pull-in after stopping; sampled native
  camera distance settled outward from roughly 635 to 874 units over the
  observation window.
- Build: `sm64ds_pc` and `smoke_input_history` built successfully with the
  Visual Studio 2026 x86 toolchain. Checks: `port.tests.test_camera_host` passed
  2/2 and CTest `smoke_input_history` passed 1/1. Both isolated runtime replays
  exited 0. Free-camera screenshots and traces are in
  `work/native-camera-free-relative-final-20261023/`; the preset idle replay is
  in `work/native-camera-preset-idle-final-20261023/`.
- The rebuilt executable remains a local diagnostic build; no files on F: were
  replaced. The angle mapping is runtime-verified, but a clear, unobstructed
  manual-style movement pass and Xbox hardware feel still need confirmation.

Camera follow-up: open-ground movement and installation
- Two 4000-tick Battlefield replays exited successfully, covering right-stick
  orbit followed by forward/backward movement and idle. Ground height remained
  unchanged and Yoshi returned to the idle state. Forward travel agreed with
  camera-relative heading within 0.053 degrees of fixed-point position error.
- Input/settings/video CTest subset passed 5/5. No further camera code changed.
- Installed executable D90129B8B82F4AB54B2CE0076A598A52A26F1C38A9CD7243B5F19C1BE52D37C0
  on F:, with rollback under native-checkpoints/camera-relative-open-ground-verified.
  Installation preserved ROM, settings and save hashes. Physical controller
  sensitivity remains subject to user feedback.
- Installed F: executable/ROM/assets check exited0 at2200 ticks and reached
  Battlefield with122 actors. Screenshot movement-2100.bmp inspected; saves
  and settings for this test were isolated on C:.

CASTLE MOAT WATER REPLAY - 2026-10-21
Verified the exact installed F: executable (SHA256 D90129B8B82F4AB54B2CE0076A598A52A26F1C38A9CD7243B5F19C1BE52D37C0) with the F: ROM/assets, using isolated C: saves. The replay reached castle-grounds route waypoint4 at tick11431, initialized Castle Water, and exited0 after18000 ticks. Frames at12000,14000,16000,18000 show the water surface continuously spanning the moat/grounds-cannon basin; stationary Yoshi and matching framing persist from12000 through18000. No water rendering failure reproduced, so no water fix or install was needed. Evidence: work/native-castle-moat-followup-20261021/, acceptance.json. Original ROM, user saves, and main settings were untouched.

NEXT USER-REPORTED RENDER ISSUE - PEACH INTRO
User screenshot codex-clipboard-5036ed7b-23bb-4b78-b359-22c98322c484.png shows the intro letter's text beginning left of its translucent panel and spilling across the panel border; Peach's crown/forehead geometry overlaps her face. Reproduce the Peach intro on the installed build, then trace DS 2D screen placement/text coordinates separately from Peach model animation/camera or material transforms. The issue is not yet fixed or accepted; current F: build is unchanged.


MOAT WATER VERIFIED; PEACH INTRO ISSUE REPRODUCED, NEEDS SOL HIGH

Moat replay: installed F: executable D90129B8B82F4AB54B2CE0076A598A52A26F1C38A9CD7243B5F19C1BE52D37C0, F: ROM/assets, private C: saves, 18000 frames, exit0. Castle Water actor initialized and route waypoint4 reached at tick11431. Screens at12000-18000 show continuous water across moat/cannon basin. No water code change or install required. acceptance.json is in work/native-castle-moat-followup-20261021/.

Peach intro: current base build reproduces misplaced letter text (starts left of translucent letter panel) and oversized yellow crown geometry crossing Peach's forehead. Evidence: work/native-castle-moat-followup-20261021/intro-2000.bmp (prior installed build), peach-offset3-2000.bmp and peach-offset3-2500.bmp (rebuilt source replay), all at 1280x720; private test saves only. A targeted +11 DS pixel patch in Message::DisplayText did not move the text; source was reverted and native build restored/relinked. Runtime probe in that function emitted no call trace at the offending intro frame, so this intro uses another display path. No lasting code change for Peach. Crown appears to be a separate model/animation/render transform issue and is still unresolved.

MODEL UPGRADE RECOMMENDED: current Luna Medium; use Sol High to trace the actual intro BG/text composition and Peach mesh/animation transforms, then make a narrowly scoped fix and replay the opening. Two focused text-path attempts failed to reach the intro renderer; stop speculative edits. Current source tree has been restored. `cmake --build build/port --target sm64ds_pc --parallel 4` succeeded with the Visual Studio 2026 x86 environment after restoration. No CTest run because no functional change remains. F: install, ROM, saves, and main settings were not modified. Current user goal remains incomplete.


PEACH INTRO FIX - 2026-10-21 - VERIFIED
Letter BG3 follows its perspective panel in widescreen; intro cloud minimap hidden. Added translucent same-ID rejection across D3D11/OpenGL/software, fixing Peach crown/forehead overlaps during alpha28/id22 fade. All targets plus game build;58/58CTest pass.4K5200-tick and software2520-tick isolated replays exit0; screenshots inspected. Source assets and matched src/include unchanged. Details: port/docs/native-peach-intro-2026-10-21.txt.


Installed F: verification: B8C990983D618934AC02DDD7624121AA3E69D54ED2E12493D9C36FAC3D7B629B completed4000 ticks at3840x2160, exit0. Private QA preferences/saves remained on C:. Letter lifecycle trace enables at1953 and disables at2675; frame3200 confirms the following Lakitu shot. Installed frame2500 has letter ink bounds761,810..1807,1833 inside the panel; prior gold artifact sample254,171,37 becomes correct skin231,205,191. Screenshots were inspected and shown in chat. Rollback: F:\AI Games\Mario 64 DS\native-checkpoints\native-checkpoint-2026-10-21-peach-intro. Source mirror includes only the reviewed port changes. ROM/settings/save preservation is recorded in installation.json; no stable full-game completion claim.


## Native trees integrated and runtime checked (2026-10-21)

TREE286 now uses native shared resources, billboard models, collision and course
cleanup. Fixed DS-only position-slot dispatch in tree grab/climb/handstand and
routed the trunk-jump callback through the existing native character resolver.
All default/game targets build;58/58 tests pass. The final24A3D2F3...645EC31
build passes castle climb/headstand/jump and separate trunk jumps (5700 ticks
each), plus11000-tick original course exit/re-entry. Castle21 nodes cleaned;
Battlefield17 nodes cleaned and17 freshly loaded. Isolated saves only.
See native-trees-2026-10-21.txt and private acceptance/installation records.
Luna Medium can handle bounded tree renderer and Battlefield regressions next.
Original/Brain Rot dialogue toggle replacing title Options Backlight, and
original-menu font/border styling for custom PC settings, are queued in
pc-settings-menu-plan-2026-10-15.txt. Replacements will be written by the user,
one exact original text box at a time; none were authored or installed now.

Installed tree build24A3D2F3...645EC31 also passes a4000-tick native4K intro
from F: with isolated saves/preferences. Screens2500/3500/3900 reviewed;
Peach letter/crown, new trees and moat water render. Main ROM/settings/saves
are verified unchanged; rollback is native-checkpoint-2026-10-21-trees.
