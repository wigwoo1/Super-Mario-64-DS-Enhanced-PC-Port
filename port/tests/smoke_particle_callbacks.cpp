#include <cstdio>
#include <cstring>
extern "C" {
using Quad=void (*)(unsigned char,unsigned char);
using Rotation=void (*)(int,int,void *);
extern Quad data_02099fb4[2];
extern Rotation data_02099fbc[2];
static int quad=-1,x=-1,y=-1;
void func_0204c24c(int a,int b){quad=0;x=a;y=b;}
void func_0204c194(int a,int b){quad=1;x=a;y=b;}
}
int main(){
    for(unsigned q=0;q<2;++q){
        data_02099fb4[q](255,254);
        if(quad!=int(q) || x!=255 || y!=254)return 1;
        data_02099fb4[q](0,7);
        if(quad!=int(q) || x!=0 || y!=7)return 2;
    }
    const int identity[12]={4096,0,0,0,4096,0,0,0,4096,0,0,0};
    int matrix[12];
    for(unsigned i=0;i<2;++i){
        std::memset(matrix,0x5a,sizeof(matrix));
        data_02099fbc[i](0,4096,matrix);
        if(std::memcmp(matrix,identity,sizeof(matrix)))return 3;
    }
    const int quarter_turn[12]={0,0,-4096,0,4096,0,4096,0,0,0,0,0};
    data_02099fbc[0](4096,0,matrix);
    if(std::memcmp(matrix,quarter_turn,sizeof(matrix)))return 4;
    std::puts("PASS: native particle quad dimensions widen correctly; original rotation workers preserve identity and quarter turn");
}
