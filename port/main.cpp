// Native entrypoint: preserve the cartridge's initialization and scene loop.
#include "ntr/mmio.h"
#include "ntr/rt.h"
#include "ntr/video.h"
#include "ntr/fs.h"
#include "ntr/input.h"
#include "dFader_c.h"
#include "dActor_c.h"
#include "Player.h"
#include "daKrb_c.h"
#include "daBombking_c.h"
#include "daBmb_c.h"
#include "daYegg_c.h"
#include "daStar_c.h"
#include "daStarBase_c.h"
#include "daObjMarioCap_c.h"
#include "daC_Jugem_c.h"
#include "daDoor_c.h"
#include "daMip_c.h"
#include "daObj_Mip_Key_c.h"
#include "daRedBombhei_c.h"
#include "daObjCannonShutter_c.h"
#include "daCnn_c.h"
#include "daWbm_c.h"
#include "SaveData.h"
#include "dCamera_c.h"
#include "hal/stage_graph_state.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <cmath>

static_assert(offsetof(dActor_c,mPosX)==0x5c,"Position telemetry must read the native actor position");
static_assert(offsetof(Player,mState)==0x370,"Player state telemetry offset");
static_assert(offsetof(Player,mIsControlDisabled)==0x6f6,"Player control telemetry offset");

extern "C" void func_0201a054(void);
extern "C" void func_020197b8(void);
extern "C" void _ZN4Heap18InitializeRootHeapEv(void);
extern "C" void _ZN4Heap9SetNodeIDEj(void *heap, unsigned int id);
extern "C" void *data_020a0e9c;
extern "C" void *data_0209fc48;
extern "C" unsigned short data_0209b274;
extern "C" dFader_c *data_0209f5bc;
extern "C" signed char data_02092110;
extern "C" signed char data_0209f2f8;
extern "C" void *data_0209f394[];
extern "C" unsigned char data_0209caa0[];
extern "C" unsigned char data_02092128[];
extern "C" unsigned char data_0209f498[];
extern "C" unsigned char data_0209f49c[],data_0209f49e[],data_0209f4a0[],data_0209f4a6[],data_0209f4ae[];
extern "C" unsigned char data_0209f2d8;
extern "C" int pc_lakitu_state_id(const void *object);
extern "C" int pc_lakitu_timer(const void *object);
extern "C" int pc_rabbit_state_id(const void *object);
extern "C" dCamera_c *data_0209f318;
extern "C" int pc_camera_state_id(const void *object);
extern "C" unsigned char data_0209f2c4, data_0209f248, data_0209f1ec, data_0209f2e0;
extern "C" unsigned short data_0209f300;
extern "C" int pc_pause_settings_page();
extern "C" int pc_pause_settings_row();
extern "C" int pc_pause_save_status();
extern "C" const char *pc_player_state_name(const Player *player);
extern "C" short data_0209f358[];
extern "C" signed char data_0209f310[];
extern "C" unsigned char data_0209f208;
extern "C" unsigned char *data_0209f344;
extern "C" unsigned int data_ov002_020ff480[];
// local extern: original data tables; structured actor/save headers do not
// expose the global course mapping or message-state observations.
extern "C" signed char SUBLEVEL_LEVEL_TABLE[];
extern "C" unsigned char data_0209d660, data_0209d684;
extern "C" int data_0209b490, data_0209b49c;
extern "C" unsigned short data_ov100_02148708[];

static int observed_story_course() {
    const int level=data_0209f2f8;
    return level>=0 && level<52 ? SUBLEVEL_LEVEL_TABLE[level] : -1;
}
static unsigned observed_power_star_total() {
    unsigned total=0;
    for(unsigned course=0;course<30;++course)
        for(unsigned mask=data_0209caa0[0x14+course];mask;mask>>=1)total+=mask&1;
    return total;
}


// Local externs: native boss callback storage has no public runtime header.
extern "C" {
extern int data_ov078_0212710c[];
extern int data_ov078_0212700c[];
extern int data_ov078_0212701c[];
extern int data_ov078_0212703c[];
extern int data_ov078_0212704c[];
extern int data_ov078_0212707c[];
extern int data_ov078_0212708c[];
extern int data_ov078_021270ac[];
extern int data_ov078_021270bc[];
extern int data_ov078_021270dc[];
extern int data_ov078_021270ec[];
extern int data_ov078_02126ffc[];
extern int data_ov078_0212702c[];
extern int data_ov078_0212706c[];
extern int data_ov078_0212709c[];
extern int data_ov078_021270cc[];
extern int data_ov078_021270fc[];
extern int data_ov078_0212705c[];
}
static unsigned observed_boss_state(const daBombking_c *boss) {
    if(reinterpret_cast<const void *>(boss->mState)==data_ov078_0212710c) return 0x0212710cu;
    if(reinterpret_cast<const void *>(boss->mState)==data_ov078_0212700c) return 0x0212700cu;
    if(reinterpret_cast<const void *>(boss->mState)==data_ov078_0212701c) return 0x0212701cu;
    if(reinterpret_cast<const void *>(boss->mState)==data_ov078_0212703c) return 0x0212703cu;
    if(reinterpret_cast<const void *>(boss->mState)==data_ov078_0212704c) return 0x0212704cu;
    if(reinterpret_cast<const void *>(boss->mState)==data_ov078_0212707c) return 0x0212707cu;
    if(reinterpret_cast<const void *>(boss->mState)==data_ov078_0212708c) return 0x0212708cu;
    if(reinterpret_cast<const void *>(boss->mState)==data_ov078_021270ac) return 0x021270acu;
    if(reinterpret_cast<const void *>(boss->mState)==data_ov078_021270bc) return 0x021270bcu;
    if(reinterpret_cast<const void *>(boss->mState)==data_ov078_021270dc) return 0x021270dcu;
    if(reinterpret_cast<const void *>(boss->mState)==data_ov078_021270ec) return 0x021270ecu;
    if(reinterpret_cast<const void *>(boss->mState)==data_ov078_02126ffc) return 0x02126ffcu;
    if(reinterpret_cast<const void *>(boss->mState)==data_ov078_0212702c) return 0x0212702cu;
    if(reinterpret_cast<const void *>(boss->mState)==data_ov078_0212706c) return 0x0212706cu;
    if(reinterpret_cast<const void *>(boss->mState)==data_ov078_0212709c) return 0x0212709cu;
    if(reinterpret_cast<const void *>(boss->mState)==data_ov078_021270cc) return 0x021270ccu;
    if(reinterpret_cast<const void *>(boss->mState)==data_ov078_021270fc) return 0x021270fcu;
    if(reinterpret_cast<const void *>(boss->mState)==data_ov078_0212705c) return 0x0212705cu;
    return 0;
}

// Local extern: the camera-relative joystick uses this original angle helper.
extern "C" short GetAngleToCamera(int player);
extern "C" unsigned char data_0209f2d4;
// Input-only course-route QA: decimal level:x:z:radius waypoints in world
// units, with optional :1 to jump near the waypoint. Never writes player
// positions, save flags, actors or scene state.
static bool replay_course_route() {
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    const char *route=std::getenv("SM64DS_PC_TEST_COURSE_ROUTE");
    if(!automation || std::strcmp(automation,"1") || !route || !*route)return false;
    static unsigned waypoint=0;
    const char *segment=route;
    for(unsigned i=0;i<waypoint && segment;++i) {
        const char *next=std::strchr(segment,',');segment=next?next+1:nullptr;
    }
    if(!segment)return false;
    int level=0,x=0,z=0,radius=0,consumed=0,jump=0,jump_consumed=0;
    if(std::sscanf(segment,"%d:%d:%d:%d%n",&level,&x,&z,&radius,&consumed)!=4 ||
       consumed<=0 || (segment[consumed] && segment[consumed]!=',' && segment[consumed]!=':') ||
       level<0 || level>=52 || radius<1 || radius>1000 ||
       x< -32768 || x>32767 || z< -32768 || z>32767)return false;
    if(segment[consumed]==':') {
        if(std::sscanf(segment+consumed,":%d%n",&jump,&jump_consumed)!=1 ||
           jump!=1 || (segment[consumed+jump_consumed] && segment[consumed+jump_consumed]!=','))return false;
    }
    auto *player=static_cast<Player *>(data_0209f394[0]);
    if(!player || player->aliveState!=1)return false;
    if(data_0209f2f8!=level) {
        const char *next=std::strchr(segment,',');int next_level=-1;
        if(next && std::sscanf(next+1,"%d:",&next_level)==1 && next_level==data_0209f2f8) {
            std::fprintf(stderr,"[native course route] original transition %d -> %d tick=%llu\n",
                level,next_level,static_cast<unsigned long long>(ntr::rt_frame()));
            ++waypoint;
        }
        return false;
    }
    static int logged_level=-1;
    if(logged_level!=level) {
        logged_level=level;
        std::fprintf(stderr,"[native course route] level=%d player=%d,%d,%d flags1=%08x\n",
            level,player->mPosX,player->mPosY,player->mPosZ,
            reinterpret_cast<const SaveData *>(data_0209caa0)->flags1);
        for(auto *a=dActor_c::FindWithActorID(353,nullptr);a;a=dActor_c::FindWithActorID(353,a))
            std::fprintf(stderr,"[native course route] door param=%08x pos=%d,%d,%d heading=%d\n",
                a->param1,a->mPosX,a->mPosY,a->mPosZ,a->mAngleY);
    }
    static uint64_t jump_until=0,next_jump=0;
    static uint64_t settle_until=0;
    const char *settle=std::getenv("SM64DS_PC_TEST_ROUTE_SETTLE");
    const bool precise=settle && !std::strcmp(settle,"1");
    if(precise && ntr::rt_frame()<settle_until) {
        ntr::input_test_controls(true,0,0,0);return true;
    }
    uint16_t buttons=0;int16_t sx=0,sy=0;
    if(data_0209d660) {
        if(ntr::rt_frame()%45<3)buttons=1;
    } else if(!player->mIsNoControl && !player->mIsControlDisabled) {
        const double dx=x*4096.0-player->mPosX,dz=z*4096.0-player->mPosZ;
        const auto tick=ntr::rt_frame();
        const char *state=pc_player_state_name(player);
        const char *dodge=std::getenv("SM64DS_PC_TEST_ROUTE_DODGE_GOOMBAS");
        if(dodge && !std::strcmp(dodge,"1") && !player->mIsAirborne && tick>=next_jump &&
           (!std::strcmp(state,"St_Wait_Main") || !std::strcmp(state,"St_Walk_Main"))) {
            bool close=false;
            for(unsigned kind:{200u,201u,202u})
                for(auto *a=dActor_c::FindWithActorID(kind,nullptr);a;a=dActor_c::FindWithActorID(kind,a)) {
                    if(a->aliveState==1 && std::abs(a->mPosY-player->mPosY)<200*4096 &&
                       std::hypot(double(a->mPosX)-player->mPosX,double(a->mPosZ)-player->mPosZ)<360*4096.0)close=true;
                }
            if(close) {jump_until=tick+12;next_jump=tick+60;}
        }
        if(jump && std::hypot(dx,dz)<320*4096.0 && tick>=next_jump &&
           (!std::strcmp(state,"St_Wait_Main") || !std::strcmp(state,"St_Walk_Main"))) {
            jump_until=tick+12;next_jump=tick+75;
        }
        if(tick<jump_until)buttons|=2;
        if(std::hypot(dx,dz)<=radius*4096.0) {
            std::fprintf(stderr,"[native course route] reached waypoint=%u level=%d target=%d,%d tick=%llu\n",
                waypoint,level,x,z,static_cast<unsigned long long>(ntr::rt_frame()));
            ++waypoint;
            if(precise)settle_until=tick+20;
        } else {
            const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
            double magnitude=precise?std::hypot(dx,dz)/(180*4096.0)*22000:22000.0;
            // Keep low-speed travel above the controller's radial deadzone.
            if(magnitude<12000)magnitude=12000;
            if(magnitude>22000)magnitude=22000;
            sx=static_cast<int16_t>(std::sin(angle)*magnitude);
            sy=static_cast<int16_t>(-std::cos(angle)*magnitude);
        }
    }
    ntr::input_test_controls(true,buttons,sx,sy);
    return true;
}

