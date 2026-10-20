#pragma once
#include <algorithm>
#include <cstdint>

namespace ntr {
// Matches the recovered SoundEngine 8-bit-fraction interpolation. Advance on
// game sound updates, independently of device sample rate or sequencer pause.
struct AudioVolumeRamp {
    int current=127<<8,start=127<<8,target=127<<8;
    int elapsed=0,duration=0;
    void set(int volume,int frames) {
        start=current;target=std::max(0,std::min(127,volume))<<8;
        elapsed=0;duration=std::max(0,frames);
        if(!duration)current=target;
    }
    void tick() {
        if(elapsed<duration){
            ++elapsed;
            current=start+int(int64_t(target-start)*elapsed/duration);
        }
    }
    int value() const {return current>>8;}
    bool done() const {return elapsed>=duration;}
};
}
