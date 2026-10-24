// Exercise the actual game clock against Windows elapsed time. The original
// course HUD converts OS ticks to bus cycles with <<6 before computing seconds.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cmath>
#include <cstdio>

extern "C" long long func_02059650();

int main() {
    LARGE_INTEGER frequency{},before{},after{};
    if(!QueryPerformanceFrequency(&frequency) || frequency.QuadPart<=0)return 1;
    QueryPerformanceCounter(&before);
    const auto first=func_02059650();
    Sleep(125);
    const auto last=func_02059650();
    QueryPerformanceCounter(&after);
    const double wall=double(after.QuadPart-before.QuadPart)/double(frequency.QuadPart);
    const double original_hud=double(last-first)*64.0/33514000.0;
    std::printf("game clock: wall=%.6f seconds original-HUD=%.6f seconds\n",wall,original_hud);
    if(last<first || std::abs(original_hud-wall)>.030) {
        std::fprintf(stderr,"FAIL: game OS tick units disagree with original timer conversion\n");
        return 1;
    }
    auto previous=last;
    for(int i=0;i<1000;++i) {
        const auto current=func_02059650();
        if(current<previous)return 1;
        previous=current;
    }
    return 0;
}