// Input-only rabbit capture replay. The original route handles the opening
// and locked-door guide first; this controller then follows the live rabbit.
static bool rabbit_replay_waypoint(int px,int pz,int tx,int tz,double &dx,double &dz) {
    const char *path=std::getenv("SM64DS_PC_TEST_NAV_GRID");
    if(!path || !*path)return true;
    static int header[6]={},parent[20000],queue[20000];
    static unsigned char blocked[20000];
    static bool loaded=false;
    if(!loaded) {
        FILE *file=std::fopen(path,"rb");
        if(!file)return false;
        const bool valid=std::fread(header,sizeof(header),1,file)==1 && header[0]==0x3156414e &&
            header[1]>0 && header[2]>0 && header[1]*header[2]<=20000 && header[5]>0;
        loaded=valid && std::fread(blocked,1,header[1]*header[2],file)==static_cast<size_t>(header[1]*header[2]);
        std::fclose(file);if(!loaded)return false;
    }
    const int w=header[1],h=header[2],step=header[5];
    auto cell=[&](int x,int z) {
        const int cx=static_cast<int>(std::floor((x/4096.0-header[3])/step));
        const int cz=static_cast<int>(std::floor((z/4096.0-header[4])/step));
        return cx>=0 && cx<w && cz>=0 && cz<h ? cz*w+cx : -1;
    };
    int start=cell(px,pz),goal=cell(tx,tz);
    if(start<0 || goal<0)return false;
    if(blocked[goal]) {
        int best=-1;double distance=1e100;
        for(int y=-3;y<=3;++y)for(int x=-3;x<=3;++x) {
            const int cx=goal%w+x,cz=goal/w+y;
            if(cx<0 || cx>=w || cz<0 || cz>=h || blocked[cz*w+cx])continue;
            const double d=x*x+y*y;
            if(d<distance){best=cz*w+cx;distance=d;}
        }
        if(best<0)return false;goal=best;
    }
    for(int i=0;i<w*h;++i)parent[i]=-1;
    int head=0,tail=0;queue[tail++]=goal;parent[goal]=goal;
    while(head<tail && parent[start]<0) {
        const int c=queue[head++],cx=c%w,cz=c/w;
        const int neighbors[]={cx>0?c-1:-1,cx<w-1?c+1:-1,cz>0?c-w:-1,cz<h-1?c+w:-1};
        for(int next:neighbors)if(next>=0 && (!blocked[next] || next==start) && parent[next]<0) {
            parent[next]=c;queue[tail++]=next;
        }
    }
    if(parent[start]<0)return false;
    int next=parent[start];
    if(next==goal && std::hypot(dx,dz)<220*4096.0)return true;
    dx=(header[3]+(next%w+.5)*step)*4096.0-px;
    dz=(header[4]+(next/w+.5)*step)*4096.0-pz;
    return true;
}

static bool replay_rabbit_controls() {
    const char *enabled=std::getenv("SM64DS_PC_TEST_RABBIT_BOT");
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(!enabled || std::strcmp(enabled,"1") || !automation || std::strcmp(automation,"1") || ntr::rt_frame()<6500)return false;
    auto *player=static_cast<Player *>(data_0209f394[0]);
    if(!player || player->aliveState!=1)return false;
    unsigned flags1=0,flags2=0;
    std::memcpy(&flags1,data_0209caa0+4,4);std::memcpy(&flags2,data_0209caa0+8,4);
    if(!(flags2&0x20000))return false;
    const auto tick=ntr::rt_frame();
    uint16_t buttons=0;int16_t sx=0,sy=0;
    if(data_0209d660) {
        if(tick%45<3)buttons=1;
    } else if(!(flags1&0x40) && !player->mIsNoControl && !player->mIsControlDisabled) {
        daMip_c *rabbit=nullptr;
        for(auto *actor=dActor_c::FindWithActorID(187,nullptr);actor;actor=dActor_c::FindWithActorID(187,actor)) {
            auto *r=static_cast<daMip_c *>(actor);
            if(actor->aliveState==1 && r->mRabbitId==7 && !r->mIsDisabled){rabbit=r;break;}
        }
        if(rabbit && !rabbit->mEatenByYoshi && rabbit->unk_426!=2) {
            double dx=double(rabbit->mPosX)-player->mPosX,dz=double(rabbit->mPosZ)-player->mPosZ;
            if(!rabbit_replay_waypoint(player->mPosX,player->mPosZ,rabbit->mPosX,rabbit->mPosZ,dx,dz)) {
                ntr::input_test_controls(true,0,0,0);return true;
            }
            const double heading=std::atan2(dx,dz);
            const double angle=heading-GetAngleToCamera(0)*6.283185307179586/65536.0;
            sx=static_cast<int16_t>(std::sin(angle)*28000);
            sy=static_cast<int16_t>(-std::cos(angle)*28000);
            const short desired=static_cast<short>(heading*65536.0/6.283185307179586);
            const int turn=std::abs(static_cast<int>(static_cast<short>(desired-player->mAngleY)));
            static uint64_t next_press=0,press_until=0;
            const double target_distance=std::hypot(double(rabbit->mPosX)-player->mPosX,double(rabbit->mPosZ)-player->mPosZ);
            if(target_distance<250*4096.0 && turn<0x1800 && tick>=next_press) {
                press_until=tick+3;next_press=tick+35;
            }
            if(tick<press_until)buttons=1;
        }
    }
    ntr::input_test_controls(true,buttons,sx,sy);
    return true;
}

// Observe an original mission1 dropper, take a hit, then leave its range.
static bool replay_waterbomb_controls() {
    const char *mode=std::getenv("SM64DS_PC_TEST_WATERBOMB");
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(!mode || std::strcmp(mode,"1") || !automation || std::strcmp(automation,"1"))return false;
    const char *route=std::getenv("SM64DS_PC_TEST_WATERBOMB_ROUTE");
    int observe_x=0,observe_z=0,retreat_x=0,retreat_z=0;
    if(!route || std::sscanf(route,"%d:%d:%d:%d",&observe_x,&observe_z,&retreat_x,&retreat_z)!=4)return false;
    if(observe_x< -32768 || observe_x>32767 || observe_z< -32768 || observe_z>32767 ||
       retreat_x< -32768 || retreat_x>32767 || retreat_z< -32768 || retreat_z>32767)return false;
    auto *p=static_cast<Player *>(data_0209f394[0]);
    if(data_0209f2f8!=6 || !p || p->aliveState!=1)return false;
    enum {APPROACH,OBSERVE,RETREAT,DRAIN,COMPLETE};
    static int phase=APPROACH,initial_health=0,retreat_leg=0;
    static bool fragments_seen=false;
    const auto tick=ntr::rt_frame();
    const char *state=pc_player_state_name(p);
    const bool free=!p->mIsNoControl && !p->mIsControlDisabled;
    uint16_t buttons=0;int16_t sx=0,sy=0;
    auto event=[&](const char *name) {
        std::fprintf(stderr,"[native waterbomb replay] event=%s tick=%llu health=%d pos=%d,%d,%d\n",
            name,static_cast<unsigned long long>(tick),p->GetHealth(),p->mPosX,p->mPosY,p->mPosZ);
    };
    if(phase==APPROACH) {
        if(!free || std::hypot(p->mPosX-observe_x*4096.0,p->mPosZ-observe_z*4096.0)>60*4096.0 ||
           (std::strcmp(state,"St_Wait_Main") && std::strcmp(state,"St_Walk_Main")))return false;
        initial_health=p->GetHealth();phase=OBSERVE;event("observe");
    }
    unsigned moving=0;
    for(auto *a=dActor_c::FindWithActorID(208,nullptr);a;a=dActor_c::FindWithActorID(208,a)) {
        if(a->aliveState!=1)continue;
        auto *bomb=static_cast<daWbm_c *>(a);
        if(bomb->mVariant!=0)++moving;
        if(phase==OBSERVE && bomb->mVariant==2)fragments_seen=true;
    }
    if(phase==OBSERVE && fragments_seen && p->GetHealth()<initial_health) {
        phase=RETREAT;event("damage-and-fragments");
    }
    if(phase==RETREAT && free) {
        const double dx=retreat_x*4096.0-p->mPosX,dz=(retreat_leg?retreat_z:observe_z)*4096.0-p->mPosZ;
        if(std::hypot(dx,dz)<60*4096.0) {
            if(retreat_leg==0){retreat_leg=1;event("retreat-corner");}
            else {phase=DRAIN;event("outside-dropper-range");}
        }
        else if(!std::strcmp(state,"St_Wait_Main") || !std::strcmp(state,"St_Walk_Main")) {
            const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
            sx=static_cast<int16_t>(std::sin(angle)*22000);sy=static_cast<int16_t>(-std::cos(angle)*22000);
        } else if(!std::strcmp(state,"St_StomachSlide_Main") && tick%75<3)buttons=2;
    }
    if(phase==DRAIN && moving==0 && free && !p->mIsAirborne && !std::strcmp(state,"St_Wait_Main")) {
        phase=COMPLETE;event("complete");
    }
    ntr::input_test_controls(true,buttons,sx,sy);return true;
}

static void capture_waterbomb_state() {
    const char *path=std::getenv("SM64DS_PC_WATERBOMB_CAPTURE");
    if(!path || !*path)return;
    static FILE *file=nullptr;static uint64_t previous=~uint64_t(0);
    const auto tick=ntr::rt_frame();if(tick==previous)return;previous=tick;
    if(!file) {
        file=std::fopen(path,"w");if(!file)return;
        std::fputs("tick,level,id,kind,variant,state,phase,timer,bounces,x,y,z,vertical_speed,horizontal_speed,hit,other,player_id,px,py,pz,player_state,player_step,health,hurt_damage,airborne,disabled,no_control\n",file);
    }
    auto *p=static_cast<Player *>(data_0209f394[0]);
    const int health=p?p->GetHealth():-1;
    auto emit=[&](dActor_c *a,int variant,int state,int phase,int timer,int bounces,unsigned hit,unsigned other) {
        std::fprintf(file,"%llu,%d,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%d,%d,%d,%s,%u,%d,%d,%u,%u,%u\n",
            static_cast<unsigned long long>(tick),data_0209f2f8,a->uniqueID,unsigned(a->actorID),variant,state,phase,timer,bounces,
            a->mPosX,a->mPosY,a->mPosZ,a->mVertSpeed,a->mHorzSpeed,hit,other,p?p->uniqueID:0,
            p?p->mPosX:0,p?p->mPosY:0,p?p->mPosZ:0,pc_player_state_name(p),p?p->mStateStep:0,
            health,p?p->mHurtDamage:0,p?p->mIsAirborne:0,p?p->mIsControlDisabled:0,p?p->mIsNoControl:0);
    };
    if(p && p->aliveState==1)emit(p,p->mCharacter,-1,p->mStatePhase,p->mStateTimer,0,0,0);
    for(unsigned kind:{207u,208u})for(auto *a=dActor_c::FindWithActorID(kind,nullptr);a;a=dActor_c::FindWithActorID(kind,a)) {
        if(a->aliveState!=1)continue;
        if(kind==207) {
            auto *c=static_cast<daCnn_c *>(a);
            if(c->mVariant!=2)emit(c,c->mVariant,c->mState,c->mLoadStep,c->mTimer,0,c->mdCcAc_c.hitFlags,c->mdCcAc_c.otherOwner);
        } else {
            auto *b=static_cast<daWbm_c *>(a);
            emit(b,b->mVariant,b->mState,b->mBouncePhase,b->mStateTimer,b->mBounceCount,b->mdCcAc_c.hitFlags,b->mdCcAc_c.otherOwner);
        }
    }
    std::fflush(file);
}

