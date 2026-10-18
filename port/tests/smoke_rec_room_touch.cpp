// Exercise the actual generated Rec Room Back handler with the original
// touch-history writer and a native fader vtable whose start/end differ.
#include "dScMiniGm_c.h"
#include "dFader_c.h"
#include "ntr/input.h"
#include "ntr/mmio.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
dFader_c::~dFader_c() {}
struct TestFader : dFader_c {
    bool start=true;
    unsigned starts=0,ends=0;
    void AdvanceFade() override {}
    int SetBackwardTime(u32) override {return 0;}
    int SetForwardTime(u32) override {return 0;}
    int IsAtStart() override {++starts;return start;}
    int IsAtEnd() override {++ends;return !start;}
    int IsBetweenStartAndEnd() override {return 0;}
    void SetToEnd() override {}
    void SetToStart() override {}
};
extern "C" {
void _ZN6Memory16operator_delete2EPv(void *) {std::abort();}
dFader_c *data_0209f5bc;
unsigned char gActivePlayerSlot=0;
static unsigned sounds=0;
int func_02012790(int sound) {if(sound!=0x63)std::abort();++sounds;return 0;}
void func_ov005_020c0250(char *);
void func_0203bb60();
struct Sample {unsigned short x,y,pressed;};
Sample *func_0203dabc(int) {static Sample empty{};return &empty;}
}
int main() {
    if(!ntr::io_init()) return 1;
    // The handler only reads/writes scalar scene fields; no scene lifecycle or
    // game allocator is involved in this isolated ABI/input test.
    alignas(dScMiniGm_c) unsigned char storage[sizeof(dScMiniGm_c)]={};
    auto *scene=reinterpret_cast<dScMiniGm_c *>(storage);
    TestFader fader;data_0209f5bc=&fader;
    auto sample=[](bool pressed,unsigned x,unsigned y) {ntr::input_set_touch(pressed,x,y);func_0203bb60();};
    auto back=[&]() {func_ov005_020c0250(reinterpret_cast<char *>(scene));};
    sample(true,232,160);back();
    if(scene->mExitTimer!=28 || sounds!=1 || fader.starts!=1 || fader.ends) return 2;
    // Holding the pen cannot enqueue a second exit once the old timer is clear.
    scene->mExitTimer=0;sample(true,232,160);back();
    if(scene->mExitTimer || sounds!=1) return 3;
    sample(false,232,160);sample(true,100,20);back();
    if(scene->mExitTimer || sounds!=1) return 4;
    // At the opposite fade endpoint, a fresh valid tap must be gated out.
    fader.start=false;sample(false,232,160);sample(true,232,160);back();
    if(scene->mExitTimer || sounds!=1 || fader.ends) return 5;
    // The real page/transition guards still suppress otherwise valid taps.
    fader.start=true;scene->mPageFlipped=1;sample(false,232,160);sample(true,232,160);back();
    if(scene->mExitTimer || sounds!=1) return 6;
    std::puts("Rec Room Back: native fade-start gate, canonical touch edge, bounds, hold and transition guards pass");
    return 0;
}
