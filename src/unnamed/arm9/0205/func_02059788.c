#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#define PC_TIMER_TRACE(label) do { \
    const char *trace = getenv("SM64DS_PC_TRACE"); \
    if (trace && trace[0] && trace[0] != '0') { \
        fprintf(stderr, "[sm64ds] timer init: %s\n", label); \
        fflush(stderr); \
    } \
} while (0)
#else
#define PC_TIMER_TRACE(label) ((void)0)
#endif

extern unsigned short data_020a6430;
extern int data_020a6438;
extern int data_020a6434;
extern void func_02059700(void);
extern void func_02059624(int bit);
extern void _ZN3IRQ13SetIRQHandlerEjPFvvE(unsigned int bit, void (*handler)(void));
extern void _ZN3IRQ10EnableIRQsEj(unsigned int bits);

void func_02059788(void)
{
    PC_TIMER_TRACE("entry");
    if (data_020a6430 != 0)
        return;
    data_020a6430 = 1;
    func_02059624(0);
    PC_TIMER_TRACE("interrupt bit cleared");
    (&data_020a6438)[0] = 0;
    (&data_020a6438)[1] = 0;
    PC_TIMER_TRACE("counter cleared");
    *(volatile unsigned short *)0x4000102 = 0;
    *(volatile unsigned short *)0x4000100 = 0;
    *(volatile unsigned short *)0x4000102 = 0xc1;
    PC_TIMER_TRACE("timer registers written");
    _ZN3IRQ13SetIRQHandlerEjPFvvE(8, func_02059700);
    PC_TIMER_TRACE("IRQ handler installed");
    _ZN3IRQ10EnableIRQsEj(8);
    PC_TIMER_TRACE("IRQ enabled");
    data_020a6434 = 0;
}