// Input-only repeated cannon/course-lifecycle replay. All travel is ordinary
// stick/button input; pointers, actors, camera fields and saves are only read.
static bool replay_cannon_cycle_controls() {
    const char *mode=std::getenv("SM64DS_PC_TEST_CANNON");
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(!mode || (std::strcmp(mode,"repeat-reentry") && std::strcmp(mode,"other-cannon")) || !automation || std::strcmp(automation,"1"))return false;
    const bool other_cannon=!std::strcmp(mode,"other-cannon");
    enum {RIDE,RETURN,EXIT,CASTLE,FINISHED};
    static int phase=RIDE,visit=1,rides=0,waypoint=0;
    static unsigned target=0;
    static unsigned selected_target=0;
    static bool shot=false;
    static uint64_t aim_tick=0,land_tick=0,jump_until=0,next_jump=0;
    const auto tick=ntr::rt_frame();
    if(other_cannon && !selected_target) {
        if(const char *value=std::getenv("SM64DS_PC_TEST_CANNON_TARGET"))selected_target=static_cast<unsigned>(std::strtoul(value,nullptr,10));
    }
    auto *p=static_cast<Player *>(data_0209f394[0]);
    uint16_t buttons=0;int16_t sx=0,sy=0;
    auto controls=[&]() {ntr::input_test_controls(true,buttons,sx,sy);return true;};
    auto event=[&](const char *name) {
        std::fprintf(stderr,"[native cannon cycle] visit=%d ride=%d event=%s tick=%llu id=%u\n",
            visit,rides+1,name,static_cast<unsigned long long>(tick),target);
    };
    auto ready=[&]() {return p && p->aliveState==1 && !p->mIsNoControl && !p->mIsControlDisabled;};
    auto move=[&](double x,double z,int radius) {
        const double dx=x*4096-p->mPosX,dz=z*4096-p->mPosZ;
        if(std::hypot(dx,dz)<=radius*4096.0)return true;
        const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
        sx=static_cast<int16_t>(std::sin(angle)*22000);sy=static_cast<int16_t>(-std::cos(angle)*22000);
        return false;
    };
    if(phase==FINISHED)return controls();
    if(phase==EXIT) {
        if(data_0209f2f8==2 && p && p->aliveState==1) {
            event("castle-return");phase=CASTLE;waypoint=0;target=0;
        } else {
            if(!data_0209f2c4) {if(ready() && tick%45<3)buttons=8;}
            else if(data_0209f248==3 && tick%45<3)buttons=data_0209f2e0==3?1:0x80;
            return controls();
        }
    }
    if(phase==CASTLE) {
        if(data_0209f2f8==6 && p && p->aliveState==1) {
            visit=2;rides=0;phase=RIDE;shot=false;aim_tick=0;target=0;event("course-reentry");
        } else {
            // Original mission selection accepts confirm after its own fade.
            if(!p || p->aliveState!=1) {if(tick%60<3)buttons=1;return controls();}
            if(data_0209d660) {if(tick%45<3)buttons=1;return controls();}
            if(!ready())return controls();
            const int route[][3]={{-1000,1400,60},{-2100,600,50},{-2300,500,40},
                {-2750,50,40},{-2800,-700,50},{-3400,-700,50},{-3700,-450,40},{-5200,-450,40}};
            const int last=int(sizeof(route)/sizeof(route[0]))-1;
            const int index=waypoint>last?last:waypoint;
            if(index==last && std::hypot(route[index][0]*4096.0-p->mPosX,route[index][1]*4096.0-p->mPosZ)<320*4096.0 &&
               tick>=next_jump && (!std::strcmp(pc_player_state_name(p),"St_Wait_Main") || !std::strcmp(pc_player_state_name(p),"St_Walk_Main"))) {
                jump_until=tick+12;next_jump=tick+75;
            }
            if(tick<jump_until)buttons=2;
            if(move(route[index][0],route[index][1],route[index][2]) && waypoint<=last) {
                std::fprintf(stderr,"[native cannon cycle] castle-waypoint=%d tick=%llu\n",waypoint,static_cast<unsigned long long>(tick));++waypoint;
            }
            return controls();
        }
    }
    if(data_0209f2f8!=6 || !p || p->aliveState!=1)return false;
    unsigned cannon_flags=0;std::memcpy(&cannon_flags,data_0209caa0+0x10,4);
    if(!target && (!(cannon_flags&1) || !ready() || data_0209d660))return false;
    daCnn_c *c=nullptr;double best=1e100;
    for(auto *a=dActor_c::FindWithActorID(207,nullptr);a;a=dActor_c::FindWithActorID(207,a)) {
        auto *candidate=static_cast<daCnn_c *>(a);
        if(a->aliveState!=1 || candidate->mVariant!=2 || (target && a->uniqueID!=target) ||
           (other_cannon && selected_target && a->uniqueID!=selected_target))continue;
        const double distance=std::hypot(double(a->mPosX)-p->mPosX,double(a->mPosZ)-p->mPosZ);
        if(distance<best){c=candidate;best=distance;}
    }
    // Let the input-only course route reach this cannon before taking over.
    if(!c || (!target && best>(other_cannon?650:1200)*4096.0))return false;
    if(!target) {target=c->uniqueID;event("approach");}
    if(phase==RETURN) {
        if(tick<land_tick+90 || !ready())return controls();
        const int route[][2]={{0,6500},{-5950,6500}};
        if(waypoint<2) {
            // The second landmark is the Buddy's position; stop outside his
            // collision body before turning toward the nearby cannon hole.
            if(move(route[waypoint][0],route[waypoint][1],waypoint==1?180:70)) {++waypoint;event("return-waypoint");}
        } else {phase=RIDE;shot=false;aim_tick=0;event("repeat-ready");}
        return controls();
    }
    if(c->mState==3 && c->mLoadStep==3 && !shot) {
        if(!aim_tick){aim_tick=tick;event("aim");}
        const auto elapsed=tick-aim_tick;
        if(elapsed<60) {
            short pitch=0;if(data_0209f318)std::memcpy(&pitch,reinterpret_cast<const char *>(data_0209f318)+0x17e,2);
            if(pitch>0x1000)sy=24000;
            if(elapsed<20)sx=18000;
        }
        if(elapsed>=120 && elapsed<123)buttons=1;
    } else if(c->mState==3 && c->mLoadStep==4) {
        if(!shot)event("launch");shot=true;
    } else if(!shot && ready()) {
        // The second cannon's mound has steep sides; jump into its opening.
        if(other_cannon && !p->mIsAirborne && tick>=next_jump &&
           (!std::strcmp(pc_player_state_name(p),"St_Wait_Main") || !std::strcmp(pc_player_state_name(p),"St_Walk_Main"))) {
            jump_until=tick+12;next_jump=tick+75;
        }
        if(other_cannon && tick<jump_until)buttons=2;
        move(c->mPosX/4096.0,c->mPosZ/4096.0,15);
    }
    if(shot && ready() && !p->mIsAirborne && !std::strcmp(pc_player_state_name(p),"St_Wait_Main")) {
        event("grounded-idle");++rides;land_tick=tick;
        if(other_cannon) {phase=FINISHED;event("complete");}
        else if(visit==2) {phase=FINISHED;event("complete");}
        else if(rides==2) {phase=EXIT;event("exit-request");}
        else {phase=RETURN;waypoint=0;}
    }
    return controls();
}

// Input-only single-cannon acceptance replay. Never changes an actor or save.
static bool replay_cannon_controls() {
    const char *mode=std::getenv("SM64DS_PC_TEST_CANNON");
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(!mode || std::strcmp(mode,"fire") || !automation || std::strcmp(automation,"1"))return false;
    auto *p=static_cast<Player *>(data_0209f394[0]);
    if(data_0209f2f8!=6 || !p || p->aliveState!=1)return false;
    static unsigned target=0;
    static bool shot=false,done=false;
    static uint64_t aim_tick=0;
    unsigned cannon_flags=0;std::memcpy(&cannon_flags,data_0209caa0+0x10,4);
    // Let original entry/dialogue and the Buddy opening finish before riding.
    if(!target && (!(cannon_flags&1) || data_0209d660 || p->mIsNoControl || p->mIsControlDisabled))return false;
    const auto tick=ntr::rt_frame();
    daCnn_c *c=nullptr;double best=1e100;
    for(auto *a=dActor_c::FindWithActorID(207,nullptr);a;a=dActor_c::FindWithActorID(207,a)) {
        auto *candidate=static_cast<daCnn_c *>(a);
        if(a->aliveState!=1 || candidate->mVariant!=2 || (target && a->uniqueID!=target))continue;
        const double distance=std::hypot(double(a->mPosX)-p->mPosX,double(a->mPosZ)-p->mPosZ);
        if(distance<best){c=candidate;best=distance;}
    }
    if(!c || (!target && best>1200*4096.0))return false;
    if(!target) {
        target=c->uniqueID;
        std::fprintf(stderr,"[native cannon replay] target=%u pos=%d,%d,%d tick=%llu\n",target,c->mPosX,c->mPosY,c->mPosZ,static_cast<unsigned long long>(tick));
    }
    uint16_t buttons=0;int16_t sx=0,sy=0;
    if(c->mState==3 && c->mLoadStep==3 && !shot) {
        if(!aim_tick){aim_tick=tick;std::fprintf(stderr,"[native cannon replay] aiming tick=%llu\n",static_cast<unsigned long long>(tick));}
        const auto elapsed=tick-aim_tick;
        if(elapsed<60) {
            short camera_pitch=0;if(data_0209f318)std::memcpy(&camera_pitch,reinterpret_cast<const char *>(data_0209f318)+0x17e,2);
            if(camera_pitch>0x1000)sy=24000;
            if(elapsed<20)sx=18000;
        }
        if(elapsed>=120 && elapsed<123)buttons=1;
    } else if(c->mState==3 && c->mLoadStep==4) {
        if(!shot)std::fprintf(stderr,"[native cannon replay] launched tick=%llu\n",static_cast<unsigned long long>(tick));
        shot=true;
    } else if(!shot && !p->mIsNoControl && !p->mIsControlDisabled) {
        const double angle=std::atan2(double(c->mPosX)-p->mPosX,double(c->mPosZ)-p->mPosZ)-GetAngleToCamera(0)*6.283185307179586/65536.0;
        if(best>15*4096.0){sx=static_cast<int16_t>(std::sin(angle)*18000);sy=static_cast<int16_t>(-std::cos(angle)*18000);}
    }
    if(shot && !done && !p->mIsAirborne && !p->mIsControlDisabled && !p->mIsNoControl && std::strcmp(pc_player_state_name(p),"St_Cannon_Main")) {
        done=true;std::fprintf(stderr,"[native cannon replay] landed tick=%llu state=%s\n",static_cast<unsigned long long>(tick),pc_player_state_name(p));
    }
    ntr::input_test_controls(true,buttons,sx,sy);return true;
}

