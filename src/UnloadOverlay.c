#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#define PC_UNLOAD_OVERLAY_TRACE(label) do { \
    const char *trace = getenv("SM64DS_PC_TRACE"); \
    if (trace && trace[0] && trace[0] != '0') { \
        fprintf(stderr, "[sm64ds] UnloadOverlay: %s\\n", label); \
        fflush(stderr); \
    } \
} while (0)
#else
#define PC_UNLOAD_OVERLAY_TRACE(label) ((void)0)
#endif
extern void Crash(void);
extern void func_0205dc0c(int a, int b);
extern void func_02018c00(void *rgn, int b, int c);
extern int func_0203d7b8(void);
extern void func_02017fd0(void *rgn);

struct Rec { int a, b, c; };
struct CodeRegion { int w[10]; };   /* 0x28 */
extern struct Rec data_0209d3c4[];

void UnloadOverlay(int id)
{
#if defined(SM64DS_PLATFORM_PC)
    if (id == 0 || id == 1 || id == 2) {
        PC_UNLOAD_OVERLAY_TRACE("overlay remains statically linked");
        return;
    }
    PC_UNLOAD_OVERLAY_TRACE("dynamic overlay unload skipped; loader is pending");
    return;
#else
    struct CodeRegion rgn;
    struct Rec *found = 0;
    unsigned int i;
    struct Rec *r = data_0209d3c4;
    for (i = 0; i < 0xc; i++, r++) {
        if (r->a == id) {
            r->a = -1;
            r->b = 0;
            found = r;
            r->c = 0;
        }
    }
    if (found == 0)
        Crash();
    func_0205dc0c(0, id);
    func_02018c00(&rgn, 0, id);
    if (func_0203d7b8() != 0)
        return;
    func_02017fd0(&rgn);
#endif
}
