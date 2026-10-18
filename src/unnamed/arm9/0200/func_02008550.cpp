//cpp
#if defined(SM64DS_PLATFORM_PC)
#include <cstdio>
#include <cstdlib>
#endif
struct Obj { int v; };
#if defined(SM64DS_PLATFORM_PC)
typedef int (*PMF)(Obj *, unsigned char *, int, int);
#define DECLARE_PC_MESSAGE_HANDLER(name) extern "C" int name(void *, ...);
DECLARE_PC_MESSAGE_HANDLER(func_02008500)
DECLARE_PC_MESSAGE_HANDLER(func_020084b0)
DECLARE_PC_MESSAGE_HANDLER(func_02008474)
DECLARE_PC_MESSAGE_HANDLER(func_02008450)
DECLARE_PC_MESSAGE_HANDLER(func_0200840c)
DECLARE_PC_MESSAGE_HANDLER(func_020082f8)
DECLARE_PC_MESSAGE_HANDLER(func_020082a0)
DECLARE_PC_MESSAGE_HANDLER(func_02008200)
DECLARE_PC_MESSAGE_HANDLER(func_0200814c)
DECLARE_PC_MESSAGE_HANDLER(func_020080f0)
DECLARE_PC_MESSAGE_HANDLER(func_020080b0)
DECLARE_PC_MESSAGE_HANDLER(func_02008080)
DECLARE_PC_MESSAGE_HANDLER(func_02008028)
DECLARE_PC_MESSAGE_HANDLER(func_02008008)
DECLARE_PC_MESSAGE_HANDLER(func_02007fcc)
DECLARE_PC_MESSAGE_HANDLER(func_02007f14)
DECLARE_PC_MESSAGE_HANDLER(func_02007e5c)
DECLARE_PC_MESSAGE_HANDLER(func_02007d58)
DECLARE_PC_MESSAGE_HANDLER(func_02007b98)
DECLARE_PC_MESSAGE_HANDLER(func_02007b0c)
DECLARE_PC_MESSAGE_HANDLER(func_02007a8c)
DECLARE_PC_MESSAGE_HANDLER(func_020079ac)
DECLARE_PC_MESSAGE_HANDLER(func_02007910)
DECLARE_PC_MESSAGE_HANDLER(func_020078c4)
DECLARE_PC_MESSAGE_HANDLER(func_02007870)
DECLARE_PC_MESSAGE_HANDLER(func_020077f0)
DECLARE_PC_MESSAGE_HANDLER(func_02007758)
DECLARE_PC_MESSAGE_HANDLER(func_02007698)
DECLARE_PC_MESSAGE_HANDLER(func_0200762c)
DECLARE_PC_MESSAGE_HANDLER(func_020075b4)
DECLARE_PC_MESSAGE_HANDLER(func_02007548)
DECLARE_PC_MESSAGE_HANDLER(func_0200751c)
DECLARE_PC_MESSAGE_HANDLER(func_020074e8)
DECLARE_PC_MESSAGE_HANDLER(func_02007484)
DECLARE_PC_MESSAGE_HANDLER(func_02007450)
DECLARE_PC_MESSAGE_HANDLER(func_02007414)
DECLARE_PC_MESSAGE_HANDLER(func_020073a0)
DECLARE_PC_MESSAGE_HANDLER(func_020072c4)
DECLARE_PC_MESSAGE_HANDLER(func_020071e0)
#undef DECLARE_PC_MESSAGE_HANDLER
#else
typedef int (Obj::*PMF)(unsigned char *, int, int);
#endif

extern "C" {
extern int data_0209b004;
extern PMF data_0209b138[39];
extern PMF data_02086c80, data_02086d80, data_02086c60, data_02086c90, data_02086bd8, data_02086d18, data_02086d98, data_02086c88, data_02086d00, data_02086bc0, data_02086e30, data_02086ce8, data_02086e38, data_02086e20, data_02086cd0, data_02086e08, data_02086df0, data_02086cb8, data_02086dd8, data_02086dc0, data_02086c20, data_02086da0, data_02086d60, data_02086d88, data_02086cf8, data_02086d68, data_02086c78, data_02086d40, data_02086d38, data_02086d20, data_02086c50, data_02086d08, data_02086cf0, data_02086c38, data_02086cd8, data_02086cc0, data_02086cb0, data_02086ca8, data_02086ca0;
}

