# FSR 3 upscaling and anti-aliasing

## Current status

The port currently runs as a 32-bit Windows process with D3D11 and Mesa
llvmpipe OpenGL renderers. The D3D11 scene already has color and sampleable
depth targets. Its motion blur reconstructs camera motion from depth and
camera transforms. The upscaler also needs motion for moving objects and a
subpixel jitter sequence. Those inputs still need to be produced.

AMD's FSR 3.1.4 reference backends are Direct3D 12 and Vulkan. SDK 1.1.4
provides a standard x64 Direct3D 12 backend. Its generic build can identify
Win32, but its standard backend list has no Win32 option. Use SDK 1.1.4 as a
reproducible prototype reference. Before release, compare supported FSR 3.1
versions, fixes, SDK requirements, and redistribution terms, then pin the
tested version and retain its notices.
[AMD SDK release](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/releases/tag/v1.1.4)
[AMD's backend build targets](https://raw.githubusercontent.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/v1.1.4/sdk/CMakeLists.txt)

On November 21, 2026, a standalone Win32 probe created Direct3D 12 direct
queues and D3D11On12 devices on the NVIDIA RTX 5070 Laptop GPU and Intel
Graphics in the development PC. Both reported Shader Model 6.6. The probe
loaded no game or SDK assets and verified device creation only.

A follow-up standalone x64 probe loaded AMD's signed 3.1.4 runtime and
dispatched the upscaler on the NVIDIA adapter. Native AA used 480×270 input.
Quality used 320×180 input. Both returned 480×270 GPU readbacks after 24
frames of synthetic motion and a history reset. The previous position of
the moving shape was absent in the first captured frame after the reset
under the probe's color threshold. This completes the synthetic dispatch
milestone.

The first cross-process test also passes on the NVIDIA adapter. A Win32
D3D11On12 producer cleared a shared color texture. An x64 D3D12 helper
waited on the shared fence and copied the image to a second GPU texture.
The producer checked every returned pixel over eight frames per size,
at 480×270 and 854×480 in separate runs. A deliberately failing helper
was detected before the ten-second fence deadline. The transfer used GPU
resources throughout and mapped a readback only for verification. The next
step was to pass the full FSR inputs through this path and dispatch the
upscaler in the helper.

The combined test now passes. The Win32 D3D11On12 producer uploads
synthetic color, depth, and motion into three shared textures. The x64
helper runs AMD FSR 3.1.4 against those inputs and writes a shared GPU
output texture. All six Win32 readbacks, across Native AA and Quality,
match the standalone x64 output byte for byte. Running the helper without
the AMD runtime produced a bounded error. The probe waits on each frame
and reads images only for comparison. Production scheduling, live resize,
timing comparisons, and connection to the live render loop remain open.

An opt-in D3D11 scene probe now converts the game's scene color and depth
to RGBA8 and R32_FLOAT on the GPU before motion blur, 2D overlays, and HUD
composition. The first capture at 854×480 shows the castle scene and its
depth separately. A muted control replay without the probe produced
byte-identical final frame captures both during and after the sampled
frame. The CPU reads the textures only for this diagnostic. The exported
depth is normalized device depth, not distance in world units.

The scene probe also exports an RG32 float camera motion field in render
pixels, directed from the current pixel to its projected position in the
previous frame. It reuses the camera transform math tested by motion blur
and drops history across scene, ownership, time, or render-size changes.
A muted castle replay measured 0.027 pixels at the 95th percentile with
an idle camera and 20.71 pixels during an orbit. The GPU field matched an
independent CPU calculation to within 0.001 pixel in both captures.
The renderer also has an opt-in trace that pairs model triangles across
consecutive frames by model instance, mesh, draw and triangle order. It
checks the original vertex coordinates and viewport before pairing them.
Two muted castle replays measured up to 21.29 pixels at the 95th percentile
for Yoshi's jumping head relative to camera-only motion, while the main
castle geometry stayed below 0.02 pixels in both captures.

The D3D11 diagnostic now rasterizes matched opaque surface motion during
the original color and depth pass. It carries previous homogeneous
positions through viewport clipping and uses camera motion where a
matching visible surface is unavailable. Three muted castle replays
verified idle and jumping Yoshi, plus a byte-identical capture-disabled
control at two presentation ticks. Yoshi's jumping pixels measured
21.13 pixels at the 95th percentile beyond camera motion. The sampled
ground stayed below 0.02 pixels. The jump used matched motion at 395,541
pixels and the fallback at 14,379.

The probe now compares reprojected positions against the immediately
preceding rendered depth buffer and exports a history rejection mask.
In the jumping capture, the check identified 2,628 matched pixels that
had a closer surface in front of them and 8,453 matched pixels that
projected outside the previous view. The jump and control outputs were
byte-identical at both checked ticks. The check uses point-sampled
non-linear D24 depth with a fixed tolerance. It has been exercised in
the castle at 480p and 60 FPS.

The translucent draw pass also exports an R8 reactive mask from the
composited material alpha. It honors the existing visibility tests and
keeps the largest reactive value where blended surfaces overlap. DS
shadow-volume draws contribute zero. Castle replays verified moat water
coverage, zero coverage on sampled opaque ground, and identical outputs
with the diagnostic enabled or disabled. The alpha mask is separate
from the previous-depth rejection diagnostic.

The diagnostic can now jitter two consecutive 60 FPS gameplay rasters
using Native AA's eight-phase Halton sequence. The current offset is
removed when calculating motion, and the previous offset is applied
when checking the previous depth grid. Castle captures confirmed the
subpixel depth shift, the motion and history formulas, and stable HUD
counter samples over the shifted scene. The diagnostic uses a fixed
Native AA phase count. Live submission must query the phase count for
each render-to-output ratio and supply the exact offset to the SDK.
Animated opaque materials and higher presentation rates remain open.

The x86 D3D11On12 game renderer now has an automation-only Native AA
test using an x64 FidelityFX helper on the same adapter. Shared GPU
textures carry live color, depth, motion and reactivity in, and the
resolved image is composited before the HUD. At 854×480, the bridge
processed 96 distinct geometry snapshots and reused 95 results for
repeated presentations. The original projection decodes as infinite
depth on this course; the helper uses the matching SDK context flag.
A missing AMD runtime returned the game to the normal renderer, with
following frames matching the control. The renderer prepares its scene
shader during startup and submits an undisplayed frame before
activation. The profiled warm-up frame took 15.5 ms, down from 120 ms
without shader preparation. The first displayed FSR frame took 6.6 ms,
compared with 441 ms in the earlier cold run. Fresh and repeated frames
had medians of 8.8 ms and 1.4 ms. These timings describe the 480p
developer replay.

The opt-in Quality path renders the 3D scene in a separate target and
resolves into the presentation target before composing the HUD and
minimap. The helper checks the scene size with the SDK and returns the
jitter period before activation. Castle replays completed 96 fresh
snapshots at both 1280×720 to 1920×1080 and 2560×1440 to 3840×2160.
The client area stayed 1280×720 and the 1080p test verified 11,251
unchanged counter pixels. A missing helper runtime took the normal
full-resolution path and matched the 1080p control.

Pause → Options → PC Settings → Graphics offers FSR 3 Off, Native AA,
Quality, Balanced, and Performance. The requested mode persists in the
settings file. It uses the D3D11 renderer with the 60, 120, and 240 FPS
settings. The upscaling presets require motion blur off. An unavailable
helper or unsupported setting leaves the
normal render path active and shows a notice in Graphics. The companion
warms while normal rendering continues, resets temporal history after
a bypass, and recreates its resources when mode or output resolution
changes. A muted restart replay verified restoring Quality at 1080p,
switching to 4K Quality, then switching to 4K Native AA with the same
window dimensions.

At 4K, Balanced renders the scene at 2258×1270 and Performance uses
1920×1080. Both keep the HUD, menus, and window layout at the existing
output settings. A muted replay verified switching from Balanced to
Performance in the Graphics menu and saving the selected preset. The
companion validates each render size against the FidelityFX API before
starting the preset.
[AMD FSR preset scaling ratios](https://gpuopen.com/manuals/fidelityfx_sdk/techniques/super-resolution-upscaler/#scaling-modes)

At higher refresh targets, the renderer passes each distinct predicted
camera view and its motion vectors to FSR. Presentations with unchanged
geometry and camera pose can reuse the last resolved image. The frame
scheduler skips extra draws that would miss the next 60 Hz game tick.
The FPS setting is a presentation target, with achieved rates depending
on the resolution, scene, and rendering cost.

The live scene conversion writes color, depth, and motion for the companion.
The opaque-scene copy, camera-only motion, previous depth, and history
rejection outputs are allocated and written for diagnostic captures.
This reduces intermediate texture storage by 20 bytes per scene pixel
during gameplay, before any allocation padding.

If the FSR device is removed during play, the game tries native D3D11 in
the same window and resumes at the selected output resolution. The Graphics
page reports FSR unavailable during that fallback. Changing the FSR setting
to Off and back requests a new upscaling session. If native D3D11 cannot
start, the software renderer remains available.

For a developer build, compile `port/tools/fsr3_helper.cpp` as x64
with the FidelityFX API 3.1.4 header include root and link `d3d12.lib`
and `dxgi.lib`. Place the resulting `sm64ds_fsr3_helper.exe` and matching
`amd_fidelityfx_dx12.dll` beside the x86 port executable. The helper and
DLL are optional when FSR is off. Higher presentation rates and
additional upscaling modes remain open.

The F: launcher originally encountered DXGI Present E_INVALIDARG at
startup with Quality already saved. Direct launch of the same binary
on F: reproduced normal GPU rendering. The FSR renderer now creates a
flip-model swap chain with its D3D12 queue, draws with wrapped back
buffers, and releases each buffer to PRESENT before displaying it.
Both launch paths pass the startup check. The window test also covers
client resize, maximize, and switching off and back to FSR.
[Microsoft's D3D11On12 presentation guidance](https://learn.microsoft.com/en-us/windows/win32/direct3d12/direct3d-11-on-12)

The installed 1080p Quality preview passed an original Bob-omb
Battlefield entry, Exit Course, and re-entry. The mission chooser and
pause captures matched the Off control. Quality cleared its history on
the first resolved frame in each newly entered scene, and all 1,401
sampled player states and positions matched the Off replay.

A separate 4K Performance check on the NVIDIA RTX 5070 Laptop GPU
completed the same original Battlefield entry, pause, exit to the castle,
and course re-entry at the 60 FPS setting. The helper resolved 2,156
snapshots from 1920×1080 to 3840×2160, with a history reset at the first
resolved frame in each gameplay scene. The client stayed at 1280×720.
The selected menu and pause captures and all 1,401 sampled gameplay
states matched Off. Both replays measured about 60 presentations per
second over the sampled Battlefield interval. This covers one route
and device at the 60 FPS cap.

The same 4K Performance replay at the 240 FPS cap matched the 60 FPS
run's 1,401 sampled gameplay states and course-transition ticks. Over
the measured Battlefield interval it presented 93.58 frames per second
while gameplay stayed at 59.99 simulation ticks per second. The menu
captures still matched and the client remained 1280×720. The 240 setting
is an upper limit, and actual frame rate depends on the scene and device.

## Implementation milestones

1. Build a minimal x64 Direct3D 12 FSR 3.1 sample with synthetic geometry.
   Dispatch Native AA and Quality using real depth, color and motion inputs.
   Capture a fixed test camera and an independently moving object in both
   modes. Check resource validation and reset behavior with a camera cut.
2. Prove a GPU-only exchange between the Win32 renderer and the x64 sample
   on the same adapter. Start with one shared scene texture and a shared
   fence, then add the input and output textures the sample uses. Check
   format support, ordering, resized resources, adapter matching, and
   helper exit. Compare CPU and GPU frame times against the same-adapter
   baseline. D3D11On12 is a candidate for the Win32 producer. Any in-process
   alternative needs a working Win32 FSR build or a separately verified
   x64 game host.
   [Microsoft's Direct3D 11 on 12 guidance](https://learn.microsoft.com/en-us/windows/win32/direct3d12/direct3d-11-on-12)
3. Add scene motion vectors using previous and current camera and object
   transforms, with coverage for static geometry, moving platforms and
   animated characters. Validate direction and scale with known motion.
   Mark translucent and animated materials with the reactive inputs where
   needed. Use consistent color, depth and jitter conventions. Reset
   history on camera cuts, doors and scene transitions, changes to render
   size, and the first frame after a bypass.
   [AMD's upscaler input and reset guidance](https://gpuopen.com/manuals/fidelityfx_sdk/techniques/super-resolution-upscaler/)
4. Upscale the 3D scene before drawing the HUD, menus, dialogue and minimap.
   Composite those layers at output size. Keep touch coordinates tied to
   the displayed layout. Integrate motion blur separately after resolving
   the temporal inputs. Preserve the existing simulation clock at every
   presentation rate.
5. Add a Graphics setting with Off as the default, plus Native AA and the
   supported Quality, Balanced and Performance presets once each passes
   acceptance. Use the selected Resolution as the output target and derive
   the scene's internal raster from the active preset. Native AA uses that
   target at 1×. The outer window size remains independently controlled.
   Save the requested setting and show the effective mode if a renderer,
   driver, layout or resolution requires fallback. Keep Native DS and
   other low presets available on the existing render path.
   [AMD's preset definitions](https://gpuopen.com/manuals/fidelityfx_sdk/samples/super-resolution/)
6. Verify muted castle and course runs at 1080p and 4K, including a stationary
   view, moving camera, animated player, translucent effects, pause and
   course transitions. Compare Off against each accepted mode for frame
   time, responsiveness, edge clarity, and ghosting. Exercise restart,
   unavailable SDK or device, helper failure, and renderer changes. Verify
   the existing dual-screen layouts and HUD clarity. Record actual scene
   raster and output dimensions. Frame generation has its own later
   milestone with separate latency and frame pacing checks.
