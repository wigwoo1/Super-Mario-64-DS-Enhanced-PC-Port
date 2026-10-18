/* FLOOR under 1.2/sp2p3 — same PTMF below-sp staging wall as func_0204322c (size 0x68 vs 0x5c). */
#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#define PC_PROCESS_TRACE(label) do { \
    const char *trace = getenv("SM64DS_PC_TRACE"); \
    if (trace && trace[0] && trace[0] != '0') { \
        fprintf(stderr, "[sm64ds] actor process: %s\n", label); \
        fflush(stderr); \
    } \
} while (0)
#else
#define PC_PROCESS_TRACE(label) ((void)0)
#endif
typedef struct { int a; int b; } PMF;
extern int _ZN7fBase_c7ProcessEMS_FivEMS_FbvEMS_FvjE(void *self, PMF a, PMF b, PMF c);
extern PMF data_02099e94;
extern PMF data_02099ec4;
extern PMF data_02099ebc;

int func_0204335c(void *self)
{
    PC_PROCESS_TRACE("Process: begin");
    int result = _ZN7fBase_c7ProcessEMS_FivEMS_FbvEMS_FvjE(
        self, data_02099ebc, data_02099ec4, data_02099e94);
    PC_PROCESS_TRACE("Process: complete");
    return result;
}