static bool replay_buddy_controls() {
    const char *mode=std::getenv("SM64DS_PC_TEST_BUDDY_TALK");
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(!mode || !automation || std::strcmp(automation,"1"))return false;
    const bool persistence=!std::strcmp(mode,"cannon-persistence");
    const bool repeat=persistence || !std::strcmp(mode,"cannon-repeat");
    const bool cannon=repeat || !std::strcmp(mode,"cannon");
    static unsigned target=0,rounds=0,visit=0,phase=0;
    static bool started=false,completed=false,in_course=false,saw_save=false;
    static uint64_t completed_tick=0;
    static int start_x=0,start_z=0;
    auto *p=static_cast<Player *>(data_0209f394[0]);
    const auto tick=ntr::rt_frame();
    const bool pulse=tick%45<3;
    if(data_0209f2f8!=6 && in_course) {
        target=rounds=0;started=completed=in_course=false;
    }
    // Input only: Exit Course, then castle pause -> Save & Quit.
    if(persistence && phase>=1) {
        if(phase==4) {ntr::input_test_controls(true,0,0,0);return true;}
        uint16_t buttons=0;
        if(phase==1 && data_0209f2f8==2 && p && p->aliveState==1) {
            phase=2;
            std::fprintf(stderr,"[native Buddy persistence] castle returned tick=%llu\n",static_cast<unsigned long long>(tick));
        }
        if(phase==1 || phase==2) {
            if(!data_0209f2c4) {
                if(p && !p->mIsNoControl && !p->mIsControlDisabled && pulse)buttons=8;
            } else if((phase==1 && data_0209f248==3) || (phase==2 && data_0209f248==1)) {
                if(pulse)buttons=data_0209f2e0==3?1:0x80;
            } else if(phase==2 && data_0209f248==13) {
                if(pulse)buttons=data_0209f2e0==1?1:0x80;
            } else if(phase==2 && data_0209f248==15) {
                saw_save=true;phase=3;
                std::fprintf(stderr,"[native Buddy persistence] original save dialog tick=%llu\n",static_cast<unsigned long long>(tick));
            }
        } else if(phase==3 && saw_save && !data_0209f2c4) {
            phase=4;
            std::fprintf(stderr,"[native Buddy persistence] saved; awaiting fresh-process reload tick=%llu\n",static_cast<unsigned long long>(tick));
        }
        ntr::input_test_controls(true,buttons,0,0);return true;
    }
    if(data_0209f2f8!=6 || !p || p->aliveState!=1)return false;
    if(!in_course) {in_course=true;++visit;}
    if(repeat && completed &&
       std::hypot(double(start_x)-p->mPosX,double(start_z)-p->mPosZ)<=60*4096.0 && tick>completed_tick+150) {
        if(rounds>=2) {
            if(persistence) {
                phase=1;
                std::fprintf(stderr,"[native Buddy persistence] visit=%u repeated interactions done tick=%llu\n",visit,static_cast<unsigned long long>(tick));
            }
            return false;
        }
        started=completed=false;
    }
    daRedBombhei_c *buddy=nullptr;
    if(target)buddy=static_cast<daRedBombhei_c *>(dActor_c::FindWithID(target));
    if(!target) {
        double best=1e100;
        for(auto *a=dActor_c::FindWithActorID(181,nullptr);a;a=dActor_c::FindWithActorID(181,a)) {
            if(a->aliveState!=1)continue;
            const int variant=a->mAngleX&255;
            if(cannon && variant!=1 && variant!=2)continue;
            const double distance=std::hypot(double(a->mPosX)-p->mPosX,double(a->mPosZ)-p->mPosZ);
            if(distance<best){buddy=static_cast<daRedBombhei_c *>(a);best=distance;}
        }
        if(buddy) {
            target=buddy->uniqueID;start_x=p->mPosX;start_z=p->mPosZ;
            std::fprintf(stderr,"[native Buddy replay] target=%u variant=%u param=%u mode=%s\n",target,buddy->mAngleX&255,buddy->param1,mode);
        }
    }
    if(!buddy)return false;
    if(buddy->mState!=0)started=true;
    if(started && !completed && buddy->mState==0 && (!cannon || buddy->mCannonOpened) &&
       !data_0209d660 && !p->mIsNoControl && !p->mIsControlDisabled) {
        completed=true;completed_tick=tick;++rounds;
        if(repeat)std::fprintf(stderr,"[native Buddy repeat] visit=%u interaction=%u complete tick=%llu\n",visit,rounds,static_cast<unsigned long long>(tick));
        std::fprintf(stderr,"[native Buddy replay] completed tick=%llu target=%u cannon=%u\n",
            static_cast<unsigned long long>(ntr::rt_frame()),target,buddy->mCannonOpened);
    }
    uint16_t buttons=0;int16_t sx=0,sy=0;
    if(data_0209d660 || (!completed && !p->mIsNoControl && !p->mIsControlDisabled &&
       std::hypot(double(buddy->mPosX)-p->mPosX,double(buddy->mPosZ)-p->mPosZ)<300*4096.0)) {
        if(ntr::rt_frame()%45<3)buttons=1;
    }
    if(!data_0209d660 && !p->mIsNoControl && !p->mIsControlDisabled) {
        const double dx=(completed?start_x:buddy->mPosX)-double(p->mPosX);
        const double dz=(completed?start_z:buddy->mPosZ)-double(p->mPosZ);
        if(std::hypot(dx,dz)>(completed?60:100)*4096.0) {
            const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
            sx=static_cast<int16_t>(std::sin(angle)*18000);sy=static_cast<int16_t>(-std::cos(angle)*18000);
        }
    }
    ntr::input_test_controls(true,buttons,sx,sy);
    return true;
}

static void replay_boss_controls() {
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    const char *bot=std::getenv("SM64DS_PC_TEST_BOSS_BOT");
    if(!automation || std::strcmp(automation,"1") || !bot || std::strcmp(bot,"1") || ntr::rt_frame()<1702) {
        ntr::input_test_controls(false,0,0,0);return;
    }
    const auto tick=ntr::rt_frame();auto *player=static_cast<Player *>(data_0209f394[0]);
    if(!player || player->aliveState!=1){ntr::input_test_controls(true,0,0,0);return;}
    static uint64_t press_until=0,next_press=0,jump_until=0,next_jump=0;
    uint16_t buttons=tick<press_until?1:0;int16_t sx=0,sy=0;
    auto press=[&](uint64_t delay=35) { if(tick>=next_press) {press_until=tick+3;next_press=tick+delay;buttons=1;} };
    if(data_0209f2f8==1) {
        // Course-clear menus accept input while the player's movement is
        // deliberately disabled by the original level-entry animation.
        if(data_0209f2d4==3)press();
        ntr::input_test_controls(true,buttons,0,0);return;
    }
    if(data_0209d660 || player->mIsNoControl) {
        if(data_0209d660)press();
        ntr::input_test_controls(true,buttons,0,0);return;
    }
    if(data_0209f2f8!=6){ntr::input_test_controls(false,0,0,0);return;}
    auto *boss=static_cast<daBombking_c *>(dActor_c::FindWithActorID(189,nullptr));
    auto *mouth=reinterpret_cast<dActor_c *>(player->mObjInMouth);
    double tx=0,tz=0;bool target=false,face_only=false;
    if(mouth && boss) {tx=boss->mPosX;tz=boss->mPosZ;target=true;face_only=true;}
    else if(boss) {
        // Catch a released Bob-omb, never a bomb still in the boss's hand.
        dActor_c *nearest=nullptr;double best=1e30;
        for(auto *a=dActor_c::FindWithActorID(206,nullptr);a;a=dActor_c::FindWithActorID(206,a)) {
            auto *b=static_cast<daBmb_c *>(a);void *held=nullptr;std::memcpy(&held,reinterpret_cast<char *>(a)+0xc8,4);
            const double d=std::hypot(double(a->mPosX-player->mPosX),double(a->mPosZ-player->mPosZ));
            if(a->aliveState==1 && b->mVariant==2 && b->mState!=5 && !held && b->mFuse>30 &&
               std::abs(a->mPosY-player->mPosY)<400*4096 && d<best &&
               std::hypot(double(a->mPosX-boss->mArenaPosX),double(a->mPosZ-boss->mArenaPosZ))<1100*4096 &&
               std::hypot(double(a->mPosX-boss->mPosX),double(a->mPosZ-boss->mPosZ))>200*4096) {best=d;nearest=a;}
        }
        if(nearest) {tx=nearest->mPosX;tz=nearest->mPosZ;target=true;face_only=best<190*4096;}
        else {
            const double dx=player->mPosX-boss->mPosX,dz=player->mPosZ-boss->mPosZ,d=std::hypot(dx,dz);
            if(d<500*4096 && d>0) {tx=boss->mPosX+dx/d*650*4096;tz=boss->mPosZ+dz/d*650*4096;target=true;}
        }
        if(!mouth && std::hypot(double(player->mPosX-boss->mArenaPosX),double(player->mPosZ-boss->mArenaPosZ))>950*4096) {
            tx=boss->mArenaPosX;tz=boss->mArenaPosZ;target=true;face_only=false;
        }
    } else {
        for(auto *a=dActor_c::FindWithActorID(178,nullptr);a;a=dActor_c::FindWithActorID(178,a)) {
            auto *star=static_cast<daStar_c *>(a);
            if(star->mStarID==1 && star->mState<5) {
                tx=star->mPosX;tz=star->mPosZ;
                const double distance=std::hypot(tx-player->mPosX,tz-player->mPosZ);
                target=distance>50*4096;
                const char *state=pc_player_state_name(player);
                if(distance<180*4096 && star->mPosY-player->mPosY>60*4096 && tick>=next_jump &&
                   (!std::strcmp(state,"St_Wait_Main") || !std::strcmp(state,"St_Walk_Main"))) {
                    jump_until=tick+30;next_jump=tick+75;
                }
                break;
            }
        }
    }
    if(target) {
        const double angle=std::atan2(tx-player->mPosX,tz-player->mPosZ);
        const auto desired=static_cast<int16_t>(std::lround(angle*65536.0/6.283185307179586));
        const auto error=static_cast<int16_t>(desired-player->mAngleY);
        const double relative=angle-GetAngleToCamera(0)*6.283185307179586/65536.0;
        if(!face_only || std::abs(int(error))>1800) {
            const double magnitude=face_only?11500:22000;
            sx=static_cast<int16_t>(std::sin(relative)*magnitude);
            sy=static_cast<int16_t>(-std::cos(relative)*magnitude);
        } else {
            const char *state=pc_player_state_name(player);
            if(!std::strcmp(state,"St_Wait_Main") || !std::strcmp(state,"St_Walk_Main"))press(32);
        }
    }
    if(tick<jump_until)buttons|=2;
    ntr::input_test_controls(true,buttons,sx,sy);
}

static bool trace_enabled()
{
    const char *value = std::getenv("SM64DS_PC_TRACE");
    return value && value[0] && value[0] != '0';
}

// Opt-in read-only actor census for fresh-save route investigation. The
// original object table may spawn actors whose retail profile is not yet
// named/native; enumerate the live actor IDs once at a requested frame.
static void trace_actor_census(uint64_t frame)
{
    static bool emitted = false;
    const char *when = std::getenv("SM64DS_PC_TRACE_ACTORS_AT");
    if (emitted || !when || !*when) return;
    char *end = nullptr;
    const auto target = std::strtoull(when, &end, 10);
    if (end == when || *end || frame != target) return;
    emitted = true;

    unsigned count = 0;
    for (auto *actor = dActor_c::Next(nullptr); actor && count < 4096;
         actor = dActor_c::Next(actor)) {
        ++count;
        std::fprintf(stderr,
            "[native actor census] tick=%llu id=%u unique=%u alive=%u param=%08x area=%d pos=%d,%d,%d flags=%08x\n",
            static_cast<unsigned long long>(frame), actor->actorID,
            actor->uniqueID, actor->aliveState, actor->param1,
            static_cast<int>(actor->mAreaId), actor->mPosX, actor->mPosY,
            actor->mPosZ, actor->mFlags);
    }
    std::fprintf(stderr, "[native actor census] tick=%llu total=%u\n",
        static_cast<unsigned long long>(frame), count);
    std::fflush(stderr);
}

