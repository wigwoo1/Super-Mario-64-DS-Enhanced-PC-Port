#include <cstdio>
#include <cstring>
#include <cstdint>
#include "ntr/mmio.h"
extern "C" {
void *data_ov007_0210342c;
int func_ov007_020b58e4(void*);
int func_ov007_020c31e0(int*,int*,int,int,int,int,int*);
void func_ov007_020c8b34(void*,int);
// Deliberately retain another external inline_fn, as in the linked daDemo TU.
inline unsigned inline_fn(unsigned value){return value;}
static int brightness;
void func_020551f0(void*,int value){brightness=value;}
}
#define CHECK(c) do{if(!(c)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c);return 1;}}while(0)
int main(){
    CHECK(ntr::io_init());
    CHECK(inline_fn(0x1234)==0x1234);
    int context[0x188/4]={},controller[4]={},panel[0x30/4]={},state[4]={};
    int quad[0x90/4]={},entry[2]={},record[4]={};
    context[3]=reinterpret_cast<int>(controller);data_ov007_0210342c=context;
    entry[0]=reinterpret_cast<int>(panel);entry[1]=reinterpret_cast<int>(quad);
    panel[1]=reinterpret_cast<int>(state);panel[9]=reinterpret_cast<int>(record);
    controller[0]=0;controller[3]=0;state[0]=static_cast<int>(0xffff0001u);
    CHECK(func_ov007_020b58e4(entry)==1);
    controller[3]=1;CHECK(func_ov007_020b58e4(entry)==-1);
    // The same helper name is a 64-bit sign extension in nearest-point search.
    int x[]={100000,200000},y[]={-100000,-200000},result[4]={};
    CHECK(func_ov007_020c31e0(x,y,2,0,0,300000,result)==0);
    CHECK(result[0]==141421);
    // And an integer identity in brightness interpolation.
    int fade[0x34/4]={};fade[1]=1;fade[0x2c/4]=-16;fade[0x30/4]=0;
    func_ov007_020c8b34(fade,2048);CHECK(brightness==-8);
    std::puts("PASS: actual original menu request, 64-bit nearest point and brightness helpers retain distinct implementations");
}
