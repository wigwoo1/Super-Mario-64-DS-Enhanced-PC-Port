//cpp
// @symbol _ZN5Model13GetVramOffsetEj
#include "Model.h"
#ifdef SM64DS_PLATFORM_PC
#include <cstdio>
#endif
extern "C" {
extern u32 data_020a4bc8;
extern u32 data_020a4bdc;
extern u32 data_020a4be0;
extern u32 data_020a4be8;
void Crash();
}

u32 Model::GetVramOffset(u32 size)
{
    u32 *p;
    u32 b = data_020a4be8 - data_020a4bc8;
    u32 a = data_020a4bdc - data_020a4be0;
    if ((a << 1) > b && a > size) {
        p = &data_020a4bdc;
    } else if (b > size) {
        p = &data_020a4be8;
    } else {
#ifdef SM64DS_PLATFORM_PC
        std::fprintf(stderr, "[texture arena] failed size=%08x blocks=%08x ceilingA=%08x indices=%08x ceilingB=%08x freeA=%08x freeB=%08x\n",
            size, data_020a4bc8, data_020a4be8, data_020a4be0, data_020a4bdc, b, a);
#endif
        Crash();
    }
    *p -= size;
    return *p;
}
