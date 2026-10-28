#pragma once
#include "daRNk_c.h"
#include "daRFlag_c.h"
#include "dBgCh_Gnd.h"
#include "dBgActor_c.h"
#include "pc_session.h"
extern "C" { extern unsigned char data_0209d6bc,data_0209d68c; }

// Read-only production collision queries for private route qualification.
// Input rows are x,y,z world units; output remains local to the QA case.
static void pc_probe_koopa_ground(Player *player) {
    static bool done=false;
    const char *input=std::getenv("SM64DS_PC_KOOPA_GROUND_SAMPLES");
    const char *output=std::getenv("SM64DS_PC_KOOPA_GROUND_CAPTURE");
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    const char *probeLevel=std::getenv("SM64DS_PC_GROUND_PROBE_LEVEL");
    int level=6;
    if(probeLevel){char *end=nullptr;const long parsed=std::strtol(probeLevel,&end,10);
        if(!end || *end || parsed<1 || parsed>51)return;level=static_cast<int>(parsed);}
    if(done || !input || !output || !automation || std::strcmp(automation,"1") ||
       data_0209f2f8!=level || !player || player->aliveState!=1 ||
       (!probeLevel && !dActor_c::FindWithActorID(188,nullptr)))return;
    // Moving colliders can be intentionally disabled while the real player
    // is distant. Qualify their surfaces only after an ordinary approach;
    // never enable them or move the player to satisfy a probe.
    if(const char *proximity=std::getenv("SM64DS_PC_KOOPA_GROUND_NEAR")) {
        int x,z,radius;char extra;
        if(std::sscanf(proximity,"%d:%d:%d%c",&x,&z,&radius,&extra)!=3 ||
           x< -32000 || x>32000 || z< -32000 || z>32000 || radius<1 || radius>5000)return;
        if(std::hypot(double(player->mPosX)/4096-x,double(player->mPosZ)/4096-z)>radius)return;
    }
    done=true;
    if(auto *actor=dActor_c::FindWithActorID(39,nullptr)) {
        auto *bridge=static_cast<dBgActor_c *>(actor);
        char path[4096];std::snprintf(path,sizeof(path),"%s.bridge.csv",output);
        if(FILE *snapshot=std::fopen(path,"w")) {
            auto matrix=[&](const char *name,const Matrix4x3 &m) {
                std::fprintf(snapshot,"%s",name);
                for(unsigned i=0;i<12;++i)std::fprintf(snapshot,",%d",reinterpret_cast<const int *>(&m)[i]);
                std::fputc('\n',snapshot);
            };
            matrix("model",bridge->mModel.mat4x3);
            matrix("collider",bridge->mMeshCollider.newScaledMat);
            matrix("inverse",bridge->mMeshCollider.invScaledMat);
            std::fprintf(snapshot,"state,%llu,%u,%u,%d,%d,%d\n",
                static_cast<unsigned long long>(ntr::rt_frame()),unsigned(actor->uniqueID),
                unsigned(bridge->mMeshCollider.IsEnabled()),int(actor->mAngleX),int(actor->mAngleY),int(actor->mAngleZ));
            std::fclose(snapshot);
        }
    }
    FILE *samples=std::fopen(input,"r"),*capture=std::fopen(output,"w");
    if(!samples || !capture) {
        if(samples)std::fclose(samples);
        if(capture)std::fclose(capture);
        return;
    }
    std::fputs("x,y,z,hit,height,nx,ny,nz,clps0,clps1,owner,triangle\n",capture);
    int x,y,z;unsigned count=0;
    while(count<65536 && std::fscanf(samples,"%d,%d,%d ",&x,&y,&z)==3) {
        if(std::abs(x)>32000 || std::abs(y)>32000 || std::abs(z)>32000)break;
        dBgCh_Gnd query;
        const Vector3 point={x*4096,y*4096,z*4096};
        query.SetObjAndPos(point,player);
        const int hit=query.DetectClsn();
        const auto &surface=query.surface;
        std::fprintf(capture,"%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u\n",
            x,y,z,hit,hit?int(query.clsnY):0,
            hit?surface.normal.x:0,hit?surface.normal.y:0,hit?surface.normal.z:0,
            hit?surface.clps.w0:0,hit?surface.clps.w1:0,
            hit && query.owner?unsigned(query.owner->actorID):0,
            hit?unsigned(query.triangleID):0);
        ++count;
    }
    std::fclose(samples);std::fclose(capture);
    std::fprintf(stderr,"[native Koopa ground probe] tick=%llu samples=%u; read-only production queries\n",
        static_cast<unsigned long long>(ntr::rt_frame()),count);
    if(const char *lines=std::getenv("SM64DS_PC_KOOPA_LINE_SAMPLES")) {
        char destination[4096];std::snprintf(destination,sizeof(destination),"%s.lines.csv",output);
        FILE *inputLines=std::fopen(lines,"r"),*result=std::fopen(destination,"w");
        if(inputLines && result) {
            std::fputs("ax,ay,az,bx,by,bz,hit,x,y,z,nx,ny,nz,owner,uid,triangle\n",result);
            int ax,ay,az,bx,by,bz;unsigned lineCount=0;
            while(lineCount++<4096 && std::fscanf(inputLines,"%d,%d,%d,%d,%d,%d ",&ax,&ay,&az,&bx,&by,&bz)==6) {
                if(ax< -32000 || ax>32000 || ay< -32000 || ay>32000 || az< -32000 || az>32000 ||
                   bx< -32000 || bx>32000 || by< -32000 || by>32000 || bz< -32000 || bz>32000)break;
                const Vector3 a={ax*4096,ay*4096,az*4096},b={bx*4096,by*4096,bz*4096};
                dBgCh_Lin query;query.SetObjAndLine(a,b,player);const int hit=query.DetectClsn();
                std::fprintf(result,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u\n",
                    ax,ay,az,bx,by,bz,hit,query.lineEnd.x,query.lineEnd.y,query.lineEnd.z,
                    hit?query.surface.normal.x:0,hit?query.surface.normal.y:0,hit?query.surface.normal.z:0,
                    hit && query.owner?unsigned(query.owner->actorID):0,
                    hit && query.owner?unsigned(query.owner->uniqueID):0,hit?unsigned(query.triangleID):0);
            }
        }
        if(inputLines)std::fclose(inputLines);
        if(result)std::fclose(result);
    }
}

