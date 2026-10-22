#pragma once
#include "ntr/sdat.h"
#include <algorithm>
#include <vector>

namespace ntr {
// Separate suspension preserves each song's retail pause flag and sample state.
// Menu effects keep using the ordinary sound player while this theme plays.
class PauseMusic {
    SdatEngine &engine;
    bool wanted=false,playing=false;
    std::vector<uint32_t> held;
    static constexpr uint32_t theme_slot=0xfff00001u;
public:
    explicit PauseMusic(SdatEngine &sound):engine(sound) {}
    bool requested() const {return wanted;}
    bool active() const {return playing && engine.active(theme_slot);}
    size_t retained() const {return held.size();}
    void music_started(uint32_t key) {
        if(!playing || key==theme_slot)return;
        engine.suspend(key,true);
        if(std::find(held.begin(),held.end(),key)==held.end())held.push_back(key);
    }
    void update(bool enabled,const std::vector<uint32_t> &music) {
        if(enabled==wanted)return;
        wanted=enabled;
        if(enabled) {
            if(!engine.play_sequence_isolated(5,theme_slot))return;
            playing=true;
            for(auto key:music)music_started(key);
        } else reset();
    }
    void reset() {
        engine.stop(theme_slot);
        for(auto key:held)engine.suspend(key,false);
        held.clear();wanted=playing=false;
    }
};
}
