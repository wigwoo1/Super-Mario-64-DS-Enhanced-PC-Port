cmake_minimum_required(VERSION 3.16)
project(sm64ds-port C CXX ASM_MASM)
include(CTest)
add_executable(smoke_replay_path tests/smoke_replay_path.cpp)
add_test(NAME smoke_replay_path COMMAND smoke_replay_path)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
if(WIN32)
    add_executable(smoke_game_clock tests/smoke_game_clock.cpp hal/os_time.cpp)
    target_compile_definitions(smoke_game_clock PRIVATE SM64DS_GAME_TARGET)
    add_test(NAME smoke_game_clock COMMAND smoke_game_clock)
endif()
set(TOAD_REWARD_ORIGINAL "${CMAKE_BINARY_DIR}/toad-reward-test/toad_reward_original.inc")
add_custom_command(OUTPUT "${TOAD_REWARD_ORIGINAL}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/toad_reward_test_source.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/game/actors/d_a_kinopio.cpp" "${TOAD_REWARD_ORIGINAL}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/toad_reward_test_source.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/game/actors/d_a_kinopio.cpp"
    VERBATIM)
add_executable(smoke_toad_reward tests/smoke_toad_reward.cpp "${TOAD_REWARD_ORIGINAL}")
target_include_directories(smoke_toad_reward PRIVATE "${CMAKE_BINARY_DIR}/toad-reward-test")
add_test(NAME smoke_toad_reward COMMAND smoke_toad_reward)
if(WIN32)
    execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/sdl3_dependency.py"
        "${CMAKE_BINARY_DIR}/controller-deps" RESULT_VARIABLE SDL3_RESULT OUTPUT_VARIABLE SDL3_SDK OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT SDL3_RESULT EQUAL 0)
        message(FATAL_ERROR "Could not prepare the pinned SDL gamepad SDK")
    endif()
    file(TO_CMAKE_PATH "${SDL3_SDK}" SDL3_SDK)
    configure_file("${SDL3_SDK}/lib/x86/SDL3.dll" "${CMAKE_BINARY_DIR}/SDL3.dll" COPYONLY)
    configure_file("${SDL3_SDK}/LICENSE.txt" "${CMAKE_BINARY_DIR}/SDL3-LICENSE.txt" COPYONLY)
endif()
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MAP")
add_compile_options(/Oy-)

# Gate 1: compile the manifest slice out of ../src plus the HAL, link the
# smoke runner. 32-bit host only -- platform.h enforces it at compile time.

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# The matching toolchain selects C++ by an exact first-line //cpp marker even
# when a source keeps its historical .c path. Teach the host build the same
# rule so a verified language-mode migration cannot silently break a slice.
function(sm64ds_honor_cpp_markers source_var)
    foreach(source IN LISTS ${source_var})
        if(source MATCHES "\\.c$")
            file(STRINGS "${source}" first_line LIMIT_COUNT 1)
            if(first_line STREQUAL "//cpp")
                set_source_files_properties("${source}" PROPERTIES LANGUAGE CXX)
            endif()
        endif()
    endforeach()
endfunction()


