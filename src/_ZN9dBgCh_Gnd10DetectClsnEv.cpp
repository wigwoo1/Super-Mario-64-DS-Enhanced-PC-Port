//cpp
#include "dBgCh_Gnd.h"
#if defined(SM64DS_PLATFORM_PC)
#include "dBgW.h"
#include <cstdio>
#include <cstdlib>
#define GROUND_QUERY(obj, query) (reinterpret_cast<dBgW *>(obj)->DetectClsn(*(query)))
#else
#define GROUND_QUERY(obj, query) ((obj)->v6(query))
#endif

struct Vec3 { int x, y, z; };
struct ClsnObj { char pad[0x5c]; int vec[3]; char pad2[0x48]; int flags; int hb4; int hb8; };
struct ClsnActor { virtual int v0(); virtual int v1(); virtual int v2(); virtual int v3(); virtual int v4(); virtual int v5(); virtual int v6(void*); };
extern "C" void func_02037464(void*);
extern "C" ClsnObj* func_020393b4(ClsnActor*);
extern "C" int func_020393ac(ClsnActor*);
extern "C" int func_0203939c(ClsnActor*);
extern "C" int func_0203938c(ClsnActor*);
extern "C" int func_02035354(void*, ClsnObj*);

extern "C" int Vec3_HorzDist(Vec3*, Vec3*);
extern "C" ClsnActor* data_020a0c80[];

int dBgCh_Gnd::DetectClsn()
{
#if defined(SM64DS_PLATFORM_PC)
    if (std::getenv("SM64DS_PC_TRACE")) {
        std::fprintf(stderr, "[sm64ds] Gnd DetectClsn enter self=%p registry=%p first=%p\\n",
                     this, data_020a0c80, data_020a0c80[0]);
        std::fflush(stderr);
    }
#endif
    int ret = 0;
    func_02037464(this);
    ClsnActor* o = data_020a0c80[0];
#if defined(SM64DS_PLATFORM_PC)
    if (std::getenv("SM64DS_PC_TRACE")) {
        std::fprintf(stderr, "[sm64ds] Gnd DetectClsn reset complete first=%p\\n", o);
        std::fflush(stderr);
    }
#endif
    ClsnObj* firstInfo = o != 0 ? func_020393b4(o) : 0;
    int firstCompare = o != 0 ? func_02035354(this, firstInfo) : 1;
#if defined(SM64DS_PLATFORM_PC)
    if (std::getenv("SM64DS_PC_TRACE")) {
        void **vtable = o != 0 ? *(void ***)o : 0;
        std::fprintf(stderr, "[sm64ds] Gnd DetectClsn first owner=%p info=%p compare=%d vtable=%p callback=%p\\n",
                     o != 0 ? (void *)func_020393b4(o) : 0, firstInfo, firstCompare,
                     vtable, vtable != 0 ? vtable[7] : 0);
        std::fflush(stderr);
    }
#endif
    if (o != 0 && firstCompare == 0 && GROUND_QUERY(o, this) != 0) {
        ((dBgPi *)((char *)this + 0x10))->SetCollider(0, func_020393ac(o), (dActor_c *)func_020393b4(o), (dBgW *)o);
        ret = 1;
    }
    int flag = 1;
    int one = 1;
    Vec3 pos;
    for (int i = 1; i < 24; i++) {
        ClsnActor* obj = data_020a0c80[i];
        if (obj == 0) continue;
#if defined(SM64DS_PLATFORM_PC)
        if (std::getenv("SM64DS_PC_TRACE")) {
            std::fprintf(stderr, "[sm64ds] Gnd DetectClsn candidate i=%d obj=%p vtable=%p\\n",
                         i, obj, *(void **)obj);
            std::fflush(stderr);
        }
#endif
        ClsnObj* p = func_020393b4(obj);
        if (func_02035354(this, p) != 0) continue;
        if (p != 0 && ((p->flags & 2) ? flag : 0) != 0) {
            int* pv = (int*)(&p->vec[0]);
            pos.x = pv[0];
            pos.y = pv[1];
            pos.z = pv[2];
            int thr = func_0203939c(obj);
            if (thr == -0x1000) {
                int h8 = p->hb8;
                pos.y = pos.y + p->hb4;
                thr = h8 << 3;
            } else {
                pos.y = pos.y + func_0203938c(obj);
            }
            Vec3* selfpos = (Vec3*)((long long)(int)&this->pos);
            if (selfpos->y < pos.y - thr) continue;
            if (Vec3_HorzDist(selfpos, &pos) > thr) continue;
        }
        if (GROUND_QUERY(obj, this) != 0) {
            ((dBgPi *)((char *)this + 0x10))->SetCollider(i, func_020393ac(obj), (dActor_c *)func_020393b4(obj), (dBgW *)obj);
            ret = one;
        }
    }
    return ret;
}
