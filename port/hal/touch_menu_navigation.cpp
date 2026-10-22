#include "hal/touch_menu_navigation.h"
#include "hal/wanted_pause_settings.h"
#include "dScMiniGm_c.h"
#include "dScMgBase_c.h"
#include "dFader_c.h"
#include "PlayerInput.h"
#include "ntr/input.h"
#include "ntr/video.h"
#include "ntr/rt.h"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

extern "C" {
extern int data_0208a170,data_0209b304,data_ov004_020bc150;
extern dFader_c *data_0209f5bc;
extern unsigned char data_ov004_020bf648[];
int func_ov005_020c00b4(void *,int);
int func_ov005_020c00e4(void *);
int GetSoundMode();
extern unsigned char data_0208ee3c;
void pc_mg_dMgState_c_func_ov004_020b6ddc(void *);
void pc_mg_dMgState_c_func_ov004_020b6f88(void *);
}
namespace pc_touch_menu {
namespace {
State navigation;
ReleaseLatch release;
ntr::MenuTarget focus{};
bool live=false,owns_focus=false,menu_context=false;
uint64_t last_tick=~uint64_t(0);
struct RegistryButton {
    unsigned char pad[0x10];short x,y;
    unsigned char pad14[0xc];int kind;
    unsigned char pad24[0xe];unsigned short enabled;
    unsigned char tail[0x100];
};
static_assert(sizeof(RegistryButton)==0x134 && offsetof(RegistryButton,enabled)==0x32,"shared button layout");
void add(ntr::MenuTarget *items,int &count,int id,int x,int y,int hw,int hh) {
    if(hw>0 && hh>0 && x+hw>=0 && y+hh>=0 && x-hw<=255 && y-hh<=191)
        items[count++]={id,x-hw,y-hh,x+hw,y+hh};
}
}
void reset() {navigation=State{};live=false;menu_context=false;last_tick=~uint64_t(0);}

void Frame::apply(const void *owner,int context,const ntr::MenuTarget *items,int count,int back,int page) {
    menu_context=context!=0;last_tick=ntr::rt_frame();live=false;
    if(menu_context)ntr_input_set_menu_navigation(1);
    uint16_t keys=0;ntr_input_get_ui_keys(&keys);
    // Mapping can swap A/B on a menu/gameplay transition. A held opening
    // button must not become a fresh confirm/back in the destination scene.
    if((navigation.owner!=owner || navigation.context!=context) && (keys&3))release.activate();
    const uint16_t ui_keys=release.filter(keys);
    if(context==0 && release.blocked)consumed|=3;
    slot=unsigned(gActivePlayerSlot)*4;
    if(slot>=16)return;
    const bool touch=gTouchHeld[slot]!=0;
    int chosen=navigation.poll(owner,context,ui_keys,items,count,back,touch,gTouchX[slot],gTouchY[slot]);
    if(!touch && chosen<0 && page>=0 &&
       ((page==0 && (navigation.edge&0x100)) || (page==1 && (navigation.edge&0x200)))) {
        if(ntr::menu_focus_index(items,count,1002)>=0) {chosen=1002;navigation.focused=chosen;navigation.visible=true;}
    }
    const int selected=ntr::menu_focus_index(items,count,navigation.focused);
    if(selected>=0 && navigation.visible) {focus=items[selected];live=true;}
    const int activated=ntr::menu_focus_index(items,count,chosen);
    if(activated>=0 && !touch) {
        saved[0]=gTouchHeld[slot];saved[1]=gTouchEdge[slot];saved[2]=gTouchX[slot];saved[3]=gTouchY[slot];
        const auto &target=items[activated];
        gTouchHeld[slot]=1;gTouchEdge[slot]=1;
        gTouchX[slot]=static_cast<unsigned char>(std::clamp((target.left+target.right)/2,0,255));
        gTouchY[slot]=static_cast<unsigned char>(std::clamp((target.top+target.bottom)/2,0,191));
        injected=true;
        if(keys&3)release.activate();
    }
    if(const char *path=std::getenv("SM64DS_PC_MENU_NAV_TRACE")) {
        static FILE *file=nullptr;
        if(!file && path[0]){file=std::fopen(path,"w");if(file)std::fputs("tick,context,focus,count,keys,chosen,synthetic,x,y,sound,backlight\n",file);}
        if(file && (navigation.edge || injected || ntr::rt_frame()%30==0)) {
            std::fprintf(file,"%llu,%d,%d,%d,%u,%d,%d,%u,%u,%d,%u\n",static_cast<unsigned long long>(ntr::rt_frame()),
                context,navigation.focused,count,keys,chosen,int(injected),gTouchX[slot],gTouchY[slot],GetSoundMode(),unsigned(data_0208ee3c));std::fflush(file);
        }
    }
}
Frame::~Frame() {
    if(injected) {gTouchHeld[slot]=saved[0];gTouchEdge[slot]=saved[1];gTouchX[slot]=saved[2];gTouchY[slot]=saved[3];}
}
Frame::Frame(dScMiniGm_c *scene) {
    ntr::MenuTarget items[16];int count=0;
    const int page=data_0209b304!=0;
    if(data_0209f5bc && data_0209f5bc->IsAtStart() && !scene->mExiting && !scene->mPageFlipped &&
       !scene->mPrevPageTimer && !scene->mNextPageTimer && !scene->mExitTimer && !scene->mScrollDelay) {
        const int first=page?6:0,last=page?9:6;
        for(int i=first;i<last;++i) {
            const int entry=data_0208a170+i*4;
            if(func_ov005_020c00b4(scene,entry))
                add(items,count,100+entry,(page?120:88)+(i%3)*48,page?92:60+(i/3)*72,23,35);
        }
        for(int i=0;i<4;++i)add(items,count,1000+i+10,24,24+i*48,22,22);
        if(page || func_ov005_020c00e4(scene))add(items,count,1002,page?72:232,96,16,32);
        add(items,count,1003,232,160,16,16);
    }
    apply(scene,100+data_0208a170*2+page,items,count,1003,page);
}
Frame::Frame(dScMgBase_c *scene) {
    ntr::MenuTarget items[16];int count=0,context=0,back=-1;
    const bool ready=data_0209f5bc && data_0209f5bc->IsAtStart();
    if(scene->mMenuOpen) {
        consumed=3;
        if(scene->mTouchOptions.mActive) {
            context=201;back=5;
            if(ready && scene->mTouchOptions.mActive==1) {
                for(int i: {2,0,1,6,3,4,7,5}) {
                    const auto &icon=scene->mTouchOptions.mIcons[i];
                    // Readiness gates activation in the retail handler; keep
                    // focus stable while its short press animation settles.
                    add(items,count,i,icon.unk_004,icon.unk_006,icon.unk_008,icon.unk_00a);
                }
            }
        } else {
            context=200;back=0;
            if(ready && scene->mMenuCursor<0) {
                add(items,count,0,scene->mMenuItem0X,scene->mMenuItem0Y,95,23);
                add(items,count,1,scene->mMenuItem1X,scene->mMenuItem1Y,63,18);
                add(items,count,2,scene->mMenuItem2X,scene->mMenuItem2Y,63,18);
            }
        }
    } else if(scene->mStateController.mState>=0) {
        // Retail score/save notices wait for any fresh touch before creating
        // Retry/Return buttons. They have no entry in the button registry.
        const auto behavior=scene->mStateController.mBehavior;
        const bool notice=behavior.function==pc_mg_dMgState_c_func_ov004_020b6ddc ||
                          behavior.function==pc_mg_dMgState_c_func_ov004_020b6f88;
        auto *buttons=reinterpret_cast<const RegistryButton *>(data_ov004_020bf648);
        for(int i=0;i<3;++i)if(ready && data_ov004_020bc150 && buttons[i].enabled && buttons[i].kind>=0 && buttons[i].kind<=2) {
            add(items,count,buttons[i].kind,buttons[i].x,buttons[i].y,64,19);
            if(buttons[i].kind==2 || (buttons[i].kind==1 && back<0))back=buttons[i].kind;
        }
        if(ready && notice && scene->mStateController.mTimer==0) {
            add(items,count,900,128,168,80,19);
        }
        if(count || scene->mStateController.mState==18) {context=300+scene->mStateController.mState;consumed=3;}
    }
    apply(scene,context,items,count,back);
}
}
extern "C" void pc_touch_menus_prepare() {
    using namespace pc_touch_menu;
    if(pc_wanted_settings_page())reset();
    const bool current=last_tick!=~uint64_t(0) && ntr::rt_frame()>=last_tick && ntr::rt_frame()-last_tick<=3 && !pc_wanted_settings_page();
    if(current && menu_context)ntr_input_set_menu_navigation(1);
    if(current && live) {ntr::video_set_menu_focus(focus.left,focus.top,focus.right,focus.bottom);owns_focus=true;}
    else if(owns_focus) {ntr::video_set_menu_focus(-1,0,0,0);owns_focus=false;}
}
