#include "dScStage_c.h"
#include "PlayerInput.h"
#include "ntr/input.h"
#include "ntr/video.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

extern "C" {
extern unsigned char data_0209f498[],data_0209f49c[],data_0209f49e[];
extern unsigned char data_0209f4a0[],data_0209f4a2[],data_0209f4a4[],data_0209f4a6[];
extern unsigned char data_0209f4a8[],data_0209f4a9[],data_0209f4ab[],data_0209f4ac[],data_0209f4ae[];
extern unsigned char pc_controller_state_end[],data_020a0e58[];
unsigned char gTouchHeld[16],gTouchEdge[16],gTouchX[16],gTouchY[16],gActivePlayerSlot;
unsigned char data_0209f2d8=0,data_0209d660=0,data_0209f21c=4,data_0209f350[4]={},data_0209f2c4=0;
int data_0209caa0[4]={0,0,0x80,0},data_0209fc48=0;
short data_020756b0[16],data_02082214[8192];
unsigned short keymap[16],*data_0209214c[3]={keymap,keymap,keymap};
// Numeric fixtures isolate the production CheckInput record writer from the
// game's full heap/actor closure. The ntr stick normalization remains real.
unsigned char DecIfAbove0_Byte(unsigned char *v) { if(*v)--*v;return *v; }
int _ZN4cstd4sqrtEy(unsigned long long v) { return static_cast<int>(std::sqrt(static_cast<double>(v))); }
int _ZN4cstd4fdivEii(int a,int b) { return static_cast<int>(static_cast<int64_t>(a)*4096/b); }
short _ZN4cstd5atan2E5Fix12IiES1_(int y,int x) { return static_cast<short>(std::lround(std::atan2(double(y),double(x))*65536/6.283185307179586)); }
}
namespace {
int failures;
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
// COFF interior labels intentionally alias. Keep observations in runtime
// memory accesses rather than C++ assumptions about distinct extern arrays.
__declspec(noinline) uintptr_t address(const void *p) {return reinterpret_cast<uintptr_t>(p);}
short field(const unsigned char *p,unsigned player=0) {
    const volatile unsigned char *v=p+player*24;return static_cast<short>(v[0]|(v[1]<<8));
}
void word(unsigned char *p,unsigned player,short value) {
    volatile unsigned char *v=p+player*24;v[0]=static_cast<unsigned char>(value);v[1]=static_cast<unsigned char>(value>>8);
}
}
int main() {
    const auto base=address(data_0209f498);
    const struct {unsigned char *p;unsigned offset;} labels[]={
        {data_0209f49c,4},{data_0209f49e,6},{data_0209f4a0,8},{data_0209f4a2,10},
        {data_0209f4a4,12},{data_0209f4a6,14},{data_0209f4a8,16},{data_0209f4a9,17},
        {data_0209f4ab,19},{data_0209f4ac,20},{data_0209f4ae,22}};
    for(const auto &label:labels) CHECK(address(label.p)==base+label.offset);
    CHECK(address(pc_controller_state_end)==base+96);
    for(unsigned player=0;player<4;++player) {
        word(data_0209f49c,player,short(0x100+player));
        word(data_0209f49e,player,short(0x200+player));
        word(data_0209f4a0,player,short(0x300+player));
        CHECK(field(data_0209f498+4,player)==0x100+player);
        CHECK(field(data_0209f498+6,player)==0x200+player);
        CHECK(field(data_0209f498+8,player)==0x300+player);
    }
    std::memset(data_0209f498,0,96);std::memset(data_020a0e58,0,16);
    for(unsigned i=0;i<16;++i) keymap[i]=static_cast<unsigned short>(1u<<i);
    _putenv_s("SM64DS_PC_AUTOMATION_ONLY","1");
    _putenv_s("SM64DS_PC_AUTOCONFIRM_POLL","0");_putenv_s("SM64DS_PC_AUTODOWN_POLL","0");
    _putenv_s("SM64DS_PC_TEST_BUTTONS","");
    _putenv_s("SM64DS_PC_TEST_STICK_X","20000");_putenv_s("SM64DS_PC_TEST_STICK_Y","0");
    ntr::input_poll();dScStage_c::CheckInput();
    CHECK(field(data_0209f4a0)>2550 && field(data_0209f4a0)<2750);
    CHECK(field(data_0209f49c)==0x800 && field(data_0209f49e)==0); // Run held, never a jump/attack press.
    CHECK(field(data_0209f4a2)>2550 && field(data_0209f4a4)==0);
    CHECK(field(data_0209f4a6)==0x4000 && data_0209f4ac[0]==1);
    for(unsigned p=1;p<4;++p) CHECK(field(data_0209f4a0,p)==0 && field(data_0209f49e,p)==0);
    // Exercise the actual four-record writer with independent raw key pairs.
    auto *raw=reinterpret_cast<unsigned short *>(data_020a0e58);
    raw[2]=raw[3]=2;raw[4]=raw[5]=4;raw[6]=raw[7]=8;
    dScStage_c::CheckInput();
    for(unsigned p=1;p<4;++p) CHECK(field(data_0209f49c,p)==(1u<<p) && field(data_0209f49e,p)==(1u<<p));
    CHECK(field(data_0209f49e)==0);
    // Deflections just outside the configured24% radial deadzone must clear
    // the retail joystick's stationary range.
    // Both Standard and Touch modes have different first nonzero speed steps.
    for(const char *inside: {"0","5553"}) {
        _putenv_s("SM64DS_PC_TEST_STICK_X",inside[0]=='0'?"7850":"5553");
        _putenv_s("SM64DS_PC_TEST_STICK_Y",inside);
        ntr::input_poll();dScStage_c::CheckInput();
        CHECK(field(data_0209f4a0)==0);
    }
    _putenv_s("SM64DS_PC_TEST_STICK_Y","0");
    for(unsigned mode: {0u,2u}) {
        data_0209f4ae[0]=static_cast<unsigned char>(mode);
        int previous=0;
        for(const char *sample: {"8000","9000","12000","20000","32767"}) {
            _putenv_s("SM64DS_PC_TEST_STICK_X",sample);
            ntr::input_poll();dScStage_c::CheckInput();
            int mag=field(data_0209f4a0);
            CHECK(mag>(mode==2?1450:1251) && mag>=previous && mag<=4096);
            CHECK(field(data_0209f4a2)==mag && field(data_0209f4a4)==0);
            CHECK(field(data_0209f49e)==0);previous=mag;
        }
        // Tiny diagonals must also clear the first speed step after the
        // original radius-105 integer vector/square-root quantization.
        for(const char *sample: {"5657","-5657"}) {
            _putenv_s("SM64DS_PC_TEST_STICK_X",sample);
            _putenv_s("SM64DS_PC_TEST_STICK_Y","5553");
            ntr::input_poll();dScStage_c::CheckInput();
            CHECK(field(data_0209f4a0)>(mode==2?1450:1251));
            CHECK(field(data_0209f4a4)<0 && field(data_0209f4a2)!=0);
            CHECK(field(data_0209f49e)==0);
        }
        _putenv_s("SM64DS_PC_TEST_STICK_Y","0");
    }
    data_0209f4ae[0]=0;
    // CheckCameraInput clears these two words before each real CheckInput.
    for(unsigned p=0;p<4;++p) {word(data_0209f49c,p,0);word(data_0209f49e,p,0);}
    std::memset(data_020a0e58,0,16);
    _putenv_s("SM64DS_PC_TEST_STICK_X","-32768");
    ntr::input_poll();dScStage_c::CheckInput();
    CHECK(field(data_0209f4a0)==4096 && field(data_0209f4a2)==-4096 && field(data_0209f49e)==0);
    _putenv_s("SM64DS_PC_TEST_STICK_X","0");
    word(data_0209f49c,0,0); // Production CheckCameraInput clears held each tick.
    ntr::input_poll();dScStage_c::CheckInput();
    CHECK(field(data_0209f4a0)==0 && field(data_0209f49c)==0 && field(data_0209f49e)==0);
    // Always Run acts after button mapping, on local gameplay only. A pause
    // must not synthesize Run even when the preference remains enabled.
    auto settings=ntr::video_get_settings();
    for(unsigned option: {0u,1u}) for(unsigned paused: {0u,1u}) {
        settings.always_run=option!=0;ntr::video_set_settings(settings);
        data_0209f2c4=static_cast<unsigned char>(paused);
        for(unsigned p=0;p<4;++p){word(data_0209f49c,p,0);word(data_0209f49e,p,0);}
        std::memset(data_020a0e58,0,16);raw[0]=0x10;
        dScStage_c::CheckInput();
        CHECK(field(data_0209f49c)==(0x10 | (option && !paused ? 0x800 : 0)));
        CHECK(field(data_0209f49e)==0);
        for(unsigned p=1;p<4;++p)CHECK(field(data_0209f49c,p)==0);
    }
    data_0209f2c4=0;
    std::memset(data_0209f498,0x55,96);data_0209caa0[2]=0;
    dScStage_c::CheckInput();
    for(unsigned p=0;p<4;++p) CHECK(field(data_0209f4a0,p)==0 && field(data_0209f49e,p)==0);
    if(failures)return 1;
    std::puts("PASS: four processed-controller records retain all interior labels; production CheckInput preserves partial/signed stick travel without phantom button presses, key lanes, release and intro gating");
}
