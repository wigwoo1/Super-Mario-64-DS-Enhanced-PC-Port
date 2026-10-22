// Production distance worker must start SQRT for each pointer-bound operand.
#include "ntr/mmio.h"
#include <cstdio>
extern "C" int func_02053274(int *,int *);
static bool check(int *a,int *b,int expected) {
    // Simulate another SDK user leaving a different result in the unit.
    ntr::io_write(0x040002b0,1,2);
    ntr::io_write(0x040002b8,4000000000000ull,8);
    const int got=func_02053274(a,b);
    if(got!=expected){std::fprintf(stderr,"distance got=%d expected=%d\n",got,expected);return false;}
    return true;
}
int main() {
    if(!ntr::io_init())return 1;
    int zero[3]={0,0,0},triangle[3]={3*4096,4*4096,0};
    int near_target[3]={-806880,9011222,3046853};
    int door_target[3]={-806912,9011200,3072000};
    int space[3]={0,0,12*4096},nonplanar[3]={3*4096,4*4096,12*4096};
    if(!check(zero,zero,0) || !check(zero,triangle,5*4096) ||
       !check(zero,nonplanar,13*4096) || !check(triangle,space,13*4096))return 2;
    // Real failed door sample is about6.14units from its target, below10.
    if(!check(near_target,door_target,25147) || !check(door_target,near_target,25147))return 3;
    std::puts("PASS production 3D distance ignores stale SQRT results; door sample is within10units");
    return 0;
}
