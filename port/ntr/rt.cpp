#include "ntr/rt.h"

#include "ntr/mmio.h"
#include "ntr/video.h"
#include "ntr/input.h"
#include "ntr/ppu.h"

#include <cstdio>
#include <cstdint>
#include <cstdlib>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace ntr {
namespace {

constexpr uint32_t REG_IME = 0x04000208;
constexpr uint32_t REG_IE = 0x04000210;
constexpr uint32_t REG_IF = 0x04000214;
constexpr uint32_t REG_VCOUNT = 0x04000006;
constexpr unsigned IRQ_COUNT = 22;

inline volatile uint32_t &reg32(uint32_t a) {
    return *reinterpret_cast<volatile uint32_t *>(a);
}
inline volatile uint16_t &reg16(uint32_t a) {
    return *reinterpret_cast<volatile uint16_t *>(a);
}

struct Runtime {
    void *host_fiber = nullptr;
    void *game_fiber = nullptr;
    void (*entry)() = nullptr;
    FrameHook hook = nullptr;
    uint64_t frame = 0;
    uint64_t max_frames = 0;
    bool game_done = false;
    bool stop = false;
    uint32_t target_fps = 60;
    float presentation_phase = 0;
    uint32_t cpsr_i = 0;     // 0x80 when interrupts are masked
    IrqHandler irq_handlers[IRQ_COUNT]{};
    uint64_t irq_dispatches[IRQ_COUNT]{};
    uint64_t irq_handler_calls[IRQ_COUNT]{};
};

Runtime g;

bool trace_enabled() {
    const char *value = std::getenv("SM64DS_PC_TRACE");
    return value && value[0] && value[0] != '0';
}

#if defined(_WIN32)
void CALLBACK game_trampoline(void *) {
    g.entry();
    g.game_done = true;
    // The game returned. Hand control back for good.
    for (;;) SwitchToFiber(g.host_fiber);
}
#endif

// The guest's IRQ dispatcher is ARM-only assembly. Deliver its registered
// callbacks at VBlank and HBlank boundaries, acknowledging each request before
// calling the handler just as the DS dispatcher does.
void dispatch_pending_irqs() {
    if (g.cpsr_i != 0 || reg16(REG_IME) == 0) return;
    for (unsigned delivered = 0; delivered < IRQ_COUNT; ++delivered) {
        const uint32_t pending = reg32(REG_IF) & reg32(REG_IE);
        if (pending == 0) break;
        unsigned bit = 0;
        while (bit < IRQ_COUNT && !(pending & (1u << bit))) ++bit;
        if (bit == IRQ_COUNT) break;
        const uint32_t mask = 1u << bit;
        reg32(REG_IF) &= ~mask;
        ++g.irq_dispatches[bit];
        IrqHandler handler = g.irq_handlers[bit];
        if (handler) {
            ++g.irq_handler_calls[bit];
            handler();
        }
    }
}

// Keep window registers for every visible line, then deliver HBlank with
// VCOUNT and DISPSTAT valid. The handler's writes affect the following line.
void run_scanlines() {
    ppu_begin_window_frame();
    for(uint16_t line=0;line<263;++line) {
        reg16(REG_VCOUNT)=line;
        reg16(0x04000004)=(reg16(0x04000004)&~3u) | (line>=192 && line<262 ? 1u:0u);
        if(line<192) ppu_capture_window_line(line);
        reg16(0x04000004)|=2u;
        if(reg16(0x04000004)&0x10u) reg32(REG_IF)|=IRQ_HBLANK;
        dispatch_pending_irqs();
        reg16(0x04000004)&=~2u;
    }
}

}  // namespace

void rt_vblank_wait() {
#if defined(_WIN32)
    if (!g.game_fiber) return;   // not running under rt_run; nothing to yield to
    reg32(REG_IF) |= IRQ_VBLANK;
    SwitchToFiber(g.host_fiber);
#endif
}

uint64_t rt_frame() { return g.frame; }

