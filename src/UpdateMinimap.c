#include "types.h"
#ifdef SM64DS_PLATFORM_PC
#include "hal/stage_graph_state.h"
#endif
// UpdateMinimap: copies a 16-byte descriptor into data_0209f3c8 and stores
// the four scalar params into data_0209f3c4 (+0x14..+0x20).
typedef struct MinimapDesc { u32 a, b, c, d; } MinimapDesc;

typedef struct MinimapState {
    char _pad[0x14];
    volatile u32 f14, f18, f1c, f20;
} MinimapState;

#ifndef SM64DS_PLATFORM_PC
extern MinimapDesc data_0209f3c8;
extern MinimapState data_0209f3c4;
#endif

void UpdateMinimap(MinimapDesc *desc, u32 a, u32 b, u32 c, u32 d)
{
#ifdef SM64DS_PLATFORM_PC
    /* Keep the descriptor and four offsets inside the callback's actual
       allocation instead of relying on adjacency of two generated globals. */
    data_0209f3c4.matrix[0] = desc->a;
    data_0209f3c4.matrix[1] = desc->b;
    data_0209f3c4.matrix[2] = desc->c;
    data_0209f3c4.matrix[3] = desc->d;
    data_0209f3c4.center_x = a;
    data_0209f3c4.center_y = b;
    data_0209f3c4.destination_x = c;
    data_0209f3c4.destination_y = d;
#else
    *(MinimapDesc *)(((long long)(int)&data_0209f3c8)) = *desc;
    data_0209f3c4.f14 = a;
    data_0209f3c4.f18 = b;
    data_0209f3c4.f1c = c;
    data_0209f3c4.f20 = d;
#endif
}
