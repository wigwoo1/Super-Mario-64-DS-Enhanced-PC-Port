//cpp
#include "types.h"
#if defined(SM64DS_PLATFORM_PC)
#include <cstdio>
#include <cstdlib>
#include "ntr/gx.h"
static void pc_stage_render_trace(const char *step)
{
    static unsigned int lines;
    if (lines++ < 64 && std::getenv("SM64DS_PC_TRACE")) {
        size_t triangles = 0;
        ntr::gx_polygons(triangles);
        std::fprintf(stderr, "[sm64ds] stage Render: %s triangles=%zu\n", step, triangles);
        std::fflush(stderr);
    }
}
#define PC_STAGE_TRACE(step) pc_stage_render_trace(step)
#else
#define PC_STAGE_TRACE(step) ((void)0)
#endif
struct dExtFrameCtrl_c {
    void Advance();
};

struct AnimSlot {
    dExtFrameCtrl_c* anim;
    u8 flag;
    u8 pad[7];
};

#include "Particle__SysTracker.h"

struct dExtShadowModel_c {
    static void RenderAll();
};

struct dCc_c {
    static void Process();
};

struct OamAttr;
struct Matrix2x2;

struct OAM {
    static int Render(bool, OamAttr*, int, int, int, int, Matrix2x2*);
};

struct Renderable {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void Draw(int);
};

struct AnimMgr {
    u8 pad[0x14];
    u8 count;
};

extern "C" {
    void func_020391f0(void);
    void func_0203083c(void);
    void func_ov001_020aaf40(void);
    void func_0203db3c(int i, int v);
    int SublevelToLevel(int i);

    extern u8 data_0209f2c4;
    extern u8 data_0209f20c;
    extern u8 data_0209f294;
    extern u8 data_0209f2d8;
    extern u8 data_0209fc9c;
    extern u8 data_0209f290;
    extern u8 data_0209f21c;
    extern u8 data_0209f2d4;
    extern u8 data_0209f2b0;
    extern u8 data_0209f260;
    extern signed char data_02092124;
    extern AnimMgr* data_0209f340;
    extern char* data_0209f318;
    extern char* data_0209f394[];
    extern OamAttr _ZN3OAM4COINE;
    extern OamAttr _ZN3OAM5TIMESE;
}

struct dScStage_c {
    int Render();
    void RenderFog();
    void RenderModel();
    void RenderModelTransparent();
    static void RenderVsModeCountdown();
    static void RenderVsModeNewStar();
    static void RenderBouncingArrows();
    static void PS_Render();
    static void LC_Render();
    static void RenderNumber(u8, int, int, bool, int);
};

int dScStage_c::Render() {
    PC_STAGE_TRACE("enter");
    if (((data_0209f294 | (data_0209f2c4 | data_0209f20c)) & 0xff) == 0) {
        AnimSlot* s = (AnimSlot*)((char*)this + 0x8bc);
        int i;
        for (i = 0; i < data_0209f340->count; i++, s++) {
            if (s->flag != 0 && s->anim != 0)
                s->anim->Advance();
        }
        PC_STAGE_TRACE("particle update begin");
        ((Particle::SysTracker*)((char*)this + 0x50))->Update();
        PC_STAGE_TRACE("particle update complete");
    }
    func_020391f0();
    PC_STAGE_TRACE("collision update complete");
    RenderFog();
    PC_STAGE_TRACE("fog complete");
    {
        Renderable* t = *(Renderable**)((char*)this + 0x9bc);
        if (t != 0) {
            int* dst = (int*)((char*)t + 0x1c);
            int* src = (int*)((long long)(int)(data_0209f318 + 0x8c));
            dst[9] = src[0] >> 3;
            dst[10] = src[1] >> 3;
            dst[11] = src[2] >> 3;
            PC_STAGE_TRACE("skybox begin");
            (*(Renderable**)((char*)this + 0x9bc))->Draw(0);
            PC_STAGE_TRACE("skybox complete");
        }
    }
    PC_STAGE_TRACE("model begin");
    RenderModel();
    PC_STAGE_TRACE("model complete");
    dExtShadowModel_c::RenderAll();
    PC_STAGE_TRACE("shadows complete");
    RenderModelTransparent();
    PC_STAGE_TRACE("transparent model complete");
    {
        int b = (data_0209f2d8 == 1);
        if (b) {
            if (data_0209fc9c == 0 && data_0209f2c4 == 0) {
                RenderVsModeCountdown();
                RenderVsModeNewStar();
                if (data_0209f294 != 0 && data_0209f290 != 2)
                    RenderBouncingArrows();
            }
            func_0203083c();
        }
    }
    dCc_c::Process();
    PC_STAGE_TRACE("actor collision complete");
    func_ov001_020aaf40();
    PC_STAGE_TRACE("overlay 1 render complete");
    {
        int x = 0;
        int y = 0;
        int j = 0;
        int n = data_0209f21c;
        for (; j < n; j++) {
            char* p = data_0209f394[j];
            if (p != 0) {
                u8 bit = *(u8*)(p + 0x6ff);
                int* q = (int*)((long long)(int)(p + 0x5c));
                x ^= (u8)(q[2] ^ (q[0] ^ q[1]));
                y |= (u8)((bit & 1) << j);
            }
        }
        func_0203db3c(0x10, x);
        func_0203db3c(0x10, y);
    }
    if (data_0209f2c4 != 0) {
        PS_Render();
    } else if (data_0209f20c != 0 && data_0209f2d4 != 0) {
        int v = (SublevelToLevel(data_02092124) < 0xf) ? 0x80 : 0x70;
        if (data_0209f2b0 != 0)
            v += 0x10;
        OAM::Render(0, &_ZN3OAM4COINE, 0x68, v, -1, -1, 0);
        OAM::Render(0, &_ZN3OAM5TIMESE, 0x78, v + 8, -1, -1, 0);
        RenderNumber(data_0209f260, 0x80, v, 1, 8);
    }
    LC_Render();
    PC_STAGE_TRACE("complete");
    return 1;
}
