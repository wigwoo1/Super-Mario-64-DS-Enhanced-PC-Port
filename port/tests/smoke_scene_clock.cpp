// Exercise the original scene IRQ and main-thread wait together, not just
// host VBlank frequency. Stage delta=2 must produce 30 updates per second.
#include "ntr/rt.h"
#include <chrono>
#include <cstdio>
#include <initializer_list>
namespace IRQ { void VBlankHandler(); }
extern "C" {
volatile int data_0209d514=0;
int data_0208ee44=1,data_0209d500=0,data_0209d4fc=0;
unsigned char data_0209d4f0=1;
alignas(4) char data_023c0000[0x4000]={};
static unsigned wakes=0,timer_ticks=0,updates=0,presentations=0;
void OS_WakeupThread(int *queue) {if(queue==&data_0209d500)++wakes;}
void func_02019144() {++timer_ticks;}
void func_02019100() {}
void func_0201a4bc();
}
static void game() {
    ntr::rt_irq_set_handler(ntr::IRQ_VBLANK,IRQ::VBlankHandler);
    for(;;) {++updates;func_0201a4bc();}
}
static bool present(uint64_t) {++presentations;return true;}
int main() {
    for(unsigned fps: {60u,120u,240u}) for(int interval: {1,2,3}) {
        data_0208ee44=interval;data_0209d514=0;
        wakes=timer_ticks=updates=presentations=0;
        ntr::rt_set_target_fps(fps);
        const auto start=std::chrono::steady_clock::now();
        const auto frames=ntr::rt_run(game,present,120);
        const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        std::printf("scene-clock fps=%u interval=%d: ticks=%llu updates=%u wakes=%u timers=%u seconds=%.3f presentations=%u\n",
            fps,interval,frames,updates,wakes,timer_ticks,seconds,presentations);
        if(frames!=120 || updates!=120/interval || wakes!=updates || timer_ticks!=updates ||
           seconds<1.90 || seconds>2.30 || presentations<fps*18/10 || presentations>fps*23/10) return 1;
    }
    return 0;
}
