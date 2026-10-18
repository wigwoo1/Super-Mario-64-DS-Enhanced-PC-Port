extern void* data_0209d4a8;
#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#endif
void func_02019404(void){
  void** o = (void**)data_0209d4a8;
  if(!o) return;
  void** vt = (void**)o[0];
#if defined(SM64DS_PLATFORM_PC)
  const char *trace = getenv("SM64DS_PC_TRACE");
  if (trace && trace[0] && trace[0] != '0') {
    fprintf(stderr, "[sm64ds] OAM callback invoke object=%p vtable=%p slot0=%p slot1=%p\n",
            o, vt, vt ? vt[0] : 0, vt ? vt[1] : 0);
    fflush(stderr);
  }
#endif
  ((void(*)(void*))vt[1])(o);
}
