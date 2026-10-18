#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#define PC_RUNQ_TRACE(label) do { \
    const char *trace = getenv("SM64DS_PC_TRACE"); \
    if (trace && trace[0] && trace[0] != '0') { \
        fprintf(stderr, "[sm64ds] scheduler runq: %s\\n", label); \
        fflush(stderr); \
    } \
} while (0)
#else
#define PC_RUNQ_TRACE(label) ((void)0)
#endif
struct Node58;
extern int ARMProcessorMode(void);
extern struct Node58 *func_0205801c(void);
extern int ARMSaveContext(void *ctx);
extern void ARMRestoreContext(void *ctx);

struct CtxState {
  unsigned short m0;
  unsigned short m2;
  unsigned short m4;
  unsigned short m6;
  void *m8;
  int mc;
  void (*m10)(void*, void*);
};
extern struct CtxState data_020a6134;
extern void **data_020a612c;
extern void (*data_020a6128)(void*, void*);

void func_02057f54(void){
  PC_RUNQ_TRACE("begin");
  void *r6;
  struct Node58 *r5;
  struct CtxState *s = &data_020a6134;
  if (s->m4 == 0){
    PC_RUNQ_TRACE("context scheduler disabled");
    if (ARMProcessorMode() != 0x12) goto cont;
  }
  PC_RUNQ_TRACE("scheduler mode accepted");
  s->m0 = 1;
  PC_RUNQ_TRACE("scheduler marked pending");
  return;
cont:
  PC_RUNQ_TRACE("checking run queue");
  r6 = data_020a612c[0];
  PC_RUNQ_TRACE("current context read");
  r5 = func_0205801c();
  PC_RUNQ_TRACE("runnable job lookup complete");
  if (r6 == (void*)r5) return;
  if (r5 == 0) return;
  if (ARMSaveContext(r6) != 0) return;
  if (data_020a6128 != 0) data_020a6128(r6, r5);
  if (s->m10 != 0) s->m10(r6, r5);
  data_020a6134.m8 = r5;
  ARMRestoreContext(r5);
}