static bool trace_script_progress(uint64_t frame)
{
    // The host callback is paced at presentation FPS (and may repeat a game
    // state), while route diagnostics are specified in simulation ticks.
    trace_actor_census(ntr::rt_frame());
    // Explicit QA placement only: keep course geometry/ROM and ordinary play
    // intact while exercising the original Power Star pickup and exit path.
    // The host hook runs between game ticks on the same thread.
    static bool power_star_fixture_attempted=false;
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    const char *power_fixture=std::getenv("SM64DS_PC_TEST_POWER_STAR");
    if(!power_star_fixture_attempted && ntr::rt_frame()==1700 &&
       automation && std::strcmp(automation,"1")==0 &&
       power_fixture && std::strcmp(power_fixture,"1")==0 && data_0209f2f8==6) {
        auto *player=static_cast<Player *>(data_0209f394[0]);
        power_star_fixture_attempted=true;
        if(player && player->aliveState==1 && !player->mIsNoControl && !player->mIsControlDisabled) {
            Vector3 position;
            position.x=player->mPosX;position.y=player->mPosY+120*4096;
            position.z=player->mPosZ-350*4096;
            auto *star=dActor_c::Spawn(178,1,position,nullptr,player->mAreaId,-1);
            std::fprintf(stderr,"[native QA fixture] Power Star level=6 param=0001 actor=%u pos=%d,%d,%d; original collection/save/exit unchanged\n",
                star?star->uniqueID:0u,position.x,position.y,position.z);
        }else std::fprintf(stderr,"[native QA fixture] Power Star cancelled: player not ready\n");
    }
    // Explicit QA travel only. The boss, fight, health and rewards stay on
    // their original map/native state path; the test starts at the summit.
    static bool boss_arena_placed=false;
    const char *boss_test=std::getenv("SM64DS_PC_TEST_BOSS_ARENA");
    if(!boss_arena_placed && ntr::rt_frame()==1700 && data_0209f2f8==6 &&
       automation && std::strcmp(automation,"1")==0 && boss_test && std::strcmp(boss_test,"1")==0) {
        boss_arena_placed=true;
        auto *player=static_cast<Player *>(data_0209f394[0]);
        auto *boss=static_cast<daBombking_c *>(dActor_c::FindWithActorID(189,nullptr));
        if(player && boss && player->aliveState==1 && !player->mIsNoControl && !player->mIsControlDisabled) {
            player->mPosX=player->mPrevPosX=player->mPreClsnPosX=boss->mPosX+260*4096;
            player->mPosY=player->mPrevPosY=player->mPreClsnPosY=boss->mPosY+40*4096;
            player->mPosZ=player->mPrevPosZ=player->mPreClsnPosZ=boss->mPosZ;
            player->mHorzSpeed=player->mVertSpeed=0;
            std::fprintf(stderr,"[native QA fixture] summit travel: player=%u boss=%u param=%04x pos=%d,%d,%d; boss health/state/reward unchanged\n",
                player->uniqueID,boss->uniqueID,boss->param1,player->mPosX,player->mPosY,player->mPosZ);
        } else std::fprintf(stderr,"[native QA fixture] summit travel cancelled: actor/player not ready\n");
    }
    // Presentations can repeat a simulation tick, even at a 60 Hz target.
    // Advance input-only replay state once per game tick, so a repeated
    // presentation cannot skip the neutral input at a waypoint boundary.
    static uint64_t last_controls_tick=~uint64_t(0);
    if(last_controls_tick!=ntr::rt_frame()) {
        last_controls_tick=ntr::rt_frame();
        if(!replay_waterbomb_controls() && !replay_cannon_cycle_controls() && !replay_cannon_controls() && !replay_buddy_controls() && !replay_rabbit_controls() && !replay_course_route())replay_boss_controls();
    }
    if(const char *path=std::getenv("SM64DS_PC_BOSS_CAPTURE")) {
        static FILE *file=std::fopen(path,"w");static uint64_t last=~uint64_t(0);
        const auto tick=ntr::rt_frame();
        if(file && tick>=600 && last!=tick) {
            if(last==~uint64_t(0))std::fputs("tick,id,kind,param,alive,x,y,z,state,health,action,held,spawn0,spawn1,hit,other,hit2,other2,fuse,mouth,egg,player_state,player_x,player_y,player_z,player_heading,player_no_control,player_health,message_active,power_star_total\n",file);
            last=tick;
            auto *player=static_cast<Player *>(data_0209f394[0]);
            unsigned count=0;
            for(auto *a=dActor_c::Next(nullptr);a && count++<4096;a=dActor_c::Next(a)) {
                if(a->actorID!=189 && a->actorID!=206)continue;
                unsigned state=0,hit=0,other=0,hit2=0,other2=0,held=0,spawn0=0,spawn1=0,fuse=0;int health=0,action=0;
                if(a->actorID==189) {
                    auto *b=static_cast<daBombking_c *>(a);
                    state=observed_boss_state(b);health=b->mHealth;action=b->mActionStep;
                    held=b->mHeldActor?b->mHeldActor->uniqueID:0;spawn0=b->mSpawnedId[0];spawn1=b->mSpawnedId[1];
                    hit=b->mdCcAcPos_c.hitFlags;other=b->mdCcAcPos_c.otherOwner;
                    hit2=b->mdCcAcPos_c2.hitFlags;other2=b->mdCcAcPos_c2.otherOwner;
                }else {
                    auto *b=static_cast<daBmb_c *>(a);state=b->mState;fuse=b->mFuse;action=b->mVariant;
                    hit=b->mdCc_c.hitFlags;other=b->mdCc_c.otherOwner;
                }
                auto *egg=dActor_c::FindWithActorID(9,nullptr);
                auto *mouth=player?reinterpret_cast<dActor_c *>(player->mObjInMouth):nullptr;
                std::fprintf(file,"%llu,%u,%u,%u,%u,%d,%d,%d,%u,%d,%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%s,%d,%d,%d,%d,%u,%d,%u,%u\n",
                    static_cast<unsigned long long>(tick),a->uniqueID,a->actorID,a->param1,a->aliveState,
                    a->mPosX,a->mPosY,a->mPosZ,state,health,action,held,spawn0,spawn1,hit,other,hit2,other2,fuse,
                    mouth?mouth->uniqueID:0,egg?egg->uniqueID:0,pc_player_state_name(player),
                    player?player->mPosX:0,player?player->mPosY:0,player?player->mPosZ:0,player?player->mAngleY:0,
                    player?player->mIsNoControl:0,player?player->GetHealth():0,data_0209d660,observed_power_star_total());
            }
            std::fflush(file);
        }
    }
    if (const char *path=std::getenv("SM64DS_PC_REWARD_CAPTURE")) {
        static FILE *file=std::fopen(path,"w");
        static uint64_t previous=~uint64_t(0);
        const auto tick=ntr::rt_frame();
        if(file && tick>=1500 && previous!=tick) {
            if(previous==~uint64_t(0))std::fputs("tick,id,kind,alive,param,x,y,z,flags,state,death,hit,other,target,payout_index,payout_bits,payload,course_coins,silver_stars,active_star_index,active_star_id,last_hit,tracked_star,player_anim_index,player_anim_file_id,star_id,star_kind,star_home_state,star_talk_step,star_sequence_timer,story_course,save_star_mask,power_star_total,message_active,message_answer,player_noctrl_kind\n",file);
            previous=tick;
            // Observe the exact VS sequence used by the original conditional
            // Goomba/marker workers. Story mission selection is a separate value.
            unsigned char current_star=0;
            SIZE_T bytes_read=0;
            const int active_star=(data_0209f344 && data_0209f208<12 &&
                ReadProcessMemory(GetCurrentProcess(),data_0209f344+data_0209f208,
                    &current_star,sizeof(current_star),&bytes_read) && bytes_read==1)
                ? static_cast<int>(current_star) : -1;
            const auto *observed_player=static_cast<Player *>(data_0209f394[0]);
            const unsigned anim_index=observed_player ? observed_player->mCharFileBase+(observed_player->param1&3) : 736;
            unsigned short anim_file=0;bytes_read=0;
            const int anim_file_id=(anim_index<736 && ReadProcessMemory(GetCurrentProcess(),
                reinterpret_cast<void *>(data_ov002_020ff480[anim_index]),&anim_file,sizeof(anim_file),&bytes_read) &&
                bytes_read==sizeof(anim_file)) ? static_cast<int>(anim_file) : -1;
            unsigned count=0;
            for(auto *a=dActor_c::Next(nullptr);a && count++<4096;a=dActor_c::Next(a)) {
                if(a->actorID!=9 && a->actorID!=178 && a->actorID!=179 && a->actorID!=180 && a->actorID!=269 && (a->actorID<200 || a->actorID>202) && (a->actorID<288 || a->actorID>290))continue;
                int state=-1,death=0,index=-1,bits=0,payload=0;
                unsigned hit=0,other=0,target=0,last_hit=0;
                int tracked_star=-1;
                int star_id=-1,star_kind=-1,star_home=-1,star_talk=-1,star_timer=-1;
                if(a->actorID==9) {
                    const auto *e=static_cast<daYegg_c *>(a);
                    state=e->mState;hit=e->mdCcAc_c.hitFlags;other=e->mdCcAc_c.otherOwner;
                    target=e->mTargetId;index=e->mPayoutIdx;payload=e->mParamHigh;
                    for(int i=0;i<5;++i)if(e->mPayoutDone[i])bits|=1<<i;
                }else if(a->actorID>=200 && a->actorID<=202) {
                    const auto *g=static_cast<daKrb_c *>(a);
                    state=g->mState;death=g->mDeathState;hit=g->mdCcAc_c.hitFlags;other=g->mdCcAc_c.otherOwner;
                    payload=g->unk_108;index=g->mCapId;bits=g->mRewardType;target=g->mStarID;
                    last_hit=g->mLastHitFlags;tracked_star=g->mStarTracked;
                }else if(a->actorID==178 || a->actorID==179) {
                    const auto *s=static_cast<daStar_c *>(a);
                    state=s->mState;hit=s->mdCc_c.hitFlags;other=s->mdCc_c.otherOwner;
                    target=s->mMarkerID;payload=s->mStarFlags;
                    star_id=s->mStarID;star_kind=s->mKind;star_home=s->mHomeState;
                    star_talk=s->mTalkStep;star_timer=s->mSeqTimer;
                }else if(a->actorID==180) {
                    const auto *s=static_cast<daStarBase_c *>(a);
                    state=s->mState;target=s->mLinkedStarID;
                }else if(a->actorID==269) {
                    const auto *c=static_cast<daObjMarioCap_c *>(a);
                    state=c->mType;index=c->mTakenStep;payload=c->mModelIndex;
                    hit=c->mdCcAc_c.hitFlags;other=c->mdCcAc_c.otherOwner;
                }
                std::fprintf(file,"%llu,%u,%u,%u,%u,%d,%d,%d,%u,%d,%d,%u,%u,%u,%d,%d,%d,%d,%d,%u,%d,%u,%d,%u,%d",
                    static_cast<unsigned long long>(tick),a->uniqueID,a->actorID,a->aliveState,a->param1,
                    a->mPosX,a->mPosY,a->mPosZ,a->mFlags,state,death,hit,other,target,index,bits,payload,(int)data_0209f358[0],(int)data_0209f310[0],
                    static_cast<unsigned>(data_0209f208),active_star,last_hit,tracked_star,anim_index,anim_file_id);
                const int course=observed_story_course();
                std::fprintf(file,",%d,%d,%d,%d,%d,%d,%u,%u,%u,%u,%u\n",
                    star_id,star_kind,star_home,star_talk,star_timer,course,
                    course>=0 && course<30 ? data_0209caa0[0x14+course] : 0u,
                    observed_power_star_total(),data_0209d660,data_0209d684,
                    observed_player ? observed_player->mNoCtrlKind : 255u);
            }
            std::fflush(file);
        }
    }
    // Optional per-frame observation of real tongue contacts; never changes game state.
    if (const char *path = std::getenv("SM64DS_PC_YOSHI_CAPTURE")) {
        static FILE *file = std::fopen(path, "w");
        static uint64_t previous = ~uint64_t(0);
        const auto tick = ntr::rt_frame();
        auto *p = static_cast<Player *>(data_0209f394[0]);
        if (file && tick != previous && p && tick >= 1500) {
            if (previous == ~uint64_t(0)) std::fputs("tick,state,step,mouth,px,py,pz,heading,attack_flags,attack_hit,attack_other,ax,ay,az,radius,height,goomba,gx,gy,gz,gflags,ghit,gother,eaten,egg_id,egg_x,egg_y,egg_z\n",file);
            previous = tick;
            const auto *egg=dActor_c::FindWithActorID(9,nullptr);
            for (dActor_c *a=dActor_c::Next(nullptr);a;a=dActor_c::Next(a)) {
                if(a->actorID<200 || a->actorID>202)continue;
                auto *g=static_cast<daKrb_c *>(a);
                auto &c=p->mAttackClsn;
                auto *mouth=reinterpret_cast<dActor_c *>(p->mObjInMouth);
                std::fprintf(file,"%llu,%s,%u,%u,%d,%d,%d,%d,%u,%u,%u,%d,%d,%d,%d,%d,%u,%d,%d,%d,%u,%u,%u,%u,%u,%d,%d,%d\n",
                    static_cast<unsigned long long>(tick),pc_player_state_name(p),p->mStateStep,mouth?mouth->uniqueID:0,
                    p->mPosX,p->mPosY,p->mPosZ,p->mAngleY,c.flags,c.hitFlags,c.otherOwner,
                    (int)c.pos.x,(int)c.pos.y,(int)c.pos.z,(int)c.radius,(int)c.height,
                    g->uniqueID,g->mPosX,g->mPosY,g->mPosZ,g->mFlags,g->mdCcAc_c.hitFlags,g->mdCcAc_c.otherOwner,g->mEatenByYoshi,
                    egg?egg->uniqueID:0,egg?egg->mPosX:0,egg?egg->mPosY:0,egg?egg->mPosZ:0);
            }
            std::fflush(file);
        }
    }
    // Optional bounded state sampling for reproducible gameplay checks. It
    // observes actual actors after the frame; normal play never opens a file.
    const char *state_path = std::getenv("SM64DS_PC_STATE_CAPTURE");
    if (state_path && state_path[0]) {
        static FILE *state_file = nullptr;
        static uint64_t last_tick = ~uint64_t(0);
        const uint64_t tick = ntr::rt_frame();
        const char *interval_text = std::getenv("SM64DS_PC_STATE_CAPTURE_INTERVAL");
        const uint64_t requested_interval = interval_text ? std::strtoull(interval_text, nullptr, 10) : 120;
        const uint64_t interval = requested_interval ? requested_interval : 120;
        if (last_tick != tick && (tick == 1 || tick % interval == 0)) {
            last_tick = tick;
            if (!state_file) {
                state_file = std::fopen(state_path, "w");
                if (state_file)
                    std::fprintf(state_file, "tick,level,script_active,stick_x,stick_y,player_x,player_y,player_z,actors,yellow_total,yellow_alive,red_alive,blue_alive,intro_ready,game_stick_magnitude,game_mode,lakitu_alive,lakitu_state,lakitu_timer,lakitu_param,camera_state,camera_flags,camera_look_x,camera_look_y,camera_look_z,camera_x,camera_y,camera_z,player_heading,minimap_pa,minimap_pb,minimap_pc,minimap_pd,minimap_rotate,minimap_hide_target,pause_active,pause_state,pause_next,pause_selection,pause_delay,pause_fullscreen,pc_settings_page,pc_settings_row,output_width,output_height,target_fps,hd_textures,crisp_2d,minimap_enabled,map_opacity,map_scale,map_x,map_y,player_state,player_disabled,player_no_control,player_airborne,player_horz_speed,player_vert_speed,player_attachment,player_state_step,player_state_work,player_state_timer,player_state_flags,player_pre_x,player_pre_y,player_pre_z,game_held,game_pressed,reader_magnitude,reader_angle,control_mode,texture_quality,save_character,selected_character,player_character,player_parameter,player_body_model,player_hat_character,save_status,save_course2_stars,save_course5_coins,power_star_total,story_course,story_star_mask\n");
            }
            if (state_file) {
                unsigned actors = 0, yellow = 0, alive[3] = {};
                dActor_c *lakitu = nullptr;
                unsigned lakitu_alive = 0;
                static FILE *goomba_file=nullptr;
                const char *goomba_path=std::getenv("SM64DS_PC_GOOMBA_CAPTURE");
                if(goomba_path && !goomba_file) {
                    goomba_file=std::fopen(goomba_path,"w");
                    if(goomba_file)std::fputs("tick,id,kind,alive,x,y,z,state,eaten,death,flags\n",goomba_file);
                }
                for (dActor_c *actor = dActor_c::Next(nullptr); actor && actors < 4096;
                     actor = dActor_c::Next(actor)) {
                    ++actors;
                    if(goomba_file && tick%60==0 && actor->actorID>=200 && actor->actorID<=202) {
                        const auto *g=static_cast<const daKrb_c *>(actor);
                        std::fprintf(goomba_file,"%llu,%u,%u,%u,%d,%d,%d,%d,%u,%u,%u\n",
                            static_cast<unsigned long long>(tick),g->uniqueID,g->actorID,g->aliveState,
                            g->mPosX,g->mPosY,g->mPosZ,g->mState,g->mEatenByYoshi,g->mDeathState,g->mFlags);
                    }
                    if (actor->actorID == 0x120) ++yellow;
                    if (actor->actorID >= 0x120 && actor->actorID <= 0x122 && actor->aliveState == 1)
                        ++alive[actor->actorID - 0x120];
                    if (actor->actorID == 235) {
                        lakitu = actor;
                        if (actor->aliveState == 1) ++lakitu_alive;
                    }
                }
                auto *player = static_cast<dActor_c *>(data_0209f394[0]);
                if(goomba_file)std::fflush(goomba_file);
                const ntr::InputState input = ntr::input_state();
                uint32_t intro_flags;
                int16_t game_magnitude;
                std::memcpy(&intro_flags, data_0209caa0 + 8, sizeof(intro_flags));
                std::memcpy(&game_magnitude, data_0209f498 + 8, sizeof(game_magnitude));
                std::fprintf(state_file, "%llu,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u,%u,%u,%d,%u,%u,%d,%d,%u,%d,%u,%d,%d,%d,%d,%d,%d,%u,%d,%d,%d,%d,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d",
                             static_cast<unsigned long long>(tick), static_cast<int>(data_0209f2f8),
                             data_0209fc48 ? 1 : 0, static_cast<int>(input.left_x), static_cast<int>(input.left_y),
                             player ? player->mPosX : 0, player ? player->mPosY : 0,
                             player ? player->mPosZ : 0, actors, yellow, alive[0], alive[1], alive[2],
                             (intro_flags & 0x80u) ? 1u : 0u, static_cast<int>(game_magnitude),
                             static_cast<unsigned>(data_0209f2d8), lakitu_alive,
                             pc_lakitu_state_id(lakitu), pc_lakitu_timer(lakitu),
                             lakitu ? lakitu->param1 : 0u,
                             pc_camera_state_id(data_0209f318), data_0209f318 ? data_0209f318->mFlags : 0u,
                             data_0209f318 ? (int)data_0209f318->lookAt.x : 0,
                             data_0209f318 ? (int)data_0209f318->lookAt.y : 0,
                             data_0209f318 ? (int)data_0209f318->lookAt.z : 0,
                             data_0209f318 ? (int)data_0209f318->pos.x : 0,
                             data_0209f318 ? (int)data_0209f318->pos.y : 0,
                             data_0209f318 ? (int)data_0209f318->pos.z : 0,
                             player ? (unsigned)(unsigned short)player->mAngleY : 0u,
                             data_0209f3c4.matrix[0], data_0209f3c4.matrix[1],
                             data_0209f3c4.matrix[2], data_0209f3c4.matrix[3],
                             ntr::video_get_settings().minimap_rotate_with_player ? 1u : 0u,
                             ntr::video_get_settings().minimap_hide_touch_target ? 1u : 0u,
                             (unsigned)data_0209f2c4, (unsigned)data_0209f248,
                             (unsigned)data_0209f1ec, (unsigned)data_0209f2e0,
                             (unsigned)data_0209f300, ntr::video_pause_screen() ? 1u : 0u,
                             pc_pause_settings_page(), pc_pause_settings_row());
                const auto settings = ntr::video_get_settings();
                std::fprintf(state_file, ",%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",
                    settings.width, settings.height, settings.target_fps,
                    settings.texture_quality==ntr::TextureQuality::Upscaled?1u:0u,settings.upscale_2d?1u:0u,
                    settings.minimap?1u:0u,settings.minimap_opacity,settings.minimap_scale,
                    settings.minimap_x,settings.minimap_y);
                const auto *full=static_cast<const Player *>(player);
                uint32_t attachment=0;
                if(full) std::memcpy(&attachment,&full->unk_37c,sizeof(attachment));
                uint16_t game_held=0,game_pressed=0;int16_t reader_magnitude=0,reader_angle=0;
                std::memcpy(&game_held,data_0209f49c,2);std::memcpy(&game_pressed,data_0209f49e,2);
                std::memcpy(&reader_magnitude,data_0209f4a0,2);std::memcpy(&reader_angle,data_0209f4a6,2);
                std::fprintf(state_file,",%s,%u,%u,%u,%d,%d,%u,%u,%u,%u,%u,%d,%d,%d,%u,%u,%d,%d,%u,%u",
                    pc_player_state_name(full),full?full->mIsControlDisabled:0u,
                    full?full->mIsNoControl:0u,full?full->mIsAirborne:0u,
                    full?full->mHorzSpeed:0,full?full->mVertSpeed:0,attachment,
                    full?full->mStateStep:0u,full?full->mStateWork:0u,full?full->mStateTimer:0u,
                    full?full->mStateFlags:0u,full?full->mPreClsnPosX:0,
                    full?full->mPreClsnPosY:0,full?full->mPreClsnPosZ:0,
                    game_held,game_pressed,reader_magnitude,reader_angle,data_0209f4ae[0],static_cast<unsigned>(settings.texture_quality));
                std::fprintf(state_file,",%u,%u,%u,%u,%u,%u,%d,%u,%u,%u,%d,%u\n",data_0209caa0[0x41],data_02092128[0],
                    full?full->mCharacter:255u,full?full->param1:255u,full?full->mBodyModelId:255u,full?full->mHatCharacter:255u,pc_pause_save_status(),
                    static_cast<unsigned>(SaveData::CountStarsCollectedInLevel(2)),static_cast<unsigned>(SaveData::GetCoinRecord(5)),
                    observed_power_star_total(),observed_story_course(),
                    observed_story_course()>=0 && observed_story_course()<30 ? data_0209caa0[0x14+observed_story_course()] : 0u);

                std::fflush(state_file);
            }
        }
    }
    if(const char *path=std::getenv("SM64DS_PC_CANNON_CAPTURE")) {
        static FILE *file=nullptr;static uint64_t previous=~uint64_t(0);
        const auto tick=ntr::rt_frame();
        if(tick!=previous && tick%10==0) {
            previous=tick;
            if(!file){file=std::fopen(path,"w");if(file)std::fputs("tick,level,id,kind,variant,state,step,x,y,z,pitch,yaw,hit,other,player_state,player_step,px,py,pz,vy,speed,airborne,control_disabled,camera_state,camera_yaw,camera_pitch\n",file);}
            auto *p=static_cast<Player *>(data_0209f394[0]);
            for(unsigned kind:{207u,208u})for(auto *a=dActor_c::FindWithActorID(kind,nullptr);file && a;a=dActor_c::FindWithActorID(kind,a)) {
                if(a->aliveState!=1)continue;
                auto *c=kind==207?static_cast<daCnn_c *>(a):nullptr;
                auto *b=kind==208?static_cast<daWbm_c *>(a):nullptr;
                const auto *cam=reinterpret_cast<const char *>(data_0209f318);
                short pitch=0;if(cam)std::memcpy(&pitch,cam+0x17e,2);
                std::fprintf(file,"%llu,%d,%u,%u,%u,%d,%u,%d,%d,%d,%d,%d,%u,%u,%s,%u,%d,%d,%d,%d,%d,%u,%u,%d,%d,%d\n",
                    static_cast<unsigned long long>(tick),data_0209f2f8,a->uniqueID,kind,c?unsigned(c->mVariant):unsigned(b->mVariant),c?c->mState:b->mState,c?c->mLoadStep:0,
                    a->mPosX,a->mPosY,a->mPosZ,a->mAngleX,a->mAngleY,c?c->mdCcAc_c.hitFlags:b->mdCcAc_c.hitFlags,c?c->mdCcAc_c.otherOwner:b->mdCcAc_c.otherOwner,
                    pc_player_state_name(p),p?p->mStateStep:0,p?p->mPosX:0,p?p->mPosY:0,p?p->mPosZ:0,p?p->mVertSpeed:0,p?p->mHorzSpeed:0,p?p->mIsAirborne:0,p?p->mIsControlDisabled:0,
                    pc_camera_state_id(data_0209f318),cam?reinterpret_cast<const dCamera_c *>(cam)->mAngleY:0,pitch);
            }
            if(file)std::fflush(file);
        }
    }
    capture_waterbomb_state();
    if (const char *path = std::getenv("SM64DS_PC_BUDDY_CAPTURE")) {
        static FILE *file=nullptr;
        static uint64_t previous=~uint64_t(0);
        const auto tick=ntr::rt_frame();
        if(tick!=previous && tick%10==0) {
            previous=tick;
            if(!file) {
                file=std::fopen(path,"w");
                if(file)std::fputs("tick,level,id,kind,param,variant,x,y,z,state,step,opened,shutter,hit,other,message,player_state,no_control,cannon_flags\n",file);
            }
            auto *player=static_cast<Player *>(data_0209f394[0]);
            unsigned cannon_flags=0;std::memcpy(&cannon_flags,data_0209caa0+0x10,4);
            if(file)for(unsigned kind : {181u,14u})
                for(auto *a=dActor_c::FindWithActorID(kind,nullptr);a;a=dActor_c::FindWithActorID(kind,a)) {
                    if(a->aliveState!=1)continue;
                    auto *b=kind==181?static_cast<daRedBombhei_c *>(a):nullptr;
                    auto *s=kind==14?static_cast<daObjCannonShutter_c *>(a):nullptr;
                    if(s && std::getenv("SM64DS_PC_MOVING_TRACE")) {
                        static unsigned probes=0;
                        if(probes++<6) {
                            auto &mesh=s->mMeshCollider;
                            auto **vt=*reinterpret_cast<void ***>(&mesh);
                            std::fprintf(stderr,"[moving instance] id=%u vptr=%p slot=%u scale=%d mat=%d,%d,%d inv=%d,%d,%d\n",
                                a->uniqueID,vt,mesh.slotIdx,mesh.scale,mesh.mat.t.x,mesh.mat.t.y,mesh.mat.t.z,
                                mesh.invScaledMat.t.x,mesh.invScaledMat.t.y,mesh.invScaledMat.t.z);
                            for(unsigned i=0;i<12;++i)std::fprintf(stderr,"[moving slot] %u %p\n",i,vt[i]);
                        }
                    }
                    std::fprintf(file,"%llu,%d,%u,%u,%u,%u,%d,%d,%d,%d,%u,%u,%u,%u,%u,%u,%s,%u,%u\n",
                        static_cast<unsigned long long>(tick),data_0209f2f8,a->uniqueID,kind,a->param1,
                        a->mAngleX&255,a->mPosX,a->mPosY,a->mPosZ,b?b->mState:-1,
                        b?b->mCutsceneStep:s->mOpenPhase,b?b->mCannonOpened:s->mCannonOpen,
                        b?b->mShutterID:0,b?b->mdCcAc_c.hitFlags:0,b?b->mdCcAc_c.otherOwner:0,
                        data_0209d660,pc_player_state_name(player),player?player->mIsNoControl:0,cannon_flags);
                }
            if(file)std::fflush(file);
        }
    }
    if (const char *path = std::getenv("SM64DS_PC_RABBIT_CAPTURE")) {
        static FILE *file=nullptr;
        static uint64_t previous=~uint64_t(0);
        const auto tick=ntr::rt_frame();
        if(tick!=previous && tick%10==0) {
            previous=tick;
            if(!file) {
                file=std::fopen(path,"w");
                if(file)std::fputs("tick,flags1,flags2,rabbits,unique,param,rabbit_id,state,disabled,eaten,grab,talk,x,y,z,collision,other,key_alive,key_unique,key_talk,key_source,player_state\n",file);
            }
            daMip_c *rabbit=nullptr;unsigned count=0;
            for(auto *a=dActor_c::FindWithActorID(187,nullptr);a;a=dActor_c::FindWithActorID(187,a)) {
                if(a->aliveState!=1)continue;
                ++count;auto *r=static_cast<daMip_c *>(a);
                if(!rabbit || r->mRabbitId==7)rabbit=r;
            }
            auto *key=static_cast<daObj_Mip_Key_c *>(dActor_c::FindWithActorID(229,nullptr));
            auto *player=static_cast<Player *>(data_0209f394[0]);
            unsigned flags1=0,flags2=0;
            std::memcpy(&flags1,data_0209caa0+4,4);std::memcpy(&flags2,data_0209caa0+8,4);
            if(file) {
                std::fprintf(file,"%llu,%08x,%08x,%u,%u,%u,%d,%d,%u,%u,%u,%u,%d,%d,%d,%08x,%u,%u,%u,%d,%d,%s\n",
                    static_cast<unsigned long long>(tick),flags1,flags2,count,
                    rabbit?rabbit->uniqueID:0,rabbit?rabbit->param1:0,rabbit?rabbit->mRabbitId:-1,
                    pc_rabbit_state_id(rabbit),rabbit?rabbit->mIsDisabled:0,rabbit?rabbit->mEatenByYoshi:0,
                    rabbit?rabbit->unk_426:0,rabbit?rabbit->mTalkState:0,
                    rabbit?rabbit->mPosX:0,rabbit?rabbit->mPosY:0,rabbit?rabbit->mPosZ:0,
                    rabbit?rabbit->mdCcAc_c.hitFlags:0,rabbit?rabbit->mdCcAc_c.otherOwner:0,
                    key&&key->aliveState==1?1u:0u,key?key->uniqueID:0,key?key->mTalkState:-1,key?key->unk_190:0,
                    player?pc_player_state_name(player):"none");
                std::fflush(file);
            }
        }
    }
    // Optional read-only Lakitu/intro gate trace for first-file route work.
    // Keep it separate from the broad state CSV so existing consumers retain
    // their schema. This records the save gate and native actor state together.
    if (const char *path = std::getenv("SM64DS_PC_LAKITU_CAPTURE")) {
        static FILE *file = nullptr;
        static uint64_t previous = ~uint64_t(0);
        const uint64_t tick = ntr::rt_frame();
        if (tick != previous && tick % 10 == 0) {
            previous = tick;
            if (!file) {
                file = std::fopen(path, "w");
                if (file) std::fputs("tick,flags2,lakitu_alive,lakitu_state,lakitu_timer,param,x,y,z,hidden,variant,player_x,player_y,player_z,player_state,no_control,player_angle,door_dx,door_dy,door_dz,door_angle,door_state,music_volume,message_volume,door_countdown\n", file);
            }
            if (file) {
                dActor_c *lakitu = nullptr;
                for (dActor_c *actor = dActor_c::Next(nullptr); actor; actor = dActor_c::Next(actor))
                    if (actor->actorID == 235) { lakitu = actor; break; }
                auto *npc = static_cast<daC_Jugem_c *>(lakitu);
                auto *player = static_cast<Player *>(data_0209f394[0]);
                daDoor_c *door = nullptr;
                double nearest = 1e100;
                if (player) for (auto *actor = dActor_c::FindWithActorID(353, nullptr);
                    actor; actor = dActor_c::FindWithActorID(353, actor)) {
                    const double dx = double(actor->mPosX) - player->mPosX;
                    const double dz = double(actor->mPosZ) - player->mPosZ;
                    if (dx * dx + dz * dz < nearest) {
                        nearest = dx * dx + dz * dz;
                        door = static_cast<daDoor_c *>(actor);
                    }
                }
                uint32_t flags2 = 0;
                std::memcpy(&flags2, data_0209caa0 + 8, sizeof(flags2));
                std::fprintf(file, "%llu,%08x,%u,%d,%d,%u,%d,%d,%d,%u,%d,%d,%d,%d,%s,%u,%d,%d,%d,%d,%d,%p,%d,%d,%u\n",
                    static_cast<unsigned long long>(tick), flags2,
                    lakitu && lakitu->aliveState == 1 ? 1u : 0u,
                    pc_lakitu_state_id(lakitu), pc_lakitu_timer(lakitu),
                    lakitu ? lakitu->param1 : 0u,
                    lakitu ? lakitu->mPosX : 0, lakitu ? lakitu->mPosY : 0,
                    lakitu ? lakitu->mPosZ : 0, npc ? npc->mHidden : 0,
                    npc ? npc->mVariant : -1,
                    player ? player->mPosX : 0, player ? player->mPosY : 0,
                    player ? player->mPosZ : 0,
                    player ? pc_player_state_name(player) : "none",
                    player ? player->mNoCtrlKind : 0u,
                    player ? player->mAngleY : 0,
                    door ? door->mScaleX : 0, door ? door->mScaleY : 0,
                    door ? door->mScaleZ : 0, door ? door->mAngleY : 0,
                    door ? door->mState : nullptr, data_0209b490, data_0209b49c,
                    data_ov100_02148708[0]);
                std::fflush(file);
            }
        }
    }
    if (trace_enabled() && (frame == 1 || frame % 120 == 0)) {
        auto *fader = data_0209f5bc;
        std::fprintf(stderr, "[script progress] presentation=%llu script=%p movie_frame=%u fade=%d speed=%d pending_level=%d current_level=%d\n",
                     static_cast<unsigned long long>(frame), data_0209fc48,
                     static_cast<unsigned>(data_0209b274),
                     fader ? static_cast<int>(fader->currInterp) : -1,
                     fader ? static_cast<int>(fader->speed) : 0,
                     static_cast<int>(data_02092110), static_cast<int>(data_0209f2f8));
    }
    return true;
}

