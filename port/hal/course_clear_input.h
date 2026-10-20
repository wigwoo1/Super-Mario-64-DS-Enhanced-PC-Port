#pragma once
#include "hal/touch_menu_navigation.h"
namespace pc_course_clear {
struct Result {unsigned selection;int activate=-1;};
struct State {
    uint16_t previous=0,repeat=0;
    unsigned ticks=0;
    bool was_ready=false;
    pc_touch_menu::ReleaseLatch release;
    Result poll(bool ready,uint16_t keys,unsigned selected,bool touch) {
        Result out{selected<3?selected:0};
        if(!ready) {
            previous=keys;repeat=0;ticks=0;was_ready=false;
            if(keys&3)release.activate();
            return out;
        }
        if(!was_ready) {
            was_ready=true;previous=keys;
            if(keys&3)release.activate();
        }
        const auto edge=pc_pause::events(release.filter(keys),previous,repeat,ticks);
        if(touch)return out;
        if(edge&0x40)out.selection=(out.selection+2)%3;
        if(edge&0x80)out.selection=(out.selection+1)%3;
        // B backs out through the original Continue (without save) action.
        if(edge&2)out.activate=2;
        else if(edge&0xd)out.activate=static_cast<int>(out.selection);
        if(out.activate>=0){out.selection=unsigned(out.activate);release.activate();}
        return out;
    }
};
class Frame {
public:
    Frame();
    ~Frame();
    Frame(const Frame&)=delete;
    Frame& operator=(const Frame&)=delete;
private:
    unsigned slot=16,selection_before=0,phase_before=0,delay_before=0;
    uint16_t pressed=0,keys=0;
    unsigned char saved[4]={};
    bool injected=false,ready=false,real_touch=false;
    int activated=-1;
};
}
