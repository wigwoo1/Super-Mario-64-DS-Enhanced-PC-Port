// Host implementation of the system tick, func_02059650.
//
// On the DS this reads the OS tick counter (timer hardware + IRQ-maintained
// high bits). Timer::Start/Stop/GetTime take differences of it, so any
// monotonic s64 with a stable rate is semantically faithful. The Timer TUs
// declare it with C++ linkage, which is why this is a .cpp -- a C definition
// does not mangle to what they reference.
//
// GATE 1: the tick is a manually-advanced counter so the smoke tests are
// deterministic -- sm64ds_hal_advance_ticks() stands in for time passing.
// When the main-loop seam lands, this becomes a QueryPerformanceCounter read
// scaled to the DS tick rate, and the manual hook moves behind a test flag.
typedef long long s64;

#if defined(SM64DS_GAME_TARGET) && defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Nintendo DS OS ticks run at half the ARM9 clock: 33,513,982 ticks/s.
// Convert QPC using quotient/remainder so the intermediate never multiplies
// the ever-growing counter by the full tick rate.
extern "C" s64 func_02059650()
{
    LARGE_INTEGER counter{}, frequency{};
    QueryPerformanceCounter(&counter);
    QueryPerformanceFrequency(&frequency);
    if (frequency.QuadPart <= 0) return 0;
    const unsigned long long qpc = static_cast<unsigned long long>(counter.QuadPart);
    const unsigned long long hz = static_cast<unsigned long long>(frequency.QuadPart);
    const unsigned long long ticks_per_second = 33513982ull;
    return static_cast<s64>((qpc / hz) * ticks_per_second +
                            ((qpc % hz) * ticks_per_second) / hz);
}
#else
static s64 g_ticks;

extern "C" s64 func_02059650()
{
    return g_ticks;
}

extern "C" void sm64ds_hal_advance_ticks(s64 n)
{
    g_ticks += n;
}
#endif
