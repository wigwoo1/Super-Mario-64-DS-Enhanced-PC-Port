#pragma once
#include "hal/pc_pause_settings_state.h"
#include "ntr/menu_navigation.h"
#include <cstdint>

struct dScMiniGm_c;
struct dScMgBase_c;

namespace pc_touch_menu {
struct ReleaseLatch {
    bool blocked=false;
    void activate() {blocked=true;}
    uint16_t filter(uint16_t keys) {
        if(!(keys&3))blocked=false;
        return blocked?static_cast<uint16_t>(keys&~3u):keys;
    }
};
// Reuse title/save spatial focus and PC Settings' edge/repeat policy.
struct State {
    const void *owner=nullptr;
    int context=-1,focused=-1;
    uint16_t previous=0,repeat=0,edge=0;
    unsigned ticks=0;
    bool visible=false;
    int poll(const void *next_owner,int next_context,uint16_t keys,
             const ntr::MenuTarget *targets,int count,int back,bool touch,int tx,int ty) {
        if(owner!=next_owner || context!=next_context) {
            owner=next_owner;context=next_context;focused=-1;previous=keys;
            repeat=0;ticks=0;visible=true;
        }
        edge=pc_pause::events(keys,previous,repeat,ticks);
        if(ntr::menu_focus_index(targets,count,focused)<0)focused=count?targets[0].id:-1;
        if(touch) {
            visible=false;
            for(int i=0;i<count;++i)if(tx>=targets[i].left && tx<=targets[i].right &&
                ty>=targets[i].top && ty<=targets[i].bottom) {focused=targets[i].id;break;}
            return -1;
        }
        if(edge&0xf3)visible=true;
        const int dx=(edge&0x10?1:0)-(edge&0x20?1:0);
        const int dy=(edge&0x80?1:0)-(edge&0x40?1:0);
        if(dx || dy)focused=ntr::menu_focus_move(targets,count,focused,dx,dy);
        if(edge&2) {
            if(ntr::menu_focus_index(targets,count,back)>=0){focused=back;return back;}
            return -1;
        }
        return edge&1?focused:-1;
    }
};

// A controller activation temporarily supplies the same four-byte touch record
// consumed by retail menu handlers, then restores the real input on every exit.
class Frame {
public:
    explicit Frame(dScMiniGm_c *);
    explicit Frame(dScMgBase_c *);
    ~Frame();
    Frame(const Frame &)=delete;
    Frame &operator=(const Frame &)=delete;
    uint16_t consumed_keys() const {return consumed;}
private:
    uint16_t consumed=0;
    unsigned slot=0;
    unsigned char saved[4]={};
    bool injected=false;
    void apply(const void *,int,const ntr::MenuTarget *,int,int,int page=-1);
};
void reset();
}
extern "C" void pc_touch_menus_prepare();
