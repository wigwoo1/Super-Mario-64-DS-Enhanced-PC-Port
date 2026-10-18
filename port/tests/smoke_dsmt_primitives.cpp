#include <cstdio>
#include <limits>
#include <initializer_list>
extern "C" {void func_02052ec8(int *);void Matrix3x3_LoadIdentity(int *);long long func_01ffaa34(long long,long long);void func_02059eb0(unsigned char *);}
int main(){
    unsigned char owner[0x56];for(auto &b:owner)b=0xa5;func_02059eb0(owner+1);
    if(owner[0]!=0xa5 || owner[0x55]!=0xa5 || owner[1]!=1)return 7;
    for(unsigned i=2;i<0x55;++i)if(owner[i])return 8;
    int matrix[18];for(auto &n:matrix)n=0x12345678;
    func_02052ec8(matrix+1);
    if(matrix[0]!=0x12345678 || matrix[17]!=0x12345678)return 1;
    for(unsigned i=0;i<16;++i)if(matrix[i+1]!=(i%5==0?0x1000:0))return 2;
    for(auto &n:matrix)n=0x12345678;
    Matrix3x3_LoadIdentity(matrix+1);
    if(matrix[0]!=0x12345678 || matrix[10]!=0x12345678)return 3;
    for(unsigned i=0;i<9;++i)if(matrix[i+1]!=(i%4==0?0x1000:0))return 4;
    for(long long a:{-0x100000003ll,-7ll,0ll,7ll,0x100000003ll})for(long long b:{-3ll,2ll,7ll})
        if(func_01ffaa34(a,b)!=a/b)return 5;
    if(func_01ffaa34((std::numeric_limits<long long>::min)(),-1)!=(std::numeric_limits<long long>::min)())return 6;
    std::puts("PASS: original 4x4/3x3 identity store layouts, buffer boundaries and signed 64-bit menu center division");
}
