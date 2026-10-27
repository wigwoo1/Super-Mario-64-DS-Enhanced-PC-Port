#pragma once
// Opt-in ordinary-input QA. Never writes actor/player transforms or collision.
static bool pc_replay_arrow_contact() {
    const char *target=std::getenv("SM64DS_PC_TEST_ARROW_CONTACT");
    const char *route=std::getenv("SM64DS_PC_TEST_ARROW_RETURN_ROUTE");
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(!target||!route||!automation||std::strcmp(automation,"1"))return false;
    int kind=0,x=0,z=0;char extra;
    if(std::sscanf(target,"%d:%d:%d%c",&kind,&x,&z,&extra)!=3 ||
       (kind!=299 && kind!=300)||std::abs(x)>32000||std::abs(z)>32000)return false;
    enum {APPROACH,CONTACT,REST,EXIT,CASTLE,RETURN_ROUTE,DONE};
    static int phase=APPROACH,visit=1,contacts=0;
    static unsigned uid=0;
    static uint64_t contactAt=0;
    const auto tick=ntr::rt_frame();
    auto *p=static_cast<Player *>(data_0209f394[0]);
    auto ready=[&](){return p&&p->aliveState==1&&!p->mIsNoControl&&!p->mIsControlDisabled;};
    uint16_t buttons=0;int16_t sx=0,sy=0;
    auto controls=[&](){ntr::input_test_controls(true,buttons,sx,sy);return true;};
    auto event=[&](const char *name){std::fprintf(stderr,"[native arrow contact] event=%s visit=%d tick=%llu uid=%u actor=%d level=%d\n",name,visit,static_cast<unsigned long long>(tick),uid,kind,int(data_0209f2f8));};
    if(phase==DONE)return controls();
    if(phase==EXIT){
        if(data_0209f2f8==2&&p&&p->aliveState==1){phase=CASTLE;replay_course_route_waypoint=0;replay_course_route_complete=false;event("castle-return");}
        else {
            if(!data_0209f2c4){if(ready()&&tick%45<3)buttons=8;}
            else if(data_0209f248==11&&tick%45<3)buttons=4;
            else if(data_0209f248==3&&tick%45<3)buttons=data_0209f2e0==3?1:0x80;
            return controls();
        }
    }
    if(phase==CASTLE){
        unsigned selected=0,selectionPhase=0,selectionMode=0,confirmed=0,selectionReady=0;
        if(pc_star_select_status(&selected,&selectionPhase,&selectionMode,&confirmed,&selectionReady))return false;
        if(data_0209f2f8==6&&ready()){visit=2;phase=RETURN_ROUTE;uid=0;contacts=0;contactAt=0;event("course-reentry");}
        else {if(data_0209f2f8==2&&ready()){replay_course_route(route);return true;}
            if(data_0209d660&&tick%45<3)buttons=1;return controls();}
    }
    if(phase==RETURN_ROUTE){
        if(data_0209f2f8!=6||!ready())return controls();
        if(!replay_course_route_complete){replay_course_route(route);return true;}
        phase=CONTACT;event("return-route-complete");
    }
    if(phase==APPROACH){if(!replay_course_route_complete||data_0209f2f8!=6||!ready())return false;phase=CONTACT;event("initial-route-complete");}
    if(!ready()||data_0209f2f8!=6)return controls();
    if(data_0209d660){if(tick%45<3)buttons=1;return controls();}
    dActor_c *sign=nullptr;
    for(auto *a=dActor_c::FindWithActorID(kind,nullptr);a;a=dActor_c::FindWithActorID(kind,a))
        if(a->aliveState==1&&std::hypot(double(a->mPosX)/4096-x,double(a->mPosZ)/4096-z)<10){sign=a;break;}
    if(!sign)return controls();
    if(!uid){uid=sign->uniqueID;event("selected-sign");}
    if(phase==REST){
        if(tick>=contactAt+120){if(visit==1){phase=EXIT;event("exit-request");}else{phase=DONE;event("repeat-contact-complete");}}
        return controls();
    }
    const auto *wall=p->mMeshClsn.IsOnWall()?static_cast<dBgPi *>(p->mMeshClsn.GetWallResult()):nullptr;
    if(wall&&wall->colliderIdx<24&&wall->owner==sign){
        if(!contacts)event("actual-player-wall-contact");
        if(++contacts>=15){contactAt=tick;phase=REST;event("contact-confirmed");return controls();}
    }
    const double dx=double(sign->mPosX)-p->mPosX,dz=double(sign->mPosZ)-p->mPosZ;
    const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
    sx=static_cast<int16_t>(std::sin(angle)*24000);sy=static_cast<int16_t>(-std::cos(angle)*24000);
    return controls();
}
