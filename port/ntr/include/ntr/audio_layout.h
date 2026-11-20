#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace ntr {

enum class AudioLayout : uint8_t { Stereo, Surround51, Surround71 };
enum class AudioOutput : uint8_t { Silent, Stereo, Surround51, Surround71 };
constexpr unsigned audio_max_channels=8;

constexpr unsigned audio_channel_count(AudioLayout layout) {
    return layout==AudioLayout::Surround71?8:layout==AudioLayout::Surround51?6:2;
}

// WAVEFORMATEXTENSIBLE bit order. The 5.1 mix uses side surrounds.
constexpr uint32_t audio_channel_mask(AudioLayout layout) {
    return layout==AudioLayout::Surround71?0x63fu:layout==AudioLayout::Surround51?0x60fu:0x3u;
}

constexpr uint32_t audio_surround_endpoint_mask(AudioLayout requested,unsigned channels,uint32_t mask) {
    if(requested==AudioLayout::Surround71)
        return channels>=8 && (mask&0x63fu)==0x63fu ? 0x63fu : 0u;
    if(requested==AudioLayout::Surround51 && channels>=6) {
        if((mask&0x60fu)==0x60fu)return 0x60fu; // Side surround configuration.
        if((mask&0x3fu)==0x3fu)return 0x3fu;    // Back surround configuration.
    }
    return 0;
}

inline const char *audio_layout_name(AudioLayout layout) {
    return layout==AudioLayout::Surround71?"7.1":layout==AudioLayout::Surround51?"5.1":"STEREO";
}

inline const char *audio_output_name(AudioOutput output) {
    switch(output) {
    case AudioOutput::Surround71:return "7.1";
    case AudioOutput::Surround51:return "5.1";
    case AudioOutput::Stereo:return "STEREO";
    default:return "SILENT";
    }
}

struct AudioPosition {
    int32_t x,y,z;  // Camera-space fixed point. Forward is -Z.
};
using SpeakerGains=std::array<double,audio_max_channels>;

inline SpeakerGains audio_speaker_gains(AudioLayout layout,AudioPosition position) {
    SpeakerGains gains{};
    constexpr int32_t unavailable=std::numeric_limits<int32_t>::max();
    if(position.x==unavailable || position.y==unavailable || position.z==unavailable)
        return gains;
    if(layout==AudioLayout::Stereo) {
        gains[0]=gains[1]=0.7071067811865476;
        return gains;
    }

    constexpr double pi=3.14159265358979323846;
    const double angle=position.x==0 && position.z==0?0:
        std::atan2(double(position.x),-double(position.z))*180.0/pi;
    struct Speaker { double angle; unsigned channel; };
    // Repeat the first speaker at the far end of each ring for wraparound.
    constexpr Speaker surround51[]={{-250,5},{-110,4},{-30,0},{0,2},{30,1},{110,5},{250,4}};
    constexpr Speaker surround71[]={{-210,5},{-150,4},{-90,6},{-30,0},{0,2},{30,1},{90,7},{150,5},{210,4}};
    const bool wide=layout==AudioLayout::Surround71;
    const Speaker *ring=wide?surround71:surround51;
    const unsigned count=wide?unsigned(sizeof(surround71)/sizeof(Speaker)):unsigned(sizeof(surround51)/sizeof(Speaker));
    for(unsigned i=1;i<count;++i) {
        if(angle>ring[i].angle)continue;
        const auto &first=ring[i-1],&second=ring[i];
        const double blend=(angle-first.angle)/(second.angle-first.angle)*(pi/2);
        gains[first.channel]=std::cos(blend);
        gains[second.channel]=std::sin(blend);
        break;
    }
    return gains;
}

inline void audio_mix_frame(int32_t *mix,int left,int right,unsigned channels,
                            const SpeakerGains *positional) {
    if(!positional) {
        mix[0]+=left;
        mix[1]+=right;
        return;
    }
    // Averaging keeps correlated stereo samples at unity before panning.
    // The original stereo bus and the system's bass management stay separate.
    const double signal=(left+right)*0.5;
    for(unsigned i=0;i<channels;++i) {
        const double gain=(*positional)[i];
        if(gain>0)mix[i]+=static_cast<int32_t>(std::lround(signal*gain));
    }
}
}
