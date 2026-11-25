//cpp
#include "dBgPi.h"

#ifdef SM64DS_PLATFORM_PC
#include "dBgCh_Gnd.h"
#include "dBgW.h"
#endif

extern "C" {
extern void func_02037464(void *c);
extern int func_020393b4(void *p);
extern int func_02035354(void *a, int b);
extern int func_020393ac(void *p);

}

struct Obj {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual int  v6(void *arg);
};

extern "C" Obj *data_020a0c80;

extern "C" int func_02038ea4(void *thiz)
{
#ifdef SM64DS_PLATFORM_PC
    auto &ground = *static_cast<dBgCh_Gnd *>(thiz);
    func_02037464(&ground);
    auto *world = reinterpret_cast<dBgW *>(data_020a0c80);
    if (!world || func_02035354(&ground, func_020393b4(world)) || !world->DetectClsn(ground))
        return 0;
    auto *owner = reinterpret_cast<dActor_c *>(func_020393b4(world));
    ground.SetCollider(0, func_020393ac(world), owner, world);
    return 1;
#else
    int r = 0;
    Obj *o;
    func_02037464(thiz);
    o = data_020a0c80;
    if (o != 0) {
        if (func_02035354(thiz, func_020393b4(o)) == 0) {
            if (o->v6(thiz) != 0) {
                int a = func_020393ac(o);
                int b = func_020393b4(o);
                ((dBgPi *)((char *)thiz + 0x10))->SetCollider(0, a, (dActor_c *)b, (dBgW *)o);
                r = 1;
            }
        }
    }
    return r;
#endif
}
