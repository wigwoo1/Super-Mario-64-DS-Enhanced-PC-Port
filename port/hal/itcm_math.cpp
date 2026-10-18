// PC equivalents for the ARM9 ITCM soft-float helpers reached by the native
// target's numeric formatting and state-dispatch code. MSVC already supplies
// IEEE-754 double operations, so these preserve the C ABI and value semantics
// while replacing ARM calling-convention assembly.
#include <cmath>
#include <cstdint>
#include <limits>
#include <atomic>
#include <windows.h>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <unordered_map>
#include <cstdio>

#include "ntr/rt.h"
#include "ntr/mmio.h"

namespace {
uint32_t g_fp_status = 0;
std::atomic<int> g_fiq_enabled{1};
std::unordered_map<void *, void *> g_context_fibers;
void scheduler_trace(const char *label, void *context = nullptr, void *fiber = nullptr) {
    const char *trace = std::getenv("SM64DS_PC_TRACE");
    if (trace && trace[0] && trace[0] != '0') {
        std::fprintf(stderr, "[sm64ds] fiber scheduler: %s context=%p fiber=%p\n",
                     label, context, fiber);
        std::fflush(stderr);
    }
}
// NitroSDK lock IDs 0x40..0x7f are allocated by clearing the first set bit
// across two 32-bit availability words at 0x027fffb0 on the DS.
std::atomic<uint32_t> g_lock_ids_available[2] = {
    std::atomic<uint32_t>(0xffffffffu), std::atomic<uint32_t>(0xffffffffu)
};

VOID WINAPI run_guest_context(void *context) {
    auto *words = static_cast<std::uint32_t *>(context);
    const auto entry_address = (words[0x40 / 4] - 4u) & ~1u;
    scheduler_trace("guest fiber entered", context,
                    reinterpret_cast<void *>(entry_address));
    using GuestEntry = void (__cdecl *)(void);
    reinterpret_cast<GuestEntry>(entry_address)();
    scheduler_trace("guest callback returned", context,
                    reinterpret_cast<void *>(entry_address));
}
}

namespace cstd {
void __assert(const char *, const char *, const char *, int);
char *strncpy(char *, const char *, unsigned int);
char *strchr(const char *, char);
void __builtin_trap() { std::abort(); }
}

namespace CP15 { void DrainWriteBuffer() {} }
namespace IRQ {
void Restore(unsigned int state) { (void)ntr::rt_irq_restore(state); }
void GameCardIREQMCHandler();
void IPCRxFifoNotEmptyHandler();
}

extern "C" double func_01ff8708(double a, double b) { return a * b; }
extern "C" double func_01ff8e10(double a, double b) { return a - b; }
extern "C" double func_01ff8000(double a, double b) { return a + b; }
extern "C" int func_01ff9d40(double a, double b) { return a == b; }
extern "C" int func_01ff9e2c(double a, double b) { return a < b; }

// ARM EABI callers pass/receive IEEE-754 single values in integer registers.
// Preserve that raw-bit interface while using the host's correctly rounded
// IEEE conversion and comparison.
extern "C" unsigned int func_01ffa4bc(int value) {
    const float converted = static_cast<float>(value);
    unsigned int bits = 0;
    std::memcpy(&bits, &converted, sizeof(bits));
    return bits;
}

// Host equivalent of NitroSDK's OS_GetLockID / OS_ReleaseLockID routines.
// The bitmap keeps the same ID range and first-free-bit ordering as the ARM
// implementation, and CAS preserves safety across host threads.
extern "C" int func_02057020(void) {
    for (unsigned word = 0; word != 2; ++word) {
        uint32_t available = g_lock_ids_available[word].load();
        while (available != 0) {
            unsigned bit = 0;
            uint32_t mask = 0x80000000u;
            while ((available & mask) == 0) { ++bit; mask >>= 1; }
            const uint32_t updated = available & ~mask;
            if (g_lock_ids_available[word].compare_exchange_weak(available, updated))
                return static_cast<int>(0x40 + word * 32 + bit);
        }
    }
    return -3;
}

