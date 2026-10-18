#include "OverlayID.h"
#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#define PC_OVERLAY_JOB_TRACE(label) do { \
    const char *trace = getenv("SM64DS_PC_TRACE"); \
    if (trace && trace[0] && trace[0] != '0') { \
        fprintf(stderr, "[sm64ds] boot load job: %s\\n", label); \
        fflush(stderr); \
    } \
} while (0)
#else
#define PC_OVERLAY_JOB_TRACE(label) ((void)0)
#endif

/* func_0201a2f8 at 0x0201a2f8
 *
 * Matched byte-for-byte with mwccarm 1.2/sp2p3 (arm9 main).
 */
extern void func_0201fec8(void);
extern void func_02018aa4(void);
extern void func_020134c8(void);
extern int GetSoundMode(void);
extern void SetSoundMode(int mode);
extern int func_0205d94c(int a, int b);
extern void* _ZN6Memory8AllocateEj(unsigned int);
extern void LoadOverlay(int id);
extern void func_ov001_020aa420(void);
extern void UnloadOverlay(int id);
extern void* func_0205d23c(void* p, int n);
extern void func_0205c91c(void);
extern void _ZN6Memory10DeallocateEPv(void*);
extern void _ZN5Sound19LoadGroupAndSetBankEii(int a, int b);
extern void _ZN5Sound16LoadInitialGroupEi(int group);
extern void LoadFont(int id);

extern int overlay_0;
extern int overlay_1;
extern int data_0208ee5c;

void func_0201a2f8(void) {
#if defined(SM64DS_PLATFORM_PC)
    // The DS firmware mailbox selects an alternate audio/save flow. The PC
    // host supplies offline save defaults and silent audio during startup.
    int cond = 0;
#else
    int cond = (*(unsigned short*)0x27ffc40 == 2);
#endif
    void* ptr;
    PC_OVERLAY_JOB_TRACE("begin");
    if (cond) {
        func_0201fec8();
        func_02018aa4();
        func_020134c8();
        SetSoundMode(GetSoundMode());
    }
    ptr = 0;
#if !defined(SM64DS_PLATFORM_PC)
    if (!cond) {
        int size = func_0205d94c(0, 0);
        ptr = _ZN6Memory8AllocateEj(size);
        func_0205d94c((int)ptr, size);
    }
#endif
    PC_OVERLAY_JOB_TRACE("before overlay 0");
    LoadOverlay(OVERLAY_ID(0));
    PC_OVERLAY_JOB_TRACE("overlay 0 loaded");
    func_ov001_020aa420();
    PC_OVERLAY_JOB_TRACE("overlay 1 entry returned");
    UnloadOverlay(OVERLAY_ID(0));
    if (ptr != 0) {
        func_0205d23c(&data_0208ee5c, 3);
        func_0205c91c();
        _ZN6Memory10DeallocateEPv(ptr);
    }
    PC_OVERLAY_JOB_TRACE("before overlay 1");
    LoadOverlay(OVERLAY_ID(1));
    PC_OVERLAY_JOB_TRACE("overlay 1 loaded");
#if !defined(SM64DS_PLATFORM_PC)
    if (cond) {
        _ZN5Sound19LoadGroupAndSetBankEii(0x2f, 0x36);
    } else {
        _ZN5Sound16LoadInitialGroupEi(0);
    }
#endif
    if (cond) return;
    LoadFont(0);
}
