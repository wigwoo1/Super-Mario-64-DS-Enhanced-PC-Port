#include "hal/stage_options_input.h"
#include "PlayerInput.h"
#include "dScStage_c.h"
#include "ntr/input.h"
#include <cstdio>
#include <cstdlib>
#include "ntr/rt.h"
extern "C" {
extern unsigned char data_0209f1ec;
extern unsigned short data_020a0e5a[];
int pc_stage_options_take_action();
extern unsigned char data_0209f2ec;
int GetSoundMode();
}
namespace pc_stage_options {
Frame::Frame() {
    if(data_0209f1ec!=10 || gActivePlayerSlot>=4)return;
    slot=unsigned(gActivePlayerSlot)*4;
    keys=data_020a0e5a[slot/2];
    data_020a0e5a[slot/2]&=static_cast<unsigned short>(~0xffu);
    if(ntr::input_state().touch)return; // Preserve stylus; don't leak a pad confirm.
    const int action=pc_stage_options_take_action();
    if(action==0)return;
    saved[0]=gTouchHeld[slot];saved[1]=gTouchEdge[slot];
    saved[2]=gTouchX[slot];saved[3]=gTouchY[slot];
    gTouchHeld[slot]=gTouchEdge[slot]=1;
    if(action==2) { gTouchX[slot]=0x30; gTouchY[slot]=0x60; }
    else { gTouchX[slot]=static_cast<unsigned char>(action<0?0x60:0xec); gTouchY[slot]=0x30; }
    injected=true;
    if(const char *path=std::getenv("SM64DS_PC_STAGE_OPTIONS_TRACE")) {
        static FILE *file=nullptr;
        if(!file) {file=std::fopen(path,"a");}
        if(file) {std::fprintf(file,"%llu,10,%u,0,%d,%d,%d,%u,0\n",static_cast<unsigned long long>(ntr::rt_frame()),unsigned(data_0209f1ec==10?0:1),unsigned(keys),action,GetSoundMode(),unsigned(data_0209f2ec),0u);std::fflush(file);}
    }
}
Frame::~Frame() {
    if(slot>=16)return;
    if(injected) {
        gTouchHeld[slot]=saved[0];gTouchEdge[slot]=saved[1];
        gTouchX[slot]=saved[2];gTouchY[slot]=saved[3];
    }
    data_020a0e5a[slot/2]=keys;
}
}
