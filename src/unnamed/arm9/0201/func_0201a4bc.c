#if defined(SM64DS_PLATFORM_PC)
extern void ntr_rt_vblank_wait(void);
#else
extern void OS_SleepThread(void*);
#endif
extern char data_0209d500[];

void func_0201a4bc(void) {
#if defined(SM64DS_PLATFORM_PC)
    ntr_rt_vblank_wait();
#else
    OS_SleepThread(data_0209d500);
#endif
}
