#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#define PC_BOOT_FACTORY_TRACE(label) do { \
    const char *trace = getenv("SM64DS_PC_TRACE"); \
    if ((idx == 0 || idx == 2 || idx == 3 || idx == 4) && trace && trace[0] && trace[0] != '0') { \
        fprintf(stderr, "[sm64ds] boot factory: %s\n", label); \
        fflush(stderr); \
    } \
} while (0)
extern void *dScBoot_c_classInit(void);
extern void *dScTitle_c_classInit(void);
extern void *dScStage_c_classInit(void);
extern void ntr_input_set_menu_navigation(int enabled);
#else
#define PC_BOOT_FACTORY_TRACE(label) ((void)0)
#endif

typedef int (*Fn)(void);

extern int func_02043060(void *a);
extern void func_02043180(int a, int b, int c, int d);
extern void func_020433b8(char *self);

extern unsigned short data_020a4b50;
extern unsigned char data_020a4b4c;
extern Fn **data_020a4bb8;

int func_02043098(int idx, int b, int c, int d)
{
    int r5;
    char *c_arg;
#if defined(SM64DS_PLATFORM_PC)
    if (idx == 0 || idx == 2 || idx == 3 || idx == 4)
        ntr_input_set_menu_navigation(idx == 2);
#endif

    data_020a4b50 = (unsigned short)idx;
    data_020a4b4c = 2;
    PC_BOOT_FACTORY_TRACE("profile validation: begin");
    {
        int validation = func_02043060((void *)idx);
        if (idx == 0 || idx == 2 || idx == 3 || idx == 4) {
            const char *trace = getenv("SM64DS_PC_TRACE");
            if (trace && trace[0] && trace[0] != '0') {
                fprintf(stderr, "[sm64ds] scene factory: id=%d validation=%d\n", idx, validation);
                fflush(stderr);
            }
        }
        if (validation == 3) return 0;
    }
    PC_BOOT_FACTORY_TRACE("profile validation: complete");

    data_020a4b4c = 3;
    func_02043180(idx, b, c, d);

    data_020a4b4c = 4;
#if defined(SM64DS_PLATFORM_PC)
    if (idx == 0) {
        /* The BOOT profile stores the DS address 0x02023624. Invoke its
           recovered host factory instead of branching to that ROM address. */
        PC_BOOT_FACTORY_TRACE("native BOOT factory: begin");
        r5 = (int)dScBoot_c_classInit();
        PC_BOOT_FACTORY_TRACE("native BOOT factory: complete");
    } else if (idx == 2) {
        /* TITLE uses overlay 3's recovered native factory on the host. */
        PC_BOOT_FACTORY_TRACE("native TITLE factory: begin");
        r5 = (int)dScTitle_c_classInit();
        PC_BOOT_FACTORY_TRACE("native TITLE factory: complete");
    } else if (idx == 3 || idx == 4) {
        /* STAGE's DS profile stores a ROM factory address. Construct it with
           the host ABI so native vtables and embedded members are valid. */
        PC_BOOT_FACTORY_TRACE("native STAGE factory: begin");
        r5 = (int)dScStage_c_classInit();
        PC_BOOT_FACTORY_TRACE("native STAGE factory: complete");
    } else
#endif
#if defined(SM64DS_PLATFORM_PC)
    {
        const char *trace = getenv("SM64DS_PC_TRACE");
        if (trace && trace[0] && trace[0] != '0') {
            fprintf(stderr, "[sm64ds] scene factory fallback: idx=%d\n", idx);
            fflush(stderr);
        }
        if (idx < 0 || idx >= 512 || !data_020a4bb8 || !data_020a4bb8[idx]) {
            if (trace && trace[0] && trace[0] != '0') {
                fprintf(stderr, "[sm64ds] native profile unavailable: idx=%d\n", idx);
                fflush(stderr);
            }
            r5 = 0;
        } else {
            r5 = (*data_020a4bb8[idx])();
        }
    }
#else
    r5 = (*data_020a4bb8[idx])();
#endif
    if (r5 == 0) {
        data_020a4b4c = 0;
        data_020a4b50 = 0xffff;
        return 0;
    }
    data_020a4b4c = 5;
    c_arg = (char *)r5;
    func_020433b8(c_arg);
    PC_BOOT_FACTORY_TRACE("scene linked into scheduler");
    data_020a4b4c = 0;
    data_020a4b50 = 0xffff;
    return r5;
}
