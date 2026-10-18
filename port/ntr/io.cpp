// The host I/O bus: a real memory window at the DS I/O addresses, plus side-effect
// dispatch for the registers whose writes do something.
//
// Register semantics here are from GBATEK. No SDK source is reproduced.

#include "ntr/mmio.h"

#include "ntr/gx.h"
#include "ntr/input.h"

#include <cstring>
#include <cstdio>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace ntr {
namespace {

uint8_t *g_io = nullptr;
uint16_t g_lid_latch = 0x2000u;
uint16_t g_touch_sample[2] = {};

inline uint8_t *slot(uint32_t addr) { return g_io + (addr - IO_BASE); }

inline uint64_t raw_read(uint32_t addr, unsigned width) {
    uint64_t v = 0;
    std::memcpy(&v, slot(addr), width);
    return v;
}

inline void raw_write(uint32_t addr, uint64_t value, unsigned width) {
    std::memcpy(slot(addr), &value, width);
}

// --- CP: divider and square root -----------------------------------------------
// GBATEK: 0x4000280 DIVCNT, 0x4000290 NUMER(64), 0x4000298 DENOM(64),
//         0x40002A0 RESULT(64), 0x40002A8 REMAINDER(64),
//         0x40002B0 SQRTCNT, 0x40002B4 SQRT_RESULT(32), 0x40002B8 SQRT_PARAM(64).
// The busy bit (0x8000) is never set: the host computes synchronously, so any
// `while (DIVCNT & 0x8000);` spin-wait falls straight through.

constexpr uint32_t DIVCNT = 0x4000280, DIV_NUMER = 0x4000290, DIV_DENOM = 0x4000298;
constexpr uint32_t DIV_RESULT = 0x40002A0, DIV_REM = 0x40002A8;
constexpr uint32_t SQRTCNT = 0x40002B0, SQRT_RESULT = 0x40002B4, SQRT_PARAM = 0x40002B8;

void run_divide() {
    const uint16_t mode = static_cast<uint16_t>(raw_read(DIVCNT, 2)) & 3;
    const int64_t numer = static_cast<int64_t>(raw_read(DIV_NUMER, 8));
    const int64_t denom = static_cast<int64_t>(raw_read(DIV_DENOM, 8));

    int64_t n = numer, d = denom;
    if (mode == 0) {                       // 32 / 32
        n = static_cast<int32_t>(numer);
        d = static_cast<int32_t>(denom);
    } else if (mode == 1) {                // 64 / 32
        d = static_cast<int32_t>(denom);
    }

    int64_t quot, rem;
    if (d == 0) {
        // GBATEK: division by zero leaves remainder = numerator and a sign-based
        // quotient. Mirror that rather than trapping.
        rem = n;
        quot = (n < 0) ? 1 : -1;
        if (mode == 0) quot = (n < 0) ? 1 : -1;
    } else {
        quot = n / d;
        rem = n % d;
    }
    raw_write(DIV_RESULT, static_cast<uint64_t>(quot), 8);
    raw_write(DIV_REM, static_cast<uint64_t>(rem), 8);
}

uint32_t isqrt64(uint64_t v) {
    // Integer floor(sqrt(v)) by restoring bit-by-bit; matches the DS unit's result.
    uint64_t rem = 0, root = 0;
    for (int i = 0; i < 32; ++i) {
        root <<= 1;
        rem = (rem << 2) | (v >> 62);
        v <<= 2;
        if (rem > root) {
            rem -= root | 1;
            root += 2;
        }
    }
    return static_cast<uint32_t>(root >> 1);
}

void run_sqrt() {
    const uint16_t mode = static_cast<uint16_t>(raw_read(SQRTCNT, 2)) & 1;
    uint64_t param = raw_read(SQRT_PARAM, 8);
    if (mode == 0) param &= 0xFFFFFFFFull;      // 32-bit mode
    raw_write(SQRT_RESULT, isqrt64(param), 4);
}

}  // namespace

