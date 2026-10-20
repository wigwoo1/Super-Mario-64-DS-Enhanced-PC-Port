#include "hal/course_clear_input.h"
#include "dScStage_c.h"
#include "PlayerInput.h"
#include "ntr/input.h"
#include "ntr/rt.h"
#include <cstdio>
#include <cstdlib>
extern "C" {
// local extern: PlayerInput.h documents the stride but does not declare keys.
extern unsigned short data_020a0e5a[];
extern unsigned char data_0209f20c,data_0209f2d4,data_0209f2e0,data_0209f22c;
}
namespace pc_course_clear {
namespace {State navigation;}
Frame::Frame() {
    slot=unsigned(gActivePlayerSlot)*4;
    if(slot>=16)return;
    phase_before=data_0209f2d4;selection_before=data_0209f2e0;delay_before=data_0209f22c;
    ready=data_0209f20c && phase_before==3 && !delay_before;
    ntr_input_get_menu_keys(&keys);
    real_touch=gTouchHeld[slot]!=0;
    const auto result=navigation.poll(ready,keys,selection_before,real_touch);
    // Consume only this scoped menu's pad bits, avoiding the retail rejection
    // beep and stale released-touch coordinates overriding pad selection.
    pressed=data_020a0e5a[slot/2];
    data_020a0e5a[slot/2]&=static_cast<unsigned short>(~0xffu);
    if(ready && !real_touch && result.selection!=selection_before) {
        data_0209f2e0=static_cast<unsigned char>(result.selection);
        dScStage_c::UpdateMenuButtons(false);
    }
    activated=result.activate;
    if(ready && activated>=0 && !real_touch) {
        saved[0]=gTouchHeld[slot];saved[1]=gTouchEdge[slot];saved[2]=gTouchX[slot];saved[3]=gTouchY[slot];
        gTouchHeld[slot]=gTouchEdge[slot]=1;
        gTouchX[slot]=128;gTouchY[slot]=static_cast<unsigned char>(56+40*activated);
        injected=true;
    }
}
Frame::~Frame() {
    if(slot>=16)return;
    if(injected) {gTouchHeld[slot]=saved[0];gTouchEdge[slot]=saved[1];gTouchX[slot]=saved[2];gTouchY[slot]=saved[3];}
    data_020a0e5a[slot/2]=pressed;
    if(const char *path=std::getenv("SM64DS_PC_COURSE_CLEAR_TRACE")) {
        static FILE *file=nullptr;
        if(!file && path[0]){file=std::fopen(path,"w");if(file)std::fputs("tick,phase_before,phase_after,selection_before,selection_after,keys,activate,real_touch,ready,delay_before,active_after\n",file);}
        if(file && (keys || activated>=0 || phase_before!=data_0209f2d4 || selection_before!=data_0209f2e0 || ntr::rt_frame()%30==0)) {
            std::fprintf(file,"%llu,%u,%u,%u,%u,%u,%d,%d,%d,%u,%u\n",static_cast<unsigned long long>(ntr::rt_frame()),phase_before,unsigned(data_0209f2d4),selection_before,unsigned(data_0209f2e0),unsigned(keys),activated,int(real_touch),int(ready),delay_before,unsigned(data_0209f20c));
            std::fflush(file);
        }
    }
}
}