extern "C" void func_02057078(int lock_id) {
    if (lock_id < 0x40 || lock_id >= 0x80) return;
    const unsigned index = static_cast<unsigned>(lock_id - 0x40);
    g_lock_ids_available[index / 32].fetch_or(0x80000000u >> (index % 32));
}

extern "C" unsigned int func_01ffb008(unsigned int mask,
                                         unsigned int value) {
    const unsigned int old = g_fp_status;
    g_fp_status = (old & ~mask) ^ value;
    return old;
}

extern "C" long long func_01ff859c(double value) {
    if (std::isnan(value)) return 0;
    constexpr double limit = 9223372036854775808.0;
    if (value >= limit) return (std::numeric_limits<long long>::max)();
    if (value <= -limit) return (std::numeric_limits<long long>::min)();
    return static_cast<long long>(value);
}

// CPSR interrupt-state helpers used around shared ARM9 tables. The PC runtime
// tracks IRQ masking per its frame scheduler; FIQ is represented separately.
extern "C" int func_0206da18() { return ntr::rt_irq_disable() == 0; }
extern "C" void func_0206da28(int enabled) {
    if (enabled) (void)ntr::rt_irq_restore(0);
    else (void)ntr::rt_irq_restore(0x80);
}
extern "C" int func_0206d9cc() { return g_fiq_enabled.exchange(0) != 0; }
extern "C" void func_0206d9dc(int enabled) {
    g_fiq_enabled.store(enabled != 0);
}
extern "C" unsigned int func_0206dab4(unsigned int value, void *address) {
    return static_cast<unsigned int>(InterlockedExchange(
        reinterpret_cast<volatile LONG *>(address), static_cast<LONG>(value)));
}

// PC equivalents for the SDK context initializer and two ARM atomic/runtime
// primitives that the host compiler cannot assemble. Context fields retain
// the ARM OSContext byte offsets used by the recovered scheduler.
extern "C" void func_02058568(void *context, unsigned int entry_pc,
                              unsigned int stack_pointer) {
    auto *words = static_cast<std::uint32_t *>(context);
    words[0x40 / 4] = entry_pc + 4;
    words[0x44 / 4] = stack_pointer;
    words[0x38 / 4] = stack_pointer - 0x40;
    words[0] = (entry_pc & 1) ? 0x3f : 0x1f;
    for (std::size_t offset = 4; offset <= 0x34; offset += 4)
        words[offset / 4] = 0;
    words[0x3c / 4] = 0;
}

extern "C" int ARMProcessorMode(void) { return 0x1f; }
extern "C" int ARMSaveContext(void *context) {
    scheduler_trace("save context", context, GetCurrentFiber());
    if (!context || !IsThreadAFiber()) return 1;
    g_context_fibers[context] = GetCurrentFiber();
    return 0;
}
extern "C" void ARMRestoreContext(void *context) {
    scheduler_trace("restore context", context, GetCurrentFiber());
    if (!context) return;
    auto found = g_context_fibers.find(context);
    if (found == g_context_fibers.end()) {
        scheduler_trace("create guest fiber", context);
        void *fiber = CreateFiber(256 * 1024, run_guest_context, context);
        scheduler_trace("guest fiber creation result", context, fiber);
        if (!fiber) return;
        found = g_context_fibers.emplace(context, fiber).first;
    }
    if (found->second != GetCurrentFiber()) {
        scheduler_trace("switch to guest fiber", context, found->second);
        SwitchToFiber(found->second);
        scheduler_trace("returned from guest fiber", context, found->second);
    }
}

extern "C" unsigned int func_0205a74c(unsigned int value,
                                       volatile unsigned int *address) {
    return static_cast<unsigned int>(InterlockedExchange(
        reinterpret_cast<volatile LONG *>(address), static_cast<LONG>(value)));
}

extern "C" int func_02059d8c(int value) {
    // The recovered ARM loop subtracts four while carry remains set and
    // returns the final (possibly -4) word-alignment remainder.
    while (static_cast<unsigned int>(value) >= 4u)
        value -= 4;
    return value - 4;
}

