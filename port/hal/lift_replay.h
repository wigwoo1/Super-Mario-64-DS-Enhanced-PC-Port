#pragma once
#include "daObjRotateUpdownLift_c.h"

// Isolated, opt-in input pilot. All lift/player/contact fields are read only;
// the only output is the same stick and jump input used in ordinary play.
static bool pc_replay_lift_ride(Player *p,bool routeComplete,bool courseReentered) {
    const char *enabled=std::getenv("SM64DS_PC_TEST_LIFT_RIDE");
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(!enabled || std::strcmp(enabled,"1") || !automation || std::strcmp(automation,"1") ||
       !routeComplete || data_0209f2f8!=6 || !p || p->aliveState!=1)return false;
    // Test-only variant selection lets QA exercise either authored Battlefield
    // lift without changing actor placement or gameplay behavior.
    unsigned requestedVariant=2;
    if(const char *value=std::getenv("SM64DS_PC_TEST_LIFT_VARIANT")) {
        char *end=nullptr;
        const auto parsed=std::strtoul(value,&end,10);
        if(end!=value && !*end && (parsed==2 || parsed==7))requestedVariant=static_cast<unsigned>(parsed);
    }
    enum {WAIT=1,BOARD,RIDE,DISEMBARK,WALK_AWAY,DONE,FAILED};
    static int phase=WAIT;
    static unsigned target=0,attempts=0;
    static unsigned visit=1;
    static bool releasedFinal=false;
    static uint64_t began=0,jumpUntil=0,neutralUntil=0;
    const char *lifecycle=std::getenv("SM64DS_PC_TEST_LAVA_BRIDGE_LIFECYCLE");
    const bool cycle=lifecycle && !std::strcmp(lifecycle,"1");
    const auto tick=ntr::rt_frame();
    uint16_t buttons=0;int16_t sx=0,sy=0;
    auto controls=[&](){ntr::input_test_controls(true,buttons,sx,sy);return true;};
    const bool grounded=p->mMeshClsn.IsOnGround()!=0 && !p->mIsAirborne;
    const auto *hit=grounded?static_cast<dBgPi *>(p->mMeshClsn.GetFloorResult()):nullptr;
    const auto *owner=hit && hit->colliderIdx<24?hit->owner:nullptr;
    auto *lift=target?static_cast<daObjRotateUpdownLift_c *>(dActor_c::FindWithID(target)):nullptr;
    auto event=[&](const char *name){
        std::fprintf(stderr,"[native lift ride] event=%s tick=%llu phase=%d attempt=%u target=%u variant=%u player=%d,%d,%d state=%s visit=%u\n",
            name,static_cast<unsigned long long>(tick),phase,attempts,target,requestedVariant,
            p->mPosX,p->mPosY,p->mPosZ,pc_player_state_name(p),visit);
    };
    if(cycle && courseReentered && visit==1) {
        // Reset only the pilot after the original menu/painting lifecycle has
        // loaded new actors and the normal input route reaches staging again.
        const unsigned oldTarget=target;
        visit=2;phase=WAIT;target=attempts=0;began=jumpUntil=neutralUntil=0;
        releasedFinal=false;lift=nullptr;event("reset-after-reentry");
        std::fprintf(stderr,"[native lift ride] retired-target=%u visit=2\n",oldTarget);
    }
    auto move=[&](double x,double z){
        const double dx=x*4096-p->mPosX,dz=z*4096-p->mPosZ;
        const double distance=std::hypot(dx,dz)/4096;
        if(distance<18)return true;
        const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
        const double magnitude=(std::max)(11000.0,(std::min)(24000.0,distance*160));
        sx=static_cast<int16_t>(std::sin(angle)*magnitude);
        sy=static_cast<int16_t>(-std::cos(angle)*magnitude);
        return false;
    };
    if(!began){began=tick;event("staging");}
    if(phase!=DONE && phase!=FAILED &&
       (tick-began>3600 || !std::strcmp(pc_player_state_name(p),"St_Squish_Main"))) {
        phase=FAILED;event("failed-timeout-or-squish");
    }
    if(p->mIsNoControl || p->mIsControlDisabled)return controls();
    if(phase==WAIT) {
        const double stagingZ=requestedVariant==7?4500.0:4350.0;
        const unsigned lowWaypoint=1u;
        if(move(1350,stagingZ) && grounded) {
            sx=sy=0;
            for(auto *a=dActor_c::FindWithActorID(29,nullptr);a;a=dActor_c::FindWithActorID(29,a)) {
                auto *candidate=static_cast<daObjRotateUpdownLift_c *>(a);
                if(a->aliveState==1 && a->param1==requestedVariant && !candidate->mIsDead &&
                   candidate->mWaypointIndex==lowWaypoint && a->mPosY<180*4096 &&
                   std::abs(a->mPosZ-static_cast<int>(stagingZ*4096))<300*4096 &&
                   (static_cast<unsigned short>(a->mAngleX)&0x7fff)<1024) {
                    lift=candidate;target=a->uniqueID;++attempts;phase=BOARD;
                    jumpUntil=tick+24;neutralUntil=tick+2;event("boarding-jump");break;
                }
            }
        }
    } else if(phase==BOARD) {
        if(owner && owner->uniqueID==target) {phase=RIDE;event("grounded-on-lift");}
        else if(lift && lift->aliveState==1) {
            move(double(lift->mPosX)/4096,double(lift->mPosZ)/4096);
            if(tick<jumpUntil)buttons=2;
            if(tick<neutralUntil)sx=sy=0;
            if(tick>jumpUntil+100 && grounded && p->mPosY<100*4096) {
                phase=attempts<2?WAIT:FAILED;target=0;event("boarding-missed");
            }
        } else {phase=FAILED;event("lift-unloaded");}
    } else if(phase==RIDE) {
        if(owner && owner->uniqueID==target && p->mPosY>=700*4096) {
            phase=DISEMBARK;jumpUntil=tick+24;neutralUntil=tick+2;
            buttons=2;event("upper-bank-jump");
        } else if(!owner || owner->uniqueID!=target) {phase=FAILED;event("lost-ride-contact");}
    } else if(phase==DISEMBARK) {
        move(1950,4350);
        if(tick<jumpUntil)buttons=2;
        if(tick<neutralUntil)sx=sy=0;
        if(grounded && (!owner || owner->uniqueID!=target) &&
           p->mPosX>1850*4096 && std::abs(p->mPosY-750*4096)<20*4096) {
            phase=WALK_AWAY;event("landed-upper-bank");
        }
    } else if(phase==WALK_AWAY) {
        if(move(2100,4350) && grounded && !std::strcmp(pc_player_state_name(p),"St_Wait_Main")) {
            phase=DONE;event("done-control-restored");
        }
    }
    if(const char *path=std::getenv("SM64DS_PC_LIFT_RIDE_TRACE")) {
        static FILE *file=nullptr;
        if(!file){file=std::fopen(path,"w");if(file)std::fputs("tick,phase,attempt,target,px,py,pz,state,grounded,owner_kind,owner_uid,ground_y,lx,ly,lz,node,angle,buttons,sx,sy,visit\n",file);}
        if(file){std::fprintf(file,"%llu,%d,%u,%u,%d,%d,%d,%s,%u,%u,%u,%d,%d,%d,%d,%u,%d,%u,%d,%d,%u\n",
            static_cast<unsigned long long>(tick),phase,attempts,target,p->mPosX,p->mPosY,p->mPosZ,
            pc_player_state_name(p),unsigned(grounded),owner?unsigned(owner->actorID):0,
            owner?unsigned(owner->uniqueID):0,p->mGroundY,lift?lift->mPosX:0,lift?lift->mPosY:0,
            lift?lift->mPosZ:0,lift?unsigned(lift->mWaypointIndex):0,lift?int(lift->mAngleX):0,
            unsigned(buttons),int(sx),int(sy),visit);std::fflush(file);}
    }
    if(cycle && phase==DONE && (visit==1 || !releasedFinal)) {
        // Hand control to the existing real Exit Course/painting pilot after
        // the first ride; let it record completion once after the second ride.
        releasedFinal=visit==2;
        ntr::input_test_controls(true,0,0,0);return false;
    }
    return controls();
}
