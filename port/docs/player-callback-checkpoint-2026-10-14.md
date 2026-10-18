# Native Player callback checkpoint — 2026-10-14

Status: PARTIALLY VERIFIED. The current native build completes the automated
level-entry path and 1,200 simulation frames, then exits normally. The visible
scene remains incorrect. This supersedes the null-callback runtime status in
player-callback-investigation-2026-10-14.md.

## Accepted changes

- Added PC-only PcMemberCallback records with an explicit eight-byte layout.
  Native object calls are separate from record storage; no compiler-wide
  member-pointer setting or class inheritance change is required.
- Player::State now retains the recovered 24-byte layout on PC, with main at
  +8 and cleanup at +16. The script-command table also uses eight-byte records.
- Converted all seven level-entry records to linked native methods in their
  recovered order. The one C-style worker uses an object-pointer adapter.
- Filled the synthetic Model and ModelAnim destructor slots with qualified
  calls to the recovered native destructors and the correct game deallocator.
  Empty slot 1 previously caused the extra player's cleanup to call null.

The new records contain native addresses and native object adjustments. This
does not automatically relocate every other DS callback table. Additional
state/actor coverage and any virtual/adjusted DS callbacks require an audit
before being treated as supported.

## Verification

- Native x86 Release game build succeeds.
- Five focused CTest checks pass: smoke_stage_layout, smoke_gx, smoke_clsn,
  smoke_collision_veneer, smoke_model_cleanup.
- The expanded layout check verifies Player allocation 0x768, state size 24,
  main +8, cleanup +16, callback table indexing, native object-pointer delivery,
  cleanup veto return, a void callback, and all three script arguments.
- Two generator regression tests pass.
- Production shared-data audit: 186 bindings, zero divergent addresses.
- Runtime resume85 passes the old level-entry failure, initializes all four
  players, and reaches frame 840 before revealing the empty destructor slot.
- Runtime resume86 reaches frame 1200 and exits 0 in 36.17 seconds.
- Final runtime resume87, with qualified destructor calls, reaches frame 1200
  and exits 0 in 36.66 seconds. Its latest image was displayed to the user.

Runtime duration includes startup, tracing, frame capture and teardown. It is
not a measured gameplay FPS result and does not verify 60/120/240 FPS targets.
The image remains blue textured geometry. Camera/rendering correctness,
playable gameplay, physical Xbox movement, and the intended minimap are not
proven. Actor profile 352 remains unavailable in the trace.

## Model handoff

User confirmed Sol High before the accepted explicit-record implementation.
This completed callback/cleanup milestone is suitable for a switch to Luna
Medium for the next bounded task: verify the existing PC settings and input
plumbing, including resolution presets through 4K, opacity/toggles, settings
persistence, Xbox analog mapping, and frame-pacing selection regressions.
Ask the user to physically check the controller where automatic checks cannot
prove device behavior. Keep each claim limited to what the check proves.

Pause the goal now as explicitly requested so the user can switch models. A
later camera/rendering or actor-recovery milestone may need Sol High again.
The overall goal remains the complete playable native PC port with every
requested feature; this checkpoint does not reduce that scope or complete it.

## Packaging

Saved development checkpoint: F:\AI Games\Mario 64 DS\native-checkpoints\
native-checkpoint-2026-10-14-player-callback. Its executable, latest bitmap and
README copies are checked against the local SHA256 hashes. Changed-source is
a partial snapshot; the complete authoritative source tree remains on C:.
