# Engine checkpoint — 2026-10-14

Status: PARTIALLY VERIFIED. The game now presents a native 3D Stage frame. It
still crashes on the following player collision update and is not playable.

## Verified changes

- Moved the PC collision vtable declaration to global scope so MSVC accepts its C linkage.
- Unified generated data references with their native owners. The production map now passes 186 C/C++ global-address comparisons, including live player pointers, overlay states, and resource handles.
- Corrected the PC scale-particle callback layout. MSVC does not reuse the base class's tail padding; the PC representation now preserves the cartridge field offsets. Tracker size is 0x81c; Stage model, texture slots, and collider begin at 0x86c, 0x8bc, and 0x91c; Stage size is 0x9c8.
- Corrected GXSTAT control writes and synchronous FIFO status. Game startup's IRQ-mode/error-reset write previously erased FIFO-ready flags and caused the skybox draw to spin. The register semantics follow [GBATEK's GXSTAT description](https://mgba-emu.github.io/gbatek/#4000600h---gxstat---geometry-engine-status-register-r-and-rw).
- Separated global deletion by allocation ownership. Blocks inside the original game arena use the game deallocator; native C++ allocations use the CRT. This fixes the skybox renderer attempting to free a native vector through a game heap.
- Added bounded Stage render tracing and an optional continuously updated screenshot (`SM64DS_PC_FRAME_CAPTURE_LATEST=1`).

## Build and verification

- Visual Studio x86 Release build: `sm64ds_pc`, `smoke_roots`, `smoke_gx`, and `smoke_stage_layout` succeeded.
- CTest: all 3 selected tests passed. The roots test covers native container allocation before game startup, reallocation/free afterward, game allocation/free through the same global delete, and the existing heap stress checks. The geometry test exercises the retail GXSTAT reset/control write and the game's display-list pump.
- `port/tools/check_shared_data.py build/port/sm64ds_pc.map`: 186 bindings checked, zero divergent addresses.
- Runtime `resume75`: automated Down on input polls 300–304 and A on polls 450–454, with a requested limit of 1,200 simulation frames. The process crashes before completing that limit (exit -1073741819 / access violation).
- `build/resume75.stderr.log` proves the complete Stage render sequence finishes: particles, collision, fog, skybox, model, shadows, transparent model, actor collision, and overlay rendering.
- `build/game-frame-resume75.bmp` was captured and displayed. It shows blue textured 3D geometry and a translucent overlay. It does not prove correct camera placement, level visuals, a functional minimap, or gameplay.

## Next exact failure

The next player behavior update faults in `dBgCh_Actr::UpdateExtraContinous`
(`src/engine/collision/dBgCh_Actr.cpp`, preferred address 0x00436c59; runtime
RVA 0x36c59 in this build). ESI is 0x1000, used as an actor pointer while reading
the actor's previous position at +0x68/+0x6c. Trace the query object's
initialization and native field offsets to determine why the actor reference
contains a fixed-point value. Do not bypass the collision operation.

## Remaining requirements

Playable game flow, correct level/camera rendering, native coverage for missing
actor/overlay profiles (including profile 352), physical Xbox-controller play,
4K settings verification, correct bottom-screen minimap/transparency, an
upscaled texture pack and verified toggle, 60/120/240 FPS without incorrect game
speed, and complete packaging in `F:\AI Games\Mario 64 DS` remain outstanding.

The active working source is the local `work/source-extracted/sm64ds-decomp-main`
copy in this chat's workspace. Development checkpoint files on F: are evidence
and changed-source snapshots; final packaging remains incomplete.

Recommended next model: Sol High, because collision calling conventions and
object layout remain under investigation. This is not yet a Luna handoff.
Keep the goal active; pause only for a specifically recommended model switch
or an explicit user pause request.
