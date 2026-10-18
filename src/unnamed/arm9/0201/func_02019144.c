typedef volatile unsigned int vu32;

#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
extern unsigned char data_0209e660;
extern int data_0209e664;
extern int data_0209e670;
extern unsigned int data_0209e674[];
extern unsigned int data_0209ea74[];
#endif

typedef struct UnkObj UnkObj;
typedef struct UnkVt {
    void *pad0;
    void *pad4;
    int (*func8)(UnkObj *);
} UnkVt;
struct UnkObj {
    UnkVt *vt;
};

extern UnkObj *data_0209d4a8;
extern unsigned char data_0209d45c;
extern unsigned char data_0209d454;
extern short data_0209d468;
extern short data_0209d46c;
extern short data_0209d4a4;
extern short data_0209d4a0;
extern short data_0209d49c;
extern short data_0209d478;
extern short data_0209d48c;
extern short data_0209d490;
extern short data_0209d494;
extern short data_0209d498;
extern short data_0209d484;
extern short data_0209d480;
extern short data_0209d47c;
extern short data_0209d470;
extern short data_0209d474;
extern short data_0209d488;

void _ZN3OAM5FlushEv(void);
void _ZN3OAM4LoadEv(void);
void func_02055454(int v);

void func_02019144(void)
{
    UnkObj *p = data_0209d4a8;
    int graph_result = -1;
#if defined(SM64DS_PLATFORM_PC)
    static unsigned int pc_sync_entries;
    const char *trace = getenv("SM64DS_PC_TRACE");
    ++pc_sync_entries;
#endif
    if (p != 0) {
        graph_result = p->vt->func8(p);
        if (graph_result == 0) {
#if defined(SM64DS_PLATFORM_PC)
            if (trace && trace[0] && trace[0] != '0' &&
                (pc_sync_entries <= 2 || pc_sync_entries % 120 == 0)) {
                fprintf(stderr, "[pc vblank sync] entries=%u graph=%p result=%d skipped=oam\n",
                    pc_sync_entries, (void *)p, graph_result);
                fflush(stderr);
            }
#endif
            return;
        }
    }
    _ZN3OAM5FlushEv();
    _ZN3OAM4LoadEv();
#if defined(SM64DS_PLATFORM_PC)
    {
        static unsigned int pc_oam_transfer_count;
        ++pc_oam_transfer_count;
        if (trace && trace[0] && trace[0] != '0' &&
            (pc_oam_transfer_count <= 2 || pc_oam_transfer_count % 120 == 0)) {
            fprintf(stderr,
                "[pc vblank sync] entries=%u graph=%p result=%d oam=uploaded sub_suppressed=%u main_fill=%d sub_fill=%d main_stage0=%08x sub_stage0=%08x main_hw0=%08x sub_hw0=%08x\n",
                pc_sync_entries, (void *)p, graph_result, data_0209e660, data_0209e664,
                data_0209e670, data_0209e674[0], data_0209ea74[0],
                *(volatile unsigned int *)0x07000000,
                *(volatile unsigned int *)0x07000400);
            fflush(stderr);
        }
    }
#endif
    *(vu32 *)0x4000000 = (*(vu32 *)0x4000000 & ~0x1f00) | (data_0209d45c << 8);
    *(vu32 *)0x4001000 = (*(vu32 *)0x4001000 & ~0x1f00) | (data_0209d454 << 8);
    if (!(*(vu32 *)0x4000000 & 8)) {
        *(vu32 *)0x4000010 = (data_0209d468 & 0x1ff) | (0x1ff0000 & (data_0209d46c << 16));
    } else {
        func_02055454(data_0209d468);
    }
    *(vu32 *)0x4000014 = (data_0209d4a4 & 0x1ff) | (0x1ff0000 & (data_0209d4a0 << 16));
    *(vu32 *)0x4000018 = (data_0209d49c & 0x1ff) | (0x1ff0000 & (data_0209d478 << 16));
    *(vu32 *)0x400001c = (data_0209d48c & 0x1ff) | (0x1ff0000 & (data_0209d490 << 16));
    *(vu32 *)0x4001000;
    *(vu32 *)0x4001010 = (data_0209d494 & 0x1ff) | (0x1ff0000 & (data_0209d498 << 16));
    *(vu32 *)0x4001014 = (data_0209d484 & 0x1ff) | (0x1ff0000 & (data_0209d480 << 16));
    *(vu32 *)0x4001018 = (data_0209d47c & 0x1ff) | (0x1ff0000 & (data_0209d470 << 16));
    *(vu32 *)0x400101c = (data_0209d474 & 0x1ff) | (0x1ff0000 & (data_0209d488 << 16));
}
