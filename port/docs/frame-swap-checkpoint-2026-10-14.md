# Frame swap and Player bridge checkpoint — 2026-10-14

Status: PARTIALLY VERIFIED. The native game builds and focused tests pass.
The latest scene-transition run crashes and does not reach its 1,200-frame
target. This supersedes the runtime status of the collision/cleanup checkpoint.

## Changes

- GX SWAP_BUFFERS publishes the completed geometry and clears the next build
  buffer. Repeated frames no longer append indefinitely to the same polygon
  list. A completed frame remains visible until the next swap.
- The source generator no longer aliases qualified Player member methods to
  C calling-convention entry points. Typed forwarding bridges preserve their
  object pointer and arguments across the native ABI boundary.
- Removed the earlier inactive-player resource-loading workaround. Spawn
  arguments are now valid; skipping loading left an extra player's models null.
- Crash reporting safely reads instruction bytes through ReadProcessMemory,
  so a null function call still records its caller and stack.

## Verification

- Native x86 Release build succeeds after all changes.
- Two Python generator regressions pass.
- Five focused CTest checks pass: smoke_gx, smoke_clsn, smoke_stage_layout,
  smoke_model_cleanup, smoke_collision_veneer.
- The GX check verifies repeated 492-polygon model submissions do not accumulate,
  completed images persist across swaps, and an empty frame removes old geometry.
- Runtime resume81 exposed skipped model initialization. Runtime resume82
  confirms the extra player loads its body models and animation, then encounters
  a null member callback. Runtime resume83 reproduces that failure with a usable
  stack: St_LevelEnter_Main -> data_ov002_0211075c[mStateWork].
- resume83 exits with access-violation code -1073741819. Its captured image
  remains the debug course selector. Playable gameplay is not verified.

## Next milestone

Recover and validate the native Player level-entry callback table
data_ov002_0211075c. Do not hide the missing callback by skipping the player or
silently ignoring the call. Then rerun the scene transition and investigate
camera/rendering and native actor coverage.

Recommended next model: Sol High. The remaining callback-table and rendering
work needs reverse engineering across DS and native calling conventions.
No Luna handoff is recommended yet. Keep the goal active. When a completed
milestone makes switching back to Luna appropriate, pause the goal so the user
can change models before work continues.

All requested PC features and complete F-drive packaging remain pending final
verification. The F-drive collision/cleanup checkpoint preserves the earlier
bounded successful run; this newer source and test evidence are local to C:.
