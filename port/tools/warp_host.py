"""Keep the original warp cylinder/state logic, with opt-in native tracing."""


def transform(text):
    init = 's32 daWarpkun_c::InitResources()\n{'
    request = '                        WarpPlayer(player->mPlayerNo, warpParam & 0xff);'
    assert text.count(init) == 1 and text.count(request) == 1, 'Warp trigger source changed'
    text = text.replace(init, init + '''
    if(std::getenv("SM64DS_PC_WARP_TRACE"))std::fprintf(stderr,
        "[native warp] init uid=%u param=%08x pos=%d,%d,%d\\n",
        uniqueID,param1,mPosX,mPosY,mPosZ);''')
    text = text.replace(request, '''                        if(std::getenv("SM64DS_PC_WARP_TRACE"))std::fprintf(stderr,
                            "[native warp] request tick=%llu trigger=%u player=%u destination=%u\\n",
                            static_cast<unsigned long long>(ntr::rt_frame()),uniqueID,
                            player->uniqueID,warpParam & 0xff);
''' + request)
    return '''#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include "ntr/rt.h"
#include "daWarpkun_c.h"
static_assert(sizeof(daWarpkun_c)==0x108,"warp actor layout");
static_assert(offsetof(daWarpkun_c,mdCcAc_c)==0xd4,"warp cylinder offset");
''' + text