namespace {

// Reserve one DS region at its real address. Fixed low bases keep (u32)&x exact.
void *map_fixed(uintptr_t base, size_t size) {
#if defined(_WIN32)
    void *p = VirtualAlloc(reinterpret_cast<void *>(base), size,
                           MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
    void *p = mmap(reinterpret_cast<void *>(base), size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (p == MAP_FAILED) p = nullptr;
#endif
    if (p) std::memset(p, 0, size);
    return p;
}

}  // namespace

bool io_init() {
    if (g_io) return true;
    struct { uintptr_t base; size_t size; } regions[] = {
        {MAIN_BASE, MAIN_SIZE}, {IO_BASE, IO_SIZE}, {PLTT_BASE, PLTT_SIZE},
        {VRAM_BASE, VRAM_SIZE}, {OAM_BASE, OAM_SIZE},
    };
    void *io = nullptr;
    for (const auto &r : regions) {
        void *p = map_fixed(r.base, r.size);
        // Main RAM is only needed by the ROM-image fs backend; in the port
        // build the game's globals are host-linked symbols and file images
        // live on the game heap, so a lost race for 0x02000000 (the host
        // allocator can land anything there first) is not fatal.
        if (!p && r.base != MAIN_BASE) {
#if defined(_WIN32)
            std::fprintf(stderr, "ntr::io_init: VirtualAlloc(%p, 0x%zx) failed, error %lu\n",
                         reinterpret_cast<void *>(r.base), r.size,
                         static_cast<unsigned long>(GetLastError()));
#endif
            return false;
        }
        if (r.base == IO_BASE) io = p;
    }
    g_io = static_cast<uint8_t *>(io);
    // The DS lid/touch input latches live outside the host's available address
    // space. hostgen routes their direct accesses through io_read/io_write.
    g_lid_latch = 0x2000u;
    g_touch_sample[0] = g_touch_sample[1] = 0;
    // Reset state: the geometry FIFO starts empty. GXSTAT bit 26 (empty) and
    // bit 25 (less than half full) are set on hardware at boot; the game's
    // display-list pump spin-waits on bit 25 before touching the FIFO, so the
    // latch must come up idle or the wait never falls through.
    raw_write(0x04000600u, 0x06000000u, 4);
    return true;
}

uint64_t io_read(uint32_t addr, unsigned width) {
    if (!g_io && !io_init()) return 0;
    if (addr == 0x04000130u && width == 2) return input_keyinput();
    if (addr == 0x027FFFA8u && width == 2) return g_lid_latch;
    if (addr == 0x027FFFAAu && width == 2) return g_touch_sample[0];
    if (addr == 0x027FFFACu && width == 2) return g_touch_sample[1];
    if (addr >= 0x04000600u && addr + width <= 0x04000604u) {
        // The software command engine consumes FIFO words synchronously.
        // FIFO level and engine-busy are read-only hardware status, not
        // latches that a game's GXSTAT control write can overwrite.
        const uint32_t status =
            (static_cast<uint32_t>(raw_read(0x04000600u, 4)) & ~0x09FF0000u) |
            0x06000000u;
        const uint64_t mask = width == 4 ? 0xFFFFFFFFull : (1ull << (width * 8)) - 1;
        return (status >> ((addr - 0x04000600u) * 8)) & mask;
    }
    return raw_read(addr, width);
}

void io_write(uint32_t addr, uint64_t value, unsigned width) {
    if (!g_io && !io_init()) return;
    if (addr == 0x027FFFA8u && width == 2) { g_lid_latch = static_cast<uint16_t>(value); return; }
    if (addr == 0x027FFFAAu && width == 2) { g_touch_sample[0] = static_cast<uint16_t>(value); return; }
    if (addr == 0x027FFFACu && width == 2) { g_touch_sample[1] = static_cast<uint16_t>(value); return; }
    if (addr >= 0x04000600u && addr + width <= 0x04000604u) {
        // GXSTAT: only IRQ mode bits 30-31 are R/W. Writing bit 15
        // acknowledges the stack error and resets projection stack level.
        const unsigned shift = (addr - 0x04000600u) * 8;
        const uint32_t lane = width == 4 ? 0xFFFFFFFFu :
            static_cast<uint32_t>(((1ull << (width * 8)) - 1) << shift);
        const uint32_t written = static_cast<uint32_t>(value << shift) & lane;
        const uint32_t controls = lane & 0xC0000000u;
        uint32_t status = static_cast<uint32_t>(raw_read(0x04000600u, 4));
        status = (status & ~controls) | (written & controls);
        if (written & 0x8000u) status &= ~0xA000u;
        raw_write(0x04000600u, (status & ~0x09FF0000u) | 0x06000000u, 4);
        return;
    }
    raw_write(addr, value, width);

    // Writing the low half of the operand is what starts the unit on hardware.
    if (addr == DIV_DENOM || addr == DIV_NUMER || addr == DIVCNT) run_divide();
    else if (addr == SQRT_PARAM || addr == SQRTCNT) run_sqrt();
    // Geometry: 0x4000400 is the packed FIFO, 0x4000440.. are the command ports.
    // These execute on write and leave nothing readable behind, so unlike every
    // other block they cannot be served by the memory window alone.
    else if (addr == 0x04000400u) gx_write_fifo(static_cast<uint32_t>(value));
    else if (addr >= 0x04000440u && addr <= 0x040005FFu)
        gx_write_port(addr, static_cast<uint32_t>(value));
}

}  // namespace ntr

extern "C" int ntr_fixed_divide(int numerator, int denominator) {
    // Same 64/32 operation and Q32-to-Q12 rounding as the recovered SDK helper.
    ntr::io_write(0x04000280u, 1, 2);
    ntr::io_write(0x04000290u,
        static_cast<uint64_t>(static_cast<uint32_t>(numerator)) << 32, 8);
    ntr::io_write(0x04000298u, static_cast<uint32_t>(denominator), 8);
    return static_cast<int>((static_cast<int64_t>(ntr::io_read(0x040002A0u, 8)) +
                             0x80000LL) >> 20);
}
