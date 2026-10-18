#include <cstdint>
#include <cstdlib>
#include <limits>
#include <cstring>
extern "C" {
// OS_GetOwnerInfo reads a DS firmware page at027ffc80. A native PC has no
// such personal profile; provide a deterministic English SDK record with
// empty nickname/comment fields, without reading Windows account details.
void func_02059eb0(unsigned char *out){std::memset(out,0,0x54);out[0]=1;}
// ARM SDK identity writers, directly transcribed from their store sequences.
void func_02052ec8(int *m){for(unsigned i=0;i<16;++i)m[i]=(i%5==0)?0x1000:0;}
void Matrix3x3_LoadIdentity(int *m){for(unsigned i=0;i<9;++i)m[i]=(i%4==0)?0x1000:0;}
// Signed EABI 64-bit division; the menu uses its low word for centers.
long long func_01ffaa34(long long a,long long b){
    if(!b)std::abort();
    if(a==(std::numeric_limits<long long>::min)() && b==-1)return a;
    return a/b;
}
}
