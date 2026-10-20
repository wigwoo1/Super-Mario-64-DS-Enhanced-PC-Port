//cpp
// @symbol _ZN3IRQ13VBlankHandlerEv
//
// Language-mode flip only: the compiler mangles the name, it is no longer
// spelled by hand. Signature and body preserved exactly; no codegen intent.
// See notes/plan-cpp-language-mode.md phase 1 (layout-free SDK namespaces).
#include "types.h"
#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#endif
extern "C" {
extern int data_0209d514;
extern int data_0208ee44;
extern u8 data_0209d4f0;
extern int data_0209d500;
extern int data_0209d4fc;
extern char data_023c0000[];
extern void OS_WakeupThread(int *p);
extern void func_02019144(void);
extern void func_02019100(void);
}

namespace IRQ {

void VBlankHandler(void)
{
  int *new_var;
  data_0209d514 = data_0209d514 + 1;
#if defined(SM64DS_PLATFORM_PC)
  {
    static unsigned int pc_vblank_count;
    const char *trace = getenv("SM64DS_PC_TRACE");
    ++pc_vblank_count;
    if (trace && trace[0] && trace[0] != '0' &&
        (pc_vblank_count <= 2 || pc_vblank_count % 120 == 0))
    {
      fprintf(stderr,
          "[pc vblank] calls=%u counter=%d threshold=%d update_flag=%u sync=%u\n",
          pc_vblank_count, data_0209d514, data_0208ee44, data_0209d4f0,
          (data_0209d514 >= data_0208ee44) && (data_0209d4f0 != 0));
      fflush(stderr);
    }
  }
#endif
  if ((data_0209d514 >= data_0208ee44) && (data_0209d4f0 != 0))
  {
    OS_WakeupThread(&data_0209d500);
    data_0209d514 = 0;
    func_02019144();
  }
  new_var = &data_0209d4fc;
  OS_WakeupThread(new_var);
  *(int *)(((int)data_023c0000 + 0x3ff8)) |= 1;
  func_02019100();
}

}
