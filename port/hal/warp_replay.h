#pragma once
static bool pc_warp_repeat_complete=false;

// QA only. Observe the original placed cylinders and supply ordinary controls.
// Never move actors, inject a teleport, or change game state/save data.
static bool pc_replay_warp_repeat(Player *player,bool routeComplete){
    const char *mode=std::getenv("SM64DS_PC_TEST_WARP_REPEAT");
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(!mode || std::strcmp(mode,"1") || !automation || std::strcmp(automation,"1") ||
       !player || player->aliveState!=1 || data_0209f2f8!=6)return false;
    static int phase=0;static uint64_t began=0,holdUntil=0;
    static int escapeX=0,escapeZ=0;
    if(phase>=7 || (!routeComplete && phase==0))return false;
    const auto tick=ntr::rt_frame();
    if(!began)began=tick;
    auto event=[&](const char *name){std::fprintf(stderr,
        "[native warp replay] %s tick=%llu phase=%d pos=%d,%d,%d state=%s cooldown=%u\n",
        name,static_cast<unsigned long long>(tick),phase,player->mPosX,player->mPosY,
        player->mPosZ,pc_player_state_name(player),player->mTeleportTimer);};
    if(tick-began>3000){event("timeout");phase=8;return false;}
    dActor_c *peer=nullptr,*flowers=nullptr;
    for(auto *actor=dActor_c::FindWithActorID(347,nullptr);actor;actor=dActor_c::FindWithActorID(347,actor)){
        if((actor->param1>>12)==3)peer=actor;
        if((actor->param1>>12)==2)flowers=actor;
    }
    if(!peer || !flowers)return false;
    const auto distance=[&](int x,int z){return std::hypot(double(x)-player->mPosX,double(z)-player->mPosZ);};
    const bool ready=!player->mIsAirborne && !player->mIsNoControl && !player->mIsControlDisabled;
    int16_t sx=0,sy=0;
    if(phase==0 && ready && distance(peer->mPosX,peer->mPosZ)<256*4096.0){
        phase=1;holdUntil=tick+180;event("hold-first-arrival");
    }else if(phase==1 && tick>=holdUntil && ready && player->mTeleportTimer==0){
        // Pick a short clear escape using the production ground/line queries.
        const int directions[][2]={{1,0},{0,1},{-1,0},{0,-1}};
        bool found=false;
        for(const auto &direction:directions){
            const int x=player->mPosX+direction[0]*500*4096;
            const int z=player->mPosZ+direction[1]*500*4096;
            bool clear=true;int top=player->mPosY;
            for(int i=1;i<=5 && clear;++i){
                const Vector3 point={player->mPosX+(x-player->mPosX)*i/5,
                    player->mPosY+300*4096,player->mPosZ+(z-player->mPosZ)*i/5};
                dBgCh_Gnd ground;ground.SetObjAndPos(point,player);
                clear=ground.DetectClsn() && std::abs(ground.clsnY-player->mPosY)<150*4096;
                if(clear && ground.clsnY>top)top=ground.clsnY;
            }
            for(int offset:{-80,0,80})if(clear){
                const int ox=-direction[1]*offset*4096,oz=direction[0]*offset*4096;
                const Vector3 a={player->mPosX+ox,top+120*4096,player->mPosZ+oz};
                const Vector3 b={x+ox,top+120*4096,z+oz};
                dBgCh_Lin line;line.SetObjAndLine(a,b,player);clear=!line.DetectClsn();
            }
            if(clear){escapeX=x;escapeZ=z;found=true;break;}
        }
        if(!found){event("no-clear-escape");phase=8;return false;}
        phase=2;event("walk-outside-trigger");
        std::fprintf(stderr,"[native warp replay] escape target=%d,%d\n",escapeX,escapeZ);
    }else if(phase==2 && ready && distance(escapeX,escapeZ)<35*4096.0){
        phase=3;holdUntil=tick+120;event("hold-outside-trigger");
    }else if(phase==3 && tick>=holdUntil && ready){phase=4;event("return-to-trigger");
    }else if(phase==4 && ready && distance(peer->mPosX,peer->mPosZ)<25*4096.0){
        phase=5;event("wait-return-warp");
    }else if(phase==5 && ready && distance(flowers->mPosX,flowers->mPosZ)<256*4096.0){
        phase=6;holdUntil=tick+240;event("hold-return-arrival");
    }else if(phase==6 && tick>=holdUntil && ready && player->mTeleportTimer==0){
        phase=7;pc_warp_repeat_complete=true;event("done");ntr::input_test_controls(false,0,0,0);return false;
    }
    if(ready && (phase==2 || phase==4)){
        const int x=phase==2?escapeX:peer->mPosX,z=phase==2?escapeZ:peer->mPosZ;
        const double dx=double(x)-player->mPosX,dz=double(z)-player->mPosZ;
        const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
        double magnitude=std::hypot(dx,dz)/(180*4096.0)*22000;
        if(magnitude<12000)magnitude=12000;if(magnitude>22000)magnitude=22000;
        sx=static_cast<int16_t>(std::sin(angle)*magnitude);
        sy=static_cast<int16_t>(-std::cos(angle)*magnitude);
    }
    ntr::input_test_controls(true,0,sx,sy);return true;
}

