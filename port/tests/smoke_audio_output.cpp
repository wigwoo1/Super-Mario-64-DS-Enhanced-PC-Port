#include "ntr/audio_output.h"
#include "ntr/audio_layout.h"
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
    using ntr::AudioLayout;
    using ntr::audio_surround_endpoint_mask;
    if(audio_surround_endpoint_mask(AudioLayout::Surround51,2,0x3f)!=0 ||
       audio_surround_endpoint_mask(AudioLayout::Surround51,6,0x3f)!=0x3f ||
       audio_surround_endpoint_mask(AudioLayout::Surround51,6,0x60f)!=0x60f ||
       audio_surround_endpoint_mask(AudioLayout::Surround51,8,0x63f)!=0x60f ||
       audio_surround_endpoint_mask(AudioLayout::Surround71,8,0x63f)!=0x63f ||
       audio_surround_endpoint_mask(AudioLayout::Surround71,8,0xff)!=0 ||
       audio_surround_endpoint_mask(AudioLayout::Surround71,6,0x63f)!=0)return 6;
    std::array<int16_t,16> surround;
    surround.fill(123);
    if(ntr::audio_prepare_submission(surround.data(),surround.size(),true)!=0)return 7;
    for(auto sample:surround)if(sample)return 8;
    std::puts("PASS device gate zeros muted PCM, preserves capture and leaves normal output unchanged");
}
