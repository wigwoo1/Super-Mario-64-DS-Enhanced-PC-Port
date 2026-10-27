#pragma once
#include "daObjBlockS_c.h"
#include "Player.h"
#include "ntr/rt.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static_assert(sizeof(daObjBlockS_c) == 0x608, "crate native object size");
static_assert(offsetof(daObjBlockS_c, mState) == 0x560, "crate native state offset");
static_assert(offsetof(daObjBlockS_c, mHoldingPlayer) == 0x5e4, "crate native holder offset");

namespace pc_crate_probe {
inline void event(const char *name, const daObjBlockS_c *crate) {
    const char *path=std::getenv("SM64DS_PC_CRATE_TRACE");
    if(!path || !*path || !crate)return;
    if(!std::strcmp(name,"behavior") && ntr::rt_frame()%5 && !crate->mdCcAcPos_c1.hitFlags)return;
    static FILE *file=nullptr;
    if(!file){file=std::fopen(path,"w");if(file)std::fputs("event,tick,uid,param,state,alive,x,y,z,flags,holder,eater,coins_paid,break_timer,hit,other,player_x,player_y,player_z,player_uid,player_param,player_mouth_is_crate,player_mouth_timer,player_egg_params,prev_holder,player_held_is_crate,player_state_step,horz_speed,vert_speed,heading\n",file);}
    if(!file)return;
    std::uintptr_t eater=0;
    std::memcpy(&eater,reinterpret_cast<const unsigned char *>(crate)+0xd0,sizeof(eater));
    const auto *player=static_cast<const Player *>(dActor_c::FindWithActorID(191,nullptr));
    std::fprintf(file,"%s,%llu,%u,%u,%d,%u,%d,%d,%d,%u,%u,%u,%u,%u,%u,%u,%d,%d,%d,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%d\n",
        name,static_cast<unsigned long long>(ntr::rt_frame()),unsigned(crate->uniqueID),
        unsigned(crate->param1),int(crate->mState),unsigned(crate->aliveState),
        crate->mPosX,crate->mPosY,crate->mPosZ,unsigned(crate->mFlags),
        unsigned(reinterpret_cast<std::uintptr_t>(crate->mHoldingPlayer)),unsigned(eater),
        unsigned(crate->mCoinsPaid),unsigned(crate->mBreakTimer),
        unsigned(crate->mdCcAcPos_c1.hitFlags),unsigned(crate->mdCcAcPos_c1.otherOwner),
        player?player->mPosX:0,player?player->mPosY:0,player?player->mPosZ:0,
        player?unsigned(player->uniqueID):0,player?unsigned(player->param1):0,
        player && player->mObjInMouth==reinterpret_cast<std::intptr_t>(crate),
        player?unsigned(player->mMouthHoldTimer):0,player?unsigned(player->mEggParams):0,
        unsigned(reinterpret_cast<std::uintptr_t>(crate->mPrevHolder)),
        player && player->mHeldObj==reinterpret_cast<std::intptr_t>(crate),
        player?unsigned(player->mStateStep):0,crate->mHorzSpeed,crate->mVertSpeed,int(crate->mAngleY));
    std::fflush(file);
}
}