extern "C" int func_02008550(Obj *obj, unsigned char *msg, int a2, int a3)
{
    if ((data_0209b004 & 1) == 0) {
        data_0209b004 |= 1;
#if defined(SM64DS_PLATFORM_PC)
#define PC_HANDLER(name) reinterpret_cast<PMF>(name)
        data_0209b138[0] = PC_HANDLER(func_02008500);
        data_0209b138[1] = PC_HANDLER(func_020084b0);
        data_0209b138[2] = PC_HANDLER(func_02008474);
        data_0209b138[3] = PC_HANDLER(func_02008450);
        data_0209b138[4] = PC_HANDLER(func_0200840c);
        data_0209b138[5] = PC_HANDLER(func_020082f8);
        data_0209b138[6] = PC_HANDLER(func_020082a0);
        data_0209b138[7] = PC_HANDLER(func_02008200);
        data_0209b138[8] = PC_HANDLER(func_0200814c);
        data_0209b138[9] = PC_HANDLER(func_020080f0);
        data_0209b138[10] = PC_HANDLER(func_020080b0);
        data_0209b138[11] = PC_HANDLER(func_02008080);
        data_0209b138[12] = PC_HANDLER(func_02008028);
        data_0209b138[13] = PC_HANDLER(func_02008008);
        data_0209b138[14] = PC_HANDLER(func_02007fcc);
        data_0209b138[15] = PC_HANDLER(func_02007f14);
        data_0209b138[16] = PC_HANDLER(func_02007e5c);
        data_0209b138[17] = PC_HANDLER(func_02007d58);
        data_0209b138[18] = PC_HANDLER(func_02007b98);
        data_0209b138[19] = PC_HANDLER(func_02007b0c);
        data_0209b138[20] = PC_HANDLER(func_02007a8c);
        data_0209b138[21] = PC_HANDLER(func_020079ac);
        data_0209b138[22] = PC_HANDLER(func_02007910);
        data_0209b138[23] = PC_HANDLER(func_020078c4);
        data_0209b138[24] = PC_HANDLER(func_02007870);
        data_0209b138[25] = PC_HANDLER(func_020077f0);
        data_0209b138[26] = PC_HANDLER(func_02007758);
        data_0209b138[27] = PC_HANDLER(func_02007698);
        data_0209b138[28] = PC_HANDLER(func_0200762c);
        data_0209b138[29] = PC_HANDLER(func_020075b4);
        data_0209b138[30] = PC_HANDLER(func_02007548);
        data_0209b138[31] = PC_HANDLER(func_0200751c);
        data_0209b138[32] = PC_HANDLER(func_020074e8);
        data_0209b138[33] = PC_HANDLER(func_02007484);
        data_0209b138[34] = PC_HANDLER(func_02007450);
        data_0209b138[35] = PC_HANDLER(func_02007414);
        data_0209b138[36] = PC_HANDLER(func_020073a0);
        data_0209b138[37] = PC_HANDLER(func_020072c4);
        data_0209b138[38] = PC_HANDLER(func_020071e0);
#undef PC_HANDLER
#else
        data_0209b138[0] = data_02086c80;
        data_0209b138[1] = data_02086d80;
        data_0209b138[2] = data_02086c60;
        data_0209b138[3] = data_02086c90;
        data_0209b138[4] = data_02086bd8;
        data_0209b138[5] = data_02086d18;
        data_0209b138[6] = data_02086d98;
        data_0209b138[7] = data_02086c88;
        data_0209b138[8] = data_02086d00;
        data_0209b138[9] = data_02086bc0;
        data_0209b138[10] = data_02086e30;
        data_0209b138[11] = data_02086ce8;
        data_0209b138[12] = data_02086e38;
        data_0209b138[13] = data_02086e20;
        data_0209b138[14] = data_02086cd0;
        data_0209b138[15] = data_02086e08;
        data_0209b138[16] = data_02086df0;
        data_0209b138[17] = data_02086cb8;
        data_0209b138[18] = data_02086dd8;
        data_0209b138[19] = data_02086dc0;
        data_0209b138[20] = data_02086c20;
        data_0209b138[21] = data_02086da0;
        data_0209b138[22] = data_02086d60;
        data_0209b138[23] = data_02086d88;
        data_0209b138[24] = data_02086cf8;
        data_0209b138[25] = data_02086d68;
        data_0209b138[26] = data_02086c78;
        data_0209b138[27] = data_02086d40;
        data_0209b138[28] = data_02086d38;
        data_0209b138[29] = data_02086d20;
        data_0209b138[30] = data_02086c50;
        data_0209b138[31] = data_02086d08;
        data_0209b138[32] = data_02086cf0;
        data_0209b138[33] = data_02086c38;
        data_0209b138[34] = data_02086cd8;
        data_0209b138[35] = data_02086cc0;
        data_0209b138[36] = data_02086cb0;
        data_0209b138[37] = data_02086ca8;
        data_0209b138[38] = data_02086ca0;
#endif
    }
#if defined(SM64DS_PLATFORM_PC)
    if (obj == 0) {
        if (std::getenv("SM64DS_PC_TRACE")) {
            std::fprintf(stderr, "[sm64ds] message dispatch skipped null target command=%u\n", msg[6]);
            std::fflush(stderr);
        }
        return 0;
    }
    if (std::getenv("SM64DS_PC_TRACE")) {
        std::fprintf(stderr, "[sm64ds] message dispatch obj=%p msg=%p command=%u handler=%p a2=%08x a3=%08x\n",
            (void *)obj, msg, msg[6], (void *)data_0209b138[msg[6]], a2, a3);
        std::fflush(stderr);
    }
    return data_0209b138[msg[6]](obj, msg + 7, a2, a3);
#else
    return (obj->*data_0209b138[msg[6]])(msg + 7, a2, a3);
#endif
}
