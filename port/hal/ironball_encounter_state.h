#pragma once
#include <cstdint>
// Private input-replay state. Arrival radius starts ownership, never ends it.
struct IronballEncounterState {
    enum Phase { Approach, Defend, Return, Align, Wait, Kick, Complete };
    Phase phase=Approach;
    uint64_t began=0,punch_until=0,next_punch=0;
    bool owns(bool context,double distance) {
        if(!context){*this=IronballEncounterState{};return false;}
        if(phase==Approach && distance<90)phase=Defend;
        return phase!=Approach;
    }
};
