#pragma once
#include <array>
#include <cstdint>
#include <cstdio>

namespace ntr {
constexpr unsigned binding_count=14;
struct InputBindings {
    // First twelve slots are DS key bits; the last two are host camera actions.
    std::array<uint16_t,binding_count> keys{{32,160,8,13,'D','A','W','S','E','Q','X','C','Z','F'}};
    std::array<uint32_t,binding_count> pad{{0x2000,0x1000,0x20,0x10,8,4,1,2,0x200,0x100,0x8000,0x4000,0x80,0x40}};
};
inline InputBindings ui_bindings() {
    InputBindings b;b.pad[0]=0x1000;b.pad[1]=0x2000;b.pad[10]=0x4000;b.pad[11]=0x8000;return b;
}
inline bool valid_pad_binding(uint32_t v) {return v && !(v&(v-1)) && (v&0x3f3ffu);}
inline bool valid_key_binding(unsigned v) {return v>=8 && v<=254 && v!=27 && v!=16 && v!=17 && v!=18;}
inline uint16_t mapped_buttons(const InputBindings &b,const std::array<bool,256> &keys,uint32_t pad) {
    uint16_t result=0;
    for(unsigned i=0;i<12;++i)if(keys[b.keys[i]] || (pad&b.pad[i]))result|=uint16_t(1u<<i);
    return result;
}
template<class T> inline void swap_binding(std::array<T,binding_count> &map,unsigned index,T value) {
    const T old=map[index];
    for(unsigned i=0;i<binding_count;++i)if(i!=index && map[i]==value)map[i]=old;
    map[index]=value;
}
inline const char *binding_action(unsigned i) {
    static const char *names[]={"ATTACK","JUMP","SELECT","PAUSE","RIGHT","LEFT","FORWARD","BACKWARD","CAMERA RIGHT","CAMERA LEFT","VIEW MODE","RUN","CAMERA ZOOM","FREE CAMERA"};
    return i<binding_count?names[i]:"";
}
inline void binding_name(bool pad,unsigned code,char *out,size_t size) {
    if(pad) {
        const unsigned codes[]={1,2,4,8,16,32,64,128,256,512,4096,8192,16384,32768,65536,131072};
        const char *names[]={"UP","DOWN","LEFT","RIGHT","START","BACK","L3","R3","LB","RB","A","B","X","Y","LT","RT"};
        for(unsigned i=0;i<16;++i)if(code==codes[i]){std::snprintf(out,size,"%s",names[i]);return;}
    } else {
        if((code>='A' && code<='Z') || (code>='0' && code<='9')){std::snprintf(out,size,"%c",char(code));return;}
        const unsigned codes[]={8,9,13,32,160,161,162,163,164,165,37,38,39,40};
        const char *names[]={"BKSP","TAB","ENTER","SPACE","LSHIFT","RSHIFT","LCTRL","RCTRL","LALT","RALT","LEFT","UP","RIGHT","DOWN"};
        for(unsigned i=0;i<14;++i)if(code==codes[i]){std::snprintf(out,size,"%s",names[i]);return;}
        if(code>=112 && code<=135){std::snprintf(out,size,"F%u",code-111);return;}
    }
    std::snprintf(out,size,"KEY %u",code);
}
}
