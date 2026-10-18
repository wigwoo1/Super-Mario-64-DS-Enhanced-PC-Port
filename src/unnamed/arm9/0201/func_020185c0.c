#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#define PC_FILE_TRACE(...) do { \
    const char *t = getenv("SM64DS_PC_TRACE"); \
    if (t && t[0] && t[0] != '0') { \
        fprintf(stderr, "[sm64ds] archive member: "); \
        fprintf(stderr, __VA_ARGS__); \
        fputc('\n', stderr); \
        fflush(stderr); \
    } \
} while (0)
#else
#define PC_FILE_TRACE(...) ((void)0)
#endif

struct P {
    int a;
    int b;
};

struct Entry {
    char *f0;
    int f4;
    unsigned short f8;
    unsigned short fa;
    int fc;
    int f10;
};

extern void FS_InitFile(int *s);
extern void func_02018598(struct P *c, unsigned int arg);
extern int func_02018dc4(int a, struct P v);
extern int LoadArchive(int idx);
extern void Crash(void);
extern int func_0204eda4(int r0, char *a, unsigned int idx);

extern struct Entry data_0208ecf4[];
extern unsigned char data_0208eb54[];

int func_020185c0(int a0, unsigned int a1)
{
    int result = 0;

    FS_InitFile((int *)a0);

    if (a1 < 0x8000) {
        struct P arg;
        struct P v;
        func_02018598(&v, a1);
        arg.a = v.a;
        arg.b = v.b;
        result = func_02018dc4(a0, arg);
    } else {
        unsigned int i = 0;
        struct Entry *e = data_0208ecf4;
        do {
            if (e->f8 <= a1 && a1 < e->fa) {
                if (e->f0 == 0) {
                    int loaded = LoadArchive(i);
                    if (loaded != 0)
                        data_0208eb54[0] = (unsigned char)i;
                    if (e->f0 == 0) {
                        PC_FILE_TRACE("archive load failed member=%04x archive=%u range=%04x..%04x result=%d",
                                      a1, i, e->f8, e->fa, loaded);
                        Crash();
                        return 0;
                    }
                }
                result = func_0204eda4(a0, e->f0, a1 - e->f8);
                break;
            }
            i++;
            e++;
        } while (i < 0xd);
    }

    if (result == 0) {
        PC_FILE_TRACE("archive member read failed member=%04x result=%d",
                      a1, result);
        Crash();
    }
    return result;
}