static void report_player_animation()
{
    unsigned char snapshot[0x740]={};SIZE_T read=0;
    if(!ReadProcessMemory(GetCurrentProcess(),data_0209f394[0],snapshot,sizeof(snapshot),&read) || read!=sizeof(snapshot))return;
    auto word=[&](unsigned offset) { unsigned value;std::memcpy(&value,snapshot+offset,4);return value; };
    const unsigned animation=word(0x63c)+(word(8)&3);
    std::fprintf(stderr,"[sm64ds] animation snapshot tick=%llu player=%p char=%u animation=%u held=%08x flags=%u/%u/%u/%u\n",
        static_cast<unsigned long long>(ntr::rt_frame()),data_0209f394[0],word(8),animation,word(0x358),
        snapshot[0x6f9],snapshot[0x6fb],snapshot[0x6fd],snapshot[0x6ff]);
    if(animation<736) {
        unsigned handle[2]={};read=0;
        ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void *>(data_ov002_020ff480[animation]),handle,sizeof(handle),&read);
        std::fprintf(stderr,"[sm64ds] animation handle=%08x fileID/ref=%08x file=%08x\n",data_ov002_020ff480[animation],handle[0],handle[1]);
    }
    for(unsigned i=0;i<4;++i) {
        unsigned model[0x78/4]={};read=0;
        if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void *>(word(0xdc+i*4)),model,sizeof(model),&read) || read!=sizeof(model))continue;
        unsigned header[6]={};read=0;
        ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void *>(model[0x60/4]),header,sizeof(header),&read);
        std::fprintf(stderr,"[sm64ds] model%u=%08x BCA=%08x frame=%08x other=%08x bones=%08x header=%08x/%08x/%08x/%08x/%08x/%08x\n",
            i,word(0xdc+i*4),model[0x60/4],model[0x58/4],model[0x64/4],model[0x10/4],header[0],header[1],header[2],header[3],header[4],header[5]);
    }
}
static LONG WINAPI report_unhandled_exception(EXCEPTION_POINTERS *exception)
{
    const char *crash_trace = std::getenv("SM64DS_PC_CRASH_TRACE");
    if ((trace_enabled() || (crash_trace && crash_trace[0] == '1')) && exception && exception->ExceptionRecord &&
        exception->ContextRecord) {
        const CONTEXT *context = exception->ContextRecord;
        const uintptr_t module_base = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
        std::fprintf(stderr,
                     "[sm64ds] unhandled exception: code=%08lx address=%p "
                     "module=%08lx EIP=%08lx RVA=%08lx ESP=%08lx "
                     "EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx "
                     "EBP=%08lx ESI=%08lx EDI=%08lx\n",
                     exception->ExceptionRecord->ExceptionCode,
                     exception->ExceptionRecord->ExceptionAddress,
                     static_cast<unsigned long>(module_base),
                     context->Eip,
                     static_cast<unsigned long>(context->Eip - module_base),
                     context->Esp, context->Eax, context->Ebx,
                     context->Ecx, context->Edx, context->Ebp,
                     context->Esi, context->Edi);
        std::fflush(stderr);
        unsigned char code[24] = {};
        SIZE_T bytes_read = 0;
        std::fprintf(stderr, "[sm64ds] fault bytes:");
        if (context->Eip >= 8 && ReadProcessMemory(GetCurrentProcess(),
                reinterpret_cast<const void *>(context->Eip - 8), code,
                sizeof(code), &bytes_read)) {
            for (SIZE_T i = 0; i < bytes_read; ++i)
                std::fprintf(stderr, " %02x", static_cast<unsigned>(code[i]));
        } else {
            std::fprintf(stderr, " unavailable");
        }
        std::fputc('\n', stderr);
        std::fprintf(stderr,
                     "[sm64ds] x86 regs EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx ESI=%08lx EDI=%08lx EBP=%08lx ESP=%08lx EFLAGS=%08lx\n",
                     context->Eax, context->Ebx, context->Ecx, context->Edx,
                     context->Esi, context->Edi, context->Ebp, context->Esp,
                     context->EFlags);
        // Frame pointers are retained by the host build. Read defensively so a
        // damaged stack cannot fault the exception reporter itself.
        uintptr_t caller_frame=context->Ebp;
        for(unsigned depth=0;depth<16 && caller_frame;++depth) {
            uint32_t words[6]={};SIZE_T read=0;
            if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void *>(caller_frame),
                                  words,sizeof(words),&read) || read!=sizeof(words))break;
            std::fprintf(stderr,"[sm64ds] frame%u fp=%08lx return=%08lx RVA=%08lx args=%08lx,%08lx,%08lx,%08lx\n",
                depth,static_cast<unsigned long>(caller_frame),static_cast<unsigned long>(words[1]),
                static_cast<unsigned long>(words[1]-module_base),
                static_cast<unsigned long>(words[2]),static_cast<unsigned long>(words[3]),
                static_cast<unsigned long>(words[4]),static_cast<unsigned long>(words[5]));
            if(words[0]<=caller_frame || words[0]-caller_frame>0x100000)break;
            caller_frame=words[0];
        }
        const uint32_t *stack = reinterpret_cast<const uint32_t *>(context->Esp);
        report_player_animation();
        std::fprintf(stderr, "[sm64ds] exception stack:");
        for (unsigned i = 0; i < 8; ++i)
            std::fprintf(stderr, " %08lx", static_cast<unsigned long>(stack[i]));
        std::fputc('\n', stderr);
        const uint32_t *frame = reinterpret_cast<const uint32_t *>(context->Ebp - 0x60);
        std::fprintf(stderr, "[sm64ds] exception frame (-60..+0c):");
        for (unsigned i = 0; i < 28; ++i)
            std::fprintf(stderr, " %08lx", static_cast<unsigned long>(frame[i]));
        std::fputc('\n', stderr);
        std::fflush(stderr);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

