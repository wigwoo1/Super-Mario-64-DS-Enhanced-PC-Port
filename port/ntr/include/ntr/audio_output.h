#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace ntr {
// Capture/analyze the mixer block before this device-only gate. Never mute
// the engine itself: voice completion and gameplay sound waits must advance.
inline uint64_t audio_prepare_submission(int16_t *samples,size_t count,bool muted) {
    if(muted){std::fill(samples,samples+count,int16_t(0));return 0;}
    uint64_t nonzero=0;
    for(size_t i=0;i<count;++i)nonzero+=samples[i]!=0;
    return nonzero;
}
}
