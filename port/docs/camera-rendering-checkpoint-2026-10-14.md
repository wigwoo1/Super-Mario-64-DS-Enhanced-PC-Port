# Camera and rendering checkpoint — 2026-10-14

Status: PARTIALLY VERIFIED for the complete PC port. The repaired camera and
model geometry path now displays the castle and surrounding ground. The final
automated run completes 1,200 simulation frames and exits normally. Original
gameplay, complete actor coverage, HUD/map correctness, physical controller use,
native 4K rendering, upscaled texture assets and measured FPS remain unverified.

## Accepted repairs

- Recovered G3i projection/view routines streamed matrices through raw pointers
  into mapped I/O. Those stores changed memory without executing the PC command
  engine. PC writes now use io_write; the DS branch retains volatile stores.
- The old host fixed-point divide returned a number without updating DIVCNT or
  its result registers. Camera projection reuses that state, so the depth matrix
  was wrong after vector normalization. The helper now preserves the recovered
  divider operation and rounding, including its register side effects.
- Added homogeneous view clipping, latched polygon front/back flags, far-plane
  intersection handling, and perspective-correct texture coordinates. These
  rules follow [GBATEK geometry documentation](https://mgba-emu.github.io/gbatek/).
  This does not implement every DS polygon effect, such as stencil shadows.
- The castle model's repeated matrix-scale writes had the same raw-pointer
  problem. Routing them through the geometry API restores its intended size.
- The newly exercised animation path exposed two existing resource defects:
  a 456-entry allocation for a 736-entry animation table, and address rebasing
  that assumed separately generated host globals were contiguous. The full
  original table is emitted, and exact native symbol mappings replace the
  pointer arithmetic.

## Evidence

- Native x86 Release game build succeeds.
- Six focused CTest checks pass: smoke_camera_matrix, smoke_gx_visibility,
  smoke_gx, smoke_model, smoke_frames, smoke_modelanim.
- The camera test compiles recovered camera/divider/vector routines and uses
  the production host divide helper. It verifies projected coordinates, clip W,
  depth and mirrored view/projection data after a prior divider mode change.
- Visibility tests verify near/far/behind-camera rejection, camera-plane
  intersection clipping, per-face visibility, BEGIN attribute latching and
  perspective texture sampling at a known pixel.
- Animation data audit passes: all 736 compiled table entries match the user's
  local ROM image, and 383 exact native handle symbols cover every entry. The
  one unused/unconstructed record is omitted. Run:

      python port/tools/check_player_animation_data.py

  Its --write-include option regenerates the native include from public symbols.
- Shared-data audit: 186 C/C++ bindings, zero divergent addresses.
- Before repair, resume89 emitted 1,479 triangles with an identity projection.
  resume91 has the proper projection but the castle remains too small.
- Triangle counts confirm 109 skybox triangles, 1,315 opaque castle triangles,
  and 55 transparent triangles. The expected model is handle 0x0797,
  data/stage/main_castle/main_castle_all.bmd.
- resume94/95 first display the castle, then expose the animation lookup fault.
- Final resume96 reaches simulation frame 1200, exits 0, and takes 31.21 seconds
  including startup, tracing, capture and shutdown. This is not measured FPS.
  The latest bitmap was displayed to the user. The castle is visible; this is
  evidence of corrected rendering, not proof of full game flow.

## Handoff

Pause the goal at this completed camera/rendering milestone as the user
requested. Luna Medium is appropriate next for saved PC settings and controller
checks using the existing APIs. Native actor recovery and additional game-flow
reverse engineering may need Sol High again. Runtime traces still report missing
profiles including 349 and 353, and the complete project objective is unfinished.

The verified development checkpoint is saved in F:\AI Games\Mario 64 DS\
native-checkpoints\native-checkpoint-2026-10-14-camera-rendering. The executable,
latest frame and README are checked against the local SHA256 hashes.
changed-source is a partial snapshot; the complete authoritative checkout is
still the source-extracted tree in this chat's C: workspace.
