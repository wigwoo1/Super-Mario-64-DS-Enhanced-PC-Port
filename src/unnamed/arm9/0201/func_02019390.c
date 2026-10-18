#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#define PC_OAM_TRACE(label) do { \
    const char *trace = getenv("SM64DS_PC_TRACE"); \
    if (trace && trace[0] && trace[0] != '0') { \
        fprintf(stderr, "[sm64ds] OAM update: %s\\n", label); \
        fflush(stderr); \
    } \
} while (0)
#else
#define PC_OAM_TRACE(label) ((void)0)
#endif
extern void func_0202345c(void);
extern void func_02018efc(void);
extern void _ZN3OAM5ResetEv(void);
extern int func_02019018(void);
extern void func_0200f4b4(void);
extern void func_0200f468(void);
extern void func_02018ec0(void);

typedef int (*VFN)(void* o);
typedef struct Obj {
    VFN* vt;
} Obj;

extern Obj* data_0209d4a8;

void func_02019390(void)
{
    PC_OAM_TRACE("begin");
    Obj* o = data_0209d4a8;
    if (o != 0) {
        if (o->vt[0](o) == 0) {
            func_0202345c();
            func_02018efc();
            return;
        }
    }
    _ZN3OAM5ResetEv();
    PC_OAM_TRACE("OAM reset");
    if (func_02019018() != 0) {
        PC_OAM_TRACE("sprite update required");
        func_0200f4b4();
        PC_OAM_TRACE("sprite update complete");
    }
    func_0200f468();
    PC_OAM_TRACE("OAM transfer prepared");
    func_0202345c();
    PC_OAM_TRACE("input IRQ state updated");
    func_02018efc();
    PC_OAM_TRACE("sprite callback updated");
    func_02018ec0();
    PC_OAM_TRACE("complete");
}
