#include <cstdint>
#include <cstdio>
#include <cstring>
#include "ntr/mmio.h"
#include "ntr/gx.h"
extern "C" void DMAStartTransfer(int,int,int,int);
static void transfer(const void *s,void *d,unsigned control) {
    DMAStartTransfer(0,static_cast<int>(reinterpret_cast<uintptr_t>(s)),
        static_cast<int>(reinterpret_cast<uintptr_t>(d)),static_cast<int>(control));
}
#define CHECK(c) do {if(!(c)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c);return 1;}}while(0)
int main(){
    // Real mapped I/O source and OAM destination, including a canary past fill.
    CHECK(ntr::io_init());
    auto *fill=reinterpret_cast<uint32_t*>(0x040000e0u);
    auto *oam=reinterpret_cast<uint32_t*>(0x07000000u);
    *fill=0xc0;fill[1]=0xdeadbeef;std::memset(oam,0x55,0x408);
    transfer(fill,oam,0x85000100u);
    for(unsigned i=0;i<256;++i)CHECK(oam[i]==0xc0);
    CHECK(oam[256]==0x55555555u && oam[257]==0x55555555u);
    uint32_t source[]={1,2,3,4},out[]={9,9,9,9,9};
    transfer(source,out,0x84000004u);
    CHECK(out[0]==1 && out[3]==4 && out[4]==9);
    transfer(source+3,out,0x84800004u);
    CHECK(out[0]==4 && out[3]==1 && out[4]==9);
    transfer(source,out+3,0x84200004u);
    CHECK(out[0]==4 && out[3]==1 && out[4]==9);
    transfer(source,out,0x84400004u);
    CHECK(out[0]==4 && out[1]==3 && out[4]==9);
    transfer(source,out,0x84600004u);
    CHECK(out[0]==1 && out[3]==4 && out[4]==9);
    uint16_t half[]={11,22,33,44},h[]={7,7,7,7,7};
    transfer(half,h,0x80000004u);
    CHECK(h[0]==11 && h[3]==44 && h[4]==7);
    transfer(half+2,h,0x81000004u);
    CHECK(h[0]==33 && h[3]==33 && h[4]==7);
    transfer(half,h,0x00000004u);CHECK(h[0]==33);
    // Source-fixed word delivery must still feed the geometry FIFO.
    ntr::gx_reset();ntr::gx_stream_hash_reset();uint32_t nop=0;
    transfer(&nop,reinterpret_cast<void*>(0x04000400u),0x85400004u);
    uint32_t hash=2166136261u;for(int i=0;i<4;++i)hash*=16777619u;
    CHECK(ntr::gx_stream_hash_reset()==hash);
    std::puts("PASS: actual DMA fill, word/halfword copy, address modes, bounds and FIFO path");
}