extern "C" void func_020048d8(void *end_of_compressed_data) {
    if (!end_of_compressed_data) return;
    auto *end = static_cast<std::uint8_t *>(end_of_compressed_data);
    std::uint32_t packed_header = 0, expanded_tail = 0;
    std::memcpy(&packed_header, end - 8, sizeof(packed_header));
    std::memcpy(&expanded_tail, end - 4, sizeof(expanded_tail));
    const std::uint32_t header_size = packed_header >> 24;
    const std::uint32_t compressed_size = packed_header & 0x00ffffffu;
    std::uint8_t *source = end - header_size;
    const std::uint8_t *source_begin = end - compressed_size;
    std::uint8_t *dest = end + expanded_tail;
    while (source > source_begin) {
        const std::uint8_t flags = *--source;
        for (unsigned bit = 0; bit < 8 && source > source_begin; ++bit) {
            if ((flags & (0x80u >> bit)) == 0) {
                *--dest = *--source;
                continue;
            }
            if (source - source_begin < 2) return;
            const std::uint8_t first = *--source;
            const std::uint8_t second = *--source;
            const unsigned displacement = (((second | (first << 8)) & 0x0fffu) + 2u);
            const unsigned length = (first >> 4) + 3u;
            for (unsigned i = 0; i < length; ++i)
                *--dest = dest[displacement];
        }
    }
}

extern "C" void func_02059834(void);
extern "C" void func_02059824(void) { func_02059834(); }

extern "C" void func_02068484(void);
extern "C" unsigned char data_020a9d2c[];
extern "C" int func_02068398(void) {
    void *current = *reinterpret_cast<void **>(data_020a9d2c);
    if (!current) return 0;
    void *selected = current;
    void *candidate = *reinterpret_cast<void **>(
        static_cast<unsigned char *>(current) + 4);
    if (candidate) {
        void *active = *reinterpret_cast<void **>(
            static_cast<unsigned char *>(candidate) + 0x4b4);
        if (active) selected = active;
    }
    *reinterpret_cast<void **>(data_020a9d2c + 4) = selected;
    func_02068484();
    data_020a9d2c[0xe] = *(
        static_cast<unsigned char *>(selected) + 0x4ac);
    data_020a9d2c[0xc] = 2;
    return 1;
}

extern "C" void func_020593f4(void) { std::abort(); }

extern "C" void _ZN3IRQ11SetIRQStateEb(int enabled) {
    if (enabled) (void)ntr::rt_irq_restore(0);
    else (void)ntr::rt_irq_restore(0x80);
}
extern "C" unsigned int _ZN3IRQ11SetIRQStateEb_false() {
    return ntr::rt_irq_disable() == 0;
}

extern "C" unsigned int _ZN3IRQ7SetIRQsEj(unsigned int mask) {
    const unsigned int previous = static_cast<unsigned int>(
        ntr::io_read(0x04000210u, 4));
    ntr::io_write(0x04000210u, mask, 4);
    return previous;
}

// The DS BIOS wait-loop SWI and cartridge bus lock are synchronization hints
// for hardware users. ROM reads on the PC use the host filesystem instead.
extern "C" void WaitByLoop(int count) { (void)count; }
extern "C" void func_02057014(int lock_id) { (void)lock_id; }
extern "C" void func_0207199c() {}

extern "C" void _ZN3IRQ21GameCardIREQMCHandlerEv() {
    IRQ::GameCardIREQMCHandler();
}
extern "C" void _ZN3IRQ24IPCRxFifoNotEmptyHandlerEv() {
    IRQ::IPCRxFifoNotEmptyHandler();
}

extern "C" void _ZN4cstd8__assertEPKcS1_S1_i(
    const char *file, const char *expr, const char *func, int line) {
    cstd::__assert(file, expr, func, line);
}
extern "C" char *_ZN4cstd7strncpyEPcPKcj(
    char *dst, const char *src, unsigned int count) {
    return cstd::strncpy(dst, src, count);
}
extern "C" char *_ZN4cstd6strchrEPKcc(const char *src, char ch) {
    return cstd::strchr(src, ch);
}
