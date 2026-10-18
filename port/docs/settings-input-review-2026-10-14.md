# PC settings and input review — 2026-10-14

Status: PARTIALLY VERIFIED. Resolution, map layout, scaling, texture toggle,
frame-rate selection, persistence and analog-stick math have automated checks.
Physical Xbox gameplay, complete replacement textures and actual 4K rendering
remain unverified.

## Current checks

- `smoke_video`, `smoke_input` and `smoke_frames` pass (3/3).
- Resolution presets cover 16:9, 16:10 and 21:9 through 3840x2160. The window
  still scales the native 256x192 DS image; this is not internal 4K rendering.
- Minimap layout and transparency controls, crisp/smooth output scaling and
  texture replacement fallback are covered by the video smoke test. A complete
  4K replacement texture pack is not included.
- Settings now load and save beside the executable in `sm64ds_pc.ini`. The
  saved values include resolution, 60/120/240 presentation rate, minimap state,
  opacity, size and position, 2D scaling, and the texture replacement toggle.
  Saving uses a temporary file followed by an atomic replace.
- Input code polls Xbox-compatible XInput controllers and sends the left stick
  through radial deadzone and analog magnitude handling into the game's Circle
  Pad virtual joystick. Automated checks cover deadzone, direction, diagonal
  normalization and partial travel. No physical Xbox controller was available
  for this run, so in-game controller operation still needs hands-on checking.
- Frame tests validate the allowed 60/120/240 settings and the 60 Hz simulation
  with repeated presentations at higher rates. They do not measure delivered
  display FPS.

## Limits and remaining work

- Settings persistence has a file round-trip test. Running the game creates
  `sm64ds_pc.ini` beside `sm64ds_pc.exe` after a menu option is changed.
- 3840x2160 is an output-window size. Rendering remains at 256x192 internally.
- The texture option loads hash-named replacements from `textures/4k`, falling
  back to native game textures when a replacement is missing. No complete pack
  exists yet.
- Rendering displays the castle and surrounding ground, but full gameplay,
  player/HUD/map correctness and missing actor profiles are still open.

The bounded settings persistence milestone is complete. The resumed runtime
audit identifies missing EXIT/CH_SCENE and DOOR actor registrations; integrating
those recovered classes requires deeper actor/overlay debugging, so Sol High is
recommended for that next task.
