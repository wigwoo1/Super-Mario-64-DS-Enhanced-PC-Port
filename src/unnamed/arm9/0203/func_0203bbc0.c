#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#define PC_BOOT_TRACE(label) do { \
    const char *trace = getenv("SM64DS_PC_TRACE"); \
    if (trace && trace[0] && trace[0] != '0') { \
        fprintf(stderr, "[sm64ds] ipc init: %s\\n", label); \
        fflush(stderr); \
    } \
} while (0)
#else
#define PC_BOOT_TRACE(label) ((void)0)
#endif

extern void func_0205f270(void);
extern void func_0205f1e4(void *p);
extern void func_0205f0e0(void *p);
extern void func_0205edd8(int a, int b);
extern void func_0205ea10(int mask);
extern int func_0205e9fc(int mask);
extern void Crash(void);
extern void func_0205eeac(int a, int b, void *p, int d);
extern void *data_020a0de0;
extern void *data_020a0df8;

void func_0203bbc0(void)
{
    PC_BOOT_TRACE("begin");
#if defined(SM64DS_PLATFORM_PC)
    /* Touch input and stylus latches are provided by the native input layer.
       This routine configures the DS digitizer through ARM7 IPC and waits for
       its readiness events, so there is no hardware service to initialize here. */
    PC_BOOT_TRACE("using host touch input");
    return;
#else
    func_0205f270();
    PC_BOOT_TRACE("touch handler registered");
    func_0205f1e4(&data_020a0de0);
    PC_BOOT_TRACE("calibration checked");
    func_0205f0e0(&data_020a0de0);
    PC_BOOT_TRACE("calibration applied");
    func_0205edd8(3, 0x1e);
    PC_BOOT_TRACE("touch mode IPC sent");
    func_0205ea10(8);
    PC_BOOT_TRACE("touch mode ready");
    if (func_0205e9fc(8) != 0)
        Crash();
    func_0205eeac(0, 4, &data_020a0df8, 9);
    PC_BOOT_TRACE("touch sampling IPC sent");
    func_0205ea10(2);
    PC_BOOT_TRACE("touch sampling ready");
    if (func_0205e9fc(2) != 0)
        Crash();
#endif
}
