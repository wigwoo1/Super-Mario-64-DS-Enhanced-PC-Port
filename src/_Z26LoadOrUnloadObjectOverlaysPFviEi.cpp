#include "OverlayID.h"

//cpp
// @symbol _Z26LoadOrUnloadObjectOverlaysPFviEi
extern "C" unsigned char data_0209f2d8;
extern "C" unsigned char data_02075998[][7];
struct OvTbl { unsigned int t[7]; };
extern "C" OvTbl data_02075804[];

#if !defined(SM64DS_PLATFORM_PC)
extern "C" int overlay_60;
extern "C" int overlay_98;
extern "C" int overlay_102;
#endif

void LoadOrUnloadObjectOverlays(void (*fn)(int), int idx) {
    unsigned int i;
    int b = (data_0209f2d8 == 1);
    if (b != 0) return;
    for (i = 0; i < 7; i++) {
        b = (data_0209f2d8 == 1);
        if (b == 0 || i == 6) {
            unsigned char v = data_02075998[idx][i];
            if (v != 0) {
                fn(data_02075804[i].t[v]);
            }
        }
    }
    if (idx == 0x24 || idx == 0x26 || idx == 0x28) {
#if defined(SM64DS_PLATFORM_PC)
        fn(60);
#else
        fn(OVERLAY_ID(60));
#endif
        return;
    }
    b = (data_0209f2d8 == 1);
    if (b == 0) {
#if defined(SM64DS_PLATFORM_PC)
        fn(98);
#else
        fn(OVERLAY_ID(98));
#endif
    }
#if defined(SM64DS_PLATFORM_PC)
    fn(102);
#else
    fn(OVERLAY_ID(102));
#endif
}
