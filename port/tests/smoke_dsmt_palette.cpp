#include "ntr/mmio.h"
#include "ntr/gx.h"
#include <cstdio>
#include <cstring>
extern "C" void func_ov007_020c1404(int *);
struct GridPosition {int x,y,z;};
struct GridNormal {short x,y,z;};
extern "C" void func_ov007_020caeac(GridPosition*,GridNormal*,int*,int*,int,int,int,int,int,int,int,int,int);
extern "C" {
void func_ov007_020c4684(unsigned char*,int,int,int,int,short*);
int data_ov007_0210325c=4096;
int _ZN4cstd3divEii(int a,int b){return a/b;}
void func_ov007_020c49bc(int,int,int,int,int,int,unsigned short*,int){}
}
#define CHECK(c) do {if(!(c)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c);return 1;}}while(0)
static unsigned sampled() {
    ntr::gx_write_port(0x04000500,0);
    for(unsigned xy:{0u,4096u,0x10000000u}) {
        ntr::gx_write_port(0x0400048c,xy);ntr::gx_write_port(0x0400048c,0);
    }
    ntr::gx_write_port(0x04000504,0);
    size_t count=0;const auto *tri=ntr::gx_polygons(count);
    return count && tri[count-1].tex ? tri[count-1].tex[0] : 0;
}
int main() {
    CHECK(ntr::io_init());
    auto *tex=reinterpret_cast<unsigned char*>(0x06800000);
    auto *pal=reinterpret_cast<unsigned short*>(0x06880000);
    std::memset(pal,0,0x1000);pal[1]=0x001f;pal[0x390/2+1]=0x03e0;
    for(int format:{2,3,4}) {
        ntr::gx_reset();std::memset(tex,format==2?0x55:format==3?0x11:1,64);
        ntr::gx_teximage_param(unsigned(format)<<26);
        int record[10]={};record[1]=format;record[9]=0x390;
        CHECK(sampled()==0xffff0000);
        // Use the actual transformed original menu palette selector.
        func_ov007_020c1404(record);
        CHECK(sampled()==0xff00ff00);
        func_ov007_020c1404(nullptr);
        CHECK(sampled()==0xffff0000);
    }
    ntr::gx_reset();
    int texture[10]={};texture[1]=4;
    int *table[]={texture};
    alignas(4) unsigned char quad[0x88]={};
    *reinterpret_cast<int***>(quad+0x5c)=table;
    *reinterpret_cast<short*>(quad+0x0c)=8;
    *reinterpret_cast<short*>(quad+0x0e)=8;
    *reinterpret_cast<short*>(quad+0x60)=1;
    *reinterpret_cast<short*>(quad+0x62)=1;
    *reinterpret_cast<int*>(quad+0x20)=4096;
    *reinterpret_cast<int*>(quad+0x24)=4096;
    short position[3]={};
    func_ov007_020c4684(quad,0x7fff,31,0,1,position);
    size_t count=0;const auto *tris=ntr::gx_polygons(count);
    CHECK(count==2);
    CHECK(tris[0].v[0].u==0 && tris[0].v[0].v==0);
    CHECK(tris[0].v[1].u==0 && tris[0].v[1].v==8);
    CHECK(tris[0].v[2].u==8 && tris[0].v[2].v==8);
    CHECK(tris[1].v[2].u==8 && tris[1].v[2].v==0);
    ntr::gx_reset();
    GridPosition grid[]={{-4096,4096,0},{4096,4096,0},{-4096,-4096,0},{4096,-4096,0}};
    GridNormal normals[4]={};int tu[2]={},tv[2]={};
    func_ov007_020caeac(grid,normals,tu,tv,1,1,0,0,32768,32768,0,0,0);
    tris=ntr::gx_polygons(count);
    CHECK(count==2);
    CHECK(tris[0].v[0].x==0 && tris[0].v[0].y==0);
    CHECK(tris[0].v[1].x==0 && tris[0].v[1].y==192);
    CHECK(tris[0].v[2].x==256 && tris[0].v[2].y==0);
    CHECK(tris[0].v[2].u==8 && tris[0].v[1].v==8);
    std::puts("PASS: original menu palettes, quad UV corners and castle grid emitter");
}
