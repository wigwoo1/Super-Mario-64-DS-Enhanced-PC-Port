#pragma once

// Opt-in input-only pilot for the measured Battlefield iron-ball approach.
// It may select an earlier route point, but never changes game/actor state.
static bool pc_replay_ironball_approach(Player *p,const char *route,unsigned &waypoint,
                                       bool has_y,int target_y) {
    const char *mode=std::getenv("SM64DS_PC_TEST_IRONBALL_APPROACH");
    const bool after_reentry=mode && !std::strcmp(mode,"after-reentry");
    if(!mode || (!after_reentry && std::strcmp(mode,"1")) ||
       (after_reentry && !replay_lava_bridge_reentered) || data_0209f2f8!=6 || p->mCharacter!=3 ||
       p->mIsNoControl || p->mIsControlDisabled || data_0209d660)return false;
    const auto tick=ntr::rt_frame();
    const char *state=pc_player_state_name(p);
    const bool grounded=!p->mIsAirborne &&
        (!std::strcmp(state,"St_Wait_Main") || !std::strcmp(state,"St_Walk_Main"));
    static bool hurt=false;
    static bool ironball_hurt=false;
    static unsigned recoveries=0;
    static uint64_t attack_until=0,next_attack=0,swallow_until=0,next_swallow=0;
    static bool reentry_state_reset=false;
    if(replay_lava_bridge_reentered && !reentry_state_reset) {
        hurt=false;ironball_hurt=false;recoveries=0;
        attack_until=next_attack=swallow_until=next_swallow=0;
        pc_ironball_consume_player_hurt();
        reentry_state_reset=true;
        std::fprintf(stderr,"[ironball approach] reset pilot after course re-entry tick=%llu\n",
            static_cast<unsigned long long>(ntr::rt_frame()));
    }
    auto controls=[](uint16_t buttons=0,int16_t x=0,int16_t y=0){
        ntr::input_test_controls(true,buttons,x,y);return true;
    };
    if(pc_ironball_consume_player_hurt())ironball_hurt=true;
    if(!std::strcmp(state,"St_Hurt_Main")){hurt=true;return controls();}
    const char *recover_mode=std::getenv("SM64DS_PC_TEST_IRONBALL_RECOVER");
    if(ironball_hurt && recover_mode && !std::strcmp(recover_mode,"1") && grounded &&
       p->mPosZ> -1500*4096 && p->mPosZ< -300*4096 &&
       p->mPosX> -1800*4096 && p->mPosX< -500*4096 && recoveries<1) {
        // After an actual track hit, route toward the first explicitly supplied
        // safe Battlefield waypoint beyond the hazard. The caller owns that
        // waypoint; this pilot only advances the route index and submits normal
        // stick input. It never changes player pose, health, collision or actor
        // state. This is opt-in test recovery, not game behavior.
        const char *segment=route;unsigned index=0,safe_waypoint=waypoint;bool found=false;
        while(segment) {
            int level,x,y,z,radius,jump;
            if(index>waypoint &&
               std::sscanf(segment,"%d:%d:%d:%d:%d:%d",&level,&x,&y,&z,&radius,&jump)==6 &&
               level==6 && y<=80 && x<=-1800 && z<=-1500) {
                safe_waypoint=index;found=true;break;
            }
            segment=std::strchr(segment,',');if(segment)++segment;
            ++index;
        }
        if(found) {
            std::fprintf(stderr,"[ironball approach] hit recovery tick=%llu waypoint=%u->%u\n",
                static_cast<unsigned long long>(tick),waypoint,safe_waypoint);
            waypoint=safe_waypoint;++recoveries;hurt=false;ironball_hurt=false;return controls();
        }
    }
    if(hurt && grounded && has_y && target_y>240 && p->mPosY<120*4096 &&
       p->mPosZ>4800*4096 && recoveries<2) {
        const char *segment=route;unsigned best=waypoint;
        double distance=1e100;
        for(unsigned i=0;segment && i<waypoint;++i) {
            int level,x,y,z,radius,jump;
            if(std::sscanf(segment,"%d:%d:%d:%d:%d:%d",&level,&x,&y,&z,&radius,&jump)==6 &&
               level==6 && y<=80 && y>=-20) {
                const double d=std::hypot(x*4096.0-p->mPosX,z*4096.0-p->mPosZ);
                if(d<distance){best=i;distance=d;}
            }
            segment=std::strchr(segment,',');if(segment)++segment;
        }
        if(best<waypoint) {
            std::fprintf(stderr,"[ironball approach] recovery tick=%llu waypoint=%u->%u\n",
                static_cast<unsigned long long>(tick),waypoint,best);
            waypoint=best;++recoveries;hurt=false;return controls();
        }
    }
    // Keep normal route input outside the initial field/slope encounter.
    if(p->mPosZ<4800*4096 || p->mPosZ>6600*4096 ||
       p->mPosX< -4000*4096 || p->mPosX>1000*4096)return false;
    if(p->mObjInMouth) {
        if(tick>=next_swallow) {
            swallow_until=tick+8;next_swallow=tick+100;
            std::fprintf(stderr,"[ironball approach] swallow tick=%llu\n",
                static_cast<unsigned long long>(tick));
        }
        return controls(tick<swallow_until?0x100:0);
    }
    if(tick<attack_until)return controls(1);
    if(!grounded && tick<next_attack)return controls();
    daKrb_c *target=nullptr;double nearest=320*4096.0;
    for(unsigned kind:{200u,201u})
        for(auto *a=dActor_c::FindWithActorID(kind,nullptr);a;a=dActor_c::FindWithActorID(kind,a)) {
            auto *g=static_cast<daKrb_c *>(a);
            if(a->aliveState!=1 || g->mEatenByYoshi || g->mDeathState ||
               std::abs(a->mPosY-p->mPosY)>150*4096)continue;
            const double d=std::hypot(double(a->mPosX)-p->mPosX,double(a->mPosZ)-p->mPosZ);
            if(d<nearest){nearest=d;target=g;}
        }
    if(!target || !grounded)return false;
    const double angle=std::atan2(double(target->mPosX)-p->mPosX,double(target->mPosZ)-p->mPosZ);
    const int error=static_cast<int16_t>(int(std::lround(angle*65536.0/6.283185307179586))-p->mAngleY);
    if(nearest<260*4096.0 && std::abs(error)<2500) {
        if(tick>=next_attack) {
            attack_until=tick+8;next_attack=tick+80;
            std::fprintf(stderr,"[ironball approach] tongue tick=%llu target=%u distance=%.1f\n",
                static_cast<unsigned long long>(tick),target->uniqueID,nearest/4096.0);
        }
        return controls(tick<attack_until?1:0);
    }
    const double relative=angle-GetAngleToCamera(0)*6.283185307179586/65536.0;
    return controls(0,static_cast<int16_t>(std::sin(relative)*11500),
                      static_cast<int16_t>(-std::cos(relative)*11500));
}
