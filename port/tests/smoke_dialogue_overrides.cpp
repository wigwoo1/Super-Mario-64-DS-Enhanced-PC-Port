#include "hal/dialogue_overrides.h"
#include <cstdio>
#include <cstring>

static void word(std::vector<uint8_t>&b,uint32_t n){for(int i=0;i<4;++i)b.push_back(uint8_t(n>>(i*8)));}
static std::vector<uint8_t> pack(const std::vector<uint8_t>&original,const std::vector<uint8_t>&text,const uint8_t*info){
    std::vector<uint8_t>b={'S','M','6','4','T','X','T','1'};word(b,1);word(b,1);word(b,7);
    word(b,pc_dialogue::crc32(original.data(),original.size(),pc_dialogue::crc32(info,4)));
    word(b,uint32_t(text.size()));b.insert(b.end(),text.begin(),text.end());return b;
}
int main(){
    if(pc_dialogue::crc32(reinterpret_cast<const uint8_t *>("123456789"),9)!=0xcbf43926u)return 20;
    pc_dialogue::Overrides mod;
    const uint8_t info[]={32,0,2,0};uint8_t widths[256];std::memset(widths,8,sizeof(widths));
    const std::vector<uint8_t> original={10,11,0xfd,0xfe,3,1,12,0xf4,0xf5,0xff};
    const std::vector<uint8_t> edited={13,0xfd,0xfe,3,1,14,0xf4,0xf5,0xff};
    auto file=pack(original,edited,info);
    if(!mod.load(file.data(),file.size(),1))return 1;
    const auto *chosen=mod.resolve(7,original.data(),original.size(),info,widths);
    if(chosen==original.data() || std::memcmp(chosen,edited.data(),edited.size()))return 2;
    if(mod.resolve(8,original.data(),original.size(),info,widths)!=original.data())return 3;
    auto changed=original;changed[0]=15;
    if(mod.resolve(7,changed.data(),changed.size(),info,widths)!=changed.data())return 4;
    auto bad=edited;bad[4]=2;file=pack(original,bad,info);mod.load(file.data(),file.size(),1);
    if(mod.resolve(7,original.data(),original.size(),info,widths)!=original.data())return 5;
    bad=edited;bad.erase(bad.begin()+1);file=pack(original,bad,info);mod.load(file.data(),file.size(),1);
    if(mod.resolve(7,original.data(),original.size(),info,widths)!=original.data())return 6;
    bad=edited;bad.insert(bad.begin(),8,10);file=pack(original,bad,info);mod.load(file.data(),file.size(),1);
    if(mod.resolve(7,original.data(),original.size(),info,widths)!=original.data())return 7;
    file=pack(original,edited,info);
    if(mod.load(file.data(),file.size(),2) || mod.count())return 8;
    for(size_t n=0;n<file.size();++n)if(mod.load(file.data(),n,1) || mod.count())return 9;
    auto duplicate=file;duplicate[12]=2;duplicate.insert(duplicate.end(),file.begin()+16,file.end());
    if(mod.load(duplicate.data(),duplicate.size(),1))return 10;
    if(!mod.load(file.data(),file.size(),1))return 11;
    const auto *stable=mod.resolve(7,original.data(),original.size(),info,widths);
    mod.resolve(99,original.data(),original.size(),info,widths);
    if(std::memcmp(stable,edited.data(),edited.size()))return 12;
    bad=edited;bad.insert(bad.begin(),256,10);file=pack(original,bad,info);mod.load(file.data(),file.size(),1);
    std::memset(widths,0,sizeof(widths));
    if(mod.resolve(7,original.data(),original.size(),info,widths)!=original.data())return 13;
    std::puts("PASS: approved text resolution, stable buffers, missing/stale fallback, width/control/page/icon preservation, wrong language, duplicate and truncated files");
}
