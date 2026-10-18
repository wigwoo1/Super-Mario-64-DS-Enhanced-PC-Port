# Collision and cleanup checkpoint — 2026-10-14

Status: PARTIALLY VERIFIED. The latest native level test reaches all 1,200
simulation frames and exits with code 0. The scene is visibly incorrect and
slow; playable gameplay and the requested display/controller features remain
unverified. This checkpoint supersedes the previous collision-crash status.

## What changed

- The PC `func_020383e4` wrapper now forwards its collision-object pointer to
  `dBgCh_Actr::UpdateExtraContinous`. Its old zero-argument declaration dropped
  the pointer that Player passed, causing the worker to read an unrelated stack
  object and interpret 0x1000 as an actor address.
- The game target now links the recovered `ModelBase` destructor, actor
  destructor, and actor-list node destructor. The ModelBase destructor releases
  its owned file; the actor destructor unlinks its registry node. The previous
  smoke-only placeholder destructors are excluded from the production game.
- Static colliders retain the compiler's native C++ vtable. The ground and line
  registry scans use typed native virtual calls on PC, while retaining the DS
  slot-number calls in the non-PC branch. This removes the mixed DS/MSVC slot
  ordering that caused the later sphere query to dispatch a trap at slot 5.
- Removed a diagnostic's hard-coded PE import-table address. Matrix tracing
  now reports the destination without reading an address that changes as the
  executable layout changes.

## Verification

- The native x86 Release game build succeeds.
- `smoke_collision_veneer` passes: the actual compiled wrapper passes the same
  object pointer to its collision worker.
- `smoke_model_cleanup` passes: the recovered native destructor releases one
  owned file exactly once and safely destroys an empty model.
- `smoke_clsn` passes: real castle-ground KCL ray probes agree between native
  virtual dispatch and qualified worker calls, shorten their rays, and report
  upward-facing floor normals. Faults or dispatch disagreement fail the test.
- Production shared-data audit: 186 C/C++ bindings, zero divergent addresses.
- Runtime `resume79`: Down on polls 300–304, confirm on polls 450–454; the
  trace reaches simulation frame 1200, returns from the frame runtime, closes
  the game window, and exits normally (code 0). Earlier traces `resume76`–
  `resume78` document the successive cleanup and vtable failures.
- Latest captured frame: `build/game-frame-resume79.bmp`, displayed to the user.
  It remains blue textured geometry with a small overlay and is not proof of
  correct level/camera rendering or a working minimap.

## Next work and remaining scope

Investigate camera/model rendering and the slow runtime. Recover native actor
profile coverage; profile 352 remains unavailable. Audit the earlier PC-only
inactive-player initialization guard now that spawn parameters are forwarded
correctly, rather than assuming that workaround is necessary for final play.
Preserve the crash fixes and their regression checks during that work.

The full goal still requires playable game flow, correct visuals and scene
transitions, physical Xbox analog movement, resolution settings through 4K,
the bottom-screen map as a translucent minimap, a usable upscaled texture pack
and toggle, 60/120/240 FPS with correct game speed, and complete F-drive packaging.

Recommended next model: Sol High for rendering/camera and actor ABI debugging.
No switch back to Luna is recommended at this checkpoint. Keep the goal active.