file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate1.txt" SLICE_LINES)
set(SLICE_SOURCES "")
foreach(line IN LISTS SLICE_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()
sm64ds_honor_cpp_markers(SLICE_SOURCES)

add_executable(smoke
    tests/smoke.cpp
    hal/cstd_div.c
    hal/os_time.cpp
    hal/shims.cpp
    hal/mem_delete2.cpp
    ${SLICE_SOURCES})

target_include_directories(smoke PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")

target_compile_definitions(smoke PRIVATE SM64DS_PLATFORM_PC)
target_link_libraries(smoke PRIVATE ntr)
if(MSVC)
    # The decomp C uses guest idioms MSVC warns about; keep the build quiet
    # enough that a NEW warning is visible. /W3 stays on.
    target_compile_options(smoke PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 2: the allocator family + torture smoke, its own binary so gate 1
# stays a stable baseline.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate2.txt" SLICE2_LINES)
set(SLICE2_SOURCES "")
foreach(line IN LISTS SLICE2_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE2_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()
sm64ds_honor_cpp_markers(SLICE2_SOURCES)

add_executable(smoke_heap tests/smoke_heap.cpp hal/heap_globals.cpp ${SLICE2_SOURCES})
target_include_directories(smoke_heap PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_compile_definitions(smoke_heap PRIVATE SM64DS_PLATFORM_PC)
if(MSVC)
    target_compile_options(smoke_heap PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 3a: root heap + Memory:: layer over the gate-2 allocator.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate3a.txt" SLICE3A_LINES)
set(SLICE3A_SOURCES "")
foreach(line IN LISTS SLICE3A_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE3A_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()
sm64ds_honor_cpp_markers(SLICE3A_SOURCES)

add_executable(smoke_roots tests/smoke_roots.cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/_ZdlPv.cpp"
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/mem_delete2.cpp
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES})
target_include_directories(smoke_roots PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_compile_definitions(smoke_roots PRIVATE SM64DS_PLATFORM_PC)
if(MSVC)
    target_compile_options(smoke_roots PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# The ntr platform layer: imported host DS hardware (see ntr/README.md).
# Compiled as its own lib so every gate binary can link the pieces it needs.
add_library(ntr STATIC
    ntr/sdat.cpp ntr/audio.cpp
    third_party/sseqplayer/Player.cpp third_party/sseqplayer/Track.cpp
    third_party/sseqplayer/Channel.cpp third_party/sseqplayer/SSEQ.cpp
    third_party/sseqplayer/SBNK.cpp third_party/sseqplayer/SWAR.cpp
    third_party/sseqplayer/SWAV.cpp third_party/sseqplayer/INFOEntry.cpp
    third_party/sseqplayer/NDSStdHeader.cpp
    ntr/texture_assets.cpp
    hal/oam_buffers.asm
    hal/controller_state.asm
    ntr/io.cpp ntr/gx.cpp ntr/gpu.cpp ntr/gl_software.cpp ntr/minimap_hud.cpp ntr/texture.cpp ntr/ppu.cpp ntr/video.cpp ntr/input.cpp ntr/sdl_gamepad.cpp ntr/lz77.cpp
    ntr/rt.cpp ntr/runtime.cpp ntr/fs.cpp ntr/bmd.cpp ntr/screen_layout.cpp)
target_include_directories(ntr PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/ntr/include")
if(MSVC)
    target_include_directories(ntr PRIVATE "${SDL3_SDK}/include")
    target_compile_options(ntr PRIVATE /wd4311 /wd4312)
    target_link_libraries(ntr PUBLIC windowscodecs ole32 d3d11 dxgi d3dcompiler winmm)
endif()

# Controller-input smoke: verify the native stick's radial deadzone and its
# continuous mapping into the SM64DS virtual joystick's normalized vector.
add_executable(smoke_input tests/smoke_input.cpp)
target_link_libraries(smoke_input PRIVATE ntr)
if(WIN32)
    add_executable(smoke_sdl_gamepad tests/smoke_sdl_gamepad.cpp)
    target_include_directories(smoke_sdl_gamepad PRIVATE "${SDL3_SDK}/include")
    target_link_libraries(smoke_sdl_gamepad PRIVATE ntr "${SDL3_SDK}/lib/x86/SDL3.lib")
    add_test(NAME smoke_sdl_gamepad COMMAND smoke_sdl_gamepad)
endif()
add_test(NAME smoke_input COMMAND smoke_input)
add_executable(smoke_sdat tests/smoke_sdat.cpp)
target_link_libraries(smoke_sdat PRIVATE ntr)
add_test(NAME smoke_sdat COMMAND smoke_sdat)
add_executable(smoke_audio_output tests/smoke_audio_output.cpp)
target_link_libraries(smoke_audio_output PRIVATE ntr)
add_test(NAME smoke_audio_output COMMAND smoke_audio_output)
add_executable(probe_sdat tests/probe_sdat.cpp)
target_link_libraries(probe_sdat PRIVATE ntr)
add_executable(smoke_screen_layout tests/smoke_screen_layout.cpp)
target_link_libraries(smoke_screen_layout PRIVATE ntr)
add_test(NAME smoke_screen_layout COMMAND smoke_screen_layout)
if(WIN32)
    add_executable(smoke_video_window tests/smoke_video_window.cpp)
    target_link_libraries(smoke_video_window PRIVATE ntr)
    add_test(NAME smoke_video_window COMMAND smoke_video_window)
endif()
add_executable(smoke_pause_settings tests/smoke_pause_settings.cpp hal/pc_pause_settings_state.cpp)
target_include_directories(smoke_pause_settings PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(smoke_pause_settings PRIVATE ntr)
add_test(NAME smoke_pause_settings COMMAND smoke_pause_settings)
add_executable(smoke_rebinding tests/smoke_rebinding.cpp hal/pc_pause_settings_state.cpp)
target_include_directories(smoke_rebinding PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(smoke_rebinding PRIVATE ntr)
add_test(NAME smoke_rebinding COMMAND smoke_rebinding)
add_executable(smoke_camera_controls tests/smoke_camera_controls.cpp)
target_include_directories(smoke_camera_controls PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/ntr/include")
add_test(NAME smoke_camera_controls COMMAND smoke_camera_controls)
add_executable(smoke_minimap_hud tests/smoke_minimap_hud.cpp ntr/minimap_hud.cpp)
target_include_directories(smoke_minimap_hud PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/ntr/include")
add_test(NAME smoke_minimap_hud COMMAND smoke_minimap_hud)
add_executable(smoke_render_distance tests/smoke_render_distance.cpp)
target_include_directories(smoke_render_distance PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/ntr/include")
add_test(NAME smoke_render_distance COMMAND smoke_render_distance)
add_executable(smoke_wanted_settings_surface tests/smoke_wanted_settings_surface.cpp
    hal/wanted_settings_surface.cpp hal/pc_pause_settings_state.cpp)
target_include_directories(smoke_wanted_settings_surface PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(smoke_wanted_settings_surface PRIVATE ntr)
add_test(NAME smoke_wanted_settings_surface COMMAND smoke_wanted_settings_surface)
add_executable(smoke_texture_quality tests/smoke_texture_quality.cpp)
target_link_libraries(smoke_texture_quality PRIVATE ntr)
add_test(NAME smoke_texture_quality COMMAND smoke_texture_quality)
add_executable(texture_inventory tools/texture_inventory.cpp)
target_link_libraries(texture_inventory PRIVATE ntr)
add_executable(smoke_texture_assets tests/smoke_texture_assets.cpp)
target_link_libraries(smoke_texture_assets PRIVATE ntr)
add_test(NAME smoke_texture_assets COMMAND smoke_texture_assets)
add_executable(smoke_pc_session tests/smoke_pc_session.cpp hal/pc_session.cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/engine/save/SaveData.cpp")
target_include_directories(smoke_pc_session PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_compile_definitions(smoke_pc_session PRIVATE SM64DS_PLATFORM_PC)
if(MSVC)
    target_compile_options(smoke_pc_session PRIVATE /Gy)
    target_link_options(smoke_pc_session PRIVATE /OPT:REF)
endif()
add_test(NAME smoke_pc_session COMMAND smoke_pc_session)
add_executable(smoke_dsmt_primitives tests/smoke_dsmt_primitives.cpp hal/dsmt_primitives.cpp)
add_test(NAME smoke_dsmt_primitives COMMAND smoke_dsmt_primitives)
add_executable(smoke_dma tests/smoke_dma.cpp)
target_link_libraries(smoke_dma PRIVATE ntr)
add_test(NAME smoke_dma COMMAND smoke_dma)
add_executable(smoke_native_save tests/smoke_native_save.cpp hal/native_save.cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/engine/save/SaveData.cpp")
target_include_directories(smoke_native_save PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_compile_definitions(smoke_native_save PRIVATE SM64DS_PLATFORM_PC)
add_test(NAME smoke_native_save COMMAND smoke_native_save)
set_tests_properties(smoke_native_save PROPERTIES ENVIRONMENT
    "TEMP=${CMAKE_CURRENT_BINARY_DIR};TMP=${CMAKE_CURRENT_BINARY_DIR}")

add_executable(smoke_texture_performance tests/smoke_texture_performance.cpp)
target_link_libraries(smoke_texture_performance PRIVATE ntr)
add_test(NAME smoke_texture_performance COMMAND smoke_texture_performance)
add_executable(smoke_texture_streaming tests/smoke_texture_streaming.cpp)
target_link_libraries(smoke_texture_streaming PRIVATE ntr)
add_test(NAME smoke_texture_streaming COMMAND smoke_texture_streaming)
add_executable(smoke_input_state tests/smoke_input_state.cpp hal/input_state.asm
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/unnamed/arm9/0203/func_0203bc7c.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/unnamed/arm9/0203/func_0203bc50.c")
target_include_directories(smoke_input_state PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_compile_definitions(smoke_input_state PRIVATE SM64DS_PLATFORM_PC)
target_link_libraries(smoke_input_state PRIVATE ntr)
add_test(NAME smoke_input_state COMMAND smoke_input_state)
add_executable(smoke_input_history tests/smoke_input_history.cpp hal/input_state.asm
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/unnamed/arm9/0203/func_0203dafc.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/unnamed/arm9/0203/func_0203e0ac.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/GetAngleToCamera.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/GetPlayerFlagByte.c")
target_include_directories(smoke_input_history PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_compile_definitions(smoke_input_history PRIVATE SM64DS_PLATFORM_PC)
add_test(NAME smoke_input_history COMMAND smoke_input_history)

add_executable(smoke_video tests/smoke_video.cpp)
target_link_libraries(smoke_video PRIVATE ntr)
add_executable(smoke_dialogue_overrides tests/smoke_dialogue_overrides.cpp hal/dialogue_overrides.cpp)
target_include_directories(smoke_dialogue_overrides PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME smoke_dialogue_overrides COMMAND smoke_dialogue_overrides)
add_executable(smoke_gpu tests/smoke_gpu.cpp)
target_link_libraries(smoke_gpu PRIVATE ntr)
add_test(NAME smoke_gpu COMMAND smoke_gpu)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../build/mesa-runtime/x86/opengl32.dll")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/renderers/mesa")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../build/mesa-runtime/x86/opengl32.dll" "${CMAKE_CURRENT_BINARY_DIR}/renderers/mesa/opengl32.dll" COPYONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../build/mesa-runtime/x86/libgallium_wgl.dll" "${CMAKE_CURRENT_BINARY_DIR}/renderers/mesa/libgallium_wgl.dll" COPYONLY)
    add_test(NAME smoke_opengl_cpu COMMAND smoke_gpu --opengl)
endif()

# Optional ROM-backed overlay verification. Kept opt-in because each developer
# supplies their own cartridge image and extracted overlay comparison set.
add_executable(smoke_overlay EXCLUDE_FROM_ALL tests/smoke_overlay.cpp)
target_include_directories(smoke_overlay PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/ntr/include")
target_link_libraries(smoke_overlay PRIVATE ntr)

# Gate 3b: SharedFilePtr over the host card seam; assets come from the
# repo's extracted/ tree + asset catalog, root baked in (env-overridable).
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate3b.txt" SLICE3B_LINES)
set(SLICE3B_SOURCES "")
foreach(line IN LISTS SLICE3B_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE3B_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()
sm64ds_honor_cpp_markers(SLICE3B_SOURCES)

get_filename_component(PORT_REPO_ROOT_ABS "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# Resolve hostgen symbols through the decomp's production enrollment table.  A
# reconstructed translation unit can own several symbols under one source path, so
# src/<symbol>.c|.cpp probing is no longer a valid source lookup.  Dedupe by resolved
# path: two requested symbols in the same TU must compile that TU once, not twice.
function(sm64ds_hostgen_symbols out_var)
    set(generated "")
    set(seen_sources "")
    foreach(sym IN LISTS ARGN)
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" "${PORT_REPO_ROOT_ABS}/tools/srcpath.py" "${sym}"
            WORKING_DIRECTORY "${PORT_REPO_ROOT_ABS}"
            RESULT_VARIABLE resolve_result
            OUTPUT_VARIABLE sym_rel
            ERROR_VARIABLE resolve_error
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(NOT resolve_result EQUAL 0 OR sym_rel MATCHES ": not in src/$")
            message(FATAL_ERROR
                "hostgen source resolution failed for ${sym}: ${sym_rel}${resolve_error}")
        endif()
        set(sym_src "${PORT_REPO_ROOT_ABS}/${sym_rel}")
        if(NOT EXISTS "${sym_src}")
            message(FATAL_ERROR "hostgen resolved ${sym} to missing ${sym_rel}")
        endif()
        list(FIND seen_sources "${sym_src}" seen_index)
        if(NOT seen_index EQUAL -1)
            continue()
        endif()
        list(APPEND seen_sources "${sym_src}")
        string(REGEX REPLACE "\\.(c|cpp)$" ".cpp" sym_out_rel "${sym_rel}")
        set(sym_out "${CMAKE_BINARY_DIR}/host-src/${sym_out_rel}")
        set(hostgen_extra_deps "")
        if(sym STREQUAL "func_0204a5c8")
            list(APPEND hostgen_extra_deps "${CMAKE_CURRENT_SOURCE_DIR}/tools/particle_host.py")
        endif()
        add_custom_command(OUTPUT "${sym_out}"
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
                    --out "${CMAKE_BINARY_DIR}/host-src" ${sym}
            DEPENDS "${sym_src}"
                    "${PORT_REPO_ROOT_ABS}/tools/srcpath.py"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
                    ${hostgen_extra_deps}
            COMMENT "hostgen ${sym_rel}")
        list(APPEND generated "${sym_out}")
    endforeach()
    set(${out_var} "${generated}" PARENT_SCOPE)
endfunction()

sm64ds_hostgen_symbols(DSMT_INLINE_GEN func_ov007_020b58e4 func_ov007_020c31e0 func_ov007_020c8b34)
add_executable(smoke_dsmt_inline tests/smoke_dsmt_inline.cpp ${DSMT_INLINE_GEN})
target_link_libraries(smoke_dsmt_inline PRIVATE ntr)
target_include_directories(smoke_dsmt_inline PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_compile_definitions(smoke_dsmt_inline PRIVATE SM64DS_PLATFORM_PC)
add_test(NAME smoke_dsmt_inline COMMAND smoke_dsmt_inline)

sm64ds_hostgen_symbols(DSMT_ROTATION_GEN func_ov007_020c39f8)
add_executable(smoke_dsmt_rotation tests/smoke_dsmt_rotation.cpp ${DSMT_ROTATION_GEN})
target_include_directories(smoke_dsmt_rotation PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_dsmt_rotation PRIVATE ntr)
target_compile_definitions(smoke_dsmt_rotation PRIVATE SM64DS_PLATFORM_PC)
add_test(NAME smoke_dsmt_rotation COMMAND smoke_dsmt_rotation)
add_executable(smoke_dsmt_input tests/smoke_dsmt_input.cpp hal/dsmt_input.cpp
    ../src/unnamed/arm9/0203/func_0203da9c.c
    ../src/unnamed/arm9/0203/func_0203dabc.c
    ../src/unnamed/arm9/0203/func_0203dae4.c)
target_include_directories(smoke_dsmt_input PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(smoke_dsmt_input PRIVATE ntr)
add_test(NAME smoke_dsmt_input COMMAND smoke_dsmt_input)
sm64ds_hostgen_symbols(DSMT_CONSTRUCTOR_GEN func_ov007_020c798c func_ov007_020c2b9c)
add_executable(smoke_dsmt_constructor tests/smoke_dsmt_constructor.cpp ${DSMT_CONSTRUCTOR_GEN}
    ../src/unnamed/ov007/func_ov007_020c78b0.c)
target_include_directories(smoke_dsmt_constructor PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_dsmt_constructor PRIVATE ntr)
target_compile_definitions(smoke_dsmt_constructor PRIVATE SM64DS_PLATFORM_PC)
add_test(NAME smoke_dsmt_constructor COMMAND smoke_dsmt_constructor)
sm64ds_hostgen_symbols(DSMT_PALETTE_GEN func_ov007_020c1404 func_ov007_020c4684 func_ov007_020c1448 func_ov007_020caeac)
add_executable(smoke_dsmt_palette tests/smoke_dsmt_palette.cpp ${DSMT_PALETTE_GEN})
target_link_libraries(smoke_dsmt_palette PRIVATE ntr)
target_include_directories(smoke_dsmt_palette PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_compile_definitions(smoke_dsmt_palette PRIVATE SM64DS_PLATFORM_PC)
add_test(NAME smoke_dsmt_palette COMMAND smoke_dsmt_palette)




# Compile the production controller-input update with the native analog adapter.
# This object target is linked into the eventual standalone game target; keeping
# it independently buildable lets the host-side input integration stay covered
# while the game executable's remaining framework slices are still incomplete.
sm64ds_hostgen_symbols(PC_ANALOG_INPUT_GEN _ZN10dScStage_c10CheckInputEv)
add_library(pc_analog_input OBJECT ${PC_ANALOG_INPUT_GEN})
target_include_directories(pc_analog_input PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    "${CMAKE_CURRENT_SOURCE_DIR}/ntr/include")
target_compile_definitions(pc_analog_input PRIVATE SM64DS_PLATFORM_PC)
if(MSVC)
    target_compile_options(pc_analog_input PRIVATE /wd4311 /wd4312 /wd4068)
endif()

add_executable(smoke_controller_state tests/smoke_controller_state.cpp
    hal/input_state.asm $<TARGET_OBJECTS:pc_analog_input>)
target_include_directories(smoke_controller_state PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_compile_definitions(smoke_controller_state PRIVATE SM64DS_PLATFORM_PC)
target_link_libraries(smoke_controller_state PRIVATE ntr)
add_test(NAME smoke_controller_state COMMAND smoke_controller_state)
sm64ds_hostgen_symbols(TOUCH_HISTORY_GEN func_0203bb60)
add_executable(smoke_touch_state tests/smoke_touch_state.cpp hal/touch_state.asm ${TOUCH_HISTORY_GEN})
target_include_directories(smoke_touch_state PRIVATE "${PORT_REPO_ROOT_ABS}/include")
target_link_libraries(smoke_touch_state PRIVATE ntr)
target_compile_definitions(smoke_touch_state PRIVATE SM64DS_PLATFORM_PC)
add_test(NAME smoke_touch_state COMMAND smoke_touch_state)
set(REC_ROOM_TOUCH_GEN "${CMAKE_BINARY_DIR}/host-src/rec_room_touch_fixture.cpp")
add_custom_command(OUTPUT "${REC_ROOM_TOUCH_GEN}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/rec_room_touch_fixture.py"
            "${PORT_REPO_ROOT_ABS}/src/actors/dScMiniGm_c.cpp" "${REC_ROOM_TOUCH_GEN}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/rec_room_touch_fixture.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
            "${PORT_REPO_ROOT_ABS}/src/actors/dScMiniGm_c.cpp")
add_executable(smoke_rec_room_touch tests/smoke_rec_room_touch.cpp hal/touch_state.asm ${TOUCH_HISTORY_GEN} "${REC_ROOM_TOUCH_GEN}")
target_include_directories(smoke_rec_room_touch PRIVATE "${PORT_REPO_ROOT_ABS}/include")
target_link_libraries(smoke_rec_room_touch PRIVATE ntr)
target_compile_definitions(smoke_rec_room_touch PRIVATE SM64DS_PLATFORM_PC)
target_compile_options(smoke_rec_room_touch PRIVATE /Gy /wd4068 /wd4311 /wd4312)
target_link_options(smoke_rec_room_touch PRIVATE /OPT:REF)
add_test(NAME smoke_rec_room_touch COMMAND smoke_rec_room_touch)

# Gate 4a: the game's display-list submission machinery over the ntr GX.
# These src/ files touch MMIO textually, so they build from hostgen-emitted
# copies under the build tree -- src/ itself is never edited.
set(GATE4A_SYMS
    func_0205532c func_02055388 func_020553a4 func_02055780
    func_0205a358 func_0205a290 func_0205a21c func_02056e98 func_02059fa8)
sm64ds_hostgen_symbols(GATE4A_GEN ${GATE4A_SYMS})

add_executable(smoke_gx tests/smoke_gx.cpp ${GATE4A_GEN})
target_include_directories(smoke_gx PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_gx PRIVATE ntr)
target_compile_definitions(smoke_gx PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_gx PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 4b: the whole Model pipeline. Adds the material-bind and matrix
# feeders to the hostgen set and the Model/render slice on top of everything.
set(GATE4B_SYMS func_02044b30 Geometry_MatrixMultiply3x3 func_02055998
    func_02044534 func_0204488c NormalizeVec3)
sm64ds_hostgen_symbols(GATE4B_GEN ${GATE4B_SYMS})

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate4b.txt" SLICE4B_LINES)
set(SLICE4B_SOURCES "")
foreach(line IN LISTS SLICE4B_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE4B_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()
sm64ds_honor_cpp_markers(SLICE4B_SOURCES)
# Host copies used by the GX texture upload path. The physical DS memory
# window is mapped at its original address by ntr/io.cpp, so these routines
# retain their ordinary pointer semantics on the PC host.
list(APPEND SLICE4B_SOURCES
    "${PORT_REPO_ROOT_ABS}/src/DMASyncHalfTransfer.c"
    "${PORT_REPO_ROOT_ABS}/src/MultiCopyHalf.c")

# ROM constants (sine table, default matrices): generated locally from the
# extracted arm9 image so Nintendo bytes never enter the repo.
set(ROMDATA_C "${CMAKE_BINARY_DIR}/host-src/romdata.c")
add_custom_command(OUTPUT "${ROMDATA_C}"
    COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tools/romdata.py" "${ROMDATA_C}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/romdata.py"
    COMMENT "romdata from extracted/arm9_dec.bin")

# The actor slice reaches a handful of ov002 globals. Generate their real
# file-backed bytes and zero-initialized BSS storage from the local ROM image.
set(OV002_DATA_SYMBOLS
    data_ov002_020ff480
    data_ov002_0210d9c8 data_ov002_0210da00
    data_ov002_0211010c data_ov002_02110124
    data_ov002_02110364 data_ov002_02110394
    data_ov002_02110574 data_ov002_021105bc
    data_ov002_021105ec data_ov002_0211067c
    data_ov002_020ff128 data_ov002_020ff2f0
    data_ov002_02110094 data_ov002_021100ac
    data_ov002_0211013c data_ov002_0211022c
    data_ov002_0211058c data_ov002_021106ac
    data_ov002_0210c080 data_ov002_0211110c
    data_ov002_0210d7f4
    data_ov002_0210d794 data_ov002_0210d8f4
    data_ov002_0210d854 data_ov002_0210d894
    data_ov002_0210d834
    data_ov002_02110064 data_ov002_020ff120 data_ov002_021101cc)
set(OV002_DATA_CPP "${CMAKE_BINARY_DIR}/host-src/ov002_data.cpp")
set(PC_GAME_LINK_HISTORY "${PORT_REPO_ROOT_ABS}/build/pc-game-link-history.log")
# Public, ROM-free symbol requests make a fresh checkout independent of a
# developer's old linker failures. Keep accumulated local history when present.
if(NOT EXISTS "${PC_GAME_LINK_HISTORY}")
    file(MAKE_DIRECTORY "${PORT_REPO_ROOT_ABS}/build")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/build-support/pc-game-link-requests.txt"
                   "${PC_GAME_LINK_HISTORY}" COPYONLY)
endif()
add_custom_command(OUTPUT "${OV002_DATA_CPP}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/overlay_data.py"
            --out "${OV002_DATA_CPP}" --link-log "${PC_GAME_LINK_HISTORY}"
            ${OV002_DATA_SYMBOLS}
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/overlay_data.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/link_data.py"
            "${PORT_REPO_ROOT_ABS}/config/arm9/overlays/ov002/symbols.txt"
            "${PORT_REPO_ROOT_ABS}/extracted/overlays/overlay_0002.bin"
            "${PC_GAME_LINK_HISTORY}"
    COMMENT "ov002 data from the local ROM extraction")

# Block collision material row three points into object overlay 15. Keep its
# exact source bytes local and reconstruct them from the user's ROM as needed.
set(OV015_DATA_CPP "${CMAKE_BINARY_DIR}/host-src/ov015_data.cpp")
add_custom_command(OUTPUT "${OV015_DATA_CPP}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/overlay_data.py"
            --out "${OV015_DATA_CPP}" --overlay 15 data_ov015_021135f4
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/overlay_data.py"
            "${PORT_REPO_ROOT_ABS}/config/arm9/overlays/ov015/symbols.txt"
            "${PORT_REPO_ROOT_ABS}/extracted/overlays/overlay_0015.bin"
    COMMENT "ov015 block collision material from the local ROM extraction")

# Basement BTA pointers and CLPS bytes are rebuilt only from the local ROM.
set(OV012_DATA_CPP "${CMAKE_BINARY_DIR}/host-src/ov012_data.cpp")
add_custom_command(OUTPUT "${OV012_DATA_CPP}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/basement_data.py"
            "${PORT_REPO_ROOT_ABS}" "${OV012_DATA_CPP}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/basement_data.py"
            "${PORT_REPO_ROOT_ABS}/extracted/overlays/overlay_0012.bin")

# The retail title overlay stores course labels as ARM9 pointers. Rebuild that
# table with native pointers into strings copied from the user's local ROM.
set(TITLE_OVERLAY_DATA_CPP "${CMAKE_BINARY_DIR}/host-src/title_overlay_data.cpp")
add_custom_command(OUTPUT "${TITLE_OVERLAY_DATA_CPP}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/title_overlay_data.py"
            "${PORT_REPO_ROOT_ABS}" "${TITLE_OVERLAY_DATA_CPP}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/title_overlay_data.py"
            "${PORT_REPO_ROOT_ABS}/extracted/overlays/overlay_0003.bin"
    COMMENT "Native title course-label pointers from overlay 3")

# ARM9 level and object overlay lookup tables, emitted from the local
# decompressed ROM image. These are table data, not host pointers.
set(LEVEL_OVERLAY_TABLE_CPP "${CMAKE_BINARY_DIR}/host-src/level_overlay_tables.cpp")
add_custom_command(OUTPUT "${LEVEL_OVERLAY_TABLE_CPP}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/level_overlay_tables.py"
            "${PORT_REPO_ROOT_ABS}" "${LEVEL_OVERLAY_TABLE_CPP}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/level_overlay_tables.py"
            "${PORT_REPO_ROOT_ABS}/extracted/arm9_dec.bin"
    COMMENT "Level overlay tables from the local ROM extraction")

# Overlay 8 contains only data and self-relative pointers. Emit its image at a
# host address and apply the relocation table from the supplied ROM, allowing
# native sources to reference the data without mapping DS virtual addresses.
set(OV008_IMAGE_CPP "${CMAKE_BINARY_DIR}/host-src/ov008_image.cpp")
add_custom_command(OUTPUT "${OV008_IMAGE_CPP}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/overlay8_image.py"
            --out "${OV008_IMAGE_CPP}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/overlay8_image.py"
            "${PORT_REPO_ROOT_ABS}/config/arm9/overlays/ov008/symbols.txt"
            "${PORT_REPO_ROOT_ABS}/config/arm9/overlays/ov008/relocs.txt"
            "${PORT_REPO_ROOT_ABS}/config/arm9/overlays/ov008/delinks.txt"
            "${PORT_REPO_ROOT_ABS}/extracted/overlays/overlay_0008.bin"
    COMMENT "Overlay 8 data image and relocations from the local ROM extraction")
add_executable(smoke_overlay8 tests/smoke_overlay8.cpp "${OV008_IMAGE_CPP}")

# Overlay 9 owns the stage-data record selected by the level table.
set(OV009_IMAGE_CPP "${CMAKE_BINARY_DIR}/host-src/ov009_image.cpp")
add_custom_command(OUTPUT "${OV009_IMAGE_CPP}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/overlay9_image.py"
            --out "${OV009_IMAGE_CPP}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/overlay9_image.py"
            "${PORT_REPO_ROOT_ABS}/config/arm9/overlays/ov009/relocs.txt"
            "${PORT_REPO_ROOT_ABS}/config/arm9/overlays/ov009/delinks.txt"
            "${PORT_REPO_ROOT_ABS}/extracted/overlays/overlay_0009.bin"
    COMMENT "Overlay 9 stage-data image and relocations from the local ROM extraction")
add_executable(smoke_overlay9 tests/smoke_overlay9.cpp "${OV009_IMAGE_CPP}")

# Course data is loaded from the mounted ROM into a separate host image for
# each level. Emit relocation metadata only; ARM overlay code is never run.
set(LEVEL_OVERLAY_DATA_CPP "${CMAKE_BINARY_DIR}/host-src/level_overlay_data.cpp")
file(GLOB LEVEL_OVERLAY_METADATA_INPUTS
    "${PORT_REPO_ROOT_ABS}/config/arm9/overlays/ov0*/delinks.txt"
    "${PORT_REPO_ROOT_ABS}/config/arm9/overlays/ov0*/relocs.txt"
    "${PORT_REPO_ROOT_ABS}/extracted/overlays/overlay_00*.bin")
add_custom_command(OUTPUT "${LEVEL_OVERLAY_DATA_CPP}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/level_overlay_data.py"
            --out "${LEVEL_OVERLAY_DATA_CPP}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/level_overlay_data.py"
            "${PORT_REPO_ROOT_ABS}/extracted/arm9_dec.bin" ${LEVEL_OVERLAY_METADATA_INPUTS}
    COMMENT "Course overlay bounds and validated self-references")
add_executable(smoke_level_overlay EXCLUDE_FROM_ALL tests/smoke_level_overlay.cpp
    hal/level_overlay.cpp "${LEVEL_OVERLAY_DATA_CPP}")
target_include_directories(smoke_level_overlay PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(smoke_level_overlay PRIVATE ntr)

# Overlay 1 contains the active shadow-model BMD templates at addresses that
# hold unrelated strings in overlay 0. Emit local bytes and their ROM relocations.
set(OV001_IMAGE_CPP "${CMAKE_BINARY_DIR}/host-src/ov001_image.cpp")
add_custom_command(OUTPUT "${OV001_IMAGE_CPP}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/overlay1_image.py"
            --out "${OV001_IMAGE_CPP}" --symbols
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/overlay1_image.py"
            "${PORT_REPO_ROOT_ABS}/config/arm9/overlays/ov001/symbols.txt"
            "${PORT_REPO_ROOT_ABS}/config/arm9/overlays/ov001/relocs.txt"
            "${PORT_REPO_ROOT_ABS}/config/arm9/overlays/ov001/delinks.txt"
            "${PORT_REPO_ROOT_ABS}/extracted/overlays/overlay_0001.bin"
    COMMENT "overlay 1 image and relocations from the local ROM extraction")

set(GATE6_SYMS _ZN3OAM4LoadEv func_020566dc func_02056674)
sm64ds_hostgen_symbols(GATE6_GEN ${GATE6_SYMS})

add_executable(smoke_model tests/smoke_model.cpp
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp hal/oam_bridge.cpp hal/cstd_div.c
    hal/model_host.cpp hal/frame_gx_bank_bridge.cpp hal/oam_state.cpp hal/gx_upload_bridge.cpp hal/ctor_bridge.cpp
    hal/mem_delete2.cpp
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} ${SLICE4B_SOURCES}
    ${GATE4A_GEN} ${GATE4B_GEN} ${GATE6_GEN} "${ROMDATA_C}")
target_include_directories(smoke_model PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_model PRIVATE ntr)
target_compile_definitions(smoke_model PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_model PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 5: the fiber frame loop -- game-shaped frames, host-rastered.
add_executable(smoke_scene_clock tests/smoke_scene_clock.cpp
    ../src/unnamed/arm9/0201/func_0201a4bc.c ../src/_ZN3IRQ13VBlankHandlerEv.cpp)
target_include_directories(smoke_scene_clock PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_compile_definitions(smoke_scene_clock PRIVATE SM64DS_PLATFORM_PC)
target_link_libraries(smoke_scene_clock PRIVATE ntr)
add_test(NAME smoke_scene_clock COMMAND smoke_scene_clock)

add_executable(smoke_frames tests/smoke_frames.cpp
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    hal/model_host.cpp hal/frame_gx_bank_bridge.cpp hal/oam_state.cpp hal/gx_upload_bridge.cpp hal/ctor_bridge.cpp hal/mem_delete2.cpp
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} ${SLICE4B_SOURCES}
    ${GATE4A_GEN} ${GATE4B_GEN} "${ROMDATA_C}")
target_include_directories(smoke_frames PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_frames PRIVATE ntr)
target_compile_definitions(smoke_frames PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_frames PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 4d: the catalog soak -- every model through the game pipeline.
add_executable(smoke_soak tests/smoke_soak.cpp
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    hal/model_host.cpp hal/frame_gx_bank_bridge.cpp hal/oam_state.cpp hal/gx_upload_bridge.cpp hal/ctor_bridge.cpp hal/mem_delete2.cpp
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} ${SLICE4B_SOURCES}
    ${GATE4A_GEN} ${GATE4B_GEN} "${ROMDATA_C}")
target_include_directories(smoke_soak PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_soak PRIVATE ntr)
target_compile_definitions(smoke_soak PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_soak PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 8: collision -- the octree walk over real KCL data. cstd::sqrt
# drives the hardware sqrt unit through literal MMIO -> hostgen.
set(GATE8_SYMS _ZN4cstd4sqrtEy _ZN4cstd10fdiv_asyncE5Fix12IiE5Fix12IiE
    _ZN4cstd4fdivEii _ZN4cstd11fdiv_resultEv _ZN4cstd16reciprocal_asyncE5Fix12IiE
    Matrix4x3_ApplyInPlaceToScale)
sm64ds_hostgen_symbols(GATE8_GEN ${GATE8_SYMS})

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate8.txt" SLICE8_LINES)
set(SLICE8_SOURCES "")
foreach(line IN LISTS SLICE8_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    # This hostgen-adopted copy predates the full ITCM decomp TU, which now
    # provides the same DetectClsn(dBgCh_Lin&) method.
    if(line STREQUAL "port/unmatched/MeshCollider_DetectClsn_RaycastLine.cpp")
        continue()
    endif()
    list(APPEND SLICE8_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()
sm64ds_honor_cpp_markers(SLICE8_SOURCES)
list(APPEND SLICE8_SOURCES
    "${PORT_REPO_ROOT_ABS}/src/_ZN5dBgChC2Ev.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN5dBgChD1Ev.cpp"
    "${PORT_REPO_ROOT_ABS}/src/engine/collision/dBgPi.cpp"
    "${PORT_REPO_ROOT_ABS}/src/engine/collision/dBgCh_SphCrr.cpp"
    "${PORT_REPO_ROOT_ABS}/src/engine/collision/dBgCh_SphCrr_query.cpp"
    "${PORT_REPO_ROOT_ABS}/src/engine/collision/dM3dGSph.cpp"
    "${PORT_REPO_ROOT_ABS}/src/actors/dM3dGLin.cpp"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0203/func_02035394.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0203/func_020353b0.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0203/func_02035354.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0203/func_02037e58.c"
    "${PORT_REPO_ROOT_ABS}/src/DotVec3.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020fea4c.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020fea68.c")

add_executable(smoke_clsn tests/smoke_clsn.cpp
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    hal/model_host.cpp hal/frame_gx_bank_bridge.cpp hal/oam_state.cpp hal/gx_upload_bridge.cpp hal/ctor_bridge.cpp hal/cstd_div.c
    hal/clsn_vtable.cpp hal/bg_ctor_bridge.cpp hal/mem_delete2.cpp
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} ${SLICE4B_SOURCES}
    ${SLICE8_SOURCES} ${GATE4A_GEN} ${GATE4B_GEN} ${GATE8_GEN} "${ROMDATA_C}")
target_include_directories(smoke_clsn PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_clsn PRIVATE ntr)
target_compile_definitions(smoke_clsn PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_clsn PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 7: ModelAnim -- the game owns frame progression.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate7.txt" SLICE7_LINES)
set(SLICE7_SOURCES "")
foreach(line IN LISTS SLICE7_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE7_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()
sm64ds_honor_cpp_markers(SLICE7_SOURCES)
# C1/C2 are separate cartridge symbols, but MSVC emits one host constructor
# symbol for both. Keep the canonical C1 TU in PC targets.
list(FILTER SLICE7_SOURCES EXCLUDE REGEX "/src/_ZN9ModelAnimC2Ev\\.cpp$")

add_executable(smoke_modelanim tests/smoke_modelanim.cpp
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    hal/model_host.cpp hal/frame_gx_bank_bridge.cpp hal/oam_state.cpp hal/gx_upload_bridge.cpp hal/ctor_bridge.cpp hal/mem_delete2.cpp hal/anim_bridge.cpp hal/anim_ctor_bridge.cpp
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} ${SLICE4B_SOURCES}
    ${SLICE7_SOURCES} ${GATE4A_GEN} ${GATE4B_GEN} "${ROMDATA_C}")
target_include_directories(smoke_modelanim PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_modelanim PRIVATE ntr)
target_compile_definitions(smoke_modelanim PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_modelanim PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 6: the 2D sprite engine's software side. OAM::Load reads POWCNT
# textually -> hostgen; the rest of the slice is plain.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate6.txt" SLICE6_LINES)
set(SLICE6_SOURCES "")
foreach(line IN LISTS SLICE6_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    # These are promoted translation units: hostgen emits OAM.cpp and the two
    # OAM upload helpers, so do not also add their original sources here.
    if(line MATCHES "src/engine/oam/OAM\\.cpp$" OR
       line MATCHES "src/func_020566dc\\.c$" OR
       line MATCHES "src/func_02056674\\.c$")
        continue()
    endif()
    list(APPEND SLICE6_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()
sm64ds_honor_cpp_markers(SLICE6_SOURCES)

add_executable(smoke_oam tests/smoke_oam.cpp
    hal/oam_state.cpp hal/oam_bridge.cpp hal/oam_cpp_aliases.cpp hal/cstd_div.c
    "${PORT_REPO_ROOT_ABS}/src/DMASyncWordTransfer.c"
    ${SLICE6_SOURCES} ${GATE6_GEN} "${ROMDATA_C}")
target_include_directories(smoke_oam PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_oam PRIVATE ntr)
target_compile_definitions(smoke_oam PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_oam PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 5b: the animation soak -- every compatible model+BCA pair.
add_executable(smoke_soak_anim tests/smoke_soak_anim.cpp
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    hal/model_host.cpp hal/frame_gx_bank_bridge.cpp hal/oam_state.cpp hal/gx_upload_bridge.cpp hal/ctor_bridge.cpp hal/mem_delete2.cpp
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} ${SLICE4B_SOURCES}
    ${GATE4A_GEN} ${GATE4B_GEN} "${ROMDATA_C}")
target_include_directories(smoke_soak_anim PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_soak_anim PRIVATE ntr)
target_compile_definitions(smoke_soak_anim PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_soak_anim PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 4c: animation -- same slice (the 4c files live in slice_gate4b.txt),
# different driver: the piano posed by its BCA through the game's walk.
add_executable(smoke_anim tests/smoke_anim.cpp
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    hal/model_host.cpp hal/frame_gx_bank_bridge.cpp hal/oam_state.cpp hal/gx_upload_bridge.cpp hal/ctor_bridge.cpp hal/mem_delete2.cpp
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} ${SLICE4B_SOURCES}
    ${GATE4A_GEN} ${GATE4B_GEN} "${ROMDATA_C}")
target_include_directories(smoke_anim PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_anim PRIVATE ntr)
target_compile_definitions(smoke_anim PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_anim PRIVATE /wd4311 /wd4312 /wd4068)
endif()

add_executable(smoke_fs tests/smoke_fs.cpp
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    hal/mem_delete2.cpp
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES})
target_include_directories(smoke_fs PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_fs PRIVATE ntr)
target_compile_definitions(smoke_fs PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_fs PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 9: the actor framework -- ArrowSignRight's full lifecycle.
# One .c file carries C++ syntax (mwccarm accepted it); hostgen's
# emit-as-cpp with extern "C" wrap makes it legal.
set(GATE9_SYMS _ZN7fBase_c21AfterCleanupResourcesEj)
sm64ds_hostgen_symbols(GATE9_GEN ${GATE9_SYMS})

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate9.txt" SLICE9_LINES)
set(SLICE9_SOURCES "")
foreach(line IN LISTS SLICE9_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE9_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()
sm64ds_honor_cpp_markers(SLICE9_SOURCES)
# GATE9_GEN emits the complete ActorBase.cpp translation unit from the source
# containing the host-sensitive AfterCleanupResources body; don't compile the
# untransformed source a second time.
list(FILTER SLICE9_SOURCES EXCLUDE REGEX "/src/actors/ActorBase\\.cpp$")
list(APPEND SLICE9_SOURCES
    "${PORT_REPO_ROOT_ABS}/src/engine/collision/dBgW_KcMbg.cpp"
    "${PORT_REPO_ROOT_ABS}/src/engine/collision/dClipper.cpp"
    "${PORT_REPO_ROOT_ABS}/src/CrossVec3.c"
    "${PORT_REPO_ROOT_ABS}/src/_ZN10ModelAnim213Func_020162C4Eji5Fix12IiEt.c"
    "${PORT_REPO_ROOT_ABS}/src/engine/animation/dExtFrameCtrl_c.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9fLiNdBa_cD1Ev.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_.c"
    "${PORT_REPO_ROOT_ABS}/src/_ZN8Particle7Manager9AddSystemEiR7Vector3.cpp"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0204/func_0204d958.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0204/func_0204ae2c.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0204/func_0204d9a0.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/unmatched/fixed_atan2.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/unmatched/player_methods.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/unmatched/spawn_number_attach.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/unmatched/camera_jitter.cpp"
    "${PORT_REPO_ROOT_ABS}/src/DeathTable_GetBit.c"
    "${PORT_REPO_ROOT_ABS}/src/DeathTable_ClearBit.c"
    "${PORT_REPO_ROOT_ABS}/src/DeathTable_SetBit.c"
    "${PORT_REPO_ROOT_ABS}/src/DecIfAbove0_Byte.c"
    "${PORT_REPO_ROOT_ABS}/src/GiveCoins.c"
    "${PORT_REPO_ROOT_ABS}/src/IsAreaShowing.c"
    "${PORT_REPO_ROOT_ABS}/src/Matrix4x3_ApplyInPlaceToTranslation.c"
    "${PORT_REPO_ROOT_ABS}/src/Matrix4x3_ApplyInPlaceToRotationZXYExt.c"
    "${PORT_REPO_ROOT_ABS}/src/Matrix4x3_ApplyInPlaceToRotationXYZExt.c"
    "${PORT_REPO_ROOT_ABS}/src/Matrix4x3_FromTranslation.c"
    "${PORT_REPO_ROOT_ABS}/src/RandomIntInternal.c"
    "${PORT_REPO_ROOT_ABS}/src/Vec3_Lsl.c"
    "${PORT_REPO_ROOT_ABS}/src/Vec3_HorzLen.c"
    "${PORT_REPO_ROOT_ABS}/src/Vec3_HorzAngle.c"
    "${PORT_REPO_ROOT_ABS}/src/Vec3_Asr.c"
    "${PORT_REPO_ROOT_ABS}/src/Vec3_Add.c"
    "${PORT_REPO_ROOT_ABS}/src/AddVec3.c"
    "${PORT_REPO_ROOT_ABS}/src/_ZN6Player7TryGrabER8dActor_c.cpp"
    "${PORT_REPO_ROOT_ABS}/src/SetStarMarker.c"
    "${PORT_REPO_ROOT_ABS}/src/SublevelToLevel.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0204/func_02043f98.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0204/func_02043f4c.c"
    "${PORT_REPO_ROOT_ABS}/src/GetLevelPart.c"
    "${PORT_REPO_ROOT_ABS}/src/Matrix4x3_FromRotationX.c"
    "${PORT_REPO_ROOT_ABS}/src/Matrix4x3_FromRotationZ.c"
    "${PORT_REPO_ROOT_ABS}/src/_ZN6Player7IsStateERNS_5StateE.cpp"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020dacb4.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020e496c.c"
    "${PORT_REPO_ROOT_ABS}/src/IsStarCollectedInCurLevel.c"
    "${PORT_REPO_ROOT_ABS}/src/IsStarCollectedInLevel.c"
    "${PORT_REPO_ROOT_ABS}/src/IsStarCollected.c"
    "${PORT_REPO_ROOT_ABS}/src/engine/collision/dBgCh_Gnd.cpp"
    "${PORT_REPO_ROOT_ABS}/src/engine/collision/dBgCh_Actr.cpp"
    "${PORT_REPO_ROOT_ABS}/src/engine/heap/Heap.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9SolidHeapC1EPvjP4HeapP18SolidHeapAllocator.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN4Heap15CreateSolidHeapEjPS_i.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN4Heap10ReallocateEPvj.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9SolidHeap10VGetNodeIDEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9SolidHeap10VSetNodeIDEj.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9SolidHeap11VDeallocateEPv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9SolidHeap11VMemoryLeftEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9SolidHeap11VReallocateEPvj.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9SolidHeap12VResizeToFitEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9SolidHeap14VDeallocateAllEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9SolidHeap19VMaxAllocatableSizeEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9SolidHeap22VMaxAllocationUnitSizeEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9SolidHeap7VIntactEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9SolidHeap7VRescueEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9SolidHeap7VSizeofEPv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9SolidHeap8VDestroyEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9SolidHeap9VAllocateEji.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9SolidHeapD0Ev.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9SolidHeapD1Ev.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN4Heap21MaxAllocationUnitSizeEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN4Heap6SizeofEPv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN4Heap6RescueEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN4Heap11ResizeToFitEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN11fLiNdBaPr_cD1Ev.cpp"
    "${PORT_REPO_ROOT_ABS}/src/engine/model/dExtShadowModel_c.cpp"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0201/func_02016e70.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0203/func_0203c178.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0204/func_02046120.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0204/func_02046088.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020d8838.c"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9dBgCh_Gnd10DetectClsnEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9dBgCh_Lin10DetectClsnEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN5dBgCh19StartDetectingWaterEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN5dBgCh18StopDetectingWaterEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/Vec3_HorzDist.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0203/func_02037dc4.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0203/func_020353e0.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0203/func_020353f4.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0203/func_02035414.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0203/func_02035428.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0203/func_0203553c.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0203/func_02037464.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0203/func_02038234.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0203/func_02038298.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0203/func_02038324.cpp"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0203/func_0203842c.cpp"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0203/func_0203859c.cpp")
sm64ds_honor_cpp_markers(SLICE9_SOURCES)

add_library(smoke_actor_game STATIC
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp hal/mem_delete2.cpp
    hal/model_host.cpp hal/frame_gx_bank_bridge.cpp hal/oam_state.cpp hal/gx_upload_bridge.cpp hal/ctor_bridge.cpp hal/cstd_div.c
    hal/clsn_vtable.cpp hal/bg_ctor_bridge.cpp hal/actor_vtables.cpp hal/actor_ctor_bridge.cpp hal/anim_bridge.cpp hal/anim_ctor_bridge.cpp
    hal/cxxname_bridge.cpp hal/input_state.asm hal/megachar_stub.cpp
    hal/player_heal.cpp hal/player_coin_reward.cpp hal/player_mega_kill.cpp
    hal/one_up_logo_attach.cpp hal/star_add_marker.cpp
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} ${SLICE4B_SOURCES}
    ${SLICE7_SOURCES} ${SLICE8_SOURCES} ${SLICE9_SOURCES}
    ${GATE4A_GEN} ${GATE4B_GEN} ${GATE8_GEN} ${GATE9_GEN}
    "${PORT_REPO_ROOT_ABS}/src/actors/ActorDerived.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN6Player11ChangeStateERNS_5StateE.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN6Player4HurtERK7Vector3j5Fix12IiEjjj.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/unmatched/player_ov002_methods.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN15dExtFrameCtrl_c8LoadFileER13SharedFilePtr.cpp"
    "${PORT_REPO_ROOT_ABS}/src/AngleDiff.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020d4540.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020d5cec.cpp"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020d82f0.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020d91b8.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020d91e0.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020da9d4.cpp"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020db54c.cpp"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020e6780.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0204/func_02046bbc.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0204/func_02046d50.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0204/func_02047144.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0204/func_02046590.c"
    "${PORT_REPO_ROOT_ABS}/src/func_020471ac.c"
    hal/smoke_sound_bridge.cpp
    hal/actor_bss.cpp
    hal/actor_runtime_bridge.cpp
    hal/actor_smoke_link_fixtures.cpp
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0206/func_0206e330.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0204/func_02042ffc.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0204/func_02042fe4.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0204/func_02043060.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0204/func_02043180.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0204/func_020433b8.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0204/func_0204335c.c"
    "${PORT_REPO_ROOT_ABS}/src/engine/heap/SolidHeapAllocator.cpp"
    "${PORT_REPO_ROOT_ABS}/src/GiveHealth.c"
    "${PORT_REPO_ROOT_ABS}/src/GiveLives.c"
    "${PORT_REPO_ROOT_ABS}/src/Event.cpp"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0202/FUN_0202a130.c"
    "${ROMDATA_C}" "${OV002_DATA_CPP}" "${OV001_IMAGE_CPP}")
add_executable(smoke_actor tests/smoke_actor.cpp)
target_include_directories(smoke_actor_game PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_actor_game PUBLIC ntr)
target_include_directories(smoke_actor PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_actor PRIVATE smoke_actor_game)
target_compile_definitions(smoke_actor_game PRIVATE SM64DS_PLATFORM_PC
    SMOKE_ACTOR_LINK_FIXTURES
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
target_compile_definitions(smoke_actor PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_actor_game PRIVATE /wd4311 /wd4312 /wd4068 /Gy)
    target_compile_options(smoke_actor PRIVATE /wd4311 /wd4312 /wd4068 /Gy)
    target_link_options(smoke_actor PRIVATE /OPT:REF)
endif()

# The native executable follows the cartridge's ARM9 startup and scene loop.
# Discover its statically reachable translation units and prepare host-side
# MMIO rewrites at configure time. It is opt-in until its full ROM data/overlay
# link closure is complete; the smoke targets remain the default build.
find_package(Python3 COMPONENTS Interpreter REQUIRED)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/boot_closure.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/buddy_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/tree_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/warp_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/chomp_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/lava_bridge_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/seesaw_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/block_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/item_block_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/floor_switch_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/battlefield_platform_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/koopa_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/particle_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/basement_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/cannon_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/rabbit_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/texture_consumer_hooks.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/native_save_hooks.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/dsmt_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/dialogue_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/sound_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/bird_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/toad_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/star_gate_host.py"
      "${CMAKE_CURRENT_SOURCE_DIR}/tools/lakitu_host.py"
      "${CMAKE_CURRENT_SOURCE_DIR}/tools/camera_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/level_overlay_tables.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/minigame_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/intro_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/goomba_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/reward_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/bombking_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/picture_gate_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/star_select_host.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/minigame_data.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt"
    "${PORT_REPO_ROOT_ABS}/build/pc-game-link-history.log")
file(GLOB_RECURSE PC_BOOT_SOURCE_INPUTS CONFIGURE_DEPENDS
    "${PORT_REPO_ROOT_ABS}/src/*.c"
    "${PORT_REPO_ROOT_ABS}/src/*.cpp")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${PC_BOOT_SOURCE_INPUTS})
set(BOOT_CLOSURE_CMAKE "${CMAKE_BINARY_DIR}/pc-boot-sources.cmake")
set(PC_BOOT_SOURCE_CHANGED FALSE)
foreach(source IN LISTS PC_BOOT_SOURCE_INPUTS)
    if("${source}" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}")
        set(PC_BOOT_SOURCE_CHANGED TRUE)
        break()
    endif()
endforeach()
if(NOT EXISTS "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_LIST_FILE}" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR NOT EXISTS "${PORT_REPO_ROOT_ABS}/build/pc-game-link-history.log"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/boot_closure.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/buddy_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/tree_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/warp_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/chomp_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/lava_bridge_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/seesaw_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/block_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/item_block_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/floor_switch_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/battlefield_platform_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/koopa_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/particle_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/basement_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/cannon_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/rabbit_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/texture_consumer_hooks.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/native_save_hooks.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/dsmt_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/dialogue_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/sound_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/bird_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/toad_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/star_gate_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
    OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/lakitu_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
    OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/camera_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/minigame_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/intro_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/goomba_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/reward_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/bombking_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/picture_gate_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/star_select_host.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${CMAKE_CURRENT_SOURCE_DIR}/tools/minigame_data.py" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR "${PORT_REPO_ROOT_ABS}/build/pc-game-link-history.log" IS_NEWER_THAN "${BOOT_CLOSURE_CMAKE}"
   OR PC_BOOT_SOURCE_CHANGED)
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/boot_closure.py"
                --decomp "${PORT_REPO_ROOT_ABS}"
                --json "${CMAKE_BINARY_DIR}/pc-boot-audit.json"
                --cmake "${BOOT_CLOSURE_CMAKE}"
                --generated-dir "${CMAKE_BINARY_DIR}/pc-boot-src"
                --host-overrides
                    func_02059650 func_02058d58 Crash func_01ffde98
                    func_02054168 func_0205417c DMAStartTransferFB DMAStartTransfer
                    func_02058cd0 func_02058ea0 func_02058eb4 func_02059040
                    func_02018a24
                    _ZN8SaveData14SaveDataToCartEPcjj _ZN8SaveData16ReadDataFromCartEPcjj
                    func_0201a458
                    func_02059eb0
                    _ZN3IRQ7DisableEv _ZN3IRQ6EnableEv _ZN3IRQ7RestoreEj
                    _ZN3IRQ10DisableAllEv _ZN3IRQ10RestoreAllEj
                    _ZN4CP1510EnableDTCMEv _ZN4CP1514MPUDataRegion1Ej
                    _ZN4CP1514MPUDataRegion7Ej _ZN4CP1517MPUGetDataRegion7Ev
                    _ZN4CP1516WaitForInterruptEv
                    ARMProcessorMode ARMSaveContext ARMRestoreContext
                    func_02058568 func_020593f4
                    func_02059d8c func_0205a74c
                --link-log "${PORT_REPO_ROOT_ABS}/build/pc-game-link-history.log"
        RESULT_VARIABLE BOOT_AUDIT_RESULT)
    if(NOT BOOT_AUDIT_RESULT EQUAL 0)
        message(FATAL_ERROR "PC boot source audit failed: ${BOOT_AUDIT_RESULT}")
    endif()
endif()
include("${BOOT_CLOSURE_CMAKE}")
add_executable(smoke_distance tests/smoke_distance.cpp
    "${CMAKE_BINARY_DIR}/pc-boot-src/src/unnamed/arm9/0205/func_02053274.cpp")
target_link_libraries(smoke_distance PRIVATE ntr)
target_include_directories(smoke_distance PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/ntr/include")
add_test(NAME smoke_distance COMMAND smoke_distance)
list(FILTER BOOT_CLOSURE_SOURCES EXCLUDE REGEX "/src/main\\.c$")
list(FILTER BOOT_CLOSURE_SOURCES EXCLUDE REGEX "/src/_ZN5dBgChD2Ev\\.cpp$")
list(FILTER BOOT_CLOSURE_SOURCES EXCLUDE REGEX "/src/_ZN15dExtFrameCtrl_cC2Ev\\.cpp$")
list(FILTER BOOT_CLOSURE_SOURCES EXCLUDE REGEX "/src/_ZN9ModelAnimC2Ev\\.cpp$")
list(FILTER BOOT_CLOSURE_SOURCES EXCLUDE REGEX "/src/_ZN4HeapD2Ev\\.cpp$")
list(FILTER BOOT_CLOSURE_SOURCES EXCLUDE REGEX "/src/_ZThn80_N9ModelAnimD[01]Ev\\.cpp$")
list(FILTER BOOT_CLOSURE_SOURCES EXCLUDE REGEX "/src/_ZThn80_N10ModelAnim2D[01]Ev\\.cpp$")
list(FILTER BOOT_CLOSURE_SOURCES EXCLUDE REGEX "/src/_ZN15dExtFrameCtrl_cD2Ev\\.cpp$")
list(FILTER BOOT_CLOSURE_SOURCES EXCLUDE REGEX "/src/_ZN5dPa_c7level_c16simpleCallback_cC2Ev\\.cpp$")
list(FILTER BOOT_CLOSURE_SOURCES EXCLUDE REGEX "/src/_ZN8dActor_cC2Ev\\.cpp$")
list(FILTER BOOT_CLOSURE_SOURCES EXCLUDE REGEX "/src/actors/dScTitle_c\\.cpp$")
list(FILTER BOOT_CLOSURE_SOURCES EXCLUDE REGEX "/src/_ZN8SaveData(14SaveDataToCart|16ReadDataFromCart)EPcjj\\.cpp$")
sm64ds_honor_cpp_markers(BOOT_CLOSURE_SOURCES)

file(GLOB_RECURSE PC_HEADER_INPUTS CONFIGURE_DEPENDS
    "${PORT_REPO_ROOT_ABS}/include/*.h")
set(PC_HEADER_STAMP "${CMAKE_BINARY_DIR}/pc-host-include.stamp")
add_custom_command(OUTPUT "${PC_HEADER_STAMP}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/host_headers.py"
            "${PORT_REPO_ROOT_ABS}" "${CMAKE_BINARY_DIR}/pc-host-include"
            "${PC_HEADER_STAMP}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/host_headers.py" ${PC_HEADER_INPUTS}
    COMMENT "Prepare host-only header ABI adaptations")
add_custom_target(pc_host_headers DEPENDS "${PC_HEADER_STAMP}")

# Resolve ROM-backed globals from the local extracted image, without checking
# ROM bytes into source. The linker log is an input so each subsequent build
# learns only the data symbols the previous link proved it needs.
set(BOOT_LINK_DATA_CPP "${CMAKE_BINARY_DIR}/host-src/pc_link_data.cpp")
if(NOT EXISTS "${CMAKE_BINARY_DIR}/host-src/pc_link_data.json")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/host-src")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/build-support/pc_link_data.json"
                   "${CMAKE_BINARY_DIR}/host-src/pc_link_data.json" COPYONLY)
endif()
set(BOOT_LINK_LOG "${PORT_REPO_ROOT_ABS}/build/pc-game-link-history.log")
add_custom_command(OUTPUT "${BOOT_LINK_DATA_CPP}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/link_data.py"
            "${PORT_REPO_ROOT_ABS}" "${BOOT_LINK_LOG}" "${BOOT_LINK_DATA_CPP}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/link_data.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/minigame_host.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/minigame_data.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/dsmt_host.py"
              "${CMAKE_CURRENT_SOURCE_DIR}/tools/sound_host.py"
            "${PORT_REPO_ROOT_ABS}/config/arm9/symbols.txt"
            "${PORT_REPO_ROOT_ABS}/extracted/arm9_dec.bin"
            "${BOOT_LINK_LOG}"
    COMMENT "Host-link ROM data globals from the local image")

# Host-link the recovered particle renderer slice through hostgen so literal
# Nintendo DS register accesses route through the PC GX backend.
set(PARTICLE_RENDER_SYMS
    func_02049ee8 func_0204a5c8 func_0204af3c func_0204af38
    func_0204b244 func_0204be40 func_0204b81c func_0204b028
    func_0204bbd8 func_0204b460 func_0204c24c)
sm64ds_hostgen_symbols(PARTICLE_RENDER_GEN ${PARTICLE_RENDER_SYMS})

# Player Render is an explicit game source outside the closure. Apply the
set(DSMT_DATA_ASM "${CMAKE_BINARY_DIR}/host-src/dsmt_data.asm")
set(MINIGAME_MENU_DATA_ASM "${CMAKE_BINARY_DIR}/host-src/minigame_menu_data.asm")
add_custom_command(OUTPUT "${MINIGAME_MENU_DATA_ASM}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/minigame_menu_data.py"
            "${PORT_REPO_ROOT_ABS}" "${MINIGAME_MENU_DATA_ASM}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/minigame_menu_data.py"
            "${PORT_REPO_ROOT_ABS}/config/arm9/overlays/ov005/symbols.txt"
            "${PORT_REPO_ROOT_ABS}/config/arm9/overlays/ov005/relocs.txt"
            "${PORT_REPO_ROOT_ABS}/extracted/overlays/overlay_0005.bin")
set(PC_BOOT_DSMT_GEN "${CMAKE_BINARY_DIR}/host-src/pc_boot_dsmt.cpp")
add_custom_command(OUTPUT "${PC_BOOT_DSMT_GEN}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/dsmt_host.py"
      "${PORT_REPO_ROOT_ABS}" "${PC_BOOT_DSMT_GEN}" --boot-source "${PORT_REPO_ROOT_ABS}/src/actors/dScBoot_c.cpp"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/dsmt_host.py" "${PORT_REPO_ROOT_ABS}/src/actors/dScBoot_c.cpp")
add_custom_command(OUTPUT "${DSMT_DATA_ASM}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/dsmt_host.py" "${PORT_REPO_ROOT_ABS}" "${DSMT_DATA_ASM}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/dsmt_host.py"
      "${PORT_REPO_ROOT_ABS}/config/arm9/overlays/ov007/symbols.txt"
      "${PORT_REPO_ROOT_ABS}/config/arm9/overlays/ov007/relocs.txt"
      "${PORT_REPO_ROOT_ABS}/config/arm9/symbols.txt"
      "${PORT_REPO_ROOT_ABS}/extracted/overlays/overlay_0007.bin")

# Player Render is an explicit game source outside the closure. Apply the
# same consumer hook through a generated PC copy, preserving retail bytes.
set(PLAYER_RENDER_GEN "${CMAKE_BINARY_DIR}/host-src/player_render.cpp")
add_custom_command(OUTPUT "${PLAYER_RENDER_GEN}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/texture_consumer_hooks.py"
            "${PORT_REPO_ROOT_ABS}/src/_ZN6Player6RenderEv.cpp" "${PLAYER_RENDER_GEN}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/texture_consumer_hooks.py"
            "${PORT_REPO_ROOT_ABS}/src/_ZN6Player6RenderEv.cpp"
    COMMENT "Generate native Player draw ownership scope")

set(PC_TITLE_GEN "${CMAKE_BINARY_DIR}/host-src/pc_title_session.cpp")
add_custom_command(OUTPUT "${PC_TITLE_GEN}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/pc_title_session.py"
            "${PORT_REPO_ROOT_ABS}/src/actors/dScTitle_c.cpp" "${PC_TITLE_GEN}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/pc_title_session.py"
            "${PORT_REPO_ROOT_ABS}/src/actors/dScTitle_c.cpp"
    COMMENT "Generate PC course-picker save/character handoff")

# These recovered sources are already supplied explicitly by the game target.
# The original-menu closure reaches them too; keep one host implementation.
foreach(symbol IN LISTS PARTICLE_RENDER_SYMS)
    list(FILTER BOOT_CLOSURE_SOURCES EXCLUDE REGEX "/${symbol}\\.(c|cpp)$")
endforeach()
list(FILTER BOOT_CLOSURE_SOURCES EXCLUDE REGEX "/_ZN2GX(11LoadBG0Char|7LoadOBJ)EPKvjj\\.cpp$")
add_executable(smoke_dsmt_storage tests/smoke_dsmt_storage.cpp "${DSMT_DATA_ASM}"
    "${PORT_REPO_ROOT_ABS}/src/_ZN8SaveData13GetCoinRecordEj.cpp")
target_include_directories(smoke_dsmt_storage PRIVATE "${PORT_REPO_ROOT_ABS}/include")
target_compile_definitions(smoke_dsmt_storage PRIVATE SM64DS_PLATFORM_PC)
add_test(NAME smoke_dsmt_storage COMMAND smoke_dsmt_storage)
add_executable(sm64ds_pc EXCLUDE_FROM_ALL
    "${MINIGAME_MENU_DATA_ASM}"
    "${DSMT_DATA_ASM}" hal/dsmt_scene.cpp hal/dsmt_input.cpp hal/dsmt_primitives.cpp
    hal/native_save.cpp
    hal/bombking_storage.cpp
    main.cpp
    ${BOOT_CLOSURE_SOURCES}
    # The main-loop's first scene is profile 0 (BOOT); keep its recovered
    # class methods and GX bank setup in the native spawn closure.
    "${PC_BOOT_DSMT_GEN}"
    # Offline PC startup transitions from BOOT to the retail TITLE profile.
    "${PC_TITLE_GEN}"
    # The retail STAGE scene is profile 4 and is requested by TITLE confirm.
    "${PORT_REPO_ROOT_ABS}/src/d_s_stage.cpp"
    # The retail PLAYER actor is profile 191 and is spawned by level entrances.
    # Production cleanup uses the recovered destructor, including its owned
    # model-file release. The vtable-only smoke bridge is not a game destructor.
    "${PORT_REPO_ROOT_ABS}/src/_ZN9ModelBaseD1Ev.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN8dActor_cD1Ev.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN9fLiNdBa_cD1Ev.cpp"
    "${PORT_REPO_ROOT_ABS}/src/d_a_ply.cpp"
    # SOUND_OBJ's native actor factory is installed at profile ID359. Keep its
    # recovered callbacks in the production game target; the generic closure
    # does not discover C-linkage factories from profile tables automatically.
    "${PORT_REPO_ROOT_ABS}/src/game/actors/d_a_sound_obj.cpp"
    # Castle moat and grounds cannon water use mixed actor overlay 9 data.
    "${PORT_REPO_ROOT_ABS}/src/game/actors/d_a_obj_mc_water.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN6PlayerC1Ev.cpp"
    # Emit the native Player vftable used by PlayerC1's PC-side vtable alias.
    "${PORT_REPO_ROOT_ABS}/src/_ZN6PlayerD1Ev.cpp"
    # The vftable refers to these six virtual Player overrides.
    "${PORT_REPO_ROOT_ABS}/src/_ZN6Player13InitResourcesEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN6Player16CleanupResourcesEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN6Player8BehaviorEv.cpp"
    "${PLAYER_RENDER_GEN}"
    "${PORT_REPO_ROOT_ABS}/src/_ZN6Player16OnPendingDestroyEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN6Player13OnYoshiTryEatEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020d869c.cpp"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020d8158.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020db704.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020e5948.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020e4bb8.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020e3e00.cpp"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/__sinit_ov002_021019d0.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/__sinit_ov002_02100560.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020e6ad4.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0201/func_0201973c.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0201/func_02013a00.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020d6334.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020d8118.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020d8360.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020d85fc.cpp"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020d9298.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN10ModelAnim2C1Ev.cpp"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020e4990.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020e4768.cpp"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020e6bb0.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020d853c.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/func_ov002_020e640c.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0201/func_0201628c.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0201/func_020162a4.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0204/func_02045e2c.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov003/func_ov003_020adcbc.cpp"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/ov003/func_ov003_020adc74.cpp"
    "${PORT_REPO_ROOT_ABS}/src/SetNumPlayers.c"
    "${PORT_REPO_ROOT_ABS}/src/SetPlayerGlobals.c"
    "${PORT_REPO_ROOT_ABS}/src/InitControllerMode.c"
    "${PORT_REPO_ROOT_ABS}/src/UnloadArchives.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0203/func_020308d0.c"
    "${PORT_REPO_ROOT_ABS}/src/_ZN2GX11LoadBG0CharEPKvjj.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN2GX7LoadOBJEPKvjj.cpp"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0201/func_0201a244.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0201/func_0201a1bc.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0201/func_0201cd08.cpp"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0202/func_020233f4.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0205/func_02057f38.c"
    # Player SharedFilePtr constructors resolve ordinary asset handles through
    # the same PC catalog-backed pathname bridge used by the game filesystem.
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0205/func_0205d644.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0201/func_02018e3c.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0201/func_020189f0.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0201/func_02018a40.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0201/func_02018a58.cpp"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0201/func_02017e48.c"
    "${PORT_REPO_ROOT_ABS}/src/func_02017e0c.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0205/func_0205d94c.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0205/func_0205c91c.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0205/func_0205c9b8.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0205/func_0205c7e4.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0205/func_0205d28c.c"
    # Native callbacks for the ROM function-pointer table at 0x02086758.
    # The retail table contains ARM9 addresses; dispatch these indices to
    # recovered host functions instead of executing addresses from the ROM.
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0205/func_0205c448.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0205/func_0205c410.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0205/func_0205c378.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0205/func_0205c264.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0205/func_0205c048.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0205/func_0205bc88.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0205/func_0205bc0c.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0205/func_0205bbe4.c"
    "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0205/func_0205bbdc.c"
    "${PORT_REPO_ROOT_ABS}/src/func_0201a2f8.c"
    # Several core heap entry points are recovered but not reachable through
    # this startup closure's symbol graph. Include their real method bodies so
    # actors and ROM-backed allocators can call the host heap faithfully.
    "${PORT_REPO_ROOT_ABS}/src/_ZN4Heap6IntactEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN4Heap6RescueEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN4Heap21MaxAllocationUnitSizeEv.cpp"
    "${PORT_REPO_ROOT_ABS}/src/_ZN8Particle9RenderAllEv.cpp"
    ${PARTICLE_RENDER_GEN}
    "${BOOT_LINK_DATA_CPP}"
    hal/cstd_div.c hal/os_time.cpp hal/shims.cpp hal/host_primitives.cpp hal/mem_delete2.cpp
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    hal/model_host.cpp hal/oam_state.cpp hal/gx_upload_bridge.cpp hal/ctor_bridge.cpp
    hal/clsn_vtable.cpp hal/bg_ctor_bridge.cpp hal/actor_vtables.cpp hal/oam_bridge.cpp
    hal/oam_namespace_bridge.cpp
    hal/actor_ctor_bridge.cpp hal/player_factory_bridge.cpp hal/actor_profile_link_bridge.cpp hal/anim_bridge.cpp hal/anim_ctor_bridge.cpp
    hal/cxxname_bridge.cpp hal/game_api_bridge.cpp hal/level_overlay.cpp hal/itcm_math.cpp hal/megachar_stub.cpp
    hal/demo_model_bridge.cpp hal/player_state_callbacks.cpp hal/float_bits.cpp
    hal/pc_session.cpp hal/pc_pause_settings.cpp hal/pc_pause_settings_state.cpp hal/touch_menu_navigation.cpp hal/course_clear_input.cpp hal/stage_options_input.cpp
    hal/dialogue_overrides.cpp hal/dialogue_runtime.cpp
    hal/wanted_pause_settings.cpp hal/wanted_settings_surface.cpp hal/distant_actor_shadows.cpp
    hal/particle_render_callbacks.cpp
    hal/actor_bss.cpp hal/actor_runtime_bridge.cpp hal/minimap_hud_art.cpp hal/stage_graph_state.cpp hal/gx_bank_state.asm hal/input_state.asm hal/touch_state.asm
    "${ROMDATA_C}" "${OV002_DATA_CPP}" "${OV015_DATA_CPP}" "${OV012_DATA_CPP}" "${OV001_IMAGE_CPP}"
    "${LEVEL_OVERLAY_TABLE_CPP}" "${OV002_DATA_CPP}" "${TITLE_OVERLAY_DATA_CPP}"
    "${LEVEL_OVERLAY_DATA_CPP}" "${OV009_IMAGE_CPP}")
target_include_directories(sm64ds_pc BEFORE PRIVATE "${CMAKE_BINARY_DIR}/pc-host-include")
target_include_directories(sm64ds_pc PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_compile_definitions(sm64ds_pc PRIVATE SM64DS_PLATFORM_PC SM64DS_GAME_TARGET
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
add_dependencies(sm64ds_pc pc_host_headers)
target_link_libraries(sm64ds_pc PRIVATE ntr windowscodecs ole32 winmm)
if(MSVC)
    # Reconstructed ARM routines can intentionally fall through while leaving
    # their result in the return register; retain that control flow on host.
    target_compile_options(sm64ds_pc PRIVATE /wd4311 /wd4312 /wd4068 /wd4716 /Gy)
    target_link_options(sm64ds_pc PRIVATE /OPT:REF)
endif()

if(BUILD_TESTING)
    add_executable(smoke_gx_bank_state tests/smoke_gx_bank_state.cpp
        hal/gx_bank_state.asm
        "${PORT_REPO_ROOT_ABS}/src/_ZN2GX22SetBankForSubBGExtPlttEt.cpp"
        "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0205/func_02054118.c"
        "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0205/func_020541b8.c")
    target_include_directories(smoke_gx_bank_state PRIVATE "${PORT_REPO_ROOT_ABS}/include")
    target_compile_definitions(smoke_gx_bank_state PRIVATE SM64DS_PLATFORM_PC)
    target_link_libraries(smoke_gx_bank_state PRIVATE ntr)
    add_test(NAME smoke_gx_bank_state COMMAND smoke_gx_bank_state)
    add_executable(smoke_stage_graph tests/smoke_stage_graph.cpp
        hal/stage_graph_state.cpp
        "${PORT_REPO_ROOT_ABS}/src/unnamed/ov002/__sinit_ov002_0210804c.c"
        "${PORT_REPO_ROOT_ABS}/src/UpdateMinimap.c"
        "${PORT_REPO_ROOT_ABS}/src/_ZN10dScStage_c14GraphCallback2Ev.cpp"
        "${PORT_REPO_ROOT_ABS}/src/engine/gx/G2x.cpp")
    target_include_directories(smoke_stage_graph PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PORT_REPO_ROOT_ABS}/include")
    target_compile_definitions(smoke_stage_graph PRIVATE SM64DS_PLATFORM_PC)
    target_link_libraries(smoke_stage_graph PRIVATE ntr)
    add_test(NAME smoke_stage_graph COMMAND smoke_stage_graph)
    sm64ds_hostgen_symbols(PARTICLE_CALLBACK_MATH_GEN func_0204c0a8 func_0204c0e8)
    add_executable(smoke_particle_callbacks tests/smoke_particle_callbacks.cpp
        hal/particle_render_callbacks.cpp ${PARTICLE_CALLBACK_MATH_GEN})
    target_include_directories(smoke_particle_callbacks PRIVATE "${PORT_REPO_ROOT_ABS}/include")
    target_compile_definitions(smoke_particle_callbacks PRIVATE SM64DS_PLATFORM_PC)
    target_link_libraries(smoke_particle_callbacks PRIVATE ntr)
    add_test(NAME smoke_particle_callbacks COMMAND smoke_particle_callbacks)
    add_executable(smoke_ppu_affine tests/smoke_ppu_affine.cpp)
    target_link_libraries(smoke_ppu_affine PRIVATE ntr)
    add_test(NAME smoke_ppu_affine COMMAND smoke_ppu_affine)
    add_executable(smoke_ppu_windows tests/smoke_ppu_windows.cpp)
    target_link_libraries(smoke_ppu_windows PRIVATE ntr)
    add_test(NAME smoke_ppu_windows COMMAND smoke_ppu_windows)
    add_executable(smoke_ppu_obj tests/smoke_ppu_obj.cpp)
    target_link_libraries(smoke_ppu_obj PRIVATE ntr)
    add_test(NAME smoke_ppu_obj COMMAND smoke_ppu_obj)
    add_executable(smoke_float_compare tests/smoke_float_compare.cpp hal/float_bits.cpp)
    add_test(NAME smoke_float_compare COMMAND smoke_float_compare)
    add_executable(smoke_gx_visibility tests/smoke_gx_visibility.cpp)
    target_link_libraries(smoke_gx_visibility PRIVATE ntr)
    add_executable(smoke_camera_matrix tests/smoke_camera_matrix.cpp
        "${CMAKE_BINARY_DIR}/pc-boot-src/src/engine/gx/G3i.cpp"
        "${CMAKE_BINARY_DIR}/pc-boot-src/src/NormalizeVec3.cpp"
        hal/cstd_div.c
        "${CMAKE_BINARY_DIR}/pc-boot-src/src/_ZN4cstd11fdiv_resultEv.cpp"
        "${CMAKE_BINARY_DIR}/pc-boot-src/src/_ZN4cstd11ldiv_resultEv.cpp"
        "${PORT_REPO_ROOT_ABS}/src/CrossVec3.c"
        "${PORT_REPO_ROOT_ABS}/src/DotVec3.c")
    target_include_directories(smoke_camera_matrix PRIVATE "${PORT_REPO_ROOT_ABS}/include")
    target_compile_definitions(smoke_camera_matrix PRIVATE SM64DS_PLATFORM_PC)
    target_link_libraries(smoke_camera_matrix PRIVATE ntr)
    add_executable(smoke_model_cleanup tests/smoke_model_cleanup.cpp
        hal/mem_delete2.cpp
        "${PORT_REPO_ROOT_ABS}/src/_ZN9ModelBaseC2Ev.cpp"
        "${PORT_REPO_ROOT_ABS}/src/_ZN9ModelBaseD1Ev.cpp")
    target_include_directories(smoke_model_cleanup PRIVATE "${PORT_REPO_ROOT_ABS}/include")
    target_compile_definitions(smoke_model_cleanup PRIVATE SM64DS_PLATFORM_PC)
    add_executable(smoke_collision_veneer tests/smoke_collision_veneer.cpp
        "${PORT_REPO_ROOT_ABS}/src/unnamed/arm9/0203/func_020383e4.c")
    target_compile_definitions(smoke_collision_veneer PRIVATE SM64DS_PLATFORM_PC)
    add_executable(smoke_stage_layout tests/smoke_stage_layout.cpp)
    target_include_directories(smoke_stage_layout PRIVATE
        "${CMAKE_BINARY_DIR}/pc-host-include" "${PORT_REPO_ROOT_ABS}/include")
    target_compile_definitions(smoke_stage_layout PRIVATE SM64DS_PLATFORM_PC)
    add_dependencies(smoke_stage_layout pc_host_headers)
    add_executable(smoke_minigame_layout tests/smoke_minigame_layout.cpp)
    target_include_directories(smoke_minigame_layout PRIVATE
        "${CMAKE_BINARY_DIR}/pc-host-include" "${PORT_REPO_ROOT_ABS}/include")
    target_compile_definitions(smoke_minigame_layout PRIVATE SM64DS_PLATFORM_PC)
    target_link_libraries(smoke_minigame_layout PRIVATE ntr)
    add_dependencies(smoke_minigame_layout pc_host_headers)
    set(PORT_SMOKE_TARGETS
        smoke_minigame_layout
        smoke_gx_visibility
        smoke_camera_matrix
        smoke_model_cleanup
        smoke_collision_veneer
        smoke_stage_layout
        smoke
        smoke_heap
        smoke_roots
        smoke_gx
        smoke_model
        smoke_frames
        smoke_soak
        smoke_clsn
        smoke_modelanim
        smoke_oam
        smoke_soak_anim
        smoke_anim
        smoke_fs
        smoke_actor
        smoke_overlay8
        smoke_overlay9
        smoke_video)
    foreach(smoke_target IN LISTS PORT_SMOKE_TARGETS)
        add_test(NAME ${smoke_target} COMMAND $<TARGET_FILE:${smoke_target}>)
        set_tests_properties(${smoke_target} PROPERTIES
            WORKING_DIRECTORY "${PORT_REPO_ROOT_ABS}")
    endforeach()
endif()




