#pragma once
#include "daObjFallBlock_c.h"

// Input-only crossing QA. Observe the original linked-row reset before
// returning across it; never accept a later respawn as a successful return.
static bool pc_replay_fall_block_crossing() {
    const char *enabled=std::getenv("SM64DS_PC_TEST_FALL_BLOCK_CROSSING");
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(!enabled||std::strcmp(enabled,"1")||!automation||std::strcmp(automation,"1")||
       data_0209f2f8!=46)return false;
    const char *cycle=std::getenv("SM64DS_PC_TEST_FALL_BLOCK_SCENE_CYCLE");
    const bool scene_cycle=cycle&&!std::strcmp(cycle,"1");
    static unsigned phase=0,visit=1,first_row_uid=0;
    if(!phase&&!replay_course_route_complete)return false;
    auto *p=static_cast<Player *>(data_0209f394[0]);
    if(!p||p->aliveState!=1)return false;
    daObjFallBlock_c *row[7]={};
    for(auto *a=dActor_c::FindWithActorID(66,nullptr);a;a=dActor_c::FindWithActorID(66,a)) {
        auto *b=static_cast<daObjFallBlock_c *>(a);
        if(b->mRestPos.z!=0)continue;
        const int offset=b->mRestPos.x-2350*4096;
        if(offset>=0&&offset<=600*4096&&offset%(100*4096)==0)
            row[offset/(100*4096)]=b;
    }
    auto event=[&](const char *name){
        std::fprintf(stderr,"[native falling-bridge crossing] event=%s tick=%llu phase=%u x=%d y=%d z=%d state=%s visit=%u row_uid=%u\n",
            name,static_cast<unsigned long long>(ntr::rt_frame()),phase,p->mPosX,p->mPosY,p->mPosZ,pc_player_state_name(p),
            visit,row[0]?unsigned(row[0]->uniqueID):0);
    };
    // The original wrong door reloads level46 at entrance2. Observe fresh
    // actor identities before allowing a second crossing; never load a level
    // or reset an actor from the test pilot itself.
    if(scene_cycle&&phase==7) {
        // Entrance2 is across the same gap from the fixed island. The initial
        // diagnostic entrance drops directly onto that island, so its first
        // walking waypoint cannot be reused until this ordinary jump lands.
        if(!p->mIsNoControl&&!p->mIsControlDisabled) {
            if(replay_course_route("46:0:0:1400:30:0,46:0:0:700:30:1,46:300:0:700:40:0"))return true;
            if(replay_course_route_complete) {
                event("original-entrance-island-reached");
                phase=0;replay_course_route_waypoint=0;replay_course_route_complete=false;
            }
        }
        ntr::input_test_controls(true,0,0,0);return true;
    }
    if(scene_cycle&&phase==5) {
        if(row[0]&&row[0]->aliveState==1&&row[0]->uniqueID!=first_row_uid) {
            visit=2;event("original-maze-scene-reloaded");
            phase=7;replay_course_route_waypoint=0;replay_course_route_complete=false;
        } else if(!p->mIsNoControl&&!p->mIsControlDisabled) {
            // Align on the fixed central island before crossing the gap.
            // A diagonal approach rides the spinning floor into the corridor
            // wall. Live floor/line queries qualify the straight center path.
            // The spinning floor can carry Wario around its center while he
            // walks toward the fixed island. Use the normal route jump while
            // leaving the floor so the reset-door path can reach solid ground.
            if(replay_course_route("46:1500:0:-75:40:0,46:300:0:-75:35:1,46:300:0:700:40:0,46:0:0:700:25:0,46:0:0:1400:30:1,46:0:0:1950:30:0"))return true;
        }
        ntr::input_test_controls(true,0,0,0);return true;
    }
    if(!phase){phase=1;first_row_uid=row[0]?unsigned(row[0]->uniqueID):0;event("forward-bank-reached");}
    bool reset=true;
    for(auto *b:row)reset=reset&&b&&b->aliveState==1&&b->mState==0&&b->mReady;
    if(phase==1&&reset&&!p->mIsAirborne&&!p->mIsNoControl&&!p->mIsControlDisabled) {
        event("original-group-reset-observed");
        phase=2;replay_course_route_waypoint=0;replay_course_route_complete=false;
    }
    if(phase==2) {
        if(p->mPosY< -500*4096){phase=4;event("return-failed-fall");}
        else if(!p->mIsNoControl&&!p->mIsControlDisabled) {
            const bool driving=replay_course_route("46:3150:0:0:30:0,46:2150:0:0:40:0");
            if(driving)return true;
            if(replay_course_route_complete){phase=3;event("return-bank-reached");}
        }
    }
    if(scene_cycle&&phase==3) {
        if(visit==1) {
            phase=5;replay_course_route_waypoint=0;replay_course_route_complete=false;
            event("original-wrong-door-approach");
        } else {phase=6;event("scene-cycle-complete");}
    }
    ntr::input_test_controls(true,0,0,0);return true;
}

