#pragma once
#include "daObjTatefuda_c.h"

// Input-only sign acceptance pilot. Actor/player fields are observations;
// jump and R use the same input path as ordinary play.
static bool pc_sign_pound_replay_enabled() {
    const char *enabled=std::getenv("SM64DS_PC_TEST_SIGN_POUND");
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    return enabled && !std::strcmp(enabled,"1") && automation && !std::strcmp(automation,"1");
}

static bool pc_replay_sign_pound(Player *p) {
    if(!pc_sign_pound_replay_enabled() || !p || p->aliveState!=1)return false;
    daObjTatefuda_c *sign=nullptr;
    for(auto *a=dActor_c::FindWithActorID(184,nullptr);a;a=dActor_c::FindWithActorID(184,a))
        if(a->aliveState==1 && a->param1==1800) {sign=static_cast<daObjTatefuda_c *>(a);break;}
    if(!sign)return false;
    const auto tick=ntr::rt_frame();
    const char *state=pc_player_state_name(p);
    static int lastPounds=-1;
    static uint64_t holdRUntil=0;
    static bool complete=false;
    if(lastPounds!=sign->mPoundsLeft) {
        lastPounds=sign->mPoundsLeft;
        std::fprintf(stderr,"[native sign pound replay] observed tick=%llu uid=%u pounds=%u y=%d player=%d,%d,%d state=%s\n",
            static_cast<unsigned long long>(tick),sign->uniqueID,unsigned(sign->mPoundsLeft),sign->mPosY,
            p->mPosX,p->mPosY,p->mPosZ,state);
    }
    if(sign->mPoundsLeft==0) {
        if(!complete && !p->mIsAirborne && !p->mIsNoControl && !p->mIsControlDisabled &&
           (!std::strcmp(state,"St_Wait_Main") || !std::strcmp(state,"St_Walk_Main"))) {
            complete=true;
            std::fprintf(stderr,"[native sign pound replay] complete tick=%llu\n",static_cast<unsigned long long>(tick));
        }
        ntr::input_test_controls(true,0,0,0);return true;
    }
    if(tick<holdRUntil || !std::strcmp(state,"St_GroundPound_Main")) {
        ntr::input_test_controls(true,tick<holdRUntil?0x100:0,0,0);return true;
    }
    const double distance=std::hypot(double(sign->mPosX)-p->mPosX,double(sign->mPosZ)-p->mPosZ);
    if(p->mIsAirborne && !std::strcmp(state,"St_Jump_Main") && distance<15*4096.0 &&
       p->mPosY>sign->mPosY+100*4096) {
        holdRUntil=tick+4;
        std::fprintf(stderr,"[native sign pound replay] press-R tick=%llu distance=%.2f height=%.2f\n",
            static_cast<unsigned long long>(tick),distance/4096.0,(p->mPosY-sign->mPosY)/4096.0);
        ntr::input_test_controls(true,0x100,0,0);return true;
    }
    return false;
}
