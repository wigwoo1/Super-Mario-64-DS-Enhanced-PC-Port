#include "hal/dialogue_overrides.h"
#include "ntr/video.h"
#include "ntr/rt.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

// local extern: these message-bank globals and unnamed helpers have no typed
// shared declaration; MessageBank.h supplies aliases but not declarations.
extern "C" {
extern uint8_t *data_0209d6fc,*data_0209d708,*data_0209d6f4;
extern int16_t data_0209d6d4;
extern uint8_t data_0208f074[];
int GetOwnerLanguage();
extern uint8_t data_0209d6bc,data_0209d6b4,data_0209d68c,data_0209d684,data_0209d694,data_0209d660;
}
extern "C" void pc_dialogue_observe(){
    const char *path=std::getenv("SM64DS_PC_DIALOGUE_CAPTURE");
    if(!path || !*path)return;
    static FILE *file=nullptr;static uint32_t previous[7]={};static bool observed=false;
    const uint32_t current[]={static_cast<uint16_t>(data_0209d6d4),data_0209d6bc,
        data_0209d6b4,data_0209d68c,data_0209d684,data_0209d694,data_0209d660};
    if(observed && !std::memcmp(previous,current,sizeof(current)))return;
    observed=true;std::memcpy(previous,current,sizeof(current));
    if(!file){file=std::fopen(path,"w");if(file)std::fputs("tick,message,state,line,selection,answer,choice_mode,active\n",file);}
    if(file){std::fprintf(file,"%llu,%u,%u,%u,%u,%u,%u,%u\n",static_cast<unsigned long long>(ntr::rt_frame()),
        current[0],current[1],current[2],current[3],current[4],current[5],current[6]);std::fflush(file);}
}
extern "C" void pc_dialogue_begin(){
    if(!ntr::video_get_settings().brain_rot_text || !data_0209d6fc || !data_0209d708)return;
    auto *bank=data_0209d6fc;const unsigned id=static_cast<uint16_t>(data_0209d6d4);
    const unsigned count=bank[0x28]|unsigned(bank[0x29])<<8;
    if(id>=count)return;
    auto word=[](const uint8_t*p){uint32_t n;std::memcpy(&n,p,4);return n;};
    auto *entry=data_0209d708+id*8;
    auto *text=bank+0x28+word(bank+0x24);
    auto *original=text+word(entry);
    if(data_0209d6f4!=original)return;
    const unsigned language=GetOwnerLanguage();
    const unsigned normalized=language>=2 && language<=5?language:1;
    static pc_dialogue::Overrides replacements;
    static bool loaded=false;static unsigned loaded_language=0;
    if(!loaded || loaded_language!=normalized){
        // Loading happens only at message start. Buffers stay stable throughout
        // the dialogue; editing a mod file takes effect on the next launch.
        loaded=true;loaded_language=normalized;
        const char *override_path=std::getenv("SM64DS_PC_DIALOGUE_FILE");
        static const char *names[]={"","eng","frn","gmn","itl","spn"};
        char path[128];std::snprintf(path,sizeof(path),"mods/brainrot/%s.smbt",names[normalized]);
        FILE *file=std::fopen(override_path && *override_path?override_path:path,"rb");
        bool valid=false;
        if(file){
            if(std::fseek(file,0,SEEK_END)==0){
                const long size=std::ftell(file);
                if(size>=16 && size<=4*1024*1024 && std::fseek(file,0,SEEK_SET)==0){
                    std::vector<uint8_t> bytes(static_cast<size_t>(size));
                    if(std::fread(bytes.data(),1,bytes.size(),file)==bytes.size())valid=replacements.load(bytes.data(),bytes.size(),normalized);
                }
            }
            std::fclose(file);
        }
        if(!valid)replacements.load(nullptr,0,normalized);
        if(std::getenv("SM64DS_PC_DIALOGUE_TRACE"))std::fprintf(stderr,"[dialogue mod] valid=%d entries=%u language=%u\n",valid,unsigned(replacements.count()),normalized);
    }
    // Bound reads by the next entry. Last-message padding is part of DAT1.
    const uint32_t offset=word(entry);
    const uint32_t end=id+1<count?word(entry+8):word(bank+0x20+word(bank+0x24)+4)-8;
    if(end<=offset || end-offset>8192)return;
    size_t size=0;
    while(size<end-offset){
        const unsigned c=original[size++];
        if(c==0xff)break;
        if(c==0xfe){if(size>=end-offset || original[size]<3 || original[size]-1>end-offset-size)return;size+=original[size]-1;}
    }
    if(!size || original[size-1]!=0xff)return;
    const auto *chosen=replacements.resolve(id,original,size,entry+4,data_0208f074);
    data_0209d6f4=const_cast<uint8_t *>(chosen);
    if(std::getenv("SM64DS_PC_DIALOGUE_TRACE"))std::fprintf(stderr,"[dialogue mod] id=%u replacement=%d\n",id,chosen!=original);
}
