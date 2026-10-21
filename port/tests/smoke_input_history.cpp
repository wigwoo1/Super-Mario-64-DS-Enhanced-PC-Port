#include <cstdint>
#include <cstring>
#include <cstdio>
#include <initializer_list>
extern "C" {
extern unsigned char data_020a1040[], data_020a1044[], data_020a104e[], data_020a1050[], data_020a1052[], pc_raw_input_end[];
extern unsigned char data_020a1154[], data_020a1160[], data_020a1162[], data_020a1164[], data_020a1166[], data_020a1178[], data_020a117c[], data_020a11a0[], data_020a11c4[], pc_input_history_end[];
void func_0203dafc(int angle);
void func_0203e0ac();
short GetAngleToCamera(int player);
void pc_camera_set_movement_yaw_offset(short offset, int active);
unsigned char GetPlayerFlagByte(int player,int index);
}
int main() {
    const uintptr_t raw=reinterpret_cast<uintptr_t>(data_020a1040);
    const uintptr_t history=reinterpret_cast<uintptr_t>(data_020a1154);
    if(reinterpret_cast<uintptr_t>(data_020a1050)!=raw+0x10 || reinterpret_cast<uintptr_t>(pc_raw_input_end)!=raw+0x24)return 1;
    const unsigned char *labels[]={data_020a1160,data_020a1162,data_020a1164,data_020a1166,data_020a1178,data_020a117c,data_020a11a0,data_020a11c4,pc_input_history_end};
    const unsigned offsets[]={0xc,0xe,0x10,0x12,0x24,0x28,0x4c,0x70,0x90};
    for(unsigned i=0;i<9;++i)if(reinterpret_cast<uintptr_t>(labels[i])!=history+offsets[i])return 2;
    for(int angle:{-32768,-17460,-1,0,16384,32767}) {
        std::memset(data_020a1040,0,0x24);
        std::memset(data_020a1154,0,0x90);
        const unsigned short key=0x204;
        std::memcpy(data_020a104e,&key,2);
        data_020a1052[5]=0x9d;
        func_0203dafc(angle);
        func_0203e0ac();
        for(int player=0;player<4;++player) {
            if(GetAngleToCamera(player)!=angle || GetPlayerFlagByte(player,5)!=0x9d)return 3;
            unsigned short actual=0;std::memcpy(&actual,data_020a1162+player*0x24,2);
            if(actual!=key)return 4;
        }
    }
    // Separate player headings must remain independently addressable.
    for(int player=0;player<4;++player) {
        const short heading=static_cast<short>(-23000+player*11000);
        std::memcpy(data_020a1154+player*0x24+0x10,&heading,2);
    }
    for(int player=0;player<4;++player)if(GetAngleToCamera(player)!=-23000+player*11000)return 5;
    pc_camera_set_movement_yaw_offset(8192,1);
    if(GetAngleToCamera(0)!=-14808)return 6;
    for(int player=1;player<4;++player)if(GetAngleToCamera(player)!=-23000+player*11000)return 7;
    pc_camera_set_movement_yaw_offset(0,0);
    for(int player=0;player<4;++player)if(GetAngleToCamera(player)!=-23000+player*11000)return 8;
    std::puts("PASS: original camera history and PC free-camera movement yaw remain isolated to player 0");
}
