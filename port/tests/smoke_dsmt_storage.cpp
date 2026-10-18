#include "SaveData.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern "C" {
extern unsigned char data_0209caa0[],data_0209cab4[],data_0209cad2[],data_0209cae4[],data_0209caf4[];
extern unsigned char data_0209b33c[],data_0209b340[],data_0209b34b[],data_0209b34e[],data_0209b3d8[];
// This storage-only harness never executes menu callbacks. Real game links
// their recovered implementations; reaching a fixture callback is a failure.
void func_ov007_020c3e4c(){std::abort();}void func_ov007_020c3e64(){std::abort();}
void func_ov007_020ccab4(){std::abort();}void func_ov007_020cca98(){std::abort();}
void func_ov007_020cca80(){std::abort();}void func_ov007_020cca74(){std::abort();}
void func_ov007_020cca68(){std::abort();}void func_ov007_020cc600(){std::abort();}
}
int main(){
    if(data_0209cab4!=data_0209caa0+0x14 || data_0209cad2!=data_0209caa0+0x32 ||
       data_0209cae4!=data_0209caa0+0x44 || data_0209caf4!=data_0209caa0+0x54)return 1;
    if(data_0209b340!=data_0209b33c+4 || data_0209b34b!=data_0209b340+0xb ||
       data_0209b34e!=data_0209b340+0xe || data_0209b3d8!=data_0209b340+0x98)return 2;
    std::memset(data_0209caa0,0,0x400);
    data_0209caa0[0x32+5]=77;
    if(SaveData::GetCoinRecord(5)!=77)return 3;
    // Observe COFF interior labels through actual memory accesses. Optimizers
    // may otherwise cache a named extern array as distinct from another name.
    static_cast<volatile unsigned char *>(data_0209cab4)[2]=0x3f;
    if(static_cast<volatile unsigned char *>(data_0209caa0)[0x16]!=0x3f)return 4;
    static_cast<volatile unsigned char *>(data_0209cae4)[0x100]=0x96;
    if(static_cast<volatile unsigned char *>(data_0209caa0)[0x144]!=0x96)return 5;
    std::puts("PASS: original live save/star/coin/minigame interior labels and three-file summary share canonical storage; real coin getter sees saved bytes");
}