uint32_t rt_irq_disable() {
    const uint32_t prev = g.cpsr_i;
    g.cpsr_i = 0x80;
    return prev;
}

uint32_t rt_irq_enable() {
    const uint32_t prev = g.cpsr_i;
    g.cpsr_i = 0;
    return prev;
}

uint32_t rt_irq_restore(uint32_t prev) {
    const uint32_t was = g.cpsr_i;
    g.cpsr_i = prev & 0x80;
    return was;
}

bool rt_irq_masked() { return g.cpsr_i != 0; }

void rt_irq_set_handler(uint32_t mask, IrqHandler handler) {
    for (unsigned bit = 0; bit < IRQ_COUNT; ++bit)
        if (mask & (1u << bit)) g.irq_handlers[bit] = handler;
}

IrqHandler rt_irq_get_handler(uint32_t mask) {
    for (unsigned bit = 0; bit < IRQ_COUNT; ++bit)
        if (mask & (1u << bit)) return g.irq_handlers[bit];
    return nullptr;
}

void rt_set_target_fps(uint32_t fps) {
    if (fps == 60 || fps == 120 || fps == 240) g.target_fps = fps;
}

uint32_t rt_target_fps() { return g.target_fps; }
float rt_presentation_phase() { return g.presentation_phase; }

