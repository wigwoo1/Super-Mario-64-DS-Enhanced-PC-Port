# Runtime profile audit — 2026-10-14

## What the latest trace proves

The earlier Player animation-table setup fault is resolved in the current
checkpoint: runtime `resume96` reached 1,200 simulation frames and exited 0.
That run does not prove full gameplay; it still logged missing native actor
profiles.

The two repeated missing profiles are:

- **349 — EXIT / CH_SCENE**, from overlay 2. The class factory
  `daChScene_c_classInit` and class implementation are present in
  `src/game/actors/d_a_ch_scene.cpp`.
- **353 — DOOR**, from overlay 100. The class factory
  `daDoor_c_classInit` and implementation are present in `src/actors/daDoor_c.cpp`.

The runtime log labels both as unavailable in the host actor table. The current
link map includes `pc_actor_spawn_table_init`, but not either native class
factory. The table currently seeds only the boot/title/stage/player/camera
profiles in `port/hal/actor_vtables.cpp`. Thus these entries are skipped by the
fallback path; this is separate from the already-fixed animation setup crash.

Evidence sources are `build/port/sm64ds_pc.map`,
`outputs/native-checkpoint-2026-10-14-camera-rendering/resume96.stderr.log`,
`symbols/profile_reconstruction_registry.tsv`, and the two source files above.

## Recommended next investigation

Integrate the recovered EXIT and DOOR factories into the native actor profile
table, including their profile metadata and any overlay-owned initialization
they require. Then run the normal game flow and capture a screenshot. This is
deeper actor/runtime integration work; Sol High is recommended over Luna Low.
Do not treat quieting the fallback trace as a fix—the actors must construct and
complete their expected lifecycle in the game.
