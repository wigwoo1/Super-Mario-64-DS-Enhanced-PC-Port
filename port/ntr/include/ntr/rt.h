// Host runtime: the frame clock, the interrupt state, and the thing that makes
// `VBlankIntrWait()` mean something off-hardware.
//
// A DS game is not a tick function. It runs a loop that blocks in the middle on
// VBlank, and 11,000 decompiled functions assume exactly that shape. Restructuring
// them into something a host frame loop can call is not on the table.
//
// So the game runs on its own fiber. VBlankIntrWait switches back to the host
// fiber, which ends the frame -- scan out, present, advance the clock -- and
// switches back. Game control flow is untouched, and nothing races, because only
// one fiber is ever running. The same mechanism generalises to OS_SleepThread /
// OS_WakeupThread when those are needed.

#ifndef NTR_RT_H
#define NTR_RT_H

#include <stdint.h>

namespace ntr {

// DS interrupt bits, as they appear in IE/IF (0x4000210 / 0x4000214).
enum : uint32_t {
    IRQ_VBLANK = 1u << 0,
    IRQ_HBLANK = 1u << 1,
    IRQ_VCOUNT = 1u << 2,
};

// Called for each host presentation frame. Above 60 Hz the latest game state
// may be presented more than once. Return false to stop the loop.
using FrameHook = bool (*)(uint64_t frame);
using IrqHandler = void (*)();

// Register host-visible game IRQ callbacks. The PC runtime invokes callbacks
// when the matching IE/IF bits are active and global interrupts are enabled.
void rt_irq_set_handler(uint32_t mask, IrqHandler handler);
IrqHandler rt_irq_get_handler(uint32_t mask);

// Select host presentation pacing. Only 60, 120, and 240 Hz are accepted.
// The DS game still advances one simulation frame per VBlank; above 60 Hz
// the host repeats the latest game frame.
void rt_set_target_fps(uint32_t fps);
uint32_t rt_target_fps();

// Run `game` on its own fiber at 60 simulation ticks per second, presenting at
// the selected host rate until it returns or `hook` asks to stop. `max_frames`
// and the return value count completed game simulation ticks.
// Optional before_present uploads game graphics state before host scanout.
uint64_t rt_run(void (*game)(), FrameHook hook, uint64_t max_frames = 0,
                void (*before_present)() = nullptr);

// Block until the next VBlank. Called from game code (via the VBlankIntrWait
// shim). Yields to the host fiber; returns when the frame has been presented.
void rt_vblank_wait();

// Number of completed game simulation ticks (nominally 60 per second).
uint64_t rt_frame();

// CPSR I-bit equivalent. Mirrors IRQ::Disable / Enable / Restore, which return
// the *previous* masked state (0x80 when interrupts were disabled).
uint32_t rt_irq_disable();
uint32_t rt_irq_enable();
uint32_t rt_irq_restore(uint32_t prev);
bool rt_irq_masked();

}  // namespace ntr

#endif  // NTR_RT_H
