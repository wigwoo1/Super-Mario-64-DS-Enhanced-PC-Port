#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#define PC_SCHED_TRACE(label) do { \
    const char *trace = getenv("SM64DS_PC_TRACE"); \
    if (trace && trace[0] && trace[0] != '0') { \
        fprintf(stderr, "[sm64ds] scheduler wake: %s\\n", label); \
        fflush(stderr); \
    } \
} while (0)
#else
#define PC_SCHED_TRACE(label) ((void)0)
#endif
#include "types.h"
struct Obj {
    char pad[0x64];
    u32 unk64;
};

u32 _ZN3IRQ7DisableEv(void);
void _ZN3IRQ7RestoreEj(u32 state);
void func_02057f54(void);

void func_02058048(struct Obj *self) {
    PC_SCHED_TRACE("wake: begin");
    u32 r4 = _ZN3IRQ7DisableEv();
    PC_SCHED_TRACE("interrupts disabled");
    self->unk64 = 1;
    PC_SCHED_TRACE("job marked runnable");
    func_02057f54();
    PC_SCHED_TRACE("scheduler yielded/returned");
    _ZN3IRQ7RestoreEj(r4);
    PC_SCHED_TRACE("interrupt state restored");
}
