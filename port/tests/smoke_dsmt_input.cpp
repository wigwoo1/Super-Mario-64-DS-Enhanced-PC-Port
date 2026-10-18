#include "hal/dsmt_scene.h"
#include "ntr/input.h"
#include "ntr/menu_navigation.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
extern "C" {
unsigned char data_020a0f10;
unsigned char *data_ov007_0210342c=nullptr;
unsigned int data_ov007_020d77dc[24][3]={};
int data_ov007_02102ddc[7]={};
alignas(4) unsigned char data_020a1154[4*0x24];
alignas(4) unsigned char data_020a1162[4*0x24];
static unsigned short received[4];static int received_buttons;
int func_ov007_020b7090(unsigned short a,unsigned short b,unsigned short c,unsigned short d,int keys) {
    received[0]=a;received[1]=b;received[2]=c;received[3]=d;received_buttons=keys;
    return 3+data_020a0f10;
}
}
int main() {
    for(int index=0;index<4;++index) {
        unsigned short sample[4]={static_cast<unsigned short>(20+index),static_cast<unsigned short>(80+index),1,0};
        unsigned short keys=static_cast<unsigned short>(1u<<index);
        std::memcpy(data_020a1154+index*0x24+4,sample,sizeof(sample));
        std::memcpy(data_020a1162+index*0x24,&keys,2);
    }
    for(int index=3;index>=1;--index) {
        data_020a0f10=static_cast<unsigned char>(index);
        if(pc_dsmt_input_update()!=3+index || received[0]!=20+index || received[1]!=80+index ||
           received[2]!=1 || received[3]!=0 || received_buttons!=(1<<index))return 1;
    }
    data_020a0f10=0;
    _putenv_s("SM64DS_PC_AUTOMATION_ONLY","1");
    _putenv_s("SM64DS_PC_AUTOCONFIRM_POLL","1");
    ntr::input_poll();
    ntr::input_set_touch(true,128,96);
    if(pc_dsmt_input_update()!=3 || received[0]!=128 || received[1]!=96 ||
       received[2]!=1 || received[3]!=0 || received_buttons!=1)return 2;
    ntr::input_set_touch(false,128,96);
    pc_dsmt_input_update();
    if(received[2]!=0)return 3;
    const ntr::MenuTarget files[]={{1,20,50,80,110},{2,96,50,156,110},{3,172,50,232,110},{4,20,145,80,180}};
    if(ntr::menu_focus_move(files,4,1,1,0)!=2 || ntr::menu_focus_move(files,4,2,1,0)!=3 ||
       ntr::menu_focus_move(files,4,2,-1,0)!=1 || ntr::menu_focus_move(files,4,1,0,1)!=4 ||
       ntr::menu_focus_move(files,4,4,0,-1)!=1)return 4;
    // Exercise the production adapter with the original menu object layout.
    alignas(4) unsigned char context[0x190]={},objects[4][0x2c]={},placement[4][16]={};
    short root[8]={5},sub[8]={0},states[4][8]={};
    int rectangles[4][5]={{0,20,50,80,110},{0,100,50,160,110},{0,180,50,240,110},{0,180,140,240,180}};
    void *entries[4][2]={};
    *reinterpret_cast<void **>(context+4)=root;*reinterpret_cast<void **>(context+8)=sub;
    for(int i=0;i<4;++i) {
        *reinterpret_cast<unsigned short *>(objects[i])=static_cast<unsigned short>(i==3?20:14+i);
        *reinterpret_cast<void **>(objects[i]+4)=states[i];
        *reinterpret_cast<int *>(objects[i]+0x10)=4096;
        *reinterpret_cast<void **>(objects[i]+0x28)=rectangles[i];
        entries[i][0]=objects[i];entries[i][1]=placement[i];
        *reinterpret_cast<void **>(context+0x114+4*i)=entries[i];
    }
    data_ov007_0210342c=context;data_ov007_02102ddc[1]=4;
    _putenv_s("SM64DS_PC_AUTOCONFIRM_POLL","0");
    ntr::input_poll();pc_dsmt_input_update();
    _putenv_s("SM64DS_PC_TEST_STICK_X","25000");
    ntr::input_poll();pc_dsmt_input_update();
    if(received_buttons || received[2])return 5;
    _putenv_s("SM64DS_PC_TEST_STICK_X","0");
    ntr::input_poll();pc_dsmt_input_update();
    _putenv_s("SM64DS_PC_AUTOCONFIRM_POLL","5");
    ntr::input_poll();pc_dsmt_input_update();
    if(received_buttons || received[2]!=1 || received[0]!=130 || received[1]!=80)return 6;
    ntr::input_poll();pc_dsmt_input_update();
    if(received[2]!=1 || received[0]!=130)return 7; // stable multi-frame tap
    ntr::input_set_touch(true,30,40);pc_dsmt_input_update();
    if(received[0]!=30 || received[1]!=40 || received[2]!=1)return 8;
    _putenv_s("SM64DS_PC_AUTOCONFIRM_POLL","0");
    _putenv_s("SM64DS_PC_TEST_BUTTONS","7:2");
    ntr::input_poll();pc_dsmt_input_update();
    if(received_buttons || received[2]!=1 || received[0]!=210 || received[1]!=160)return 9;
    root[0]=0;sub[0]=11;
    for(int i=0;i<4;++i)*reinterpret_cast<void **>(context+0x114+4*i)=nullptr;
    _putenv_s("SM64DS_PC_TEST_BUTTONS","8:8");
    ntr::input_poll();pc_dsmt_input_update();
    if(received_buttons || received[2]!=1 || received[0]!=128 || received[1]!=96)return 10;
    data_ov007_0210342c=nullptr;
    std::puts("PASS: original menu reads touch and buttons from the selected original input record");
}
