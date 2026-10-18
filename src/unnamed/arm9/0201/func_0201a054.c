#include "types.h"
#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#define PC_INIT_TRACE(label) do { \
  const char *trace = getenv("SM64DS_PC_TRACE"); \
  if (trace && trace[0] && trace[0] != '0') { \
    fprintf(stderr, "[sm64ds] init: %s\n", label); \
    fflush(stderr); \
  } \
} while (0)
#else
#define PC_INIT_TRACE(label) ((void)0)
#endif
extern void func_020427f8(void);
extern u32 func_02042784(void);
extern void func_02053a8c(void);
extern void func_02053c40(void);
extern void func_0205b858(void);
extern void func_0203d740(void);
extern void func_02013e64(void);
extern void func_0201a4e4(void);
extern void _ZN3IRQ10EnableIRQsEj(u32 m);
extern void _ZN3IRQ6EnableEv(void);
extern int func_02053be0(int enable);
extern void func_0203bbc0(void);
extern void func_0203bb5c(void);
extern void func_0201fec8(void);
extern void func_02042f68(int a0, void* src);
extern void func_02018aa4(void);
extern void _ZN3IRQ11DisableIRQsEj(u32 m);
extern void func_0203ad84(void);
extern void func_0201a9fc(void* c);
extern void _ZN4Heap18InitializeGameHeapEjPS_(u32 sz, void* p);
extern void func_02019440(void);
extern void func_020134c8(void);
extern void func_0203b684(void);
extern void func_020233f0(void);
extern void _ZN8dScene_c18PrepareToSpawnBootEv(void);
extern void func_020196cc(void);
extern void func_0201a5cc(void);
#if defined(SM64DS_PLATFORM_PC)
extern void __sinit_02075154(void);
#endif

extern char data_0208ee50[];
extern char data_0209d574[];
extern char ACTOR_SPAWN_TABLE[];
extern void* data_020a4bb8;
#if defined(SM64DS_PLATFORM_PC)
extern void pc_actor_spawn_table_init(void);
#endif
struct S0209ee90 { char pad[0x44]; u32 f44; };
extern struct S0209ee90 data_0209ee90;

void func_0201a054(void){
  PC_INIT_TRACE("begin");
#if defined(SM64DS_PLATFORM_PC)
  /* A PC run has no DS boot-mode latch at 0x027FFC40; use the normal retail
     startup path, which is the false branch of this hardware check. */
  int r4 = 0;
#else
  int r4 = (*(u16*)0x27ffc40 == 2);
#endif
  PC_INIT_TRACE("boot mode selected");
  func_020427f8();
  PC_INIT_TRACE("func_020427f8");
  func_02042784();
  PC_INIT_TRACE("func_02042784");
  func_02053a8c();
  PC_INIT_TRACE("func_02053a8c");
  func_02053c40();
  PC_INIT_TRACE("func_02053c40");
  func_0205b858();
  PC_INIT_TRACE("func_0205b858");
  func_0203d740();
  PC_INIT_TRACE("func_0203d740");
  func_02013e64();
  PC_INIT_TRACE("func_02013e64");
  func_0201a4e4();
  PC_INIT_TRACE("func_0201a4e4");
  _ZN3IRQ10EnableIRQsEj(1);
  {
    volatile u16* p = (volatile u16*)0x4000208;
    u16 old = *p;
    *p = 1;
  }
  _ZN3IRQ6EnableEv();
  func_02053be0(1);
  PC_INIT_TRACE("func_02053be0");
  func_0203bbc0();
  PC_INIT_TRACE("func_0203bbc0");
  func_0203bb5c();
  PC_INIT_TRACE("func_0203bb5c");
  if (!r4) {
#if defined(SM64DS_PLATFORM_PC)
    /* The DS save/config recovery reads the ARM7 shared-WRAM mailbox. Native
       settings and save storage are handled by the host runtime instead. */
    PC_INIT_TRACE("using host save defaults");
#else
    func_0201fec8();
    PC_INIT_TRACE("func_0201fec8");
#endif
  }
  func_02042f68(0xd01, data_0208ee50);
  PC_INIT_TRACE("func_02042f68");
  if (!r4) {
#if defined(SM64DS_PLATFORM_PC)
    PC_INIT_TRACE("DS wireless services unavailable; using offline mode");
#else
    func_02018aa4();
    PC_INIT_TRACE("func_02018aa4");
#endif
  }
  if (r4)
    _ZN3IRQ11DisableIRQsEj(0x100000);
  if (!r4) {
    func_0203ad84();
    PC_INIT_TRACE("func_0203ad84");
  }
  func_0201a9fc(data_0209d574);
  PC_INIT_TRACE("heap data init");
  _ZN4Heap18InitializeGameHeapEjPS_(0x3b000, 0);
  PC_INIT_TRACE("game heap init");
  func_0201a9fc(data_0209d574);
  func_02019440();
  PC_INIT_TRACE("func_02019440");
  if (!r4) {
#if defined(SM64DS_PLATFORM_PC)
    /* The retail sound bootstrap configures ARM7/DS audio hardware and waits
       for its sound driver. Native audio is not wired yet, so leave the sound
       engine disabled while continuing to scene startup. */
    PC_INIT_TRACE("using host silent audio");
#else
    func_020134c8();
#endif
  }
  func_0203b684();
  PC_INIT_TRACE("func_0203b684");
#if defined(SM64DS_PLATFORM_PC)
  /* The NDS CRT runs this constructor before ARM9 main. Our native entrypoint
     bypasses that CRT, so initialize the actor lifecycle manager callbacks. */
  __sinit_02075154();
  PC_INIT_TRACE("actor lifecycle static initialization");
  pc_actor_spawn_table_init();
#else
  data_020a4bb8 = ACTOR_SPAWN_TABLE;
#endif
  func_020233f0();
  PC_INIT_TRACE("func_020233f0");
  _ZN8dScene_c18PrepareToSpawnBootEv();
  PC_INIT_TRACE("PrepareToSpawnBoot");
#if defined(SM64DS_PLATFORM_PC)
  /* This reads DS configuration/save state and changes the DS sound mode. */
  PC_INIT_TRACE("using host settings defaults");
#else
  func_020196cc();
  PC_INIT_TRACE("func_020196cc");
#endif
  data_0209ee90.f44 = 0x100;
  *(volatile u32*)0x40004c8 = 0x296a5800;
  *(volatile u32*)0x40004cc = 0x7fff;
  *(volatile u32*)0x40004c0 = 0x7fff;
  *(volatile u32*)0x40004c4 = 0;
  func_0201a5cc();
  PC_INIT_TRACE("func_0201a5cc");
}