// Reuse the original pause/painting/mission-selection input path for a reload.
static bool pc_replay_warp_lifecycle(Player *player){
    const char *mode=std::getenv("SM64DS_PC_TEST_WARP_LIFECYCLE");
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(!mode || std::strcmp(mode,"1") || !automation || std::strcmp(automation,"1") ||
       !pc_warp_repeat_complete)return false;
    static int phase=0,waypoint=0,lastLevel=-1;
    static uint64_t began=0,until=0,jumpUntil=0,nextJump=0;
    if(phase>=4)return false;
    const auto tick=ntr::rt_frame();
    auto event=[&](const char *name){std::fprintf(stderr,
        "[native warp lifecycle] %s tick=%llu level=%d phase=%d\n",
        name,static_cast<unsigned long long>(tick),int(data_0209f2f8),phase);};
    if(!began){began=tick;phase=1;event("exit-request");}
    if(tick-began>6000){event("timeout");phase=5;return false;}
    uint16_t buttons=0;int16_t sx=0,sy=0;
    auto controls=[&](){ntr::input_test_controls(true,buttons,sx,sy);return true;};
    const bool ready=player && player->aliveState==1 && !player->mIsNoControl && !player->mIsControlDisabled;
    if(phase==1){
        if((data_0209f2f8==1 || data_0209f2f8==2) && ready){phase=2;event("castle-return");}
        else{
            if(!data_0209f2c4){if(ready && tick%45<3)buttons=8;}
            else if(data_0209f248==3 && tick%45<3)buttons=data_0209f2e0==3?1:0x80;
            return controls();
        }
    }
    if(phase==2){
        unsigned selected=0,selectionPhase=0,selectionMode=0,confirmed=0,selectionReady=0;
        if(pc_star_select_status(&selected,&selectionPhase,&selectionMode,&confirmed,&selectionReady)){
            if(selectionReady && !confirmed && selectionMode==0 && tick%20<3)
                buttons=selected==0 && selectionPhase==2?1:0x20;
            return controls();
        }
        if(data_0209f2f8==6 && ready){phase=3;until=tick+240;event("course-reentry");}
        else{
            if(!ready)return controls();
            if(data_0209d660){if(tick%45<3)buttons=1;return controls();}
            if(lastLevel!=data_0209f2f8){lastLevel=data_0209f2f8;waypoint=0;}
            const int outside[][3]={{-75,5800,60},{-75,-1250,40}};
            const int inside[][3]={{-1000,1400,60},{-2100,600,50},{-2300,500,40},
                {-2750,50,40},{-2800,-700,50},{-3400,-700,50},{-3700,-450,40},{-5200,-450,40}};
            const bool exterior=data_0209f2f8==1;
            if(!exterior && data_0209f2f8!=2)return controls();
            const int last=exterior?1:7,index=waypoint>last?last:waypoint;
            const auto &point=exterior?outside[index]:inside[index];
            const double dx=point[0]*4096.0-player->mPosX,dz=point[1]*4096.0-player->mPosZ;
            const double distance=std::hypot(dx,dz);
            const char *state=pc_player_state_name(player);
            if(!exterior && index==last && distance<320*4096.0 && tick>=nextJump &&
                (!std::strcmp(state,"St_Wait_Main") || !std::strcmp(state,"St_Walk_Main"))){jumpUntil=tick+12;nextJump=tick+75;}
            if(tick<jumpUntil)buttons=2;
            if(distance<=point[2]*4096.0){
                if(waypoint<=last){++waypoint;event("castle-waypoint");}
            }else{
                const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
                sx=static_cast<int16_t>(std::sin(angle)*22000);sy=static_cast<int16_t>(-std::cos(angle)*22000);
            }
            return controls();
        }
    }
    if(phase==3 && tick>=until && ready && !player->mIsAirborne &&
       !std::strcmp(pc_player_state_name(player),"St_Wait_Main")){
        phase=4;event("done");ntr::input_test_controls(false,0,0,0);return false;
    }
    return controls();
}
