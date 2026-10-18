#include "hal/pc_session.h"
#include "SaveData.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <initializer_list>
extern "C" {
unsigned char data_0209caa0[0x400]={},data_02092128[4]={},data_0209f2d8[1]={};
void CpuCopy8(const void *src,void *dst,unsigned size){std::memcpy(dst,src,size);}
void func_0205a588(void *dst,int value,int bytes){std::memset(dst,value,bytes);}
}
unsigned reads=0;
int file_result=2,mg_result=2;
unsigned char saved_file[0x44]={},saved_mg[0x2e4]={};
int SaveData::ReadDataFromCart(char *out,unsigned size,unsigned slot){
    ++reads;const int result=slot==3?mg_result:file_result;
    if(!result)std::memcpy(out,slot==3?saved_mg:saved_file,size);
    return result;
}
int SaveData::SaveDataToCart(char *,unsigned,unsigned){std::puts("UNEXPECTED_CART_WRITE");return 2;}
int main(){
    _putenv_s("SM64DS_PC_AUTOMATION_ONLY","");_putenv_s("SM64DS_PC_TEST_MISSION","3");
    if(pc_test_mission()!=1)return 20;
    _putenv_s("SM64DS_PC_AUTOMATION_ONLY","1");
    if(pc_test_mission()!=3)return 21;
    for(const char *value : {"0","8","3junk","-1","99999999999999"}) {
        _putenv_s("SM64DS_PC_TEST_MISSION",value);if(pc_test_mission()!=1)return 22;
    }
    _putenv_s("SM64DS_PC_TEST_MISSION","");_putenv_s("SM64DS_PC_AUTOMATION_ONLY","");
    _putenv_s("SM64DS_PC_TEST_CONDITIONAL_STAR_ID","0");
    if(pc_test_conditional_goomba_param(0xf12f,0)!=0xf12f)return 23;
    _putenv_s("SM64DS_PC_AUTOMATION_ONLY","1");
    for(unsigned star=0;star<5;++star) {
        char value[8];std::snprintf(value,sizeof(value),"%u",star);
        _putenv_s("SM64DS_PC_TEST_CONDITIONAL_STAR_ID",value);
        if(pc_test_conditional_goomba_param(0xf12f,0)!=(0x12f|(star<<12)))return 24;
        if(pc_test_conditional_goomba_param(0xf12f,6)!=0xf12f ||
           pc_test_conditional_goomba_param(0xff1f,0)!=0xff1f)return 25;
    }
    for(const char *value : {"","-1","5","0junk","999999999999999"}) {
        _putenv_s("SM64DS_PC_TEST_CONDITIONAL_STAR_ID",value);
        if(pc_test_conditional_goomba_param(0xf12f,0)!=0xf12f)return 26;
    }
    _putenv_s("SM64DS_PC_TEST_CONDITIONAL_STAR_ID","");_putenv_s("SM64DS_PC_AUTOMATION_ONLY","");
    if(!pc_prepare_level_session())return 10;unsigned magic;std::memcpy(&magic,data_0209caa0,4);
    if(magic!=0x30303038 || data_0209caa0[0x41]!=3 || data_02092128[0]!=3 || !(data_0209caa0[8]&8))return 1;
    for(unsigned character=0;character<4;++character){
        data_0209caa0[0x41]=character;data_0209caa0[0x20]=0x8a;data_02092128[0]=99;
        unsigned char before[0x400];std::memcpy(before,data_0209caa0,sizeof(before));
        if(!pc_prepare_level_session() || reads!=2)return 11;
        if(data_02092128[0]!=character || std::memcmp(before,data_0209caa0,sizeof(before)))return 2;
    }
    // A nonempty unknown block is never silently reset as a new game.
    std::memset(data_0209caa0,0,sizeof(data_0209caa0));data_0209caa0[0x20]=9;data_02092128[0]=2;
    if(pc_prepare_level_session() || data_0209caa0[0x20]!=9 || data_02092128[0]!=2)return 3;
    SaveData::SetDefaultValues(reinterpret_cast<FileSaveData *>(saved_file));
    SaveData::SetDefaultValuesMg(reinterpret_cast<MinigameSaveData *>(saved_mg));
    saved_file[0x41]=2;saved_file[0x20]=0x96;saved_file[0xc]=0xff;saved_mg[4]=0x5a;
    file_result=mg_result=0;std::memset(data_0209caa0,0,sizeof(data_0209caa0));
    if(!pc_prepare_level_session() || data_02092128[0]!=2 || data_0209caa0[0x20]!=0x96 ||
       data_0209caa0[0xc]!=0x5a || data_0209caa0[0x48]!=0x5a || data_0209caa0[0x328]!=0)return 12;
    for(unsigned failing=0;failing<2;++failing) {
        std::memset(data_0209caa0,0,sizeof(data_0209caa0));data_02092128[0]=99;
        file_result=failing?0:1;mg_result=failing?1:0;
        if(pc_prepare_level_session() || data_02092128[0]!=99)return 13;
        for(unsigned char b:data_0209caa0)if(b)return 14;
    }
    std::puts("PASS: retail Yoshi defaults, valid live-state preservation, disk character/progress/minigame mask loading and all-or-nothing read failure");
}
