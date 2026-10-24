#pragma once
#include "daRNk_c.h"
#include "daRFlag_c.h"

// Opt-in, isolated QA. Observe original actors and submit ordinary buttons and
// camera-relative stick input. No placement, state, result or reward writes.
static bool pc_replay_koopa(Player *player,bool routeComplete) {
    const char *mode=std::getenv("SM64DS_PC_TEST_KOOPA");
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(!mode || (std::strcmp(mode,"start") && std::strcmp(mode,"race")) || !automation || std::strcmp(automation,"1"))return false;
    const bool pilot=!std::strcmp(mode,"race");
    const auto tick=ntr::rt_frame();
    uint16_t buttons=0;int16_t sx=0,sy=0;
    auto controls=[&](){ntr::input_test_controls(true,buttons,sx,sy);return true;};
    unsigned selection=0,phase=0,selectionMode=0,confirm=0,ready=0;
    if(pc_star_select_status(&selection,&phase,&selectionMode,&confirm,&ready)) {
        if(ready && !confirm && selectionMode==0 && tick%30<3) {
            if(phase==0)buttons=1;
            else if(selection!=1)buttons=selection>1?0x20:0x10;
            else buttons=1;
        }
        return controls();
    }
    if(data_0209f2f8!=6 || !routeComplete || !player || player->aliveState!=1)return false;
    auto *koopa=static_cast<daRNk_c *>(dActor_c::FindWithActorID(188,nullptr));
    if(!koopa || koopa->aliveState!=1)return false;
    static int previousState=-1;static bool pathLogged=false;
    if(!pathLogged) {
        for(int i=0;i<koopa->mNumPathPts;++i) {
            Vector3 point;koopa->mPathPtr.GetNode(point,i);
            std::fprintf(stderr,"[native Koopa path] node=%d pos=%d,%d,%d\n",i,point.x,point.y,point.z);
        }
        pathLogged=true;
    }
    if(previousState!=koopa->mState) {
        previousState=koopa->mState;
        std::fprintf(stderr,"[native Koopa replay] tick=%llu state=%d player_state=%s\n",
            static_cast<unsigned long long>(tick),koopa->mState,pc_player_state_name(player));
    }
    if(koopa->mState==0 && !player->mIsNoControl && !player->mIsControlDisabled) {
        const double dx=double(koopa->mPosX)-player->mPosX,dz=double(koopa->mPosZ)-player->mPosZ;
        if(std::hypot(dx,dz)>140*4096.0) {
            const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
            sx=static_cast<int16_t>(std::sin(angle)*24000);
            sy=static_cast<int16_t>(-std::cos(angle)*24000);
        }
    } else if(koopa->mState==1 && tick%45<3)buttons=1;
    else if(pilot && koopa->mState>=2) {
        static unsigned node=1,lastJumpNode=~0u;
        static Vector3 detour[512];static unsigned detourCount=0,detourIndex=0;
        static bool detourLoaded=false;
        if(!detourLoaded) {
            detourLoaded=true;
            if(const char *path=std::getenv("SM64DS_PC_TEST_KOOPA_DETOUR")) {
                if(FILE *file=std::fopen(path,"r")) {
                    int x,y,z;
                    while(detourCount<512 && std::fscanf(file,"%d,%d,%d ",&x,&y,&z)==3) {
                        if(std::abs(x)>32767 || std::abs(y)>32767 || std::abs(z)>32767)break;
                        detour[detourCount++]={x*4096,y*4096,z*4096};
                    }
                    std::fclose(file);
                }
                std::fprintf(stderr,"[native Koopa walking detour] loaded=%u\n",detourCount);
            }
        }
        static uint64_t jumpUntil=0,nextJump=0,lastProgress=0;
        static double bestDistance=1e100;
        if(data_0209d660) {if(tick%45<3)buttons=1;return controls();}
        if(player->mIsNoControl || player->mIsControlDisabled)return controls();
        Vector3 point={player->mPosX,player->mPosY,player->mPosZ};
        bool move=false;
        if(koopa->mState==2 && node<unsigned(koopa->mNumPathPts)) {
            koopa->mPathPtr.GetNode(point,node);move=true;
            const bool onDetour=node==8 && detourIndex<detourCount;
            if(onDetour)point=detour[detourIndex];
            double distance=std::hypot(double(point.x)-player->mPosX,double(point.z)-player->mPosZ);
            if(onDetour && distance<70*4096.0 && std::abs(double(point.y)-player->mPosY)<180*4096.0) {
                std::fprintf(stderr,"[native Koopa walking detour] tick=%llu reached=%u\n",
                    static_cast<unsigned long long>(tick),detourIndex);
                ++detourIndex;bestDistance=1e100;lastProgress=tick;
                if(detourIndex<detourCount)point=detour[detourIndex];
                else koopa->mPathPtr.GetNode(point,node);
                distance=std::hypot(double(point.x)-player->mPosX,double(point.z)-player->mPosZ);
            }
            if(!onDetour && distance<120*4096.0 && std::abs(double(point.y)-player->mPosY)<220*4096.0) {
                std::fprintf(stderr,"[native Koopa player route] tick=%llu reached=%u pos=%d,%d,%d\n",
                    static_cast<unsigned long long>(tick),node,player->mPosX,player->mPosY,player->mPosZ);
                ++node;bestDistance=1e100;lastProgress=tick;
                if(node<unsigned(koopa->mNumPathPts))koopa->mPathPtr.GetNode(point,node);
                if(node==8 && detourIndex<detourCount)point=detour[detourIndex];
                distance=std::hypot(double(point.x)-player->mPosX,double(point.z)-player->mPosZ);
            }
            if(distance+20*4096.0<bestDistance){bestDistance=distance;lastProgress=tick;}
            const bool authoredJump=node==koopa->mPathPtToJumpAt1 || node==koopa->mPathPtToJumpAt2;
            if(!player->mIsAirborne && tick>=nextJump &&
               ((authoredJump && lastJumpNode!=node) || tick-lastProgress>180)) {
                jumpUntil=tick+18;nextJump=tick+180;lastJumpNode=node;
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
                if(star->aliveState!=1 || star->mState!=4)continue;
                point={star->mPosX,star->mPosY,star->mPosZ};
                if(point.y-player->mPosY>120*4096 && !player->mIsAirborne && tick>=nextJump) {
                    jumpUntil=tick+18;nextJump=tick+90;
                }
                break;
            }
        }
        const double dx=double(point.x)-player->mPosX,dz=double(point.z)-player->mPosZ;
        if(move && std::hypot(dx,dz)>30*4096.0) {
            const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
            sx=static_cast<int16_t>(std::sin(angle)*30000);
            sy=static_cast<int16_t>(-std::cos(angle)*30000);
        }
        if(tick<jumpUntil)buttons=2;
    }
    return controls();
}