static void trace(const char *message)
{
    if (trace_enabled()) {
        std::fprintf(stderr, "[sm64ds] %s\n", message);
        std::fflush(stderr);
    }
}

static void run_game()
{
    trace("game entry");
    _ZN4Heap18InitializeRootHeapEv();
    _ZN4Heap9SetNodeIDEj(data_020a0e9c, 1);
    trace("root heap initialized");
    func_0201a054();
    trace("ARM9 initialization returned; entering main scene");
    func_020197b8();
    trace("main scene returned");
}

static uint64_t runtime_frame_limit()
{
    const char *value = std::getenv("SM64DS_PC_MAX_FRAMES");
    if (!value || !value[0]) return 0;
    char *end = nullptr;
    const unsigned long long parsed = std::strtoull(value, &end, 10);
    return end != value && *end == '\0' ? static_cast<uint64_t>(parsed) : 0;
}

static bool copy_path(char *destination, size_t capacity, const char *source)
{
    if (!destination || capacity == 0 || !source || !source[0]) return false;
    const size_t length = std::strlen(source);
    if (length >= capacity) return false;
    std::memcpy(destination, source, length + 1);
    return true;
}

static bool executable_directory(char *directory, size_t capacity)
{
    trace("ROM path lookup: get executable path");
    if (!directory || capacity == 0) return false;
    const DWORD length = GetModuleFileNameA(nullptr, directory,
                                             static_cast<DWORD>(capacity));
    trace("ROM path lookup: executable path returned");
    if (!length || length >= capacity) return false;
    char *slash = std::strrchr(directory, '\\');
    if (!slash) slash = std::strrchr(directory, '/');
    if (!slash) return false;
    *slash = '\0';
    trace("ROM path lookup: executable directory found");
    return true;
}