// Opt-in ordinary-input recovery after the original fragile bridge gives way.
// Actor state and player transforms are observed only.
static bool pc_replay_fall_block_recovery() {
    static unsigned phase=0;
    const char *enabled=std::getenv("SM64DS_PC_TEST_FALL_BLOCK_RECOVERY");
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(!enabled||std::strcmp(enabled,"1")||!automation||std::strcmp(automation,"1")||
       data_0209f2f8!=46||(!replay_course_route_complete && !phase))return false;
    auto *p=static_cast<Player *>(data_0209f394[0]);
    if(!p||p->aliveState!=1||p->mIsNoControl||p->mIsControlDisabled)return false;
    daObjFallBlock_c *block=nullptr;
    for(auto *a=dActor_c::FindWithActorID(66,nullptr);a;a=dActor_c::FindWithActorID(66,a)) {
        auto *candidate=static_cast<daObjFallBlock_c *>(a);
        if(candidate->mRestPos.x==2350*4096 && candidate->mRestPos.z==0){block=candidate;break;}
    }
    if(!block)return false;
    static bool began=false,done=false;
    static uint64_t jumpUntil=0;
    const auto tick=ntr::rt_frame();const char *state=pc_player_state_name(p);
    auto event=[&](const char *name){std::fprintf(stderr,"[native falling-block recovery] event=%s tick=%llu uid=%u state=%s x=%d y=%d z=%d block_state=%u ready=%u flags=%08x collider=%d\n",
        name,static_cast<unsigned long long>(tick),unsigned(block->uniqueID),state,p->mPosX,p->mPosY,p->mPosZ,
        unsigned(block->mState),unsigned(block->mReady),unsigned(block->mFlags),block->mMeshCollider.IsEnabled());};
    const char *recycle=std::getenv("SM64DS_PC_TEST_FALL_BLOCK_RECYCLE");
    if(recycle && !std::strcmp(recycle,"1")) {
        // Input-only QA for the original linked-row rearm rule.  The pilot
        // waits for the actual fall, walks through the original room door,
        // observes the same loaded row reset, then returns and stands again.
        // Level46 uses a ten-unit separation plus the original offscreen bit.
        // It never writes actor/player state or collision data.
        uint16_t buttons=0;int16_t sx=0,sy=0;
        auto steer=[&](double targetX,double targetZ) {
            const double dx=targetX*4096.0-p->mPosX,dz=targetZ*4096.0-p->mPosZ;
            const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
            sx=static_cast<int16_t>(std::sin(angle)*18000);
            sy=static_cast<int16_t>(-std::cos(angle)*18000);
            return std::hypot(dx,dz);
        };
        if(!phase) {
            if(block->mState!=3)return false;
            phase=1;began=true;event("original-fall-complete");
        }
        if(phase==1) {
            if(!jumpUntil && !std::strcmp(state,"St_LedgeHang_Main")) {
                jumpUntil=tick+12;event("ledge-jump-input");
            }
            if(tick<jumpUntil)buttons=2;
            if(!p->mIsAirborne && (!std::strcmp(state,"St_Wait_Main")||!std::strcmp(state,"St_Walk_Main"))) {
                const double distance=steer(2150.0,0.0);
                if(distance<30*4096.0) {phase=2;jumpUntil=0;event("normal-control-restored");sx=sy=0;}
            }
        } else if(phase==2) {
            if(!p->mIsAirborne && steer(1500.0,0.0)<30*4096.0) {
                phase=3;sx=sy=0;event("original-door-retreat-complete");
            }
        } else if(phase==3) {
            if(block->mState==0 && block->mReady) {
                phase=4;event("native-group-reset");
                replay_course_route_waypoint=0;replay_course_route_complete=false;
            }
        } else if(phase==4) {
            if(block->mState>=1) {
                phase=5;sx=sy=0;buttons=0;event("repeat-triggered");
            } else return replay_course_route("46:1600:0:-75:35:1,46:2050:0:-75:30:0,46:2200:0:0:25:1,46:2350:0:0:20:0");
        } else if(phase==5 && block->mState==3) {
            phase=6;jumpUntil=0;event("repeat-fall-complete");
        } else if(phase==6) {
            if(!jumpUntil && !std::strcmp(state,"St_LedgeHang_Main")) {
                jumpUntil=tick+12;event("repeat-ledge-jump-input");
            }
            if(tick<jumpUntil)buttons=2;
            if(!p->mIsAirborne && (!std::strcmp(state,"St_Wait_Main")||!std::strcmp(state,"St_Walk_Main"))) {
                if(steer(2150.0,0.0)<30*4096.0) {
                    phase=7;sx=sy=0;event("repeat-normal-control-restored");
                }
            }
        }
        ntr::input_test_controls(true,buttons,sx,sy);return true;
    }
    if(block->mState!=3)return false;
    uint16_t buttons=0;int16_t sx=0,sy=0;
    if(!began){began=true;event("original-fall-complete");}
    if(!done && !jumpUntil && !std::strcmp(state,"St_LedgeHang_Main")) {
        jumpUntil=tick+12;event("ledge-jump-input");
    }
    if(tick<jumpUntil)buttons=2;
    if(!done && !p->mIsAirborne && (!std::strcmp(state,"St_Wait_Main")||!std::strcmp(state,"St_Walk_Main"))) {
        const double dx=2150.0*4096-p->mPosX,dz=-double(p->mPosZ);
        if(std::hypot(dx,dz)<30*4096.0){done=true;event("normal-control-restored");}
        else {const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
            sx=static_cast<int16_t>(std::sin(angle)*18000);sy=static_cast<int16_t>(-std::cos(angle)*18000);}
    }
    ntr::input_test_controls(true,buttons,sx,sy);return true;
}
