#include "ntr/input.h"
#include "ntr/mmio.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
extern "C" {
extern unsigned char gTouchHeld[],gTouchEdge[],gTouchX[],gTouchY[],pc_touch_state_end[];
void func_0203bb60();
struct Sample { uint16_t x,y,pressed; };
static Sample remote[4]={{19,29,1},{31,41,1},{51,61,0},{71,81,1}};
static unsigned remote_reads[4];
Sample *func_0203dabc(int i) { ++remote_reads[i];return &remote[i]; }
}
namespace {
int failures;
#define CHECK(x) do { if(!(x)) {std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);++failures;} }while(0)
__declspec(noinline) uintptr_t address(const void *p) {return reinterpret_cast<uintptr_t>(p);}
uint8_t read(const unsigned char *base,unsigned offset=0) {return static_cast<const volatile unsigned char *>(base)[offset];}
}
int main() {
    CHECK(ntr::io_init());
    CHECK(address(gTouchEdge)==address(gTouchHeld)+1);
    CHECK(address(gTouchX)==address(gTouchHeld)+2);
    CHECK(address(gTouchY)==address(gTouchHeld)+3);
    CHECK(address(pc_touch_state_end)==address(gTouchHeld)+16);
    ntr::input_set_touch(true,88,60);func_0203bb60();
    CHECK(read(gTouchHeld)==1 && read(gTouchEdge)==1 && read(gTouchX)==88 && read(gTouchY)==60);
    CHECK(read(gTouchHeld,4)==1 && read(gTouchEdge,4)==1 && read(gTouchX,4)==31 && read(gTouchY,4)==41);
    CHECK(read(gTouchHeld,8)==0 && read(gTouchEdge,8)==0 && read(gTouchX,8)==51 && read(gTouchY,8)==61);
    CHECK(read(gTouchHeld,12)==1 && read(gTouchEdge,12)==1 && read(gTouchX,12)==71 && read(gTouchY,12)==81);
    func_0203bb60();
    CHECK(read(gTouchHeld)==1 && read(gTouchEdge)==0);
    CHECK(read(gTouchEdge,4)==0 && read(gTouchEdge,12)==0);
    ntr::input_set_touch(true,200,120);func_0203bb60();
    CHECK(read(gTouchHeld)==1 && read(gTouchEdge)==0 && read(gTouchX)==200 && read(gTouchY)==120);
    ntr::input_set_touch(false,200,120);func_0203bb60();
    CHECK(read(gTouchHeld)==0 && read(gTouchEdge)==1);
    func_0203bb60();CHECK(read(gTouchHeld)==0 && read(gTouchEdge)==0);
    CHECK(remote_reads[0]==0 && remote_reads[1]==5 && remote_reads[2]==5 && remote_reads[3]==5);
    std::puts("Touch history: canonical labels, native tap/hold/drag/release and three independent wireless records checked");
    return failures?1:0;
}
