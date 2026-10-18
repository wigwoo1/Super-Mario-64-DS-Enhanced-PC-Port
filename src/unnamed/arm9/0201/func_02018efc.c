#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#define PC_CALLBACK_TRACE(label, object, method) do { \
  const char *trace = getenv("SM64DS_PC_TRACE"); \
  if (trace && trace[0] && trace[0] != '0') { \
    fprintf(stderr, "[sm64ds] scene callback: %s object=%p method=%p\\n", label, object, method); \
    fflush(stderr); \
  } \
} while (0)
#else
#define PC_CALLBACK_TRACE(label, object, method) ((void)0)
#endif
extern void* data_0209d4ac;
#if defined(SM64DS_PLATFORM_PC)
extern void dFdColor_AdvanceFade_PC(void *);
#endif
void func_02018efc(void){
  void** o = (void**)data_0209d4ac;
  PC_CALLBACK_TRACE("callback slot read", o, 0);
  if(!o) return;
#if defined(SM64DS_PLATFORM_PC)
  extern void dFdColor_AdvanceFade_PC(void *);
  PC_CALLBACK_TRACE("before native color fade", o, (void *)dFdColor_AdvanceFade_PC);
  dFdColor_AdvanceFade_PC(o);
  PC_CALLBACK_TRACE("after native color fade", o, (void *)dFdColor_AdvanceFade_PC);
#else
  void** vt = (void**)o[0];
  void (*callback)(void *) = (void(*)(void*))vt[2];
  callback(o);
#endif
}
