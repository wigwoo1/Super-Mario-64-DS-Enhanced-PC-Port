#include "types.h"
#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#define PC_ARCHIVE_TRACE(label, idx) do { const char *t = getenv("SM64DS_PC_TRACE"); if (t && t[0] && t[0] != '0') { fprintf(stderr, "[sm64ds] archive %d: %s\n", idx, label); fflush(stderr); } } while (0)
#else
#define PC_ARCHIVE_TRACE(label, idx) ((void)0)
#endif
struct Entry {
    int field_0;     /* 0x0 */
    int field_4;     /* 0x4 */
    int field_8;     /* 0x8 */
    int field_c;     /* 0xc */
    int field_10;    /* 0x10 */
};

extern struct Entry data_0208ecf4[];
extern void *data_020a0ea0;
#if defined(SM64DS_PLATFORM_PC)
/* The DS table stores ARM pointers; translate them to the host asset catalog. */
static const char *const pc_archive_names[] = {
    "arc0", "ar1", "vs1", "vs2", "vs3", "vs4", "c2d",
    "en1", "cee", "cef", "ceg", "cei", "ces"
};
static const char *const pc_archive_paths[] = {
    "ARCHIVE/arc0.narc", "ARCHIVE/ar1.narc",
    "ARCHIVE/vs1.narc", "ARCHIVE/vs2.narc",
    "ARCHIVE/vs3.narc", "ARCHIVE/vs4.narc",
    "ARCHIVE/c2d.narc", "ARCHIVE/en1.narc",
    "ARCHIVE/cee.narc", "ARCHIVE/cef.narc",
    "ARCHIVE/ceg.narc", "ARCHIVE/cei.narc",
    "ARCHIVE/ces.narc"
};
#endif

extern int func_02018934(int a, int b, int c);

int LoadArchive(int idx)
{
    struct Entry *e = &data_0208ecf4[idx];
    PC_ARCHIVE_TRACE("load begin", idx);
    if (e->field_0 == 0) {
        e->field_4 = (int)data_020a0ea0;
        PC_ARCHIVE_TRACE("archive file open begin", idx);
#if defined(SM64DS_PLATFORM_PC)
        if (idx < 0 || idx >= (int)(sizeof(pc_archive_paths) / sizeof(pc_archive_paths[0]))) {
            PC_ARCHIVE_TRACE("unknown archive index", idx);
            return 0;
        }
        /* A host string literal may sit above the 32-bit ARM address space;
           pass the archive table index as a small token instead. */
        e->field_0 = (int)func_02018934((int)pc_archive_names[idx],
            idx + 1, e->field_4);
#else
        e->field_0 = func_02018934(e->field_c, e->field_10, e->field_4);
#endif
        PC_ARCHIVE_TRACE("archive file open complete", idx);
    }
    PC_ARCHIVE_TRACE("load complete", idx);
    return e->field_0 != 0;
}
