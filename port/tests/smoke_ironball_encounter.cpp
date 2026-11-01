#include "../hal/ironball_encounter_state.h"
#include <cmath>
#include <cstdio>
int main() {
    IronballEncounterState p;
    if(p.owns(true,200))return 1;
    // Recorded r3 turn: enter at22240, cross old boundary by22255/22260.
    const double positions[][2]={{-2932071,-3257449},{-3337928,-3307283},{-3415145,-3413061}};
    for(auto &v:positions)if(!p.owns(true,std::hypot(v[0]/4096+750,v[1]/4096+750)))return 2;
    p.phase=IronballEncounterState::Return;
    if(!p.owns(true,300))return 3; // Route cannot override recovery either.
    p.phase=IronballEncounterState::Kick;p.began=123;
    if(!p.owns(true,130))return 4; // Jump drift retains attack ownership.
    if(p.owns(false,20) || p.began || p.phase!=IronballEncounterState::Approach)return 5;
    if(p.owns(true,200))return 6; // Next scene cannot inherit old ownership.
    std::puts("PASS: recorded turn boundary, return, jump drift, scene reset");
}
