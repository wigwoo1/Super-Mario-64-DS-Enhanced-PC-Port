#pragma once
#include "daObjBlockS_c.h"
#include "daYegg_c.h"
#include "hal/replay_path.h"
#include <vector>

// QA lifecycle bookkeeping only. A scene change must discard the old
// authored actor identity before looking for the newly spawned crate.
static unsigned pc_crate_reentry_epoch=0;
static unsigned pc_crate_completed_rounds=0;

// Opt-in acceptance pilot: observe an authored crate and supply ordinary
// jump/R/stick input. Never change actor state, position, rewards or saves.
static bool pc_replay_crate_pound(Player *p) {
    const char *mode=std::getenv("SM64DS_PC_TEST_CRATE");
    const bool repeat=mode && !std::strcmp(mode,"pound-reset");
    const bool reentry=mode && !std::strcmp(mode,"pound-reentry");
    const bool carryThrow=mode && !std::strcmp(mode,"carry-throw");
    const bool pickupAfterTwo=carryThrow && std::getenv("SM64DS_PC_TEST_CRATE_PICKUP_AFTER_TWO") &&
        !std::strcmp(std::getenv("SM64DS_PC_TEST_CRATE_PICKUP_AFTER_TWO"),"1");
    const bool spit=mode && !std::strcmp(mode,"eat-spit");
    const bool eat=(mode && !std::strcmp(mode,"eat")) || spit;
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(!mode || (!eat && !repeat && !reentry && !carryThrow && std::strcmp(mode,"pound")) || !automation || std::strcmp(automation,"1") ||
       !p || p->aliveState!=1)return false;
    static unsigned target=0;
    static unsigned originalCoinMax=0,rewards[3]={},rewardCount=0;
    static unsigned round=1;
    struct Point {int x,y,z,radius;};
    static std::vector<Point> resetRoute;
    static bool parsed=false,respawnObserved=false;
    static int travelWaypoint=0;
    static bool pickupRouteStarted=false;
    if((repeat || carryThrow) && !parsed) {
        parsed=true;
        const char *segment=std::getenv(carryThrow?"SM64DS_PC_TEST_CRATE_PICKUP_ROUTE":"SM64DS_PC_TEST_CRATE_RESET_ROUTE");
        while(segment && *segment && resetRoute.size()<1024) {
            Point point;int consumed=0;
            if(std::sscanf(segment,"%d:%d:%d:%d%n",&point.x,&point.y,&point.z,&point.radius,&consumed)!=4 ||
               consumed<=0 || (segment[consumed] && segment[consumed]!=',') ||
               point.x< -32000 || point.x>32000 || point.y< -32000 || point.y>32000 ||
               point.z< -32000 || point.z>32000 || point.radius<1 || point.radius>300) {
                resetRoute.clear();break;
            }
            resetRoute.push_back(point);
            segment=segment[consumed]?segment+consumed+1:nullptr;
        }
    }
    if(repeat && resetRoute.empty())return false;
    static int phase=0;
    static uint64_t began=0,brokenAt=0;
    static int approachX=0,approachZ=0;
    static int lastFlightX=0,lastFlightZ=0;
    static unsigned observedEpoch=0;
    if(reentry && observedEpoch!=pc_crate_reentry_epoch) {
        observedEpoch=pc_crate_reentry_epoch;
        target=originalCoinMax=rewardCount=0;
        phase=0;began=brokenAt=0;round=pc_crate_completed_rounds+1;
    }
    if(reentry && (data_0209f2f8!=13 || phase==9))return false;
    daObjBlockS_c *crate=nullptr;
    for(auto *a=dActor_c::FindWithActorID(194,nullptr);a;a=dActor_c::FindWithActorID(194,a)) {
        if(a->aliveState!=1)continue;
        if(target ? a->uniqueID==target :
           std::abs(a->mPosY-p->mPosY)<200*4096 &&
           std::hypot(double(a->mPosX)-p->mPosX,double(a->mPosZ)-p->mPosZ)<220*4096.0) {
            crate=static_cast<daObjBlockS_c *>(a);break;
        }
    }
    if(!crate)return false;
    const auto tick=ntr::rt_frame();
    const char *state=pc_player_state_name(p);
    uint16_t buttons=0;int16_t sx=0,sy=0;
    auto controls=[&](){ntr::input_test_controls(true,buttons,sx,sy);return true;};
    auto event=[&](const char *name){
        std::fprintf(stderr,"[native crate pound replay] %s tick=%llu uid=%u crate_state=%d player=%s distance=%.2f height=%.2f round=%u flags=%x paid=%u\n",
            name,static_cast<unsigned long long>(tick),crate->uniqueID,crate->mState,state,
            std::hypot(double(crate->mPosX)-p->mPosX,double(crate->mPosZ)-p->mPosZ)/4096.0,
            (p->mPosY-crate->mPosY)/4096.0,round,unsigned(crate->mFlags),unsigned(crate->mCoinsPaid));
    };
    auto steer=[&](double x,double z){
        const double dx=x-p->mPosX,dz=z-p->mPosZ,distance=std::hypot(dx,dz);
        if(distance<10*4096.0)return;
        const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
        double magnitude=distance/(160*4096.0)*22000;
        if(magnitude<12000)magnitude=12000;
        if(magnitude>22000)magnitude=22000;
        sx=static_cast<int16_t>(std::sin(angle)*magnitude);
        sy=static_cast<int16_t>(-std::cos(angle)*magnitude);
    };
    auto walkRoute=[&](bool reverse) {
        // Reuse the tested projected-path follower so full-speed movement
        // cannot orbit a small floor marker after overshooting it.
        if(carryThrow) {
            std::vector<replay_path::Point> points;
            for(const auto &point:resetRoute)points.push_back({double(point.x),double(point.z),double(point.radius)});
            const auto aim=replay_path::follow(points.data(),points.size(),static_cast<unsigned>(travelWaypoint),
                p->mPosX/4096.0,p->mPosZ/4096.0,100,100);
            if(travelWaypoint!=static_cast<int>(aim.waypoint) || tick%30==0)
                std::fprintf(stderr,"[native crate pickup route] tick=%llu waypoint=%d->%u aim=%.1f,%.1f\n",
                    static_cast<unsigned long long>(tick),travelWaypoint,unsigned(aim.waypoint),aim.x,aim.z);
            travelWaypoint=static_cast<int>(aim.waypoint);
            if(!aim.complete) {
                const double angle=std::atan2(aim.x*4096.0-p->mPosX,aim.z*4096.0-p->mPosZ)-GetAngleToCamera(0)*6.283185307179586/65536.0;
                const double magnitude=pickupAfterTwo && p->mIsAirborne?12000:32767;
                sx=static_cast<int16_t>(std::sin(angle)*magnitude);sy=static_cast<int16_t>(-std::cos(angle)*magnitude);
            }
            return aim.complete;
        }
        if(travelWaypoint<0 || travelWaypoint>=static_cast<int>(resetRoute.size()))return true;
        const auto &point=resetRoute[travelWaypoint];
        const double distance=std::hypot(point.x*4096.0-p->mPosX,point.z*4096.0-p->mPosZ);
        if(distance<point.radius*4096.0 && std::abs(point.y*4096-p->mPosY)<120*4096 && !p->mIsAirborne) {
            std::fprintf(stderr,"[native crate reset route] reached=%d reverse=%d tick=%llu\n",travelWaypoint,int(reverse),static_cast<unsigned long long>(tick));
            travelWaypoint+=reverse?-1:1;
        } else steer(point.x*4096.0,point.z*4096.0);
        return false;
    };
    if(!target){
        target=crate->uniqueID;phase=1;began=tick;event("acquired");
        if(carryThrow){phase=10;approachX=p->mPosX;approachZ=p->mPosZ;}
        for(auto *a=dActor_c::FindWithActorID(288,nullptr);a;a=dActor_c::FindWithActorID(288,a))
            if(a->uniqueID>originalCoinMax)originalCoinMax=a->uniqueID;
    }
    if(carryThrow && phase!=4 && phase!=5) {
        if(crate->mState==daObjBlockS_c::STATE_THROWN) {
            lastFlightX=crate->mPosX;lastFlightZ=crate->mPosZ;
        }
        if(crate->mState==daObjBlockS_c::STATE_BROKEN) {
            phase=4;brokenAt=tick;event("throw-broken");
        } else {
            if(phase==10) {
                steer(crate->mPosX,crate->mPosZ);
                const auto distance=std::hypot(double(crate->mPosX)-p->mPosX,double(crate->mPosZ)-p->mPosZ);
                if(distance<140*4096.0 && !std::strcmp(state,"St_Walk_Main")) {
                    phase=11;began=tick;buttons=1;event("press-grab-attack");
                }
            } else if(phase==11) {
                if(tick-began<8)buttons=1;
                steer(crate->mPosX,crate->mPosZ);
                if(crate->mState==daObjBlockS_c::STATE_CARRIED && crate->mHoldingPlayer==p) {
                    phase=12;began=tick;sx=sy=0;event("carried");
                } else if(tick-began>180) {phase=19;sx=sy=0;event("grab-timeout");}
            } else if(phase==12) {
                if(!std::strcmp(state,"St_HoldLight_Main") && p->mStateStep>=1) {
                    steer(approachX,approachZ);
                    if(tick-began>=60 && std::hypot(double(approachX)-p->mPosX,double(approachZ)-p->mPosZ)<25*4096.0) {
                        phase=13;began=tick;sx=sy=0;event("carry-walk-finished");
                    }
                }
            } else if(phase==13) {
                steer(crate->mHomePosX,crate->mHomePosZ);
                const double angle=std::atan2(double(crate->mHomePosX)-p->mPosX,double(crate->mHomePosZ)-p->mPosZ)*65536.0/6.283185307179586;
                const int delta=static_cast<int16_t>(static_cast<int>(angle)-p->mAngleY);
                if(tick-began>=10 && std::abs(delta)<0x1000 && !std::strcmp(state,"St_HoldLight_Main")) {
                    phase=14;began=tick;buttons=1;sx=sy=0;event("press-throw-attack");
                }
            } else if(phase==14) {
                if(tick-began<8)buttons=1;
                if(crate->mState==daObjBlockS_c::STATE_THROWN) {phase=15;began=tick;event("crate-in-flight");}
                else if(tick-began>180){phase=19;event("throw-timeout");}
            } else if(phase==15 && crate->mState==daObjBlockS_c::STATE_IDLE) {
                phase=19;event("throw-landed");
            }
            return controls();
        }
    }
    if(eat) {
        if(phase==1) {
            steer(crate->mPosX,crate->mPosZ);
            const double distance=std::hypot(double(crate->mPosX)-p->mPosX,double(crate->mPosZ)-p->mPosZ);
            if(!p->mIsAirborne && distance<120*4096.0 && !std::strcmp(state,"St_Walk_Main")) {
                phase=2;began=tick;buttons=1;sx=sy=0;event("press-attack");
            }
        } else if(phase==2) {
            if(tick-began<8)buttons=1;
            if(p->mObjInMouth==reinterpret_cast<std::intptr_t>(crate)) {
                phase=3;event("captured-in-mouth");
            }
        } else if(phase==3 && crate->mState==daObjBlockS_c::STATE_BROKEN) {
            phase=4;began=tick;event("turned-into-egg");
        } else if(phase==4 && spit && tick-began>=180) {
            phase=5;began=tick;buttons=1;event("press-egg-attack");
        } else if(phase==5) {
            if(tick-began<8)buttons=1;
            auto *egg=static_cast<daYegg_c *>(dActor_c::FindWithActorID(9,nullptr));
            if(egg && egg->aliveState==1 && (egg->mFlags & daYegg_FLAG_FLYING)) {
                phase=6;event("egg-in-flight");
            }
        }
        return controls();
    }
    if(crate->mState==daObjBlockS_c::STATE_BROKEN && phase<4) {
        phase=4;brokenAt=tick;event("broken");
    }
    if(phase==1) {
        steer(crate->mPosX,crate->mPosZ);
        const double distance=std::hypot(double(crate->mPosX)-p->mPosX,double(crate->mPosZ)-p->mPosZ);
        if(!p->mIsAirborne && distance<145*4096.0 && !std::strcmp(state,"St_Walk_Main")) {
            phase=2;began=tick;event("running-jump");
        }
    }
    if(phase==2) {
        if(tick-began<8)buttons=2;
        steer(crate->mPosX,crate->mPosZ);
        // Preserve running momentum instead of starting from a neutral jump.
        const double dx=crate->mPosX-double(p->mPosX),dz=crate->mPosZ-double(p->mPosZ);
        const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
        sx=static_cast<int16_t>(std::sin(angle)*32767);
        sy=static_cast<int16_t>(-std::cos(angle)*32767);
        if(p->mIsAirborne) {
            const auto distance=std::hypot(double(crate->mPosX)-p->mPosX,double(crate->mPosZ)-p->mPosZ);
            if(!std::strcmp(state,"St_Jump_Main") && p->mPosY>crate->mPosY+110*4096 && distance<22*4096.0) {
                phase=3;began=tick;buttons=0x100;sx=sy=0;event("press-R");
            }
        }
    }
    if(phase==3 && tick-began<8)buttons=0x100;
    if(phase==4) {
        // Capture newly released coin identities at birth. A dropped coin can
        // roll far downhill; its later distance from the crate is not its ID.
        if(rewardCount<3 && tick-brokenAt<10) {
            for(auto *a=dActor_c::FindWithActorID(288,nullptr);a;a=dActor_c::FindWithActorID(288,a)) {
                if(a->aliveState!=1 || a->uniqueID<=originalCoinMax ||
                   std::hypot(double(a->mPosX)-(carryThrow?lastFlightX:crate->mHomePosX),
                              double(a->mPosZ)-(carryThrow?lastFlightZ:crate->mHomePosZ))>200*4096.0)continue;
                bool known=false;
                for(unsigned i=0;i<rewardCount;++i)if(rewards[i]==a->uniqueID)known=true;
                if(!known && rewardCount<3)rewards[rewardCount++]=a->uniqueID;
            }
        }
        unsigned liveRewards=0;
        for(unsigned i=0;i<rewardCount;++i) {
            auto *reward=dActor_c::FindWithID(rewards[i]);
            if(reward && reward->aliveState==1)++liveRewards;
        }
        if(carryThrow && rewardCount==3 && liveRewards==0) {
            phase=5;sx=sy=0;event("reward-actors-ended");return controls();
        }
        if(pickupAfterTwo && !pickupRouteStarted && rewardCount==3 && liveRewards==1) {
            pickupRouteStarted=true;event("pickup-after-two-start");
        }
        if(carryThrow && (!pickupAfterTwo || pickupRouteStarted) && travelWaypoint<static_cast<int>(resetRoute.size())) {
            if(!walkRoute(false))return controls();
            event("pickup-route-finished");
        }
        dActor_c *coin=nullptr;double nearest=1e30;
        for(auto *a=dActor_c::FindWithActorID(288,nullptr);a;a=dActor_c::FindWithActorID(288,a)) {
            bool reward=false;
            for(unsigned i=0;i<rewardCount;++i)if(rewards[i]==a->uniqueID)reward=true;
            if(a->aliveState!=1 || !reward)continue;
            const auto distance=std::hypot(double(a->mPosX)-p->mPosX,double(a->mPosZ)-p->mPosZ);
            if(distance<nearest){nearest=distance;coin=a;}
        }
        if(coin) {
            steer(coin->mPosX,coin->mPosZ);
            if(nearest>120*4096.0) {
                const double angle=std::atan2(double(coin->mPosX)-p->mPosX,double(coin->mPosZ)-p->mPosZ)-GetAngleToCamera(0)*6.283185307179586/65536.0;
                sx=static_cast<int16_t>(std::sin(angle)*32767);
                sy=static_cast<int16_t>(-std::cos(angle)*32767);
            }
        }
        if(tick-brokenAt>600){phase=5;sx=sy=0;event("collection-window-ended");}
    }
    if(reentry && phase==5) {
        ++pc_crate_completed_rounds;phase=9;event("course-round-complete");
        return false;
    }
    if(repeat && round==1 && phase==5) {
        phase=6;travelWaypoint=0;event("walk-away");
    }
    if(phase==6) {
        if(!respawnObserved && crate->mState==daObjBlockS_c::STATE_IDLE) {
            respawnObserved=true;event("original-respawn");
        }
        if(walkRoute(false)){phase=7;began=tick;event("far-point");}
    }
    if(phase==7 && crate->mState==daObjBlockS_c::STATE_IDLE && tick-began>=60) {
        phase=8;travelWaypoint=static_cast<int>(resetRoute.size())-1;event("return");
    }
    if(phase==8) {
        const auto distance=std::hypot(double(crate->mPosX)-p->mPosX,double(crate->mPosZ)-p->mPosZ);
        if(crate->mState==daObjBlockS_c::STATE_IDLE && distance<220*4096.0 &&
           std::abs(crate->mPosY-p->mPosY)<200*4096) {
            round=2;phase=1;rewardCount=0;began=tick;event("second-approach");
        } else walkRoute(true);
    }
    if(carryThrow && phase==5 && !std::strcmp(state,"St_LedgeHang_Main") && tick%45<3) {
        buttons=2; // Original ledge jump/climb input after the lower-floor pickup.
        if(tick%45==0)event("ledge-exit-jump");
    }
    return controls();
}
