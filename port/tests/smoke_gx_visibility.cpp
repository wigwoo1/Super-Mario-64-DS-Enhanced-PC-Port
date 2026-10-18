#include "ntr/gx.h"
#include <cmath>
#include <cstdio>

namespace {
constexpr uint32_t background = 0xFF101820;
int failures;
#define CHECK(value) do { if (!(value)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#value); ++failures; } } while (0)
int32_t fixed(float v) { return static_cast<int32_t>(std::lround(v * 4096)); }
void configure(uint32_t attributes = 0x001F10C0) {
    ntr::gx_reset();
    ntr::gx_write_port(0x04000440, 0);
    // Perspective, near=1, far=6, 90-degree field of view. Test vertices
    // stay inside the DS VTX_16 signed 4.12 range (-8 through almost +8).
    const float matrix[16] = {1,0,0,0, 0,1,0,0, 0,0,-7.0f/5,-1, 0,0,-12.0f/5,0};
    for (float v : matrix) ntr::gx_write_port(0x04000458, static_cast<uint32_t>(fixed(v)));
    ntr::gx_write_port(0x04000440, 1);
    ntr::gx_write_port(0x04000454, 0);
    ntr::gx_write_port(0x040004A4, attributes);
}
void vertex(float x,float y,float z,float u=0) {
    ntr::gx_write_port(0x04000488, static_cast<uint16_t>(std::lround(u*16)));
    ntr::gx_write_port(0x0400048C, uint16_t(fixed(x)) | (uint32_t(uint16_t(fixed(y))) << 16));
    ntr::gx_write_port(0x0400048C, uint16_t(fixed(z)));
}
void triangle(float depth=-2,bool reverse=false) {
    ntr::gx_write_port(0x04000500,0);
    vertex(-0.5f,-0.5f,depth);
    if(reverse) {vertex(0,0.5f,depth);vertex(0.5f,-0.5f,depth);}
    else {vertex(0.5f,-0.5f,depth);vertex(0,0.5f,depth);}
    ntr::gx_write_port(0x04000504,0);
}
unsigned raster(ntr::Framebuffer &frame) {
    for(auto &row:frame.px) for(auto &pixel:row) pixel=background;
    ntr::gx_render(frame);
    unsigned count=0;
    for(auto &row:frame.px) for(auto pixel:row) count+=pixel!=background;
    return count;
}
}
int main() {
    ntr::Framebuffer frame;
    // DS texture-coordinate generation. This is shared by all renderers and
    // character eye materials. Matrices must affect UVs, independently of the
    // position matrix, and mode1 must latch at TEXCOORD (not at VTX).
    for(unsigned mode=0;mode<4;++mode) {
        configure();
        ntr::gx_teximage_param(mode<<30);
        ntr::gx_write_port(0x04000440,3);
        const float texmat[16]={2,0,0,0, 0,3,0,0, 16,32,1,0, 48,64,0,1};
        for(float x:texmat)ntr::gx_write_port(0x04000458,static_cast<uint32_t>(fixed(x)));
        ntr::gx_write_port(0x04000440,1);
        ntr::gx_write_port(0x04000500,0);
        // Set raw UV(-2,4) and a +0.5 X normal. Matrix reset afterwards proves
        // the mode1 result was latched by TEXCOORD.
        ntr::gx_write_port(0x04000488,uint16_t(-32)|(uint32_t(64)<<16));
        ntr::gx_write_port(0x04000484,256);
        if(mode==1) {
            ntr::gx_write_port(0x04000440,3);ntr::gx_write_port(0x04000454,0);
            ntr::gx_write_port(0x04000440,1);
        }
        for(int i=0;i<3;++i) {
            ntr::gx_write_port(0x0400048c,uint16_t(fixed(1))|(uint32_t(uint16_t(fixed(2)))<<16));
            ntr::gx_write_port(0x0400048c,uint16_t(fixed(-2)));
        }
        size_t count=0;const auto *tri=ntr::gx_polygons(count);CHECK(count==1);
        const float expected_u[4]={-2,0,-1.9375f,-3.875f};
        const float expected_v[4]={4,18,4,0.375f};
        if(count)for(const auto &p:tri[0].v) {
            CHECK(std::fabs(p.u-expected_u[mode])<0.001f);
            CHECK(std::fabs(p.v-expected_v[mode])<0.001f);
        }
    }
    // Drive lighting through the game's register commands, without a host
    // light-enable helper. Pending attributes must latch only at BEGIN.
    configure(0x001F10C1);
    ntr::gx_write_port(0x040004C0, 0x001F0000); // red ambient, zero diffuse
    ntr::gx_write_port(0x040004C4, 0);          // zero emission
    ntr::gx_write_port(0x040004CC, 0x7FFF);     // white light 0
    ntr::gx_write_port(0x04000500, 0);
    ntr::gx_write_port(0x040004A4, 0x001F10C0); // disable at NEXT BEGIN
    ntr::gx_write_port(0x04000484, 0);
    vertex(-0.5f,-0.5f,-2);vertex(0.5f,-0.5f,-2);vertex(0,0.5f,-2);
    ntr::gx_write_port(0x04000504, 0);
    ntr::gx_write_port(0x04000500, 0);
    ntr::gx_write_port(0x04000484, 0);
    vertex(-0.5f,-0.5f,-2);vertex(0.5f,-0.5f,-2);vertex(0,0.5f,-2);
    ntr::gx_write_port(0x04000504, 0);
    size_t lit_count = 0;
    const auto *lit = ntr::gx_polygons(lit_count);
    CHECK(lit_count == 2);
    if (lit_count == 2) {
        CHECK(lit[0].v[0].color == 0xFFFF0000);
        CHECK(lit[1].v[0].color == 0xFF000000);
    }
    configure(0x001F1080);triangle();CHECK(raster(frame)>100);
    configure(0x001F1080);triangle(-2,true);CHECK(raster(frame)==0);
    configure(0x001F1040);triangle(-2,true);CHECK(raster(frame)>100);
    configure(0x001F1000);triangle();CHECK(raster(frame)==0);
    configure();triangle(2);CHECK(raster(frame)==0); // behind camera
    configure();triangle(-0.5f);CHECK(raster(frame)==0); // before near plane
    configure();triangle(-7);CHECK(raster(frame)==0); // beyond far plane

    configure();ntr::gx_write_port(0x04000500,0);
    vertex(-0.5f,-0.5f,-2);vertex(0.5f,-0.5f,-2);vertex(0,0.5f,0.5f);
    CHECK(raster(frame)>100); // camera-plane crossing retains visible portion

    for(unsigned enabled=0;enabled<2;++enabled) {
        configure(0x001F00C0 | (enabled<<12));ntr::gx_write_port(0x04000500,0);
        vertex(-0.5f,-0.5f,-2);vertex(0.5f,-0.5f,-2);vertex(0,0.5f,-7);
        CHECK(enabled ? raster(frame)>0 : raster(frame)==0);
    }

    configure(0x001F1080);ntr::gx_write_port(0x04000500,0);
    vertex(-0.5f,-0.5f,-2);
    ntr::gx_write_port(0x040004A4,0x001F1000); // takes effect at next BEGIN
    vertex(0.5f,-0.5f,-2);vertex(0,0.5f,-2);
    CHECK(raster(frame)>100);

    configure();
    const uint32_t texture[8]={0xFFFF0000,0xFF00FF00,0xFF0000FF,0xFFFFFFFF,
        0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF};
    ntr::gx_bind_texture(texture,8,1);
    ntr::gx_write_port(0x04000500,0);
    vertex(-0.6f,-0.6f,-1,0);vertex(2.4f,-2.4f,-4,4.5f);vertex(0,1.2f,-2,0);
    CHECK(raster(frame)>100);
    // Near the triangle centroid, perspective U is about 0.64 (red texel),
    // whereas affine U is about 1.5 (green texel).
    CHECK(frame.px[115][128]==0xFFFF0000);
    ntr::ppu_write_bmp("smoke_gx_visibility.bmp",frame);
    if(failures) return 1;
    std::puts("PASS: polygon lighting/latching, view clipping, faces, perspective texture coordinates");
}
