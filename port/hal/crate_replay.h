#pragma once
#include "daObjBlockS_c.h"
#include "daYegg_c.h"
#include <vector>

// Opt-in acceptance pilot: observe an authored crate and supply ordinary
// jump/R/stick input. Never change actor state, position, rewards or saves.
static bool pc_replay_crate_pound(Player *p) {
    const char *mode=std::getenv("SM64DS_PC_TEST_CRATE");
    const bool repeat=mode && !std::strcmp(mode,"pound-reset");
    const bool spit=mode && !std::strcmp(mode,"eat-spit");
    const bool eat=(mode && !std::strcmp(mode,"eat")) || spit;
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(!mode || (!eat && !repeat && std::strcmp(mode,"pound")) || !automation || std::strcmp(automation,"1") ||
       !p || p->aliveState!=1)return false;
    static unsigned target=0;
    static unsigned originalCoinMax=0,rewards[3]={},rewardCount=0;
    static unsigned round=1;
    struct Point {int x,y,z,radius;};
    static std::vector<Point> resetRoute;
    static bool parsed=false,respawnObserved=false;
    static int travelWaypoint=0;
    if(repeat && !parsed) {
        parsed=true;
        const char *segment=std::getenv("SM64DS_PC_TEST_CRATE_RESET_ROUTE");
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
        for(auto *a=dActor_c::FindWithActorID(288,nullptr);a;a=dActor_c::FindWithActorID(288,a))
            if(a->uniqueID>originalCoinMax)originalCoinMax=a->uniqueID;
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
                   std::hypot(double(a->mPosX)-crate->mHomePosX,double(a->mPosZ)-crate->mHomePosZ)>200*4096.0)continue;
                bool known=false;
                for(unsigned i=0;i<rewardCount;++i)if(rewards[i]==a->uniqueID)known=true;
                if(!known && rewardCount<3)rewards[rewardCount++]=a->uniqueID;
            }
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
    return controls();
}