uint64_t rt_run(void (*game)(), FrameHook hook, uint64_t max_frames, void (*before_present)()) {
#if !defined(_WIN32)
    (void)game; (void)hook; (void)max_frames; (void)before_present;
    std::fprintf(stderr, "rt_run: fiber backend is Windows-only so far\n");
    return 0;
#else
    if (!io_init()) {
        std::fprintf(stderr, "rt_run: io_init failed\n");
        return 0;
    }

    const uint32_t requested_fps = g.target_fps;
    g = Runtime{};
    g.target_fps = requested_fps;
    g.entry = game;
    g.hook = hook;
    g.max_frames = max_frames;
    uint64_t presentation_frame = 0;

    g.host_fiber = ConvertThreadToFiber(nullptr);
    if (!g.host_fiber) {
        std::fprintf(stderr, "rt_run: ConvertThreadToFiber failed\n");
        return 0;
    }
    g.game_fiber = CreateFiber(256 * 1024, game_trampoline, nullptr);
    if (!g.game_fiber) {
        std::fprintf(stderr, "rt_run: CreateFiber failed\n");
        return 0;
    }
    if (trace_enabled()) {
        std::fprintf(stderr, "[sm64ds] fibers ready; entering game fiber\n");
        std::fflush(stderr);
    }

    // The DS comes up with interrupts enabled and IME set by the CRT0.
    reg32(REG_IME) = 1;
    reg32(REG_IE) = IRQ_VBLANK;

    const HANDLE frame_timer = CreateWaitableTimerExW(
        nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_MODIFY_STATE | SYNCHRONIZE);
    LARGE_INTEGER frequency{}, first_tick{};
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&first_tick);
    const LONGLONG game_tick_period = (frequency.QuadPart + 30) / 60;
    const LONGLONG wait_margin = frequency.QuadPart / 2500;
    const LONGLONG minimum_draw_budget = frequency.QuadPart / 1000;
    LONGLONG game_tick_start = first_tick.QuadPart;
    LONGLONG extra_draw_cost = 0;
    unsigned presentation_slot = 0;

    auto wait_until = [&](LARGE_INTEGER &now,LONGLONG deadline) {
        while (now.QuadPart < deadline) {
            const LONGLONG remaining = deadline - now.QuadPart;
            if (remaining > wait_margin && frame_timer) {
                LARGE_INTEGER relative{};
                relative.QuadPart = -1 - (remaining - wait_margin) * 10000000 / frequency.QuadPart;
                if (SetWaitableTimer(frame_timer, &relative, 0, nullptr, nullptr, FALSE))
                    WaitForSingleObject(frame_timer, INFINITE);
                else
                    SwitchToThread();
            } else if (!frame_timer && remaining > frequency.QuadPart / 500) {
                Sleep(1);
            } else {
                SwitchToThread();
            }
            QueryPerformanceCounter(&now);
        }
    };

    while (!g.game_done && !g.stop) {
        const bool run_game = presentation_slot == 0;
        LARGE_INTEGER draw_start{};
        if (!run_game) QueryPerformanceCounter(&draw_start);
        if (run_game) {
            input_poll();
            SwitchToFiber(g.game_fiber);      // run until the game blocks
            if (g.game_done) break;
            reg16(REG_VCOUNT)=192;
            reg16(0x04000004)=(reg16(0x04000004)&~2u)|1u;
            dispatch_pending_irqs();
            ++g.frame;                        // emulated DS simulation ticks
            if (trace_enabled() && (g.frame == 1 || (g.frame % 120) == 0)) {
                std::fprintf(stderr, "[sm64ds] completed simulation frame %llu\n",
                             static_cast<unsigned long long>(g.frame));
                std::fflush(stderr);
            }
            if (g.max_frames && g.frame >= g.max_frames) g.stop = true;
        }

        // The game fiber stays parked between VBlanks. Only the renderer
        // consumes the fractional phase on these host presentations.
        g.presentation_phase = presentation_slot / 4.0f;
        ++presentation_frame;
        if (run_game) {
            if (before_present) before_present();
            run_scanlines();
        }
        if (video_is_open() && !video_present_frame()) g.stop = true;
        if (g.hook && !g.hook(presentation_frame)) g.stop = true;
        if (g.stop) break;

        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        if (run_game) {
            // Let a costly draw age out even if it left no room to render
            // another sample. A temporary stall must not pin us at 60 Hz.
            extra_draw_cost = extra_draw_cost * 7 / 8;
        } else {
            const auto cost = now.QuadPart - draw_start.QuadPart;
            if (cost > extra_draw_cost) extra_draw_cost = cost;
        }

        // A slightly late extra draw can still fit. Let it start within
        // half its nominal interval, with the recent draw cost and a wait
        // margin reserved before simulation resumes.
        const unsigned slot_step = 240 / g.target_fps;
        const auto late_allowance = game_tick_period * slot_step / 8;
        const auto tick_deadline = game_tick_start + game_tick_period;
        const auto draw_budget = (extra_draw_cost > minimum_draw_budget
            ? extra_draw_cost : minimum_draw_budget) + wait_margin;
        unsigned next_slot = (presentation_slot / slot_step + 1) * slot_step;
        for (; next_slot < 4; next_slot += slot_step) {
            const auto planned_start = game_tick_start + game_tick_period * next_slot / 4;
            if (planned_start + draw_budget > tick_deadline) continue;
            wait_until(now,planned_start);
            if (now.QuadPart <= planned_start + late_allowance &&
                now.QuadPart + draw_budget <= tick_deadline) break;
        }
        if (next_slot == 4) wait_until(now,tick_deadline);

        const auto elapsed = now.QuadPart - game_tick_start;
        if (elapsed >= game_tick_period) {
            // Retain the 60 Hz timeline after a stall without queuing a burst
            // of simulation steps or catching up stale render-only draws.
            game_tick_start += (elapsed / game_tick_period) * game_tick_period;
            presentation_slot = 0;
        } else {
            presentation_slot = next_slot;
        }

    }

    if (trace_enabled()) {
        std::fprintf(stderr,
            "[pc irq summary] vblank_dispatches=%llu vblank_handler_calls=%llu registered=%u\n",
            static_cast<unsigned long long>(g.irq_dispatches[0]),
            static_cast<unsigned long long>(g.irq_handler_calls[0]),
            g.irq_handlers[0] != nullptr);
        std::fflush(stderr);
    }
    DeleteFiber(g.game_fiber);
    g.game_fiber = nullptr;
    ConvertFiberToThread();
    if (frame_timer) CloseHandle(frame_timer);
    return g.frame;
#endif
}

}  // namespace ntr

extern "C" void ntr_rt_vblank_wait(void) { ntr::rt_vblank_wait(); }
