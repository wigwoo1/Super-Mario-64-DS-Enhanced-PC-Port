// Exercise the real recovered menu caller against the native int ABI.
#include <cstdio>
#include <cstdlib>
#include <cmath>
extern "C" {
short data_02082214[8192];
void func_ov007_020c39f8(void *,int,int,int);
static int expected_s,expected_c,calls;
static void check(int *m,int s,int c) {
    if(s!=expected_s || c!=expected_c) {
        std::fprintf(stderr,"rotation ABI: got %d,%d expected %d,%d\n",s,c,expected_s,expected_c);
        std::exit(1);
    }
    ++calls;
    for(int i=0;i<12;++i)m[i]=0;
    m[0]=m[4]=m[8]=4096;
}
void func_02052800(int *m,int s,int c){check(m,s,c);}
void func_02052820(int *m,int s,int c){check(m,s,c);}
void func_0205283c(int *m,int s,int c){check(m,s,c);}
void MulMat4x3Mat4x3(int *,const int *,const int *){}
}
int main() {
    int matrix[12]{};
    for(int angle=0;angle<4096;++angle) {
        expected_s=short(std::lround(std::sin(angle*6.283185307179586/4096)*4096));
        expected_c=short(std::lround(std::cos(angle*6.283185307179586/4096)*4096));
        data_02082214[2*angle]=short(expected_s);
        data_02082214[2*angle+1]=short(expected_c);
        func_ov007_020c39f8(matrix,angle*16,angle*16,angle*16);
    }
    if(calls!=12288)return 2;
    std::puts("All 4096 rotation angles preserve signed arguments on all three native axes.");
}
