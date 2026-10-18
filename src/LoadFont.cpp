//cpp
#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#define PC_FONT_TRACE(label) do { \
    const char *trace = getenv("SM64DS_PC_TRACE"); \
    if (trace && trace[0] && trace[0] != '0') { \
        fprintf(stderr, "[sm64ds] font load: %s\\n", label); \
        fflush(stderr); \
    } \
} while (0)
#else
#define PC_FONT_TRACE(label) ((void)0)
#endif
extern "C" int LoadFile(int handle);
#if defined(SM64DS_PLATFORM_PC)
extern "C" void *pc_fs_load_file(unsigned int handle);
#define LOAD_FONT_FILE(id) pc_fs_load_file(id)
#else
#define LOAD_FONT_FILE(id) LoadFile(id)
#endif
extern "C" int func_ov004_020adc4c(void);
extern "C" int LoadFileAt(int handle, int dest);
extern "C" void Deallocate(void *ptr);
extern "C" unsigned int _ZN3G2S13GetBG1CharPtrEv(void);
extern "C" int func_02054d88(void);
extern "C" void MultiCopy_Int(int *dst, int *src, int len);
extern "C" void DecompressLZ16(void *dst, void *src);

namespace GX  { void LoadBGPltt(void const *p, unsigned a, unsigned b); }
namespace GXS { void LoadBGPltt(void const *p, unsigned a, unsigned b); }

extern unsigned char data_0209d698;
extern int data_0209d6f8;
extern unsigned char data_0209d660;
extern short data_0209d6d4;

extern "C" void LoadFont(unsigned char arg);
extern "C" void LoadFont(unsigned char arg)
{
    PC_FONT_TRACE("begin");
    int file;
    int r4;
    data_0209d698 = arg;
    r4 = (int)LOAD_FONT_FILE(0x980e);
    PC_FONT_TRACE("font glyph file loaded");
    if (data_0209d698 == 2) {
        LoadFileAt(0x980e, func_ov004_020adc4c());
        data_0209d6f8 = func_ov004_020adc4c();
        file = (int)LOAD_FONT_FILE(0x237);
        GX::LoadBGPltt((void *)(file + 2), 2, 0x20);
        Deallocate((void *)file);
    } else if (data_0209d698 == 1) {
        MultiCopy_Int((int *)r4, (int *)_ZN3G2S13GetBG1CharPtrEv(), 0x4000);
        file = (int)LOAD_FONT_FILE(0x237);
        GXS::LoadBGPltt((void *)file, 0x40, 0x20);
        Deallocate((void *)file);
    } else {
        MultiCopy_Int((int *)r4, (int *)func_02054d88(), 0x4000);
        file = (int)LOAD_FONT_FILE(0x980d);
        PC_FONT_TRACE("font map file loaded");
        DecompressLZ16((void *)file, (void *)(func_02054d88() + 0x8000));
        PC_FONT_TRACE("font map decompressed");
        Deallocate((void *)file);
    }
    Deallocate((void *)r4);
    PC_FONT_TRACE("font load complete");
    data_0209d660 = 0;
    data_0209d6d4 = -1;
}
