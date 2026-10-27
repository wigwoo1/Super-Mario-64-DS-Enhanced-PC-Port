#pragma once
#include "daObjHeart_c.h"

// Input-only QA detour from the established mountain ascent. Never changes
// actor/player health, collision, position, animation, reward or save state.
static bool pc_replay_healing_heart(Player *player) {
    const char *mode=std::getenv("SM64DS_PC_TEST_HEART");
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    const bool injured=mode && (!std::strcmp(mode,"injured") || !std::strcmp(mode,"cycle"));
    if(!mode || (!injured && std::strcmp(mode,"1")) || !automation || std::strcmp(automation,"1") ||
       !player || player->aliveState!=1 || data_0209f2f8!=6)return false;
    auto *heart=static_cast<daObjHeart_c *>(dActor_c::FindWithActorID(297,nullptr));
    if(!heart || heart->aliveState!=1)return false;
    const auto tick=ntr::rt_frame();
    static unsigned damageTarget=0;
    static bool damageDone=false;
    static bool started=false;
    static unsigned long long contactAt=0;
    static unsigned trackedHeart=0;
    if(trackedHeart!=heart->uniqueID) {
        trackedHeart=heart->uniqueID;
        damageTarget=0;damageDone=false;started=false;contactAt=0;
        std::fprintf(stderr,"[native heart replay] event=instance tick=%llu heart=%u; reset input-pilot bookkeeping only\n",
            static_cast<unsigned long long>(tick),trackedHeart);
    }
    if(injured && !damageDone) {
        // The qualified ascent passes a plain Goomba on the eastern shelf.
        // Walk into it with ordinary input and retain the original hurt and
        // invulnerability rules. A low but living health target leaves room
        // for incidental course coins before the recovery-heart check.
        if(!damageTarget && !player->mIsAirborne && player->mPosY>1800*4096 && player->mPosY<2150*4096) {
            for(auto *enemy=dActor_c::FindWithActorID(200,nullptr);enemy;enemy=dActor_c::FindWithActorID(200,enemy)) {
                if(enemy->aliveState==1 && enemy->param1==65535 &&
                   std::abs(double(enemy->mPosY)-player->mPosY)<250*4096.0 &&
                   std::hypot(double(enemy->mPosX)-player->mPosX,double(enemy->mPosZ)-player->mPosZ)<1000*4096.0) {
                    damageTarget=enemy->uniqueID;
                    std::fprintf(stderr,"[native heart injury] event=approach tick=%llu enemy=%u health=%d pos=%d,%d,%d\n",
                        static_cast<unsigned long long>(tick),damageTarget,player->GetHealth(),player->mPosX,player->mPosY,player->mPosZ);
                    break;
                }
            }
        }
        if(damageTarget) {
            auto *enemy=dActor_c::FindWithID(damageTarget);
            const int health=player->GetHealth();
            if(tick%60==0 || health<=2)
                std::fprintf(stderr,"[native heart injury] event=observe tick=%llu enemy=%u health=%d hurt=%d state=%s\n",
                    static_cast<unsigned long long>(tick),damageTarget,health,player->mHurtDamage,pc_player_state_name(player));
            if(health>0 && health<=2) {
                damageDone=true;
                std::fprintf(stderr,"[native heart injury] event=damaged tick=%llu enemy=%u health=%d; ordinary course ascent resumes\n",
                    static_cast<unsigned long long>(tick),damageTarget,health);
            } else {
                int16_t sx=0,sy=0;
                if(enemy && enemy->aliveState==1 && !player->mIsNoControl && !player->mIsControlDisabled) {
                    const double dx=double(enemy->mPosX)-player->mPosX,dz=double(enemy->mPosZ)-player->mPosZ;
                    if(std::hypot(dx,dz)>25*4096.0) {
                        const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
                        sx=static_cast<int16_t>(std::sin(angle)*24000.0);
                        sy=static_cast<int16_t>(-std::cos(angle)*24000.0);
                    }
                }
                ntr::input_test_controls(true,0,sx,sy);
                return true;
            }
        }
    }
    double dx=double(heart->mPosX)-player->mPosX,dz=double(heart->mPosZ)-player->mPosZ;
    const auto distance=std::hypot(dx,dz);
    if(!started) {
        if(player->mIsAirborne || distance>650*4096.0 ||
           std::abs(double(heart->mPosY)-player->mPosY)>180*4096.0)return false;
        started=true;
        std::fprintf(stderr,"[native heart replay] event=approach tick=%llu player=%u heart=%u health=%d pos=%d,%d,%d\n",
            static_cast<unsigned long long>(tick),unsigned(player->uniqueID),unsigned(heart->uniqueID),
            player->GetHealth(),player->mPosX,player->mPosY,player->mPosZ);
    }
    if(heart->mWasTouched && !contactAt) {
        contactAt=tick;
        std::fprintf(stderr,"[native heart replay] event=contact tick=%llu health=%d timer=%u\n",
            static_cast<unsigned long long>(tick),player->GetHealth(),unsigned(heart->mHealTimer));
    }
    if(contactAt && tick==contactAt+360)
        std::fprintf(stderr,"[native heart replay] event=observed tick=%llu health=%d timer=%u\n",
            static_cast<unsigned long long>(tick),player->GetHealth(),unsigned(heart->mHealTimer));
    int16_t sx=0,sy=0;
    if(!player->mIsNoControl && !player->mIsControlDisabled && !contactAt && distance>15*4096.0) {
        const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
        const auto magnitude=distance<110*4096.0?14000.0:28000.0;
        sx=static_cast<int16_t>(std::sin(angle)*magnitude);
        sy=static_cast<int16_t>(-std::cos(angle)*magnitude);
    }
    ntr::input_test_controls(true,0,sx,sy);
    return true;
}
