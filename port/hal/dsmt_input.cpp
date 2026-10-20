#include "hal/dsmt_scene.h"
#include "ntr/input.h"
#include "ntr/video.h"
#include "ntr/rt.h"
#include "ntr/menu_navigation.h"
#include <cstdio>
#include <cstdlib>
// local extern: recovered decl_common.h omits the index arguments; these
// declarations match the linked C definitions and the native cdecl ABI.
extern "C" {
int func_0203da9c(void);
int *func_0203dabc(int index);
int func_0203dae4(int index);
int func_ov007_020b7090(unsigned short,unsigned short,unsigned short,unsigned short,int);
extern unsigned char *data_ov007_0210342c;
extern unsigned int data_ov007_020d77dc[24][3];
extern int data_ov007_02102ddc[];
// local extern: no shared include declares these original option-state globals.
int GetSoundMode();
extern unsigned char data_0208ee3c;
}

static void controller_menu_touch(unsigned short &keys,unsigned short &x,unsigned short &y,bool &touch) {
    static unsigned short previous=0;
    static int focused=-1,held_frames=0,last_layout=-1;
    static int tap_frames=0;static unsigned short tap_x=0,tap_y=0;
    static bool consume_back=false;
    static bool visible=false;
    auto *context=data_ov007_0210342c;
    if(!context)return;
    auto *root=*reinterpret_cast<short **>(context+4);
    auto *sub=*reinterpret_cast<short **>(context+8);
    if(!root || !sub)return;
    // A modal dialog uses the same root/submenu as its parent. Treat entering
    // or leaving that dialog as a new focus context, not a widget animation.
    const int layout=(root[0]<<16)|static_cast<unsigned short>(sub[0])|
        (data_ov007_02102ddc[1]!=4?0x40000000:0);
    if(layout!=last_layout){focused=-1;last_layout=layout;}
    const unsigned short edge=keys&~previous;previous=keys;
    if(!(keys&2))consume_back=false;
    const int dx=(keys&0x10?1:0)-(keys&0x20?1:0);
    const int dy=(keys&0x80?1:0)-(keys&0x40?1:0);
    const bool directional=dx||dy;
    const bool navigate=directional && ((edge&0xf0) || (++held_frames>=24 && held_frames%8==0));
    if(!directional)held_frames=0;
    if(touch){visible=false;tap_frames=0;}
    else if(edge&(0xf0|1|8))visible=true;
    ntr::MenuTarget targets[24];int count=0;
    for(int i=0;i<24;++i) {
        auto **entry=*reinterpret_cast<unsigned char ***>(context+0x114+i*4);
        if(!entry || !entry[0] || !entry[1])continue;
        auto *object=entry[0];auto *state=*reinterpret_cast<short **>(object+4);
        const unsigned id=*reinterpret_cast<unsigned short *>(object);
        if(id>=24 || !state || state[0]!=0)continue;
        if(data_ov007_02102ddc[1]!=4 && id!=22 && id!=23)continue;
        const auto flags=data_ov007_020d77dc[id][0];
        if((flags&1) || (*reinterpret_cast<int *>(object+0x10)<0x1000 && !(flags&2)))continue;
        auto *rect=*reinterpret_cast<int **>(object+0x28);
        if(!rect)continue;
        const int px=*reinterpret_cast<int *>(entry[1]+4)>>12,py=*reinterpret_cast<int *>(entry[1]+8)>>12;
        ntr::MenuTarget item={int(id),px+rect[1],py+rect[2],px+rect[3],py+rect[4]};
        if(item.right<0 || item.bottom<0 || item.left>255 || item.top>191 || item.right<=item.left || item.bottom<=item.top)continue;
        targets[count++]=item;
    }
    focused=ntr::menu_focus_initialize(targets,count,focused);
    if(navigate)focused=ntr::menu_focus_move(targets,count,focused,dx,dy);
    const int index=ntr::menu_focus_index(targets,count,focused);
    if(!touch && (edge&(1|8))) {
        if(root[0]==0 && (sub[0]==0 || sub[0]==11) && !count) {tap_x=128;tap_y=96;tap_frames=5;}
        else if(index>=0) {
            tap_x=static_cast<unsigned short>((targets[index].left+targets[index].right)/2);
            tap_y=static_cast<unsigned short>((targets[index].top+targets[index].bottom)/2);tap_frames=5;
        }
    }
    if(!touch && (edge&2)) {
        // Use the original visible Back/Cancel button when one is present.
        // Dialogs expose No/Cancel as item23. File-menu item20 is Return
        // (020b50e8 dispatches it according to the current file submenu),
        // and the other options use item8. Item21 is File Options, not Back.
        const int back_ids[]={data_ov007_02102ddc[1]!=4?23:-1,20,8};
        for(int id:back_ids) {
            const int back=ntr::menu_focus_index(targets,count,id);
            if(back<0)continue;
            tap_x=static_cast<unsigned short>((targets[back].left+targets[back].right)/2);
            tap_y=static_cast<unsigned short>((targets[back].top+targets[back].bottom)/2);
            tap_frames=5;consume_back=true;break;
        }
    }
    if(!touch && tap_frames>0){x=tap_x;y=tap_y;touch=true;--tap_frames;}
    if(visible && index>=0) {
        const auto &target=targets[index];
        ntr::video_set_menu_focus(target.left,target.top,target.right,target.bottom);
    } else ntr::video_set_menu_focus(-1,0,0,0);
    // The PC focus adapter owns directional navigation and confirmation.
    // Feeding those same keys to DSMT's keyboard/stylus cursor path would
    // process each action twice. B remains available to the native back flow.
    keys&=static_cast<unsigned short>(~0xf9u);
    if(consume_back)keys&=static_cast<unsigned short>(~2u);
    if(const char *path=std::getenv("SM64DS_PC_MENU_CAPTURE")) {
        static FILE *file=nullptr;
        if(!file){file=std::fopen(path,"w");if(file)std::fputs("tick,root,sub,dialog,focus,count,keys,touch,x,y,sound,backlight\n",file);}
        if(file && (ntr::rt_frame()%10==0 || edge || touch)) {
            std::fprintf(file,"%llu,%d,%d,%d,%d,%d,%u,%u,%u,%u,%d,%u\n",static_cast<unsigned long long>(ntr::rt_frame()),root[0],sub[0],data_ov007_02102ddc[1],focused,count,keys,touch?1u:0u,x,y,GetSoundMode(),unsigned(data_0208ee3c));
            std::fflush(file);
        }
    }
}
int pc_dsmt_input_update() {
    const int index=func_0203da9c();
    if(index==0) {
        // Match the native player-1 substitution in the normal key-history
        // path. The wireless records are not populated by host mouse/XInput.
        const auto input=ntr::input_state();
        unsigned short keys=0;
        ntr_input_set_menu_navigation(1);
        ntr_input_get_menu_keys(&keys);
        unsigned short x=input.touch_x,y=input.touch_y;bool touch=input.touch;
        controller_menu_touch(keys,x,y,touch);
        return func_ov007_020b7090(x,y,touch?1:0,0,keys);
    }
    const auto *sample=reinterpret_cast<const unsigned short *>(func_0203dabc(index));
    const int buttons=func_0203dae4(index);
    return func_ov007_020b7090(sample[0],sample[1],sample[2],sample[3],buttons);
}