// Opt-in, isolated QA. Observe original actors and submit ordinary buttons and
// camera-relative stick input. No placement, state, result or reward writes.
static bool pc_replay_koopa(Player *player,bool routeComplete) {
    const char *mode=std::getenv("SM64DS_PC_TEST_KOOPA");
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(!mode || (std::strcmp(mode,"start") && std::strcmp(mode,"race") && std::strcmp(mode,"select") &&
       std::strcmp(mode,"decline-retry") && std::strcmp(mode,"refusal")) || !automation || std::strcmp(automation,"1"))return false;
    const bool pilot=!std::strcmp(mode,"race");
    const bool declineRetry=!std::strcmp(mode,"decline-retry");
    const bool refusal=!std::strcmp(mode,"refusal");
    const auto tick=ntr::rt_frame();
    uint16_t buttons=0;int16_t sx=0,sy=0;
    auto controls=[&](){ntr::input_test_controls(true,buttons,sx,sy);return true;};
    unsigned selection=0,phase=0,selectionMode=0,confirm=0,ready=0;
    if(pc_star_select_status(&selection,&phase,&selectionMode,&confirm,&ready)) {
        // Reuse the validated QA mission setting while retaining Mission 2
        // for existing Koopa/scenery replays that do not specify a target.
        const unsigned target=std::getenv("SM64DS_PC_TEST_MISSION")
            ? static_cast<unsigned>(pc_test_mission()-1) : 1u;
        if(ready && !confirm && tick%30<3) {
            if(selectionMode==0) {
                if(phase==0)buttons=1;
                else if(selection!=target)buttons=selection>target?0x20:0x10;
                else buttons=1;
            } else if(selectionMode==2 && selection==target) {
                // With other characters unlocked, Yoshi's first confirm
                // opens the original character choice. Confirm Yoshi with
                // a second ordinary button edge instead of stalling there.
                buttons=1;
            }
        }
        return controls();
    }
    // Mission-selection-only QA leaves subsequent movement to the ordinary
    // course route. It must not approach Koopa after a scenery check finishes.
    if(!std::strcmp(mode,"select"))return false;
    if(data_0209f2f8!=6 || !routeComplete || !player || player->aliveState!=1)return false;
    auto *koopa=static_cast<daRNk_c *>(dActor_c::FindWithActorID(188,nullptr));
    if(!koopa || koopa->aliveState!=1)return false;
    static int previousState=-1;static bool pathLogged=false;
    static unsigned offers=0;static bool interactionDone=false,retreated=false;
    static unsigned trackedKoopa=0,node=1,lastJumpNode=~0u,detourIndex=0,pickupLeg=0;
    static uint64_t jumpUntil=0,nextJump=0,lastProgress=0;
    static double bestDistance=1e100;
    if(trackedKoopa!=koopa->uniqueID) {
        trackedKoopa=koopa->uniqueID;
        previousState=-1;pathLogged=false;offers=0;interactionDone=false;retreated=false;
        node=1;lastJumpNode=~0u;detourIndex=0;pickupLeg=0;
        jumpUntil=nextJump=lastProgress=0;bestDistance=1e100;
        std::fprintf(stderr,"[native Koopa replay] event=instance tick=%llu uid=%u; reset input-pilot bookkeeping only\n",
            static_cast<unsigned long long>(tick),trackedKoopa);
    }
    auto interaction=[&](const char *name) {
        std::fprintf(stderr,"[native Koopa interaction] event=%s tick=%llu offers=%u state=%d step=%u cooldown=%u answer=%u character=%u racing=%u\n",
            name,static_cast<unsigned long long>(tick),offers,koopa->mState,unsigned(koopa->mStep),
            unsigned(koopa->mStateTimer),unsigned(data_0209d684),unsigned(player->param1),unsigned(koopa->mIsRacing));
    };
    if(!pathLogged) {
        for(int i=0;i<koopa->mNumPathPts;++i) {
            Vector3 point;koopa->mPathPtr.GetNode(point,i);
            std::fprintf(stderr,"[native Koopa path] node=%d pos=%d,%d,%d\n",i,point.x,point.y,point.z);
        }
        pathLogged=true;
    }
    if(previousState!=koopa->mState) {
        if(koopa->mState==1) {
            ++offers;
            if(declineRetry || refusal)interaction(offers==1?"first-offer":"retry-offer");
        }
        if(previousState==1 && koopa->mState==0 && (declineRetry || refusal)) {
            interaction(refusal?"refused":"declined");
            if(refusal)interactionDone=true;
        }
        if(declineRetry && offers>=2 && koopa->mState==2) {
            interactionDone=true;interaction("accepted-retry");
        }
        previousState=koopa->mState;
        std::fprintf(stderr,"[native Koopa replay] tick=%llu state=%d player_state=%s\n",
            static_cast<unsigned long long>(tick),koopa->mState,pc_player_state_name(player));
    }
    if(interactionDone) {
        if(refusal) {
            const double dx=double(player->mPosX)-koopa->mPosX,dz=double(player->mPosZ)-koopa->mPosZ;
            if(std::hypot(dx,dz)<450*4096.0 && !player->mIsNoControl && !player->mIsControlDisabled) {
                const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
                sx=static_cast<int16_t>(std::sin(angle)*22000);
                sy=static_cast<int16_t>(-std::cos(angle)*22000);
            } else if(!retreated && !player->mIsNoControl && !player->mIsControlDisabled) {
                retreated=true;interaction("retreated");
            }
        }
        return controls();
    }
    if(koopa->mState==0 && !player->mIsNoControl && !player->mIsControlDisabled) {
        const double dx=double(koopa->mPosX)-player->mPosX,dz=double(koopa->mPosZ)-player->mPosZ;
        if(std::hypot(dx,dz)>140*4096.0) {
            const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
            sx=static_cast<int16_t>(std::sin(angle)*24000);
            sy=static_cast<int16_t>(-std::cos(angle)*24000);
        }
    } else if(koopa->mState==1 && tick%45<3) {
        // Operate the real choice menu; observe its selection rather than
        // assigning the answer. Decline once, then accept the next offer.
        const unsigned wanted=declineRetry && offers==1?1u:0u;
        if((data_0209d6bc==4 || data_0209d6bc==5) && data_0209d68c!=wanted)
            buttons=data_0209d6bc==4?(wanted?0x10:0x20):(wanted?0x80:0x40);
        else buttons=1;
    }
    else if(pilot && koopa->mState>=2) {
        static Vector3 detour[2048];static unsigned detourCount=0;
        static const int detourRadius=[](){
            const char *value=std::getenv("SM64DS_PC_TEST_KOOPA_DETOUR_RADIUS");
            return value?(std::max)(10,(std::min)(120,std::atoi(value))):70;
        }();
        static bool detourLoaded=false;
        if(!detourLoaded) {
            detourLoaded=true;
            if(const char *path=std::getenv("SM64DS_PC_TEST_KOOPA_DETOUR")) {
                if(FILE *file=std::fopen(path,"r")) {
                    int x,y,z;
                    while(detourCount<2048 && std::fscanf(file,"%d,%d,%d ",&x,&y,&z)==3) {
                        if(std::abs(x)>32767 || std::abs(y)>32767 || std::abs(z)>32767)break;
                        detour[detourCount++]={x*4096,y*4096,z*4096};
                    }
                    std::fclose(file);
                }
                std::fprintf(stderr,"[native Koopa walking detour] loaded=%u radius=%d\n",detourCount,detourRadius);
            }
        }
        if(data_0209d660) {if(tick%45<3)buttons=1;return controls();}
        if(player->mIsNoControl || player->mIsControlDisabled)return controls();
        Vector3 point={player->mPosX,player->mPosY,player->mPosZ};
        bool move=false,neutralTakeoff=false,brakeForWaypoint=false;
        // Keep following the qualified route after Koopa finishes, so a late
        // player can still reach the result talk instead of aiming through
        // the mountain at the flag. Never substitute a win or move actors.
        if(node<unsigned(koopa->mNumPathPts)) {
            koopa->mPathPtr.GetNode(point,node);move=true;
            const bool onDetour=node==8 && detourIndex<detourCount;
            if(onDetour)point=detour[detourIndex];
            double distance=std::hypot(double(point.x)-player->mPosX,double(point.z)-player->mPosZ);
            if(onDetour && !player->mIsAirborne && distance<detourRadius*4096.0 && std::abs(double(point.y)-player->mPosY)<100*4096.0) {
                std::fprintf(stderr,"[native Koopa walking detour] tick=%llu reached=%u\n",
                    static_cast<unsigned long long>(tick),detourIndex);
                ++detourIndex;bestDistance=1e100;lastProgress=tick;
                if(detourIndex<detourCount)point=detour[detourIndex];
                else koopa->mPathPtr.GetNode(point,node);
                distance=std::hypot(double(point.x)-player->mPosX,double(point.z)-player->mPosZ);
            }
            if(!onDetour && !player->mIsAirborne && distance<120*4096.0 && std::abs(double(point.y)-player->mPosY)<100*4096.0) {
                std::fprintf(stderr,"[native Koopa player route] tick=%llu reached=%u pos=%d,%d,%d\n",
                    static_cast<unsigned long long>(tick),node,player->mPosX,player->mPosY,player->mPosZ);
                ++node;bestDistance=1e100;lastProgress=tick;
                if(node<unsigned(koopa->mNumPathPts))koopa->mPathPtr.GetNode(point,node);
                if(node==8 && detourIndex<detourCount)point=detour[detourIndex];
                distance=std::hypot(double(point.x)-player->mPosX,double(point.z)-player->mPosZ);
            }
            if(distance+20*4096.0<bestDistance){bestDistance=distance;lastProgress=tick;}
            if(node==8 && detourIndex<detourCount && detourRadius<70) {
                brakeForWaypoint=true;
                if(detourIndex+1<detourCount) {
                    const double ax=double(point.x)-player->mPosX,az=double(point.z)-player->mPosZ;
                    const double bx=double(detour[detourIndex+1].x)-point.x,bz=double(detour[detourIndex+1].z)-point.z;
                    const double length=std::hypot(ax,az)*std::hypot(bx,bz);
                    if(length>0 && (ax*bx+az*bz)/length>.95)brakeForWaypoint=false;
                }
            }
            const bool authoredJump=node==koopa->mPathPtToJumpAt1 || node==koopa->mPathPtToJumpAt2;
            const char *hops=std::getenv("SM64DS_PC_TEST_KOOPA_SLOPE_HOPS");
            const char *playerState=pc_player_state_name(player);
            const bool normalTakeoff=!std::strcmp(playerState,"St_Wait_Main") || !std::strcmp(playerState,"St_Walk_Main");
            // An opt-in original-input technique: jump from Walk/Wait before
            // a slippery slope enters ButtSlide. SlopeJump intentionally
            // decelerates in the original code; never change that physics.
            const bool slopeHop=hops && !std::strcmp(hops,"1") && node==8 &&
                player->mFloorClass==3 && player->mFloorNormalY<0xf04 && normalTakeoff;
            bool hazardJump=false;
            const char *dodge=std::getenv("SM64DS_PC_TEST_ROUTE_DODGE_GOOMBAS");
            if(dodge && !std::strcmp(dodge,"1")) {
                const double dx=double(point.x)-player->mPosX,dz=double(point.z)-player->mPosZ;
                for(unsigned kind:{200u,201u,202u})
                    for(auto *a=dActor_c::FindWithActorID(kind,nullptr);a;a=dActor_c::FindWithActorID(kind,a)) {
                        const double ax=double(a->mPosX)-player->mPosX,az=double(a->mPosZ)-player->mPosZ;
                        if(a->aliveState==1 && std::abs(double(a->mPosY)-player->mPosY)<200*4096.0 &&
                           std::hypot(ax,az)<400*4096.0 && ax*dx+az*dz>0)hazardJump=true;
                    }
            }
            if(!player->mIsAirborne && tick>=nextJump &&
               ((authoredJump && lastJumpNode!=node) || hazardJump || slopeHop || (!onDetour && tick-lastProgress>180))) {
                jumpUntil=tick+(slopeHop?40:18);nextJump=tick+(slopeHop?12:180);lastJumpNode=node;
                neutralTakeoff=normalTakeoff && (slopeHop || !std::strcmp(playerState,"St_Wait_Main"));
                if(slopeHop)std::fprintf(stderr,"[native Koopa slope hop] tick=%llu pos=%d,%d,%d speed=%d; ordinary neutral takeoff\n",
                    static_cast<unsigned long long>(tick),player->mPosX,player->mPosY,player->mPosZ,player->mHorzSpeed);
            }
            if(tick%120==0)std::fprintf(stderr,
                "[native Koopa player floor] tick=%llu node=%u detour=%u class=%d surface=%d normal=%d,%d,%d pos=%d,%d,%d speed=%d state=%s\n",
                static_cast<unsigned long long>(tick),node,detourIndex,player->mFloorClass,player->mSurfaceType,
                player->mFloorNormalX,player->mFloorNormalY,player->mFloorNormalZ,
                player->mPosX,player->mPosY,player->mPosZ,player->mHorzSpeed,pc_player_state_name(player));
        } else if(koopa->mState<5) {
            auto *flag=static_cast<daRFlag_c *>(dActor_c::FindWithActorID(205,nullptr));
            if(flag && !flag->mHasTouchedFlag){point={flag->mPosX,flag->mPosY,flag->mPosZ};move=true;}
        } else {
            point={koopa->mPosX,koopa->mPosY,koopa->mPosZ};move=true;
            for(auto *actor=dActor_c::FindWithActorID(178,nullptr);actor;actor=dActor_c::FindWithActorID(178,actor)) {
                auto *star=static_cast<daStar_c *>(actor);
                // Other placed Stars can be collectible elsewhere in the
                // course. Follow only the original race reward, after its
                // normal launch/landing sequence has made it collectible.
                if(star->aliveState!=1 || star->mState!=daStar_c::STATE_IDLE ||
                   star->mStarID!=koopa->mStarID || star->mKind!=daStar_c::KIND_4)continue;
                static unsigned rewardTarget=0;
                if(rewardTarget!=star->uniqueID) {
                    rewardTarget=star->uniqueID;
                    std::fprintf(stderr,"[native Koopa reward replay] tick=%llu target=%u star=%u kind=%d; ordinary input only\n",
                        static_cast<unsigned long long>(tick),rewardTarget,unsigned(star->mStarID),int(star->mKind));
                }
                point={star->mPosX,star->mPosY,star->mPosZ};
                // Leave the finish pole on foot before attempting the elevated
                // reward. Jumping straight from the flag grabs its climb node.
                auto *flag=static_cast<daRFlag_c *>(dActor_c::FindWithActorID(205,nullptr));
                if(flag && pickupLeg<2) {
                    point={pickupLeg==0?flag->mPosX:star->mPosX,
                        flag->mPosY,flag->mPosZ+350*4096};
                    brakeForWaypoint=true;
                    if(!player->mIsAirborne && std::hypot(double(point.x)-player->mPosX,
                        double(point.z)-player->mPosZ)<60*4096.0) {
                        ++pickupLeg;
                        std::fprintf(stderr,"[native Koopa pickup approach] tick=%llu leg=%u; ordinary walking input\n",
                            static_cast<unsigned long long>(tick),pickupLeg);
                    }
                }
                const double rewardDistance=std::hypot(double(star->mPosX)-player->mPosX,
                    double(star->mPosZ)-player->mPosZ);
                if(pickupLeg>=2 && rewardDistance<250*4096.0 &&
                   point.y-player->mPosY>120*4096 && !player->mIsAirborne && tick>=nextJump) {
                    jumpUntil=tick+18;nextJump=tick+90;
                    neutralTakeoff=!std::strcmp(pc_player_state_name(player),"St_Wait_Main");
                }
                break;
            }
        }
        const double dx=double(point.x)-player->mPosX,dz=double(point.z)-player->mPosZ;
        if(move && std::hypot(dx,dz)>30*4096.0) {
            const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
            // Full-speed steering has a finite turn radius. At a precise
            // corner it can orbit outside the waypoint forever; ordinary
            // partial-stick input lets the original movement make the turn.
            const double magnitude=brakeForWaypoint?
                (std::max)(14000.0,(std::min)(30000.0,std::hypot(dx,dz)/(200*4096.0)*30000.0)):30000.0;
            sx=static_cast<int16_t>(std::sin(angle)*magnitude);
            sy=static_cast<int16_t>(-std::cos(angle)*magnitude);
        }
        if(tick<jumpUntil)buttons=2;
        // Original Wait prioritizes walking over a simultaneous jump edge.
        // Send its takeoff edge with a neutral stick, as a human can do.
        if(neutralTakeoff)sx=sy=0;
    }
    return controls();
}
