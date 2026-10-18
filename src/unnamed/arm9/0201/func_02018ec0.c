#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#define PC_FADER_B_TRACE(label, object, method) do { \
  const char *trace = getenv("SM64DS_PC_TRACE"); \
  if (trace && trace[0] && trace[0] != '0') { \
    fprintf(stderr, "[sm64ds] brightness callback: %s object=%p method=%p\\n", label, object, method); \
    fflush(stderr); \
  } \
} while (0)
#else
#define PC_FADER_B_TRACE(label, object, method) ((void)0)
#endif
extern void* data_0209d4b0;
void func_02018ec0(void){
  void** o = (void**)data_0209d4b0;
  PC_FADER_B_TRACE("slot read", o, 0);
#if defined(SM64DS_PLATFORM_PC)
  /* The DS may leave the secondary fader unset. Its update is optional. */
  if(!o) return;
  extern void dFdBrightness_AdvanceFade_PC(void *);
  PC_FADER_B_TRACE("before native brightness fade", o, (void*)dFdBrightness_AdvanceFade_PC);
  dFdBrightness_AdvanceFade_PC(o);
  PC_FADER_B_TRACE("after native brightness fade", o, (void*)dFdBrightness_AdvanceFade_PC);
#else
  if(!o) return;
  void** vt = (void**)o[0];
  void (*callback)(void*) = (void(*)(void*))vt[2];
  PC_FADER_B_TRACE("before dispatch", o, (void*)callback);
  if(callback) callback(o);
  PC_FADER_B_TRACE("after dispatch", o, (void*)callback);
#endif
}
