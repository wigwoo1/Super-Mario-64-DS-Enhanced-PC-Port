#include "PlayerInput.h"
#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
static int pc_trace_main_frames = 0;
#define PC_MAIN_TRACE(label) do { \
    const char *trace = getenv("SM64DS_PC_TRACE"); \
    if (pc_trace_main_frames < 5 && trace && trace[0] && trace[0] != '0') { \
        fprintf(stderr, "[sm64ds] main frame: %s\n", label); \
        fflush(stderr); \
    } \
} while (0)
#else
#define PC_MAIN_TRACE(label) ((void)0)
#endif
extern int data_0209d50c;
extern unsigned char data_0209d4e8;
extern unsigned short data_020a0e58[];
extern unsigned short data_0209d534[];
extern int data_0209d51c;
extern int data_020a0db0;
extern unsigned char data_0209d4f0;
extern unsigned char data_0209ee90[];
extern int data_0209d514;
extern unsigned char data_0209b480;
extern unsigned char data_0209e64c;

void _ZN3G2x18SetBlendBrightnessEPVtts(void* p, unsigned short a, unsigned short b);
void func_02019ac4(void);
void func_02019390(void);
void func_0203bb60(void);
void func_0203bc7c(void);
void func_02023498(void);
void _ZN8dScene_c16SpawnIfNecessaryEv(void);
void func_02044120(void);
void func_02019404(void);
void _ZN3IRQ11DisableIRQsEj(unsigned int);
void func_020190b8(void);
void _ZN3IRQ10EnableIRQsEj(unsigned int);
void func_0201a4bc(void);
void func_0201ffcc(void);
void func_020132d8(void);
unsigned short func_0203da80(unsigned short);
int func_02030e4c(void);

void func_020197b8(void) {
    PC_MAIN_TRACE("blend setup: begin");
    _ZN3G2x18SetBlendBrightnessEPVtts((void*)0x4000050, 0x3f, 0x1f);
    _ZN3G2x18SetBlendBrightnessEPVtts((void*)0x4001050, 0x3f, 0x1f);
    PC_MAIN_TRACE("blend setup: complete");
    do {
        unsigned short v;
        PC_MAIN_TRACE("func_02019ac4: begin");
        data_0209d50c = 0xb; func_02019ac4();
        PC_MAIN_TRACE("func_02019ac4: complete");
        data_0209d50c = 2;   func_02019390();
        PC_MAIN_TRACE("func_02019390: complete");
        data_0209d50c = 0x16; func_0203bb60();
        PC_MAIN_TRACE("func_0203bb60: complete");
        data_0209d50c = 0x17; func_0203bc7c();
        PC_MAIN_TRACE("func_0203bc7c: complete");
        v = *(unsigned short*)((char*)data_020a0e58 + (gActivePlayerSlot << 2));
        data_0209d534[data_0209d4e8] = v;
        data_0209d51c = v;
        data_0209d4e8 = (data_0209d4e8 + 1) & 0x1f;
        func_02023498();
        PC_MAIN_TRACE("func_02023498: complete");
        data_0209d50c = 3; _ZN8dScene_c16SpawnIfNecessaryEv();
        PC_MAIN_TRACE("SpawnIfNecessary: complete");
        data_0209d50c = 4; func_02044120();
        PC_MAIN_TRACE("func_02044120: complete");
        data_0209d50c = 5; func_02019404();
        PC_MAIN_TRACE("func_02019404: complete");
        data_0209d50c = 6;
        data_020a0db0 = data_020a0db0 + 1;
        _ZN3IRQ11DisableIRQsEj(1);
        func_020190b8();
        PC_MAIN_TRACE("func_020190b8: complete");
        data_0209d4f0 = 1;
        _ZN3IRQ10EnableIRQsEj(1);
        data_0209d50c = 7; func_0201a4bc();
        PC_MAIN_TRACE("func_0201a4bc: complete");
        data_0209d4f0 = 0;
        data_0209d50c = 0x15; func_0201ffcc();
        PC_MAIN_TRACE("func_0201ffcc: complete");
        if ((*(int*)(data_0209ee90 + 0x108) & 0x10) == 0)
            data_0209d514 = 0;
        data_0209d50c = 9;
#if defined(SM64DS_PLATFORM_PC)
        /* The host sound update handles both native playback and silent
           fallback without the ARM7 driver latch. */
        func_020132d8();
#else
        if (data_0209b480 != 0) func_020132d8();
#endif
        PC_MAIN_TRACE("sound update: complete");
        if (data_0209e64c != 0 || func_0203da80(0x9d) != 0)
            func_02030e4c();
#if defined(SM64DS_PLATFORM_PC)
        ++pc_trace_main_frames;
#endif
    } while (1);
}
