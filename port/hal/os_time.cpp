// Host implementation of the system tick, func_02059650.
//
// The game requires the original tick units as well as monotonicity:
// dMeter_c::RenderTimeTimer and Player's timed-Star check both multiply the
// result by64 before dividing by33514000 to obtain seconds. The host game
// uses QPC at this rate; smoke slices retain their explicit manual counter.
typedef long long s64;

#if defined(SM64DS_GAME_TARGET) && defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// The DS timer prescales its33,513,982-Hz bus clock by64 for OS ticks.
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
    const unsigned long long bus_ticks_per_second = 33513982ull;
    const auto bus_ticks=(qpc / hz) * bus_ticks_per_second +
                         ((qpc % hz) * bus_ticks_per_second) / hz;
    return static_cast<s64>(bus_ticks / 64ull);
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
