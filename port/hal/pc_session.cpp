#include "pc_session.h"
#include "SaveData.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern "C" {
extern unsigned char data_0209caa0[];
extern unsigned char data_02092128[];
}
// Test the original mission-filtered encounters through the normal level-load
// API. This cannot change a user's ordinary launch or alter actor/save data.
int pc_test_mission() {
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    const char *value=std::getenv("SM64DS_PC_TEST_MISSION");
    if(!automation || std::strcmp(automation,"1")!=0 || !value)return 1;
    char *end=nullptr;const long mission=std::strtol(value,&end,10);
    return end && *end==0 && mission>=1 && mission<=7 ? static_cast<int>(mission) : 1;
}
// The original unused test-map conditional Goomba encodes star15, while the
// retail VS sequence contains IDs0..4. Explicit QA fixtures exercise both sides
// of that comparison without changing the active sequence or normal map data.
unsigned pc_test_conditional_goomba_param(unsigned parameter, int level) {
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    const char *value=std::getenv("SM64DS_PC_TEST_CONDITIONAL_STAR_ID");
    if(level!=0 || parameter!=0xf12fu || !automation ||
       std::strcmp(automation,"1")!=0 || !value || !*value)return parameter;
    char *end=nullptr;const long star=std::strtol(value,&end,10);
    if(!end || *end || star<0 || star>4)return parameter;
    const unsigned fixture=(parameter&0xfffu)|(static_cast<unsigned>(star)<<12);
    std::fprintf(stderr,"[native QA fixture] conditional Goomba level=%d original=%04x fixture=%04x star=%ld\n",
        level,parameter,fixture,star);
    return fixture;
}
bool pc_prepare_level_session() {
    unsigned magic=0;std::memcpy(&magic,data_0209caa0,4);
    const unsigned before_character=data_0209caa0[0x41];
    bool pristine=true;
    for(unsigned i=0;i<0x44;++i)pristine&=data_0209caa0[i]==0;
    if(pristine) {
        // Read into temporary blocks so a failed/minigame read never leaves a
        // half-loaded live game. Match retail ReadFileData's minigame mask.
        alignas(4) unsigned char file[0x44]={}, minigames[0x2e4]={};
        const int game_result=SaveData::ReadDataFromCart(reinterpret_cast<char *>(file),sizeof(file),0);
        const int mg_result=SaveData::ReadDataFromCart(reinterpret_cast<char *>(minigames),sizeof(minigames),3);
        if(game_result==1 || mg_result==1)return false;
        if(game_result==2)SaveData::SetDefaultValues(reinterpret_cast<FileSaveData *>(file));
        if(mg_result==2)SaveData::SetDefaultValuesMg(reinterpret_cast<MinigameSaveData *>(minigames));
        if(game_result==0) {
            unsigned flags,mask;std::memcpy(&flags,file+0xc,4);std::memcpy(&mask,minigames+4,4);
            flags&=mask;std::memcpy(file+0xc,&flags,4);
        }
        std::memcpy(data_0209caa0,file,sizeof(file));
        std::memcpy(data_0209caa0+0x44,minigames,sizeof(minigames));
        data_0209caa0[0x328]=0;
    }
    std::memcpy(&magic,data_0209caa0,4);
    // Leave nonempty invalid state untouched: the full file-load/error path
    // remains separate work. Never clear an existing game's progress here.
    const bool valid=magic==0x30303038u && data_0209caa0[0x41]<4;
    if(valid)
        data_02092128[0]=data_0209caa0[0x41];
    const char *trace=std::getenv("SM64DS_PC_TRACE");
    if(trace && trace[0] && trace[0]!='0')
        std::fprintf(stderr,"[native session] pristine=%u before_character=%u magic=%08x saved_character=%u selected_character=%u\n",
            pristine?1u:0u,before_character,magic,data_0209caa0[0x41],data_02092128[0]);
    return valid;
}
