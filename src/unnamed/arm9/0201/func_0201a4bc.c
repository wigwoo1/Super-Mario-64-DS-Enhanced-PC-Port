#if defined(SM64DS_PLATFORM_PC)
extern void ntr_rt_vblank_wait(void);
extern volatile int data_0209d514;
#else
extern void OS_SleepThread(void*);
#endif
extern char data_0209d500[];

void func_0201a4bc(void) {
#if defined(SM64DS_PLATFORM_PC)
    /* The IRQ resets this counter only when the original scene's VBlank
       interval has elapsed. Stage gameplay uses two; menus can use one.
       Yield each physical VBlank so input, IRQs and audio still run at 60Hz. */
    do { ntr_rt_vblank_wait(); } while (data_0209d514 != 0);
#else
    OS_SleepThread(data_0209d500);
#endif
}
