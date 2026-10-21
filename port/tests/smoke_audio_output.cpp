#include "ntr/audio_output.h"
#include <array>
#include <cstdio>
int main() {
    const std::array<int16_t,8> mixed{{-32768,-100,0,100,32767,0,42,-1}};
    auto device=mixed;const auto capture=mixed;
    if(ntr::audio_prepare_submission(device.data(),device.size(),false)!=6 || device!=mixed)return 1;
    if(ntr::audio_prepare_submission(device.data(),device.size(),true)!=0)return 2;
    for(auto sample:device)if(sample)return 3;
    if(capture!=mixed)return 4;
    if(ntr::audio_prepare_submission(device.data(),device.size(),false)!=0)return 5;
    std::puts("PASS device gate zeros muted PCM, preserves capture and leaves normal output unchanged");
}