static bool find_rom_path(int argc, char **argv, char *rom_path,
                          size_t capacity)
{
    if (!rom_path || capacity == 0) return false;
    rom_path[0] = '\0';
    if (argc > 1 && argv[1] && argv[1][0])
        return copy_path(rom_path, capacity, argv[1]);
    const char *environment_path = std::getenv("SM64DS_ROM");
    if (environment_path && environment_path[0])
        return copy_path(rom_path, capacity, environment_path);

    trace("ROM path lookup: search beside executable");
    char directory[MAX_PATH] = {};
    if (!executable_directory(directory, sizeof(directory))) return false;
    WIN32_FIND_DATAA entry{};
    char pattern[MAX_PATH] = {};
    const int pattern_length = std::snprintf(pattern, sizeof(pattern), "%s\\*.nds",
                                               directory);
    if (pattern_length < 0 || static_cast<size_t>(pattern_length) >= sizeof(pattern))
        return false;
    trace("ROM path lookup: enumerate NDS files");
    HANDLE search = FindFirstFileA(pattern, &entry);
    trace("ROM path lookup: enumeration returned");
    if (search == INVALID_HANDLE_VALUE) return false;
    bool found = false;
    do {
        if (!(entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            const int path_length = std::snprintf(rom_path, capacity, "%s\\%s",
                                                   directory, entry.cFileName);
            found = path_length >= 0 && static_cast<size_t>(path_length) < capacity;
            break;
        }
    } while (FindNextFileA(search, &entry));
    FindClose(search);
    if (found) trace("ROM path lookup: selected ROM path");
    return found;
}


int main(int argc, char **argv)
{
    // Reserve the DS address ranges before reading the ROM. The ROM buffer's
    // host allocation can otherwise occupy 0x02000000, where ARM9 data objects
    // from the native game's fixed-address tables are expected to live.
    if (!ntr::io_init()) {
        trace("host MMIO initialization failed");
        return 2;
    }
    SetUnhandledExceptionFilter(report_unhandled_exception);
    trace("process start");
    trace("host MMIO initialized");
    char rom_path[MAX_PATH] = {};
    const bool has_rom_path = find_rom_path(argc, argv, rom_path,
                                             sizeof(rom_path));
    trace("ROM path lookup: completed");
    if (!has_rom_path) {
        MessageBoxA(nullptr,
                    "Place the European Mario 64 DS .nds ROM beside sm64ds_pc.exe, "
                    "or launch with the ROM path as the first argument.",
                    "Mario 64 DS - ROM required", MB_OK | MB_ICONERROR);
        return 4;
    }
    if (trace_enabled()) {
        std::fprintf(stderr, "[sm64ds] mounting ROM: %s\n", rom_path);
        std::fflush(stderr);
    }
    if (!ntr::fs_mount_rom(rom_path, false)) {
        trace("ROM mount failed");
        MessageBoxA(nullptr,
                    "The ROM could not be mounted. Check that the supplied .nds "
                    "file is a valid Mario 64 DS image.",
                    "Mario 64 DS - ROM error", MB_OK | MB_ICONERROR);
        return 5;
    }
    trace("ROM mounted");

    // Keep runtime-generated files and optional texture replacements beside
    // the distributed executable. The ROM has already been opened through its
    // explicit path, so startup no longer depends on the source checkout's cwd.
    char app_directory[MAX_PATH] = {};
    if (executable_directory(app_directory, sizeof(app_directory)))
        SetCurrentDirectoryA(app_directory);

    trace("opening game window");
    if (!ntr::video_open("Super Mario 64 DS - Native PC")) {
        trace("game window creation failed");
        return 3;
    }
    trace("game window opened; starting frame runtime");

    ntr::rt_run(run_game, trace_script_progress, runtime_frame_limit(), pc_stage_graph_prepare);
    trace("frame runtime returned");
    trace("closing game window");
    ntr::video_close();
    trace("game window closed");
    // The game replaces global delete with its game-heap allocator after
    // startup, while process-global host objects retain CRT allocations.
    // Returning from main runs their C++ destructors through the replaced
    // delete path, so terminate after closing the native window and let the
    // operating system reclaim process-owned resources.
    trace("terminating process after native shutdown");
    ExitProcess(0);
}


