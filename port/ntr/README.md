# port/ntr -- the host DS platform layer

Imported 2026-08-02 from the port research repo (MIT, project-internal;
twelve commits, each landing one seam with a visual harness). Register and
hardware semantics are from GBATEK only: no SDK source, no emulator code.

What each piece is:

| File | Layer |
|---|---|
| `io.cpp` / `mmio.h` | real memory mapped at the DS addresses (main RAM, I/O, palettes, VRAM, OAM) in the 32-bit process, plus side-effect dispatch for write-triggered registers (CP divider/sqrt, GXFIFO) |
| `gx.cpp` | the geometry engine: command decode from FIFO/port writes, matrix stacks, vertex assembly, lighting at NORMAL time, software raster |
| `gpu.cpp` | native Windows D3D11 rasterizer: shared decoded/clipped geometry, original textures, full selected resolution through 3840x2160, widescreen perspective, separate 2D/menu/minimap layers, GPU presentation and optional full-resolution readback |
| `gl_software.cpp` | optional Mesa llvmpipe OpenGL CPU rasterizer: shared geometry and texture identities, native resolution, DS alpha/shadows, dual-screen composition, and readback/GDI presentation |
| `texture.cpp` | all seven DS texture formats, decoded from mapped VRAM |
| `ppu.cpp` | 2D engine text-mode background scan-out, BMP writer |
| `rt.cpp` / `runtime.cpp` | fiber scheduler (the game blocks in VBlankIntrWait; the host frame loop lives on the other fiber), IRQ state, CpuSet/DMA/CP15 shims |
| `lz77.cpp` | LZ77 type-0x10 decode (third implementation in the project; cross-checks welcome) |
| `fs.cpp` | FNT/FAT parsing, ROM-image and extracted-directory backends (the port's live seam is port/hal/fs.cpp over the asset catalog; this one reads ROM structures directly and serves as a harness backend) |
| `bmd.cpp` | host-side BMD parsing for harnesses. The GAME parses BMDs itself in the real path; this exists to render a model without the game and to cross-check |

KNOWN OPEN BUG (inherited, documented in the research repo's handoff):
`bmd_bone_matrices` conflates record order with bone-index order
(`boneIDs = [1, 2, 0]` on the piano model breaks it) and may mix
record-space and ID-space when resolving `parent_delta`. Fix by building an
array-position -> bone-ID map and composing in topological order. This only
affects the harness parser, not the game-code render path.

The MMIO source transform lives at `port/tools/hostgen.py`: it rewrites
`*(volatile T *)0x04xxxxxx` derefs in copies of `src/` files into `NTR_MMIO()`
proxied accesses, emitted under the build tree. `src/` itself is never edited.

The window uses D3D11 hardware by default, with Windows WARP if hardware device
creation fails. `SM64DS_PC_SOFTWARE_VIDEO=1` selects the original 256x192
software reference path for diagnostics; it cannot provide native 4K detail.
The native pause menu's Graphics page also offers `OPENGL CPU`. It uses the
application-local Mesa runtime in `renderers/mesa`, verifies that
`GL_RENDERER` identifies llvmpipe, and saves `renderer=1` (`0` is D3D11).
Switching is live. Missing or rejected runtimes restore D3D11 and display a
notice in the native menu. Run `port/tools/setup_mesa_runtime.ps1` before
configuring a development build to fetch the pinned runtime and notices.
This uses no system driver installation. Mesa runs on a dedicated thread
with an 8 MiB stack created after the fixed DS regions have been reserved.
The existing raster suite also runs as `smoke_gpu --opengl` /
`smoke_opengl_cpu` when that runtime is present.
The selected resolution controls the GPU render target independently of window
size. Screenshots capture that actual target. 2D assets retain their original
resolution, while 3D polygon edges rasterize at native target resolution.
`smoke_gpu` runs offscreen on WARP and verifies native subpixel coverage at 4K,
software parity, perspective UV/depth/cutouts, clipping/culling, viewports,
completed-frame selection, widescreen frustum expansion, pause/map and resize.

Shadow volumes use the game's two passes: ID0 marks depth-failing pixels in a
stencil mask without writing color/depth; ID>0 shades marked receivers while
excluding matching caster IDs. A six-bit polygon ID and a separate stencil bit
are retained at full resolution. Later masks clear the mask bit without losing
receiver IDs. The visible pass consumes its mask and records its ID to prevent
duplicate shading. Opaque pixels precede translucent submissions; polygon/texel
alpha and translucent depth-write flags are honored. The software reference
implements the same rules independently. GPU regression tests check caster
exclusion, overlapping same/different IDs, mask clearing, missing receivers,
partial alpha, depth writes and actual 4K shadow coverage.

Optional `SM64DS_PC_POLYGON_DUMP=<csv>` with
`SM64DS_PC_POLYGON_DUMP_TICK=<tick>` records one completed polygon list's
attributes, original coordinates and texture alpha range. It is inactive by
default. See `port/docs/native-shadow-masks-checkpoint-2026-10-15.txt` for the
hardware references and bounded game evidence.

## Texture quality

Native PC Settings and F1 T cycle Original /75% /50% /25% /No Textures /Upscaled.
Reduced modes scale each dimension down with a one-texel minimum and deterministic
center sampling. No Textures keeps the full original alpha mask and uses white
RGB with material/vertex lighting. Decal materials switch to modulation in that
mode. Original logical texel coordinates scale with decoded dimensions, so lower
resolution does not move the texture. Native output resolution and 2D layers are
independent of this choice. Settings migrate the earlier hd_textures flag.

The Upscaled art pack is not present. The menu says NO PACK. Decoder ownership
roles block Character/Unknown replacements; native model scopes now provide roles; unresolved models remain Unknown
until their ownership is verified. Existing PNG load
support for explicit Level/Object descriptors is a foundation, not complete game
coverage. See texture-quality-plan-2026-10-15.txt and the checkpoint notes.

### Model ownership and source inventory

texture_assets.cpp registers505 explicit model identities/roles. Native FS loads
record origin before relocation; PC-generated ModelComponents::Render scopes
select it, and heap/free hooks remove expired origins. Character/Unknown always
retain original textures in Upscaled mode, including cache sharing/nested draws.
The VRAM cache includes role, quality and current content, with frame-safe history
pruning. Canonical texture keys hash referenced palette entries rather than VRAM
padding. texture_inventory exports validated TSVs and original-reference PNGs from
the user's ROM.63 Unknown roles and121 initial-unbound textures need further work;
no complete AI texture pack or full-pack performance result is provided here.

### Decoded texture identity reuse

Immutable decoded VRAM-cache entries now keep their RGBA key and partial-alpha
classification. Submitted triangles carry that metadata, so repeated native GPU
presentation does not hash the full image again. Mutable bindings keep the normal
per-frame detection path; new immutable content gets a new key/upload. Explicit
reserved bytes preserve the game-pump's strict byte-equality regression check.
SM64DS_PC_GPU_PROFILE=<csv> records CPU submission/material-work counters;
SM64DS_PC_GPU_FORCE_TEXTURE_SCAN=1 is a diagnostic comparison. Neither is enabled
by default. Counters/time exclude full gameplay and GPU completion; they do not
establish complete-pack FPS. See native-texture-performance-checkpoint-2026-10-15.txt.

### Replacement texture streaming

The native GPU path retains original decoded pixels in the GX CPU cache. A small
canonical asset/role reference selects eligible PNG replacements only when actual
submitted geometry is drawn. Each PNG is decoded temporarily (at most64MiB RGBA),
uploaded, and released from CPU memory. GPU images use an LRU cache, with a hardware
budget selected at startup: half DXGI's local-memory budget clamped to128MiB..4GiB (WARP256MiB).
Diagnostic gpu_set_texture_budget_bytes permits a minimum64MiB budget.

Groups retain descriptors, not cached resource pointers. Resolution happens at
draw time, including the second partial-alpha pass. Cache eviction/settings epoch
retirement waits for queued GPU commands before releasing referenced images;
there is no such wait when the working set fits. Oversized working sets can stall
or repeatedly decode. Missing/invalid/upload-failed PNGs use original art and are
memoized until a quality/backend change. Toggle Original then Upscaled to reload
edited pack files. Character/Unknown exclusions apply again at the GPU boundary.

Software runtime rendering retains original art in Upscaled mode; reduced/none
modes remain supported. Offline tooling's default CpuDecode path remains eager.
Profile CSV adds retained cache bytes/budget, evictions, replacement attempts/
uploads, staging peak payload and GX CPU cache bytes. These are decoded pixel
payload counters, not total process/driver memory or full-pack FPS measurements.
See native-texture-streaming-checkpoint-2026-10-15.txt for verification and limits.

### Source-reviewed archive roles

texture-model-source-evidence.json records29 reviewed changes. Run
port/tools/check_texture_source_roles.py to verify user-ROM overlay2 bytes/model
pointer tables, source constructors/consumers and native table rebasing. The
inventory now has120 Character/285 Object/66 Level/34 Unknown models. Full-file
identity preserves the role for anonymous archive loads. Runtime evidence labels
Mario's archive body Character and rejects a matching replacement. Shared coin
models used as enemy bodies and cap/player cosmetic models remain excluded until
per-draw use is known. See native-archive-roles-checkpoint-2026-10-15.txt; complete
role/animated-variant coverage and production AI artwork are still pending.

### Shared files and draw consumers

TextureConsumerScope surrounds reviewed actor draws. Character forces original
art; Object can promote only nine explicit texture_shared_models.inc paths whose
base role stays Unknown. It cannot override known Character models or promote
unlisted Unknowns. TextureModelScope applies that consumer to the individual
model draw and restores state on exit. Native Player/Coin scopes are generated
without retail source changes; enemy-disguise/cap transforms are reviewed for
future actor coverage. check_texture_consumers.py verifies source/ROM/link gates.
check_model_render_bridge.py ensures the recovered base renderer call does not
rerun virtual animation work and overwrite the player's prepared head transform.
