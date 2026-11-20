#pragma once
#include <array>
#include <cstdint>
#include <cstdio>

namespace ntr {
constexpr unsigned camera_turn_left_binding=14;
constexpr unsigned camera_turn_right_binding=15;
constexpr unsigned binding_count=16;
constexpr uint32_t pad_left_trigger=0x10000, pad_right_trigger=0x20000;
struct InputBindings {
    // First twelve slots are DS key bits. Later slots are host camera actions.
    std::array<uint16_t,binding_count> keys{{32,160,8,13,'D','A','W','S','E','Q','X','C','Z','F',219,221}};
    std::array<uint32_t,binding_count> pad{{0x2000,0x1000,0x20,0x10,8,4,1,2,0x200,0x100,0x8000,0x4000,0x80,0x40,pad_left_trigger,pad_right_trigger}};
};
inline InputBindings ui_bindings() {
    InputBindings b;b.pad[0]=0x1000;b.pad[1]=0x2000;b.pad[10]=0x4000;b.pad[11]=0x8000;return b;
}
inline bool valid_pad_binding(uint32_t v) {return v && !(v&(v-1)) && (v&0x3f3ffu);}
inline bool valid_key_binding(unsigned v) {return v>=8 && v<=254 && v!=27 && v!=16 && v!=17 && v!=18;}
inline bool binding_down(const InputBindings &b,unsigned index,const std::array<bool,256> &keys,uint32_t pad) {
    return (b.keys[index] && keys[b.keys[index]]) || (pad&b.pad[index]);
}
inline uint16_t mapped_buttons(const InputBindings &b,const std::array<bool,256> &keys,uint32_t pad) {
    uint16_t result=0;
    for(unsigned i=0;i<12;++i)if(binding_down(b,i,keys,pad))result|=uint16_t(1u<<i);
    return result;
}
inline uint32_t pad_with_triggers(uint16_t buttons,uint8_t left,uint8_t right) {
    return buttons | (left>128?pad_left_trigger:0) | (right>128?pad_right_trigger:0);
}
inline float camera_turn_strength(const InputBindings &b,unsigned index,const std::array<bool,256> &keys,
                                  uint16_t buttons,uint8_t left,uint8_t right) {
    if(binding_down(b,index,keys,buttons))return 1.0f;
    unsigned raw=0;
    if(b.pad[index]==pad_left_trigger)raw=left;
    if(b.pad[index]==pad_right_trigger)raw=right;
    constexpr unsigned trigger_deadzone=16;
    return raw>trigger_deadzone ? float(raw-trigger_deadzone)/float(255-trigger_deadzone) : 0.0f;
}
inline uint16_t camera_preset_buttons(bool turn_left,bool turn_right) {
    // The camera's original controls describe eye movement. Match the
    // Free Camera actions by turning the view in the requested direction.
    return turn_left==turn_right?0:turn_left?0x100:0x200;
}
template<class T> inline void swap_binding(std::array<T,binding_count> &map,unsigned index,T value) {
    const T old=map[index];
    for(unsigned i=0;i<binding_count;++i)if(i!=index && map[i]==value)map[i]=old;
    map[index]=value;
}
inline const char *binding_action(unsigned i) {
    static const char *names[]={"ATTACK","JUMP","SELECT","PAUSE","RIGHT","LEFT","FORWARD","BACKWARD","CAMERA RIGHT","CAMERA LEFT","VIEW MODE","RUN","CAMERA ZOOM","FREE CAMERA","TURN LEFT","TURN RIGHT"};
    return i<binding_count?names[i]:"";
}
inline void binding_name(bool pad,unsigned code,char *out,size_t size) {
    if(!code){std::snprintf(out,size,"UNBOUND");return;}
    if(pad) {
        const unsigned codes[]={1,2,4,8,16,32,64,128,256,512,4096,8192,16384,32768,65536,131072};
        const char *names[]={"UP","DOWN","LEFT","RIGHT","START","BACK","L3","R3","LB","RB","A","B","X","Y","LT","RT"};
        for(unsigned i=0;i<16;++i)if(code==codes[i]){std::snprintf(out,size,"%s",names[i]);return;}
    } else {
        if((code>='A' && code<='Z') || (code>='0' && code<='9')){std::snprintf(out,size,"%c",char(code));return;}
        const unsigned codes[]={8,9,13,32,160,161,162,163,164,165,37,38,39,40,219,221};
        const char *names[]={"BKSP","TAB","ENTER","SPACE","LSHIFT","RSHIFT","LCTRL","RCTRL","LALT","RALT","LEFT","UP","RIGHT","DOWN","LEFT BRACKET","RIGHT BRACKET"};
        for(unsigned i=0;i<16;++i)if(code==codes[i]){std::snprintf(out,size,"%s",names[i]);return;}
        if(code>=112 && code<=135){std::snprintf(out,size,"F%u",code-111);return;}
    }
    std::snprintf(out,size,"KEY %u",code);
}
}
