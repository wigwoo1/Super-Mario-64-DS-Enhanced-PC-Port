# EXIT/DOOR integration diagnostic checkpoint — 2026-10-14

Status: PARTIALLY VERIFIED. The native build and nine focused checks pass.
Three DOOR actors and four EXIT triggers initialize and receive behavior
updates in the actual scene. Player rendering advances beyond the old null
animation fault. The final run still crashes during scene rendering; the
overall port and this runtime integration are not complete.

## Implemented and checked

- Enrolled the recovered `daChScene_c_classInit` and `daDoor_c_classInit` as
  indirect build roots. Native profile entries 349 and 353 now contain their
  factories, priorities, flags and visibility ranges. Their unavailable-profile
  messages are absent from the final trace.
- Player's native descriptor now has all 28 original bytes of metadata. The
  previous eight-byte descriptor was over-read by the actor constructor.
- Rebased all 19 DOOR variant entries to native resource handles, and the
  shared key-model/decoration tables, including overlay 85's Mip handle.
- DOOR state records preserve the two-word callback layout and use native
  adapters for the recovered methods' different argument signatures. Nine
  compiled state records are checked against original method descriptors.
- Fixed the shared Matrix4x3 scratch allocation from 16 to 48 bytes. The old
  allocation overwrote adjacent spawn pointers and the actor list.
- Mapped all 175 named Player state callbacks to their recovered native
  methods (one C worker uses a call adapter). Conversion happens when a state
  is entered and before its main callback runs. Unknown DS callbacks remain
  explicit failures. This is native linkage coverage, not functional testing
  of every state.
- Bound newly used camera/state C++ names to their existing initialized
  storage. The shared-data audit now covers 217 bindings with no divergent
  addresses.
- The recovered greater-than float-bit comparison follows the local ITCM
  instructions at 01ff99a4. Tests include signed zero, infinities and NaN.
- Restored the omitted stomach-slide helper with the actual Player type for
  PC builds and declaration placement accepted by the host compiler. Removed
  four duplicate helper compilations now included by the expanded source
  closure. Added the missing native punch-init and landing-dust entry bridges.
- Sound playback now honors the existing disabled-engine flag before bank
  lookup. The PC build remains silent; native audio is unfinished.
- Player initialization now seeds its existing render snapshot. The final
  trace shows cached model indices 0/1/2 selecting non-null animations for
  Mario/Luigi/Wario instead of rendering an uninitialized slot on the first
  frame. The captured character appears as a dark silhouette; appearance is
  not fully correct.

## Actual verification

- Final native x86 Release build succeeds (`actor-integration-build13.log`).
- Nine checks passed after the core integration: actor lifecycle, stage
  layout, model animation, camera matrices, GX visibility, input math, video
  settings, frame pacing, and float comparisons. `actor-integration-ctest.log`
  and `LastTest.log` record the run. The last snapshot change applies only to
  the game target; its bounded game run is recorded separately.
- Final executable audits pass: three native profile records match the local
  ROM metadata, 19 DOOR variants and nine callback records match, shared key
  resources resolve, Matrix4x3 scratch has 48 bytes, all 175 state callbacks
  link, and all 736 animation-table entries/383 handle symbols still resolve.
- `resume102` initializes doors with variants 16, 12 and 12, and four EXIT
  triggers. Logs show behavior updates for all seven. The requested test limit
  is 1,200 simulation frames; the process crashes before normal completion,
  with exit -1073741819 (Windows access violation). Elapsed time is 30.0246
  seconds including startup, tracing and capture; it is not a FPS measurement.
- Every game run in this investigation produced a captured bitmap. The final
  one is `build/game-frame-resume102.bmp` and is shown to the user.

## Next exact investigation

The final fault jumps to address zero during Stage rendering. Its saved EBP is
00f23016, inside `dExtShadowModel_c::RenderAll` at preferred image address
00453016 (function start 00452ff0, runtime module base 00ed0000). Saved stack
state is not reliable enough for a normal unwind. The preceding traced actor
is Stage, its Render action is 00f19ca0, and the native DOOR/EXIT updates have
completed. A calling-convention mismatch or invalid lifetime is a hypothesis,
not a proven cause. Inspect the original fault log and current map together.

Recommended next model: **Astra Medium** for the shadow/rendering call-state
investigation. Add bounded traces around the shadow render steps or capture
register/stack state at their call boundaries before changing behavior.
Preserve the new actors and original metadata; dropping actors or suppressing
animation/rendering calls would conceal the remaining fault.

Other native profiles remain unavailable, including DEMO (352), TREE (286),
MIP (187), and MAP (335). Full door/exit transitions, playable game flow,
correct character/HUD/map appearance, audio, physical Xbox play, internal 4K
rendering, a complete replacement texture pack and delivered FPS remain open.

The diagnostic package is stored in `F:\AI Games\Mario 64 DS\native-checkpoints\
diagnostic-checkpoint-2026-10-14-exit-door`. It preserves the crashing executable,
map, latest screenshot, runtime/build/test evidence, and source changes. The
earlier camera/rendering checkpoint remains available. The full authoritative
source is the C: workspace's `work/source-extracted/sm64ds-decomp-main` tree;
`changed-source` is a partial snapshot. The learned link-history input is also
saved because the current generator uses it for source/data enrollment.
