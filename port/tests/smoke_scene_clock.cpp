// Exercise the original scene IRQ and main-thread wait together, not just
// host VBlank frequency. Stage delta=2 must produce 30 updates per second.
#include "ntr/rt.h"
#include <chrono>
#include <cstdio>
#include <initializer_list>
#include <thread>
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
static unsigned prepared,first_draws,current_fps;
static uint64_t last_tick;
static float last_phase;
static bool valid;
static bool pacing_fixture=false;
static unsigned recovered_early_draws=0, draws_after_stall=0;
static void prepare() {
    valid = valid && ntr::rt_presentation_phase()==0;
    ++prepared;
    if(pacing_fixture) {
        const auto slot=ntr::rt_frame()%12;
        const auto delay=slot==5?5:slot==9?15:0;
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(delay);
        while (std::chrono::steady_clock::now()<deadline) std::this_thread::yield();
    }
}
static bool present(uint64_t) {
    ++presentations;
    const auto tick=ntr::rt_frame();
    const auto phase=ntr::rt_presentation_phase();
    if(tick!=last_tick) {
        valid = valid && tick==last_tick+1 && phase==0;
        ++first_draws;
    } else {
        valid = valid && phase>last_phase && phase<1 &&
            (current_fps==240 || (current_fps==120 && phase==0.5f));
        if(pacing_fixture && tick%12==5 && phase==0.25f)++recovered_early_draws;
        if(pacing_fixture && tick%12==9)++draws_after_stall;
    }
    last_tick=tick;
    last_phase=phase;
    return true;
}
int main() {
    for(unsigned fps: {60u,120u,240u}) for(int interval: {1,2,3}) {
        data_0208ee44=interval;data_0209d514=0;
        wakes=timer_ticks=updates=presentations=0;
        prepared=first_draws=0;last_tick=0;last_phase=0;current_fps=fps;valid=true;
        ntr::rt_set_target_fps(fps);
        const auto start=std::chrono::steady_clock::now();
        const auto frames=ntr::rt_run(game,present,120,prepare);
        const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        std::printf("scene-clock fps=%u interval=%d: ticks=%llu updates=%u wakes=%u timers=%u seconds=%.3f presentations=%u\n",
            fps,interval,frames,updates,wakes,timer_ticks,seconds,presentations);
        if(!valid || frames!=120 || prepared!=frames || first_draws!=frames ||
           updates!=120/interval || wakes!=updates || timer_ticks!=updates ||
           seconds<1.90 || seconds>2.30 || presentations<fps*18/10 || presentations>fps*23/10) return 1;
    }
    pacing_fixture=true;
    data_0208ee44=2;data_0209d514=0;
    wakes=timer_ticks=updates=presentations=0;
    prepared=first_draws=0;last_tick=0;last_phase=0;current_fps=240;valid=true;
    ntr::rt_set_target_fps(current_fps);
    const auto start=std::chrono::steady_clock::now();
    const auto frames=ntr::rt_run(game,present,120,prepare);
    const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::printf("scene-clock with delayed first draw: ticks=%llu updates=%u seconds=%.3f recovered=%u draws_after_stall=%u\n",
        frames,updates,seconds,recovered_early_draws,draws_after_stall);
    if(!valid || frames!=120 || prepared!=frames || updates!=60 ||
       seconds<1.90 || seconds>2.30 || recovered_early_draws<8 || draws_after_stall!=0) return 1;
    return 0;
}
