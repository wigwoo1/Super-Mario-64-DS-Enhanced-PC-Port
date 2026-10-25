// Native entrypoint: preserve the cartridge's initialization and scene loop.
#include "ntr/mmio.h"
#include "ntr/rt.h"
#include "ntr/video.h"
#include "ntr/fs.h"
#include "ntr/audio.h"
#include "ntr/input.h"
#include "dFader_c.h"
#include "dActor_c.h"
#include "Player.h"
#include "dBgPi.h"
#include "daKrb_c.h"
#include "daBombking_c.h"
#include "daBmb_c.h"
#include "daYegg_c.h"
#include "daStar_c.h"
#include "daStarBase_c.h"
#include "daObjMarioCap_c.h"
#include "daC_Jugem_c.h"
#include "daDoor_c.h"
#include "daStarGate_c.h"
#include "daMip_c.h"
#include "daObj_Mip_Key_c.h"
#include "daRedBombhei_c.h"
#include "daObjCannonShutter_c.h"
#include "daCnn_c.h"
#include "daWbm_c.h"
#include "daWanwan_c.h"
#include "daObjPile_c.h"
#include "daObjWanwanShutter_c.h"
#include "daKinopio_c.h"
#include "SaveData.h"
#include "Timer.h"
#include "Event.h"
#include "daObjC0_Switch_c.h"
#include "daObjC0Water_c.h"
#include "dCamera_c.h"
#include "hal/stage_graph_state.h"
#include "hal/replay_path.h"

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
extern "C" unsigned char data_0209d4c8[];
extern "C" unsigned char data_ov002_02111184[];
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
// Local extern: verify the historic exported sound ABI through the integer
// signature used by original door/message callers, rather than its C++ wrapper.
extern "C" int _ZN5Sound7PlaySubEjjj5Fix12IiEb(unsigned,unsigned,unsigned,int,int);
extern "C" unsigned short data_ov100_02148708[];
extern "C" unsigned char data_0209f2bc,data_0209f2fc,data_0209f20c,data_0209f2d4;
extern "C" unsigned char data_ov002_02111178;
extern "C" signed char data_02092124;

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

// Read-only native completion telemetry: distinguish a pending original save
// choice from a stalled player-entry callback without changing either state.
static void trace_star_completion() {
    const char *path=std::getenv("SM64DS_PC_STAR_FLOW_TRACE");
    if(!path || !*path)return;
    static FILE *file=nullptr;static uint64_t last=~uint64_t(0);
    const auto tick=ntr::rt_frame();
    auto *p=static_cast<Player *>(data_0209f394[0]);
    if(!file) {file=std::fopen(path,"w");if(file)std::fputs("tick,level,player_state,step,work,timer,wait_timer,no_control,disabled,entry_mode,countdown,clear_active,clear_step,hud_step,pause_state,pause_step,message,answer,stars,previous_level\n",file);}
    if(file && p && p->aliveState==1 && tick!=last && tick%5==0) {
        last=tick;
        std::fprintf(file,"%llu,%d,%s,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d\n",
            static_cast<unsigned long long>(tick),int(data_0209f2f8),pc_player_state_name(p),
            unsigned(p->mStateStep),unsigned(p->mStateWork),unsigned(p->mStateTimer),unsigned(p->mStateWaitTimer),
            unsigned(p->mIsNoControl),unsigned(p->mIsControlDisabled),unsigned(data_0209f2fc),unsigned(data_0209f2bc),
            unsigned(data_0209f20c),unsigned(data_0209f2d4),unsigned(data_ov002_02111178),
            unsigned(data_0209f2c4),unsigned(data_0209f1ec),unsigned(data_0209d660),unsigned(data_0209d684),
            observed_power_star_total(),int(data_02092124));
        std::fflush(file);
    }
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
extern "C" int func_02029408(void);
extern "C" int pc_star_select_status(unsigned *,unsigned *,unsigned *,unsigned *,unsigned *);
// Input-only course-route QA: decimal level:x:z:radius waypoints in world
// units, with optional :1 to jump near the waypoint. Never writes player
// positions, save flags, actors or scene state.
#include "hal/warp_probe.h"
#include "hal/warp_replay.h"
#include "hal/koopa_replay.h"
#include "hal/lift_replay.h"
#include "hal/signpost_replay.h"
static bool replay_course_route_complete;
static bool replay_lava_bridge_reentered;
static unsigned replay_course_route_waypoint=0;
// Observes original contact/state and supplies normal input only. Never
// changes actor state, position, save flags or dialogue results.
extern "C" bool pc_toad_contact_recent(unsigned id,unsigned player);
static bool replay_toad_controls() {
    const char *mode=std::getenv("SM64DS_PC_TEST_TOAD_TALK");
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(!mode || (std::strcmp(mode,"1") && std::strcmp(mode,"cycle")) || !automation || std::strcmp(automation,"1"))return false;
    const bool cycle=!std::strcmp(mode,"cycle");
    int wanted_level=2,wanted_star=-1;
    if(const char *value=std::getenv("SM64DS_PC_TEST_TOAD_LEVEL")) {
        char *end=nullptr;const long parsed=std::strtol(value,&end,10);
        if(end==value || *end || (parsed!=2 && parsed!=4 && parsed!=5))return false;
        wanted_level=static_cast<int>(parsed);
    }
    if(const char *value=std::getenv("SM64DS_PC_TEST_TOAD_STAR")) {
        char *end=nullptr;const long parsed=std::strtol(value,&end,10);
        if(end==value || *end || parsed<0 || parsed>7)return false;
        wanted_star=static_cast<int>(parsed);
    }
    if(cycle && wanted_level!=2)return false;
    static unsigned target=0;static bool talking=false,done=false,pressed=false;
    static unsigned visit=1,completed=0;
    static bool awaiting_return=false,retreat=false;
    static uint64_t began=0;
    if(done)return false;
    if(data_0209f2f8!=wanted_level) {
        if(cycle && visit==1 && completed==2 && data_0209f2f8==1) {
            awaiting_return=true;target=0;talking=false;pressed=false;retreat=false;
        }
        return false;
    }
    if(cycle && visit==1 && completed==2) {
        if(!awaiting_return)return false;
        visit=2;completed=0;awaiting_return=false;
    }
    auto *p=static_cast<Player *>(data_0209f394[0]);
    if(!p || p->aliveState!=1)return false;
    daKinopio_c *toad=nullptr;double distance=1e100;
    for(auto *a=dActor_c::FindWithActorID(185,nullptr);a;a=dActor_c::FindWithActorID(185,a)) {
        if(a->aliveState!=1 || (target && a->uniqueID!=target))continue;
        if(wanted_star>=0 && static_cast<daKinopio_c *>(a)->mStarID!=wanted_star)continue;
        // The Hazy Maze room has overlapping walkways. A horizontal approach
        // must not seize input while the player is still on the floor above.
        if(std::abs(double(a->mPosY)-p->mPosY)>120*4096.0)continue;
        const auto d=std::hypot(double(a->mPosX)-p->mPosX,double(a->mPosZ)-p->mPosZ);
        if(d<distance){distance=d;toad=static_cast<daKinopio_c *>(a);}
    }
    if(!toad)return false;
    if(retreat) {if(distance>400*4096.0)retreat=false;return false;}
    if(!target && distance>120*4096.0)return false;
    const auto tick=ntr::rt_frame();
    auto event=[&](const char *name){std::fprintf(stderr,
        "[native Toad replay] %s tick=%llu id=%u player=%u speed=%d state=%s contact=%u visit=%u interaction=%u\n",
        name,static_cast<unsigned long long>(tick),toad->uniqueID,p->uniqueID,p->mHorzSpeed,
        pc_player_state_name(p),unsigned(pc_toad_contact_recent(toad->uniqueID,p->uniqueID)),visit,completed+1);};
    if(!target){target=toad->uniqueID;began=tick;event("approach");}
    if(toad->mState==1 && !talking){talking=true;event("talking");}
    const bool ready=!p->mIsNoControl && !p->mIsControlDisabled;
    if(talking && toad->mState==0 && ready) {
        event("complete");++completed;
        if(!cycle || visit==2)done=true;
        else {target=0;talking=false;pressed=false;retreat=completed<2;}
        return false;
    }
    if(tick-began>1800){done=true;event("timeout");return false;}
    uint16_t buttons=0;
    if(talking) {if(data_0209d660 && tick%45<3)buttons=1;}
    else if(ready && p->mHorzSpeed<0x8000 &&
            (!std::strcmp(pc_player_state_name(p),"St_Wait_Main") || !std::strcmp(pc_player_state_name(p),"St_Walk_Main")) &&
            pc_toad_contact_recent(target,p->uniqueID)) {
        buttons=1;if(!pressed){pressed=true;event("press-A");}
    }
    if(tick%120==0)event("observe");
    ntr::input_test_controls(true,buttons,0,0);return true;
}
// Opt-in isolated save-flow replay for the natural basement drainage event.
// This presses the same Start/D-pad/A/shoulder inputs a player uses; it never
// calls SaveCurrentFile directly and never edits the save flags.
static bool replay_drain_save_controls() {
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    const char *enabled=std::getenv("SM64DS_PC_TEST_DRAIN_SAVE");
    if(!automation || std::strcmp(automation,"1") || !enabled || std::strcmp(enabled,"1"))return false;
    static unsigned phase=0;
    static uint64_t pulse_until=0,next_action=0;
    const auto tick=ntr::rt_frame();
    auto *player=static_cast<Player *>(data_0209f394[0]);
    uint16_t buttons=0;
    auto event=[&](const char *name) {
        std::fprintf(stderr,"[native drain save] %s tick=%llu phase=%u level=%d pause=%u state=%u selection=%u page=%d row=%d status=%d flags2=%08x\n",
            name,static_cast<unsigned long long>(tick),phase,int(data_0209f2f8),unsigned(data_0209f2c4),
            unsigned(data_0209f1ec),unsigned(data_0209f2e0),pc_pause_settings_page(),pc_pause_settings_row(),
            pc_pause_save_status(),unsigned(reinterpret_cast<const SaveData *>(data_0209caa0)->flags2));
    };
    if(phase==0) {
        if(data_0209f2f8!=4 || !player || player->aliveState!=1 || player->mIsAirborne ||
           std::strcmp(pc_player_state_name(player),"St_Wait_Main") ||
           !(reinterpret_cast<const SaveData *>(data_0209caa0)->flags2&0x80000u))return false;
        bool first=false,second=false;
        for(auto *a=dActor_c::FindWithActorID(34,nullptr);a;a=dActor_c::FindWithActorID(34,a)) {
            auto *pillar=static_cast<daObjC0_Switch_c *>(a);
            if(a->aliveState!=1 || !pillar->mPressed)continue;
            if(a->mPosZ < -500*4096)first=true;else second=true;
        }
        if(!first || !second)return false;
        phase=1;pulse_until=tick+3;buttons=8;event("open-pause");
        ntr::input_test_controls(true,buttons,0,0);return true;
    }
    if(phase==1) {
        if(data_0209f2c4==1 && data_0209f1ec==1) {phase=2;next_action=tick+30;event("pause-ready");}
    } else if(phase==2) {
        if(data_0209f2c4==1 && data_0209f1ec==1 && data_0209f2e0>=2) {phase=4;next_action=tick+30;event("options-selected");}
        else if(tick>=next_action) {phase=3;pulse_until=tick+3;buttons=0x80;event("select-options-down");}
    } else if(phase==3) {
        if(tick>=pulse_until) {phase=2;next_action=tick+30;}
    } else if(phase==4) {
        if(tick>=next_action) {phase=5;pulse_until=tick+3;buttons=1;event("open-pc-settings");}
    } else if(phase==5) {
        if(data_0209f1ec==10 && pc_pause_settings_page()==1) {phase=6;next_action=tick+30;event("settings-root-ready");}
    } else if(phase==6) {
        if(tick>=next_action) {phase=7;pulse_until=tick+3;buttons=1;event("enter-display-settings");}
    } else if(phase==7) {
        if(pc_pause_settings_page()==2) {phase=8;next_action=tick+30;event("display-page-ready");}
    } else if(phase==8) {
        if(pc_pause_settings_page()==7) {phase=10;next_action=tick+30;event("save-page-ready");}
        else if(tick>=next_action) {phase=9;pulse_until=tick+3;buttons=0x100;event("cycle-settings-page");}
    } else if(phase==9) {
        if(tick>=pulse_until) {phase=8;next_action=tick+30;}
    } else if(phase==10) {
        if(tick>=next_action) {phase=11;pulse_until=tick+3;buttons=1;event("confirm-save-progress");}
    } else if(phase==11) {
        if(pc_pause_save_status()!=0) {phase=12;event(pc_pause_save_status()>0?"save-success":"save-failed");}
    }
    if(phase==1 && tick>=pulse_until) {phase=2;next_action=tick+30;}
    if(phase==12 && tick%60==0)event("saved-waiting");
    if(tick<pulse_until) {
        if(phase==1)buttons=8;
        else if(phase==3)buttons=0x80;
        else if(phase==5 || phase==7 || phase==11)buttons=1;
        else if(phase==9)buttons=0x100;
    }
    ntr::input_test_controls(true,buttons,0,0);return true;
}
// Select the original post-course Save & Continue choice using ordinary pad
// confirmation. Player control returns only after this screen finishes.
static bool replay_star_save_controls() {
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    const char *enabled=std::getenv("SM64DS_PC_TEST_STAR_SAVE");
    if(!automation || std::strcmp(automation,"1") || !enabled || std::strcmp(enabled,"1"))return false;
    static bool started=false,saving=false,done=false;
    const auto tick=ntr::rt_frame();
    auto event=[&](const char *name) {
        std::fprintf(stderr,"[native star save replay] %s tick=%llu level=%d clear_active=%u clear_step=%u selected=%u stars=%u\n",
            name,static_cast<unsigned long long>(tick),int(data_0209f2f8),unsigned(data_0209f20c),
            unsigned(data_0209f2d4),unsigned(data_0209f2e0),observed_power_star_total());
    };
    if(!started) {
        if(data_0209f2f8!=2 || !data_0209f20c)return false;
        started=true;event("original-clear-screen");
    }
    uint16_t buttons=0;
    if(!done && data_0209f20c && data_0209f2d4==3 && tick%45<3) {
        buttons=1;event("confirm-original-save-continue");
    }
    if(!saving && data_0209f20c && data_0209f2d4==5) {
        saving=true;event("original-save-running");
    }
    auto *p=static_cast<Player *>(data_0209f394[0]);
    // A newly reached Star threshold can show a castle message after saving.
    // Dismiss it with ordinary confirmation before checking restored control.
    if(!done && saving && !data_0209f20c && p &&
       !std::strcmp(pc_player_state_name(p),"St_Talk_Main") && tick%45<3)
        buttons=1;
    if(!done && saving && !data_0209f20c && p && p->aliveState==1 &&
       !p->mIsNoControl && !p->mIsControlDisabled) {
        done=true;event("original-save-complete-control-restored");
    }
    ntr::input_test_controls(true,buttons,0,0);return true;
}
static bool replay_pillar_controls() {
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(!automation || std::strcmp(automation,"1") || data_0209d660)return false;
    auto *player=static_cast<Player *>(data_0209f394[0]);
    if(!player || player->aliveState!=1 || player->mIsNoControl || player->mIsControlDisabled)return false;
    const int level=data_0209f2f8;const auto tick=ntr::rt_frame();
    const char *state=pc_player_state_name(player);
        const char *pillar_test=std::getenv("SM64DS_PC_TEST_DRAIN_PILLARS");
        if(level==4 && pillar_test && !std::strcmp(pillar_test,"1")) {
            static unsigned target=0;static uint64_t began=0,jump_until=0;
            static bool jumped=false;daObjC0_Switch_c *pillar=nullptr;
            for(auto *a=dActor_c::FindWithActorID(34,nullptr);a;a=dActor_c::FindWithActorID(34,a)) {
                auto *candidate=static_cast<daObjC0_Switch_c *>(a);
                if(a->aliveState!=1)continue;
                if(target) {if(a->uniqueID==target)pillar=candidate;continue;}
                if(candidate->mPressed || player->mIsAirborne ||
                   (std::strcmp(state,"St_Wait_Main") && std::strcmp(state,"St_Walk_Main")))continue;
                if(std::abs(double(a->mPosY)-player->mPosY)<80*4096.0 &&
                   std::hypot(double(a->mPosX)-player->mPosX,double(a->mPosZ)-player->mPosZ)<60*4096.0)
                    pillar=candidate;
            }
            if(pillar && pillar->mPressed) {
                std::fprintf(stderr,"[native pillar replay] pressed tick=%llu id=%u\n",static_cast<unsigned long long>(tick),target);
                target=0;pillar=nullptr;jumped=false;
            }
            if(pillar) {
                if(!target){target=pillar->uniqueID;began=tick;jumped=false;std::fprintf(stderr,"[native pillar replay] settle tick=%llu id=%u\n",static_cast<unsigned long long>(tick),target);}
                uint16_t keys=0;
                if(!jumped && tick>=began+100 && !player->mIsAirborne) {keys=2;jumped=true;jump_until=tick+8;}
                else if(jumped && player->mIsAirborne && !std::strcmp(state,"St_Jump_Main"))keys=0x100;
                else if(jumped && !player->mIsAirborne && tick<jump_until)keys=2;
                if(keys)std::fprintf(stderr,"[native pillar replay] tick=%llu id=%u state=%s buttons=%x\n",static_cast<unsigned long long>(tick),target,state,unsigned(keys));
                ntr::input_test_controls(true,keys,0,0);return true;
            }
        }
    return false;
}
static bool replay_course_route(const char *route_override=nullptr) {
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    const char *route=route_override?route_override:std::getenv("SM64DS_PC_TEST_COURSE_ROUTE");
    if(!route_override && replay_lava_bridge_reentered) {
        if(const char *reentry_route=std::getenv("SM64DS_PC_TEST_LAVA_REENTRY_ROUTE"))route=reentry_route;
    }
    if(!automation || std::strcmp(automation,"1") || !route || !*route)return false;
    if(!route_override && pc_replay_sign_pound(static_cast<Player *>(data_0209f394[0])))return true;
    auto &waypoint=replay_course_route_waypoint;
    const char *segment=route;
    for(unsigned i=0;i<waypoint && segment;++i) {
        const char *next=std::strchr(segment,',');segment=next?next+1:nullptr;
    }
    if(!segment){replay_course_route_complete=true;return false;}
    int level=0,x=0,y=0,z=0,radius=0,consumed=0,jump=0,jump_consumed=0;
    bool has_y=false;
    // Height-aware route points are level:x:y:z:radius:jump. Keep the old
    // level:x:z:radius[:jump] form for every existing route. Checking the
    // six-field form first avoids treating its coordinates as an old route.
    if(std::sscanf(segment,"%d:%d:%d:%d:%d:%d%n",
                   &level,&x,&y,&z,&radius,&jump,&consumed)==6) {
        has_y=true;
        if(consumed<=0 || (segment[consumed] && segment[consumed]!=',') ||
           (jump<0 || jump>3))return false;
    } else {
        consumed=0;jump=0;
        if(std::sscanf(segment,"%d:%d:%d:%d%n",&level,&x,&z,&radius,&consumed)!=4 ||
           consumed<=0 || (segment[consumed] && segment[consumed]!=',' && segment[consumed]!=':'))return false;
        if(segment[consumed]==':') {
            if(std::sscanf(segment+consumed,":%d%n",&jump,&jump_consumed)!=1 ||
               (jump<1 || jump>3) || (segment[consumed+jump_consumed] && segment[consumed+jump_consumed]!=','))return false;
        }
    }
    if(level<0 || level>=52 || radius<1 || radius>1000 ||
       x< -32768 || x>32767 || z< -32768 || z>32767 ||
       (has_y && (y< -32768 || y>32767)))return false;
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
    const char *slide_route=std::getenv("SM64DS_PC_TEST_SLIDE_ROUTE");
    const bool slide_course=level==29 && slide_route && !std::strcmp(slide_route,"1");
    // Castle waypoints need precise stops, but the verified slide approach
    // needs continuous input into its ledge jump. Settling there carries the
    // player beneath the box before takeoff and causes an underside collision.
    const bool precise=route_override || (settle && !std::strcmp(settle,"1") && !slide_course);
    if(precise && ntr::rt_frame()<settle_until) {
        ntr::input_test_controls(true,0,0,0);return true;
    }
    uint16_t buttons=0;int16_t sx=0,sy=0;
    if(data_0209d660) {
        if(ntr::rt_frame()%45<3)buttons=1;
    } else if(!player->mIsNoControl && !player->mIsControlDisabled) {
        const double dx=x*4096.0-player->mPosX,dz=z*4096.0-player->mPosZ;
        const double dy=has_y ? y*4096.0-player->mPosY : 0.0;
        const double horizontal=std::hypot(dx,dz);
        const double vertical_tolerance=(std::max)(120,radius)*4096.0;
        const auto tick=ntr::rt_frame();
        const char *state=pc_player_state_name(player);
        if(slide_route && !std::strcmp(slide_route,"1") &&
           !std::strcmp(state,"St_ButtSlide_Main") && !has_y && !jump) {
            // A single QA route may begin in the castle and continue into
            // Secret Slide. Once the player enters ButtSlide, project only
            // onto the remaining same-course suffix. Keep the original
            // waypoint base so lookahead retains the preceding slide point
            // on every frame; this changes test input only, never physics.
            static bool slide_route_started=false;
            static unsigned slide_route_base=0;
            if(!slide_route_started) {
                slide_route_base=waypoint;
                slide_route_started=true;
            }
            replay_path::Point points[256];unsigned count=0,route_index=0;
            bool valid=true;
            for(const char *s=route;s && *s;) {
                if(route_index>=slide_route_base) {
                    int l,px,pz,r,n=0;
                    if(count==256 || std::sscanf(s,"%d:%d:%d:%d%n",&l,&px,&pz,&r,&n)!=4 ||
                       l!=level || r<1 || r>1000 || (s[n] && s[n]!=',' && s[n]!=':')) {valid=false;break;}
                    if(s[n]==':') {
                        int action=0,used=0;
                        if(std::sscanf(s+n,":%d%n",&action,&used)!=1 || action<1 || action>3 ||
                           (s[n+used] && s[n+used]!=',')) {valid=false;break;}
                    }
                    points[count++]={double(px),double(pz),double(r)};
                }
                const char *next=std::strchr(s,',');
                s=next?next+1:nullptr;
                ++route_index;
            }
            const unsigned local_waypoint=waypoint>=slide_route_base?waypoint-slide_route_base:0;
            if(valid && count && local_waypoint<count) {
                const double speed=std::abs(double(player->mHorzSpeed))/4096.0;
                const double ahead=(std::max)(300.0,(std::min)(900.0,speed*12));
                const auto aim=replay_path::follow(points,count,local_waypoint,
                    player->mPosX/4096.0,player->mPosZ/4096.0,ahead,600);
                const unsigned absolute_waypoint=slide_route_base+unsigned(aim.waypoint);
                if(absolute_waypoint!=waypoint || tick%30==0)
                    std::fprintf(stderr,"[native slide route] tick=%llu waypoint=%u->%u aim=%.1f,%.1f speed=%.1f\n",
                        static_cast<unsigned long long>(tick),waypoint,absolute_waypoint,aim.x,aim.z,speed);
                waypoint=absolute_waypoint;
                replay_course_route_complete=aim.complete;
                if(!aim.complete) {
                    const double angle=std::atan2(aim.x-player->mPosX/4096.0,
                        aim.z-player->mPosZ/4096.0)-GetAngleToCamera(0)*6.283185307179586/65536.0;
                    sx=static_cast<int16_t>(std::sin(angle)*32767);
                    sy=static_cast<int16_t>(-std::cos(angle)*32767);
                }
                ntr::input_test_controls(true,0,sx,sy);return true;
            }
        }
        // Isolated block-contact replay: use ordinary jump plus midair R to
        // test the block's ground-pound callback. This only injects input
        // under automation; it never edits actor, player, collision or save
        // state.
        // Original swimming steers heading/pitch directly. Opt-in QA routes
        // use ordinary D-pad/stroke buttons; never write pose or player state.
        const char *swim_route=std::getenv("SM64DS_PC_TEST_SWIM_ROUTE");
        if(has_y && swim_route && !std::strcmp(swim_route,"1") &&
           !std::strcmp(state,"St_Swim_Main")) {
            const short desired_yaw=static_cast<short>(std::atan2(dx,dz)*65536.0/6.283185307179586);
            const short delta=static_cast<short>(desired_yaw-player->mAngleY);
            const int steer=int(delta)-int(player->mAngleYSpeed)*6;
            uint16_t swim_buttons=steer>768?0x20:steer< -768?0x10:0;
            const double pitch=std::abs(dy)<40*4096.0?0.0:
                std::atan2(-dy,(std::max)(horizontal,100*4096.0))*65536.0/6.283185307179586;
            const double desired_pitch=pitch/(pitch<0?0.6:1.25);
            const double pitch_error=desired_pitch-player->mPrevAngleX;
            if(pitch_error>1536)swim_buttons|=0x40;
            if(pitch_error< -1536)swim_buttons|=0x80;
            if(std::abs(int(delta))<0x2000 && tick%24<12)swim_buttons|=2;
            if(horizontal<=radius*4096.0 && std::abs(dy)<=120*4096.0) {
                std::fprintf(stderr,"[native swim route] reached waypoint=%u tick=%llu\n",waypoint,static_cast<unsigned long long>(tick));
                ++waypoint;
            }
            if(tick%120==0)std::fprintf(stderr,"[native swim route] tick=%llu target=%d,%d,%d yaw_delta=%d pitch=%d buttons=%x\n",
                static_cast<unsigned long long>(tick),x,y,z,int(delta),int(player->mPrevAngleX),unsigned(swim_buttons));
            ntr::input_test_controls(true,swim_buttons,0,0);return true;
        }
        const char *block_replay=std::getenv("SM64DS_PC_TEST_BLOCK_INTERACTION");
        int block_level=6,block_actor=15;
        if(const char *target=std::getenv("SM64DS_PC_TEST_BLOCK_TARGET")) {
            char extra=0;
            if(std::sscanf(target,"%d:%d%c",&block_level,&block_actor,&extra)!=2 ||
               block_level<0 || block_level>=52 ||
               (block_actor!=15 && block_actor!=16 && block_actor!=17 && block_actor!=19 &&
                block_actor!=21 && block_actor!=46))
                block_level=-1;
        }
        if(block_replay && !std::strcmp(block_replay,"1") && level==block_level) {
            dActor_c *block=nullptr;double block_distance=1e100;
            for(auto *candidate=dActor_c::FindWithActorID(block_actor,nullptr);candidate;
                candidate=dActor_c::FindWithActorID(block_actor,candidate)) {
                const double bx=double(candidate->mPosX)-player->mPosX;
                const double by=double(candidate->mPosY)-player->mPosY;
                const double bz=double(candidate->mPosZ)-player->mPosZ;
                if(candidate->aliveState!=1 || std::abs(by)>700*4096.0)continue;
                const double distance=std::hypot(bx,bz);
                if(distance<block_distance){block=candidate;block_distance=distance;}
            }
            static bool block_contact_seen=false;
            static int reward_x=0,reward_z=0;
            if(block && block_distance<260*4096.0) {
                block_contact_seen=true;reward_x=block->mPosX;reward_z=block->mPosZ;
            }
            if(block_contact_seen && (!block || block_distance>=260*4096.0)) {
                dActor_c *coin=nullptr;double nearest=500*4096.0;
                for(auto *a=dActor_c::FindWithActorID(288,nullptr);a;
                    a=dActor_c::FindWithActorID(288,a)) {
                    if(a->aliveState!=1 || std::hypot(double(a->mPosX)-reward_x,
                       double(a->mPosZ)-reward_z)>600*4096.0)continue;
                    double d=std::hypot(double(a->mPosX)-player->mPosX,double(a->mPosZ)-player->mPosZ);
                    if(d<nearest){nearest=d;coin=a;}
                }
                sx=sy=0;
                if(coin) {
                    double angle=std::atan2(double(coin->mPosX)-player->mPosX,
                        double(coin->mPosZ)-player->mPosZ)-GetAngleToCamera(0)*6.283185307179586/65536.0;
                    sx=static_cast<int16_t>(std::sin(angle)*18000);
                    sy=static_cast<int16_t>(-std::cos(angle)*18000);
                }
                ntr::input_test_controls(true,0,sx,sy);return true;
            }
            if(block && block_distance<260*4096.0 &&
               (!std::strcmp(state,"St_LedgeHang_Main") ||
                !std::strcmp(state,"St_Wait_Main") ||
                !std::strcmp(state,"St_Walk_Main") ||
                !std::strcmp(state,"St_OnWall_Main") ||
                !std::strcmp(state,"St_Jump_Main") ||
                !std::strcmp(state,"St_GroundPound_Main"))) {
                const double bx=double(block->mPosX)-player->mPosX;
                const double bz=double(block->mPosZ)-player->mPosZ;
                const double angle=std::atan2(bx,bz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
                sx=static_cast<int16_t>(std::sin(angle)*22000);
                sy=static_cast<int16_t>(-std::cos(angle)*22000);
                static uint64_t r_until=0,climb_until=0,next_climb=0,next_pound=0;
                static bool climb_jump_pending=false,pound_jump_pending=false;
                static bool climbed_onto_block=false;
                static uint64_t block_top_since=0;
                const bool grounded_state=!std::strcmp(state,"St_Wait_Main") ||
                    !std::strcmp(state,"St_Walk_Main") || !std::strcmp(state,"St_OnWall_Main");
                const bool above_block=player->mPosY-block->mPosY>120*4096;
                if(!player->mIsAirborne && grounded_state && above_block && block_distance<90*4096.0) {
                    climbed_onto_block=true;
                    if(!block_top_since)block_top_since=tick;
                }
                if(climbed_onto_block) { sx=0;sy=0; }
                const char *pound_from_floor=std::getenv("SM64DS_PC_TEST_BLOCK_GROUND_POUND");
                const bool direct_ground_pound=pound_from_floor && !std::strcmp(pound_from_floor,"1");
                static bool direct_pound_pending=false;
                static uint64_t direct_next_jump=0,direct_r_until=0;
                if(direct_ground_pound && !direct_pound_pending && !player->mIsAirborne && grounded_state &&
                   block_distance<180*4096.0 && tick>=direct_next_jump) {
                    buttons=2; // Ordinary jump, followed by the DS R ground-pound input in air.
                    direct_pound_pending=true;
                    direct_next_jump=tick+180;
                } else if(direct_ground_pound && player->mIsAirborne && direct_pound_pending &&
                          !std::strcmp(state,"St_Jump_Main")) {
                    buttons=0x100;
                    direct_pound_pending=false;
                    direct_r_until=tick+3;
                } else if(direct_ground_pound && tick<direct_r_until) {
                    buttons=0x100;
                } else if(!direct_ground_pound && pound_jump_pending && !player->mIsAirborne && tick>=next_pound)
                    pound_jump_pending=false;
                if(!direct_ground_pound && !climbed_onto_block && !player->mIsAirborne && grounded_state &&
                   block_distance<260*4096.0 && tick>=next_climb) {
                    buttons=2; // DS B jump; the PC A binding maps here.
                    climb_jump_pending=true;
                    climb_until=tick+5;
                    next_climb=tick+90;
                } else if(!direct_ground_pound && !climbed_onto_block && !player->mIsAirborne &&
                          climb_jump_pending && tick<climb_until) {
                    buttons=2;
                } else if(!direct_ground_pound && !climbed_onto_block && climb_jump_pending && tick>=climb_until) {
                    climb_jump_pending=false;
                } else if(!direct_ground_pound && climbed_onto_block && !player->mIsAirborne && grounded_state &&
                          !pound_jump_pending && tick>=next_pound && tick>=block_top_since+100) {
                    buttons=2; // Jump from the block before pressing R in midair.
                    pound_jump_pending=true;
                    climb_until=tick+8;
                    next_pound=tick+180;
                } else if(!direct_ground_pound && player->mIsAirborne && pound_jump_pending &&
                          !std::strcmp(state,"St_Jump_Main")) {
                    buttons=0x100; // DS R while airborne initiates a ground pound.
                    r_until=tick+3;
                    pound_jump_pending=false;
                } else if(!direct_ground_pound && pound_jump_pending && !player->mIsAirborne && tick<climb_until) {
                    buttons=2; // Hold through input polling, as during the ledge approach.
                } else if(!direct_ground_pound && tick<r_until) {
                    buttons=0x100;
                }
                static const char *last_state=nullptr;
                if(!last_state || std::strcmp(last_state,state) || tick%120==0 || buttons) {
                    std::fprintf(stderr,"[native block replay] tick=%llu id=%u distance=%.1f state=%s stick=%d,%d buttons=%x\n",
                        static_cast<unsigned long long>(tick),block->uniqueID,block_distance/4096.0,
                        state,int(sx),int(sy),unsigned(buttons));
                    last_state=state;
                }
                ntr::input_test_controls(true,buttons,sx,sy);return true;
            }
        }
        const bool horizontal_reached=horizontal<=radius*4096.0;
        const bool height_reached=!has_y ||
            (!player->mIsAirborne && std::abs(dy)<=vertical_tolerance);
        // Optional QA route mode 3: ordinary running crouch/jump inputs for
        // a wide gap. Never change player position, velocity or actor state.
        if(jump==3) {
            static uint64_t launch=0;
            if(horizontal<1750*4096.0 && !player->mIsAirborne &&
               !std::strcmp(state,"St_Walk_Main") && tick>=next_jump) {
                launch=tick;next_jump=tick+100;
            }
            if(launch && tick-launch<18)buttons|=(tick-launch<6 ? 0x100:0x102);
        }
        // Route mode 2 takes off with a neutral stick. Retail Wait prioritizes
        // entering Walk over the jump edge when both arrive together. Keep
        // the input neutral through takeoff; never alter the player state.
        // A height-aware mode2 point is a traversable step: stop retrying
        // after landing at the point. Legacy collection points have no Y and
        // retain their repeated neutral jumping until the collector finishes.
        if(jump==2 && horizontal<320*4096.0 &&
           !(has_y && horizontal_reached && height_reached)) {
            if(tick<jump_until && (!pc_sign_pound_replay_enabled() || !player->mIsAirborne)) {
                ntr::input_test_controls(true,2,0,0);return true;
            }
            if(!player->mIsAirborne && tick>=next_jump &&
               (!std::strcmp(state,"St_Wait_Main") || !std::strcmp(state,"St_Walk_Main") ||
                !std::strcmp(state,"St_OnWall_Main"))) {
                if(!std::strcmp(state,"St_Wait_Main")) {
                    jump_until=tick+12;next_jump=tick+75;
                    ntr::input_test_controls(true,2,0,0);
                } else ntr::input_test_controls(true,0,0,0);
                return true;
            }
        }
        // Optional input-only tree acceptance: keep climbing after the route
        // reaches the trunk, rest at its top, then jump off. No actor/state
        // placement or save edits are involved.
        const char *tree_test=std::getenv("SM64DS_PC_TEST_TREE_CLIMB");
        if(tree_test && !std::strcmp(tree_test,"1")) {
            static uint64_t attached_at=0;
            static const char *last_state=nullptr;
            if(!attached_at && player->mIsAirborne && !std::strcmp(state,"St_Climb_Main"))attached_at=tick;
            if(attached_at) {
                const auto elapsed=tick-attached_at;
                if(!last_state || std::strcmp(last_state,state)) {
                    std::fprintf(stderr,"[native tree replay] tick=%llu elapsed=%llu state=%s y=%d attachment=%p\n",
                        static_cast<unsigned long long>(tick),static_cast<unsigned long long>(elapsed),
                        state,player->mPosY,*reinterpret_cast<void **>(reinterpret_cast<char *>(player)+0x37c));
                    last_state=state;
                }
                ntr::input_test_controls(true,elapsed>=650 && elapsed<660 ? 2:0,
                    0,(elapsed<500 || (elapsed>=540 && elapsed<600)) ? 32767:0);
                return true;
            }
        }
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
        const double jump_range=tree_test && !std::strcmp(tree_test,"1") ? 120.0:320.0;
        if(jump==1 && horizontal<jump_range*4096.0 && tick>=next_jump &&
           (!std::strcmp(state,"St_Wait_Main") || !std::strcmp(state,"St_Walk_Main") ||
            (!player->mIsAirborne && !std::strcmp(state,"St_OnWall_Main")))) {
            jump_until=tick+12;next_jump=tick+75;
        }
        if(tick<jump_until)buttons|=2;
        // Height-aware route points represent walkable floor positions, not
        // transient jump arcs. Only advance after the player has landed at
        // the requested height; otherwise an overhead stair edge can make
        // the route skip forward while the player falls back to the lower floor.
        if(horizontal_reached && height_reached) {
            if(has_y)
                std::fprintf(stderr,"[native course route] reached waypoint=%u level=%d target=%d,%d,%d tick=%llu\n",
                    waypoint,level,x,y,z,static_cast<unsigned long long>(ntr::rt_frame()));
            else
                std::fprintf(stderr,"[native course route] reached waypoint=%u level=%d target=%d,%d tick=%llu\n",
                    waypoint,level,x,z,static_cast<unsigned long long>(ntr::rt_frame()));
            ++waypoint;
            if(precise)settle_until=tick+20;
        } else if(horizontal_reached) {
            // Stay under the height target while the normal jump input retries.
            // Horizontal-only routes and their original controls are unchanged.
            sx=0;sy=0;
        } else {
            const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
            double magnitude=precise?horizontal/(180*4096.0)*22000:22000.0;
            // Keep low-speed travel above the controller's radial deadzone.
            if(magnitude<12000)magnitude=12000;
            if(magnitude>22000)magnitude=22000;
            if(jump==3)magnitude=32767;
            sx=static_cast<int16_t>(std::sin(angle)*magnitude);
            sy=static_cast<int16_t>(-std::cos(angle)*magnitude);
        }
    }
    ntr::input_test_controls(true,buttons,sx,sy);
    return true;
}

// Input-only lava-bridge lifecycle QA: use the original Exit Course menu,
// basement painting and mission selector, then repeat the same walking route.
static bool replay_lava_bridge_lifecycle() {
    const char *mode=std::getenv("SM64DS_PC_TEST_LAVA_BRIDGE_LIFECYCLE");
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(!mode || std::strcmp(mode,"1") || !automation || std::strcmp(automation,"1"))return false;
    int course_level=14;
    if(const char *configured=std::getenv("SM64DS_PC_TEST_LAVA_COURSE_LEVEL")) {
        const int requested=std::atoi(configured);
        if(requested>=0 && requested<52)course_level=requested;
    }
    static int phase=0;static uint64_t began=0,jumpUntil=0,nextJump=0;
    const auto tick=ntr::rt_frame();
    auto event=[&](const char *name){std::fprintf(stderr,"[native lava bridge lifecycle] %s tick=%llu level=%d phase=%d\n",name,static_cast<unsigned long long>(tick),int(data_0209f2f8),phase);};
    if(phase>=4)return false;
    if(!phase) {if(!replay_course_route_complete)return false;phase=1;began=tick;event("exit-request");}
    if(tick-began>12000){phase=4;event("timeout");return false;}
    auto *p=static_cast<Player *>(data_0209f394[0]);
    const bool ready=p && p->aliveState==1 && !p->mIsNoControl && !p->mIsControlDisabled;
    uint16_t buttons=0;int16_t sx=0,sy=0;
    auto controls=[&](){ntr::input_test_controls(true,buttons,sx,sy);return true;};
    if(phase==1) {
        if((data_0209f2f8==2 || data_0209f2f8==4) && ready) {
            phase=2;event("castle-return");
            replay_course_route_waypoint=0;replay_course_route_complete=false;
        }
        else {
            if(!data_0209f2c4){if(ready && tick%45<3)buttons=8;}
            else if(data_0209f248==3 && tick%45<3)buttons=data_0209f2e0==3?1:0x80;
            return controls();
        }
    }
    if(phase==2) {
        unsigned selected=0,selectionPhase=0,selectionMode=0,confirmed=0,selectionReady=0;
        if(pc_star_select_status(&selected,&selectionPhase,&selectionMode,&confirmed,&selectionReady)) {
            if(selectionReady && !confirmed && selectionMode==0 && tick%20<3)
                buttons=selected==0 && selectionPhase==2?1:0x20;
            return controls();
        }
        if(data_0209f2f8==course_level && ready) {
            phase=3;event("original-course-reentry");
            replay_lava_bridge_reentered=true;
            replay_course_route_waypoint=0;
            const char *route=std::getenv("SM64DS_PC_TEST_LAVA_REENTRY_ROUTE");
            if(!route || !*route)route=std::getenv("SM64DS_PC_TEST_COURSE_ROUTE");
            if(route) {
                // On re-entry, resume the ordinary course route at its first
                // waypoint for this course instead of replaying the castle
                // approach from the beginning.
                unsigned index=0;const char *segment=route;
                while(segment && *segment) {
                    int route_level=-1;
                    if(std::sscanf(segment,"%d:",&route_level)==1 && route_level==course_level) {
                        replay_course_route_waypoint=index;break;
                    }
                    const char *next=std::strchr(segment,',');segment=next?next+1:nullptr;++index;
                }
            }
            replay_course_route_complete=false;
            return false;
        }
        // An entry through the development picker returns to the lobby. Feed
        // the existing route controller a private, geometry-reviewed castle
        // path; it must reach the actual basement painting through real doors.
        if(const char *castle_route=std::getenv("SM64DS_PC_TEST_LAVA_CASTLE_ROUTE")) {
            if(!replay_course_route_complete) {
                if(data_0209f2c4) {if(tick%45<3)buttons=8;return controls();}
                if(replay_course_route(castle_route))return true;
                if(!replay_course_route_complete)return controls();
                event("painting-approach");
            }
        }
        if(course_level!=14 || data_0209f2f8!=4 || !ready)return controls();
        if(data_0209d660){if(tick%45<3)buttons=1;return controls();}
        // Original level4 exit/painting at(-1350,-1100,-3850), facing south.
        const double dx=-1350*4096.0-p->mPosX,dz=-4150*4096.0-p->mPosZ;
        const char *state=pc_player_state_name(p);
        // Measure takeoff from the painting plane, not the steering target
        // behind it. An early jump lands at the wall before crossing the exit.
        // Neutralize Walk/OnWall first so Wait cannot consume the jump edge.
        const double paintingDistance=std::hypot(dx,-3850*4096.0-p->mPosZ);
        if(tick<jumpUntil){buttons=2;return controls();}
        if(paintingDistance<220*4096.0 && tick>=nextJump && !p->mIsAirborne &&
           (!std::strcmp(state,"St_Wait_Main") || !std::strcmp(state,"St_Walk_Main") ||
            !std::strcmp(state,"St_OnWall_Main"))) {
            if(!std::strcmp(state,"St_Wait_Main")) {
                jumpUntil=tick+12;nextJump=tick+75;buttons=2;event("painting-neutral-takeoff");
            }
            return controls();
        }
        const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
        sx=static_cast<int16_t>(std::sin(angle)*22000);sy=static_cast<int16_t>(-std::cos(angle)*22000);
        return controls();
    }
    if(phase==3 && replay_course_route_complete){phase=4;event("done");}
    return false;
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
    // The original intro can outlast the historical tick6500 handoff. Let
    // the input-only route actually leave the locked door before taking over;
    // its castle-to-maze navigation grid does not cover that doorway.
    const char *route=std::getenv("SM64DS_PC_TEST_COURSE_ROUTE");
    if(route && *route && !replay_course_route_complete)return false;
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
    if(!mode || (std::strcmp(mode,"1") && std::strcmp(mode,"reentry")) || !automation || std::strcmp(automation,"1"))return false;
    const bool reentry=!std::strcmp(mode,"reentry");
    const char *route=std::getenv("SM64DS_PC_TEST_WATERBOMB_ROUTE");
    int observe_x=0,observe_z=0,retreat_x=0,retreat_z=0;
    if(!route || std::sscanf(route,"%d:%d:%d:%d",&observe_x,&observe_z,&retreat_x,&retreat_z)!=4)return false;
    if(observe_x< -32768 || observe_x>32767 || observe_z< -32768 || observe_z>32767 ||
       retreat_x< -32768 || retreat_x>32767 || retreat_z< -32768 || retreat_z>32767)return false;
    auto *p=static_cast<Player *>(data_0209f394[0]);
    enum {APPROACH,OBSERVE,RETREAT,DRAIN,COMPLETE,EXIT,CASTLE,REENTERED};
    static int phase=APPROACH,initial_health=0,retreat_leg=0;
    static bool fragments_seen=false;
    static unsigned waypoint=0;
    static uint64_t jump_until=0,next_jump=0;
    const auto tick=ntr::rt_frame();
    uint16_t buttons=0;int16_t sx=0,sy=0;
    auto controls=[&]() {ntr::input_test_controls(true,buttons,sx,sy);return true;};
    auto ready=[&]() {return p && p->aliveState==1 && !p->mIsNoControl && !p->mIsControlDisabled;};
    auto event=[&](const char *name) {
        std::fprintf(stderr,"[native waterbomb replay] event=%s tick=%llu health=%d pos=%d,%d,%d\n",
            name,static_cast<unsigned long long>(tick),p?p->GetHealth():-1,p?p->mPosX:0,p?p->mPosY:0,p?p->mPosZ:0);
    };
    if(phase==EXIT) {
        if(data_0209f2f8==2 && p && p->aliveState==1) {phase=CASTLE;event("castle-return");}
        else {
            if(!data_0209f2c4) {
                // Original pause falls back to the full-screen map during Hurt.
                // Wait for its own player eligibility check before opening it.
                if(ready() && func_02029408() && tick%45<3) {buttons=8;event("pause-request");}
            }
            else if(data_0209f248==11 && tick%45<3)buttons=2;
            else if(data_0209f248==3 && tick%45<3)buttons=data_0209f2e0==3?1:0x80;
            return controls();
        }
    }
    if(phase==CASTLE) {
        unsigned selection=0,selection_phase=0,selection_mode=0,confirm=0,selection_ready=0;
        if(pc_star_select_status(&selection,&selection_phase,&selection_mode,&confirm,&selection_ready)) {
            if(selection_ready && !confirm && selection_mode==0 && tick%20<3) {
                buttons=selection==0 && selection_phase==2?1:0x20;
                if(tick%20==0)std::fprintf(stderr,"[native mission replay] tick=%llu selected=%u phase=%u buttons=%x\n",
                    static_cast<unsigned long long>(tick),selection,selection_phase,unsigned(buttons));
            }
            return controls();
        }
        if(data_0209f2f8==6 && p && p->aliveState==1) {phase=REENTERED;event("course-reentry");}
        else {
            if(!p || p->aliveState!=1)return controls();
            if(data_0209d660) {if(tick%45<3)buttons=1;return controls();}
            if(!ready())return controls();
            const int route[][3]={{-1000,1400,60},{-2100,600,50},{-2300,500,40},
                {-2750,50,40},{-2800,-700,50},{-3400,-700,50},{-3700,-450,40},{-5200,-450,40}};
            const unsigned last=sizeof(route)/sizeof(route[0])-1;
            const unsigned index=waypoint>last?last:waypoint;
            const double dx=route[index][0]*4096.0-p->mPosX,dz=route[index][1]*4096.0-p->mPosZ;
            if(index==last && std::hypot(dx,dz)<320*4096.0 && tick>=next_jump &&
               (!std::strcmp(pc_player_state_name(p),"St_Wait_Main") || !std::strcmp(pc_player_state_name(p),"St_Walk_Main"))) {
                jump_until=tick+12;next_jump=tick+75;
            }
            if(tick<jump_until)buttons=2;
            if(std::hypot(dx,dz)<=route[index][2]*4096.0) {if(waypoint<=last)++waypoint;}
            else {
                const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
                sx=static_cast<int16_t>(std::sin(angle)*22000);sy=static_cast<int16_t>(-std::cos(angle)*22000);
            }
            return controls();
        }
    }
    if(data_0209f2f8!=6 || !p || p->aliveState!=1)return false;
    const char *state=pc_player_state_name(p);
    const bool free=ready();
    if(phase==REENTERED) {
        if(free && !p->mIsAirborne && !std::strcmp(state,"St_Wait_Main")) {phase=COMPLETE;event("reentry-complete");}
        return controls();
    }
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
        if(reentry) {phase=EXIT;event("exit-request");}
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
    static FILE *bombhei_file=nullptr;
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
    const char *bombhei_path=std::getenv("SM64DS_PC_BOMBHEI_CAPTURE");
    if(bombhei_path && *bombhei_path) {
        if(!bombhei_file) {
            bombhei_file=std::fopen(bombhei_path,"w");
            if(bombhei_file)std::fputs("tick,level,id,alive,variant,state,death,fuse,carried,hit,other,flags,vuln,x,y,z\n",bombhei_file);
        }
        if(bombhei_file) {
            for(auto *a=dActor_c::FindWithActorID(206,nullptr);a;a=dActor_c::FindWithActorID(206,a)) {
                auto *b=static_cast<daBmb_c *>(a);
                std::fprintf(bombhei_file,"%llu,%d,%u,%u,%u,%d,%d,%u,%u,%u,%u,%u,%u,%d,%d,%d\n",
                    static_cast<unsigned long long>(tick),data_0209f2f8,b->uniqueID,b->aliveState,
                    unsigned(b->mVariant),b->mState,b->mDeathState,unsigned(b->mFuse),b->mCarrier?1u:0u,
                    b->mdCc_c.hitFlags,b->mdCc_c.otherOwner,b->mdCc_c.flags,b->mdCc_c.vulnFlags,b->mPosX,b->mPosY,b->mPosZ);
            }
            std::fflush(bombhei_file);
        }
    }
}

// Opt-in coin position telemetry for isolated gameplay-route validation.
// This only reads live actor fields and writes when explicitly requested.
static void capture_coin_actors() {
    const char *path=std::getenv("SM64DS_PC_COIN_ACTOR_CAPTURE");
    if(!path || !*path)return;
    static FILE *file=nullptr;
    static uint64_t previous=~uint64_t(0);
    const auto tick=ntr::rt_frame();
    if(tick==previous || tick%10!=0)return;
    previous=tick;
    if(!file) {
        file=std::fopen(path,"w");
        if(!file)return;
        std::fputs("tick,level,player_x,player_y,player_z,coin_id,coin_uid,alive,flags,x,y,z,heading,clip_result,clip_distance\n",file);
    }
    auto *player=static_cast<Player *>(data_0209f394[0]);
    unsigned live=0;
    for(unsigned kind : {288u,289u,290u}) {
        for(auto *coin=dActor_c::FindWithActorID(kind,nullptr);coin;coin=dActor_c::FindWithActorID(kind,coin)) {
            if(coin->aliveState!=1)continue;
            ++live;
            std::fprintf(file,"%llu,%d,%d,%d,%d,%u,%u,%u,%08x,%d,%d,%d,%d,%u,%d\n",
                static_cast<unsigned long long>(tick),int(data_0209f2f8),
                player?player->mPosX:0,player?player->mPosY:0,player?player->mPosZ:0,
                kind,coin->uniqueID,unsigned(coin->aliveState),unsigned(coin->mFlags),
                coin->mPosX,coin->mPosY,coin->mPosZ,coin->mAngleY,unsigned(coin->mClipResult),coin->mClipDistance);
        }
    }
    if(!live)std::fprintf(file,"%llu,%d,%d,%d,%d,0,0,0,0,0,0,0,0,0,0\n",
        static_cast<unsigned long long>(tick),int(data_0209f2f8),
        player?player->mPosX:0,player?player->mPosY:0,player?player->mPosZ:0);
    std::fflush(file);
}

extern "C" int pc_native_collision_capture_enabled(void) {
    const char *path=std::getenv("SM64DS_PC_COLLISION_CAPTURE");
    return path && *path;
}

extern "C" void pc_native_collision_trace_pair(
    unsigned int owner0, unsigned int owner1, unsigned int flags0, unsigned int flags1,
    unsigned int sharedFlags, int overlap, const Vector3 *pos0, const Vector3 *pos1) {
    const char *path=std::getenv("SM64DS_PC_COLLISION_CAPTURE");
    if(!path || !*path)return;
    dActor_c *actor0=dActor_c::FindWithID(owner0);
    dActor_c *actor1=dActor_c::FindWithID(owner1);
    static FILE *file=nullptr;
    if(!file) {
        file=std::fopen(path,"w");
        if(file)std::fputs("tick,owner0,actor0,owner1,actor1,flags0,flags1,shared_flags,overlap,x0,y0,z0,x1,y1,z1,bombhei_fuse,bombhei_state,waterbomb_variant,waterbomb_state\n",file);
    }
    if(!file)return;
    int fuse=-1,bombState=-1,waterVariant=-1,waterState=-1;
    if(actor0 && actor0->actorID==206) {auto *b=static_cast<const daBmb_c *>(actor0);fuse=b->mFuse;bombState=b->mState;}
    if(actor1 && actor1->actorID==206) {auto *b=static_cast<const daBmb_c *>(actor1);fuse=b->mFuse;bombState=b->mState;}
    if(actor0 && actor0->actorID==208) {auto *w=static_cast<const daWbm_c *>(actor0);waterVariant=w->mVariant;waterState=w->mState;}
    if(actor1 && actor1->actorID==208) {auto *w=static_cast<const daWbm_c *>(actor1);waterVariant=w->mVariant;waterState=w->mState;}
    std::fprintf(file,"%llu,%u,%u,%u,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
        static_cast<unsigned long long>(ntr::rt_frame()),owner0,actor0?unsigned(actor0->actorID):0,owner1,actor1?unsigned(actor1->actorID):0,
        flags0,flags1,sharedFlags,overlap,pos0->x,pos0->y,pos0->z,pos1->x,pos1->y,pos1->z,fuse,bombState,waterVariant,waterState);
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
        struct ReturnPoint {int x,z,radius;};
        static ReturnPoint route[128]={{0,6500,70},{-5950,6500,180}};
        static unsigned count=2;static bool loaded=false;
        if(!loaded) {
            loaded=true;
            if(const char *path=std::getenv("SM64DS_PC_TEST_CANNON_RETURN_ROUTE")) {
                count=0;
                if(FILE *file=std::fopen(path,"r")) {
                    ReturnPoint point;
                    while(count<128 && std::fscanf(file,"%d,%d,%d ",&point.x,&point.z,&point.radius)==3) {
                        if(std::abs(point.x)>32767 || std::abs(point.z)>32767 || point.radius<1 || point.radius>1000)break;
                        route[count++]=point;
                    }
                    std::fclose(file);
                }
                std::fprintf(stderr,"[native cannon return route] points=%u\n",count);
            }
        }
        if(!count)return controls();
        if(unsigned(waypoint)<count) {
            // Optional private collision-derived waypoints avoid newly
            // restored actors. This remains normal stick/button input only.
            if(move(route[waypoint].x,route[waypoint].z,route[waypoint].radius)) {++waypoint;event("return-waypoint");}
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

// Input-only collection for the labelled Power Star fixture below. A fixed
// camera-relative stick recording becomes invalid when an entry camera changes.
// Observe the actual star position and use the ordinary controller path instead.
static unsigned power_star_fixture_target;
static bool replay_power_star_controls()
{
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    const char *fixture=std::getenv("SM64DS_PC_TEST_POWER_STAR");
    if(!automation || std::strcmp(automation,"1") || !fixture ||
       std::strcmp(fixture,"1") || ntr::rt_frame()<1700)return false;
    uint16_t buttons=0;int16_t sx=0,sy=0;
    auto *player=static_cast<Player *>(data_0209f394[0]);
    if(data_0209f2f8==6 && player && player->aliveState==1 &&
       !player->mIsNoControl && !player->mIsControlDisabled && !data_0209d660) {
        for(auto *actor=dActor_c::FindWithActorID(178,nullptr);actor;
            actor=dActor_c::FindWithActorID(178,actor)) {
            auto *star=static_cast<daStar_c *>(actor);
            if(star->aliveState!=1 || star->uniqueID!=power_star_fixture_target || star->mState!=4)continue;
            static bool logged=false;
            if(!logged) {
                std::fprintf(stderr,"[native Power Star replay] target=%u tick=%llu; input-only collection\n",
                    star->uniqueID,static_cast<unsigned long long>(ntr::rt_frame()));
                logged=true;
            }
            const double dx=double(star->mPosX)-player->mPosX;
            const double dz=double(star->mPosZ)-player->mPosZ;
            if(std::hypot(dx,dz)>15*4096.0) {
                const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
                sx=static_cast<int16_t>(std::sin(angle)*18000);
                sy=static_cast<int16_t>(-std::cos(angle)*18000);
            }
            break;
        }
    }
    ntr::input_test_controls(true,buttons,sx,sy);
    return true;
}

// Opt-in read-only actor census for fresh-save route investigation. The
// original object table may spawn actors whose retail profile is not yet
// named/native; enumerate the live actor IDs once at a requested frame.
static void trace_actor_census(uint64_t frame)
{
    static uint64_t last_frame = ~uint64_t(0);
    const char *when = std::getenv("SM64DS_PC_TRACE_ACTORS_AT");
    if (last_frame == frame || !when || !*when) return;
    bool selected=false;
    while(when && *when) {
        char *end=nullptr;
        const auto target=std::strtoull(when,&end,10);
        if(end!=when && (!*end || *end==',') && frame==target)selected=true;
        const char *next=std::strchr(when,',');when=next?next+1:nullptr;
    }
    if(!selected)return;
    last_frame=frame;

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

// QA controller only: approach the live original stump, jump, then press
// crouch while airborne over it. No actor, collision or save mutation.
extern "C" unsigned short *data_0209214c[3];
static uint16_t chomp_replay_button(unsigned action) {
    // The save can select Standard, Touch or Dual-Hand control. Use the
    // original live mapping rather than assuming Standard DS button bits.
    const unsigned mode=data_0209f4ae[0];
    if(mode>2)return 0;
    for(unsigned bit=0;bit<12;++bit)if(data_0209214c[mode][bit]==action)return uint16_t(1u<<bit);
    return 0;
}
static bool replay_chomp_controls() {
    const char *mode=std::getenv("SM64DS_PC_TEST_CHOMP");
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(!mode || (std::strcmp(mode,"release") && std::strcmp(mode,"release-local") && std::strcmp(mode,"spawn-view")) || !automation || std::strcmp(automation,"1") ||
       data_0209f2f8!=6 || ntr::rt_frame()<4800)return false;
    auto *p=static_cast<Player *>(data_0209f394[0]);
    auto *c=static_cast<daWanwan_c *>(dActor_c::FindWithActorID(219,nullptr));
    auto *s=c?static_cast<daObjPile_c *>(dActor_c::FindWithID(c->mStumpUniqueID)):nullptr;
    uint16_t buttons=0;int16_t sx=0,sy=0;
    static bool released=false;
    if(c && c->mState==3 && c->mChainBroken)released=true;
    if(released && !c && std::getenv("SM64DS_PC_TEST_CHOMP_STAR")) {
        // Input-only approach through the opened fence to the original Star.
        // The release-local fixture's one initial player travel is unchanged.
        auto *f=static_cast<daObjWanwanShutter_c *>(dActor_c::FindWithActorID(41,nullptr));
        if(p && p->aliveState==1 && f && f->mDisabled &&
           !p->mIsNoControl && !p->mIsControlDisabled && !data_0209d660) {
            for(auto *a=dActor_c::FindWithActorID(178,nullptr);a;a=dActor_c::FindWithActorID(178,a)) {
                auto *star=static_cast<daStar_c *>(a);
                if(star->aliveState!=1 || star->param1!=7 || star->mState!=4)continue;
                const double dx=double(star->mPosX)-p->mPosX,dz=double(star->mPosZ)-p->mPosZ;
                const double distance=std::hypot(dx,dz);
                if(distance>15*4096.0) {
                    const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
                    const double magnitude=distance<180*4096.0?12000:22000;
                    sx=static_cast<int16_t>(std::sin(angle)*magnitude);
                    sy=static_cast<int16_t>(-std::cos(angle)*magnitude);
                }
                if(distance<200*4096.0 && star->mPosY>p->mPosY+100*4096 && ntr::rt_frame()%60<10)
                    buttons=chomp_replay_button(2);
                break;
            }
        } else if(data_0209d660 && ntr::rt_frame()%45<3)buttons=1;
        ntr::input_test_controls(true,buttons,sx,sy);return true;
    }
    uint64_t travel_tick=4800;
    if(const char *value=std::getenv("SM64DS_PC_TEST_CHOMP_TRAVEL_TICK")) {
        const auto requested=std::strtoull(value,nullptr,10);
        if(requested>=4800 && requested<=20000)travel_tick=requested;
    }
    const bool before_travel=!std::strcmp(mode,"release-local") && ntr::rt_frame()<travel_tick;
    static bool local_travel=false;
    if(!std::strcmp(mode,"release-local") && !before_travel && !local_travel && p && s && p->aliveState==1 &&
       !p->mIsNoControl && !p->mIsControlDisabled) {
        local_travel=true;
        p->mPosX=p->mPrevPosX=p->mPreClsnPosX=s->mPosX;
        p->mPosY=p->mPrevPosY=p->mPreClsnPosY=s->mPosY+220*4096;
        p->mPosZ=p->mPrevPosZ=p->mPreClsnPosZ=s->mPosZ;
        p->mHorzSpeed=p->mVertSpeed=0;
        std::fprintf(stderr,"[native chomp QA travel] tick=%llu stump=%u player=%d,%d,%d; encounter state untouched, subsequent input only\n",
            static_cast<unsigned long long>(ntr::rt_frame()),s->uniqueID,p->mPosX,p->mPosY,p->mPosZ);
    }
    if(!std::strcmp(mode,"spawn-view") || before_travel) {
        const auto tick=ntr::rt_frame();
        if(p && c && tick<4810) {
            const double angle=std::atan2(double(c->mPosX)-p->mPosX,double(c->mPosZ)-p->mPosZ)-GetAngleToCamera(0)*6.283185307179586/65536.0;
            sx=static_cast<int16_t>(std::sin(angle)*15000);
            sy=static_cast<int16_t>(-std::cos(angle)*15000);
        }
        if(tick>=4820 && tick<4900)buttons=chomp_replay_button(0x4000);
        ntr::input_test_controls(true,buttons,sx,sy);return true;
    }
    static bool at_stump_area=false;
    if(p && s && p->mPosY>s->mPosY-200*4096 &&
       std::hypot(double(s->mPosX)-p->mPosX,double(s->mPosZ)-p->mPosZ)<2200*4096.0)at_stump_area=true;
    if(!at_stump_area)return false; // original input-only route crosses the bridge first
    if(p && p->aliveState==1 && s && s->mStepsLeft && !p->mIsNoControl && !p->mIsControlDisabled && !data_0209d660) {
        const auto tick=ntr::rt_frame();
        const double dx=double(s->mPosX)-p->mPosX,dz=double(s->mPosZ)-p->mPosZ;
        const double distance=std::hypot(dx,dz);
        static uint64_t jump_tick=0,next_jump=0;
        const char *state=pc_player_state_name(p);
        const bool ground=!std::strcmp(state,"St_Wait_Main") || !std::strcmp(state,"St_Walk_Main");
        if(ground && tick>=next_jump && distance<260*4096.0) {
            jump_tick=tick;next_jump=tick+40;
        }
        if(jump_tick && tick<jump_tick+10)buttons|=chomp_replay_button(2);
        if(p->mIsAirborne && distance<18*4096.0 && p->mPosY>s->mPosY+210*4096 &&
           !std::strcmp(state,"St_Jump_Main"))buttons|=chomp_replay_button(0x400);
        if(distance>20*4096.0 && std::strcmp(state,"St_GroundPound_Main")) {
            const double angle=std::atan2(dx,dz)-GetAngleToCamera(0)*6.283185307179586/65536.0;
            const double magnitude=distance<130*4096.0?12000:22000;
            sx=static_cast<int16_t>(std::sin(angle)*magnitude);
            sy=static_cast<int16_t>(-std::cos(angle)*magnitude);
        }
    } else if(data_0209d660 && ntr::rt_frame()%45<3)buttons=1;
    ntr::input_test_controls(true,buttons,sx,sy);return true;
}

static bool trace_script_progress(uint64_t frame)
{
    if(const char *path=std::getenv("SM64DS_PC_DRAIN_CAPTURE")) {
        static FILE *file=nullptr;static uint64_t previous=~uint64_t(0);
        const auto tick=ntr::rt_frame();
        if(tick!=previous && tick%10==0) {
            previous=tick;
            if(!file){file=std::fopen(path,"w");if(file)std::fputs("tick,level,flags2,water_height,kind,id,y,pressed,lowered_y,draining,collision_enabled\n",file);}
            extern int data_0209f32c;
            if(file) {
                const auto flags=static_cast<unsigned>(reinterpret_cast<const SaveData *>(data_0209caa0)->flags2);
                std::fprintf(file,"%llu,%d,%u,%d,0,0,0,0,0,0,0\n",static_cast<unsigned long long>(tick),int(data_0209f2f8),flags,data_0209f32c);
                for(unsigned kind:{34u,35u})for(auto *a=dActor_c::FindWithActorID(kind,nullptr);a;a=dActor_c::FindWithActorID(kind,a)) {
                    if(a->aliveState!=1)continue;
                    const auto *pillar=kind==34?static_cast<daObjC0_Switch_c *>(a):nullptr;
                    const auto *water=kind==35?static_cast<daObjC0Water_c *>(a):nullptr;
                    std::fprintf(file,"%llu,%d,%u,%d,%u,%u,%d,%u,%d,%u,%u\n",static_cast<unsigned long long>(tick),int(data_0209f2f8),flags,data_0209f32c,
                        kind,a->uniqueID,int(a->mPosY),pillar?unsigned(pillar->mPressed):0,water?water->mLoweredY:0,water?unsigned(water->mWasJustDrained):0,
                        unsigned(static_cast<dBgActor_c *>(a)->mMeshCollider.IsEnabled()));
                }
                std::fflush(file);
            }
        }
    }

    // Opt-in, read-only observations of the original three-actor encounter.
    if(const char *path=std::getenv("SM64DS_PC_CHOMP_CAPTURE")) {
        static FILE *file=std::fopen(path,"w");static uint64_t last=~uint64_t(0);
        const auto tick=ntr::rt_frame();
        if(file && last!=tick && tick%5==0) {
            if(last==~uint64_t(0))std::fputs("tick,id,kind,alive,x,y,z,state,step,broken,stump,fence,steps,cooldown,busy,disabled,collision,player_x,player_y,player_z,player_state,airborne,no_control,health,flags,area,camera_x,camera_y,camera_z,clip_result\n",file);
            last=tick;
            auto *p=static_cast<Player *>(data_0209f394[0]);
            for(unsigned kind:{219u,27u,41u})
            for(auto *a=dActor_c::FindWithActorID(kind,nullptr);a;a=dActor_c::FindWithActorID(kind,a)) {
                int state=-1,step=-1,broken=-1,stump=-1,fence=-1,steps=-1,cooldown=-1,busy=-1,disabled=-1,collision=-1;
                if(kind==219) {auto *c=static_cast<daWanwan_c *>(a);state=c->mState;step=c->mReleaseStep;broken=c->mChainBroken;stump=c->mStumpUniqueID;fence=c->mFenceUniqueID;}
                if(kind==27) {auto *s=static_cast<daObjPile_c *>(a);steps=s->mStepsLeft;cooldown=s->mAttackCooldown;busy=s->mBusy;collision=s->mMeshCollider.IsEnabled();}
                if(kind==41) {auto *f=static_cast<daObjWanwanShutter_c *>(a);disabled=f->mDisabled;collision=f->mMeshCollider.IsEnabled();}
                std::fprintf(file,"%llu,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s,%u,%u,%u,%u,%d,%d,%d,%d,%u\n",
                    static_cast<unsigned long long>(tick),a->uniqueID,kind,a->aliveState,a->mPosX,a->mPosY,a->mPosZ,state,step,broken,stump,fence,steps,cooldown,busy,disabled,collision,
                    p?p->mPosX:0,p?p->mPosY:0,p?p->mPosZ:0,pc_player_state_name(p),p?p->mIsAirborne:0,p?p->mIsNoControl:0,p?p->GetHealth():0,
                    a->mFlags,int(a->mAreaId),a->mCamSpacePosX,a->mCamSpacePosY,a->mCamSpacePosZ,unsigned(a->mClipResult));
            }
            std::fflush(file);
        }
    }
    // The host callback is paced at presentation FPS (and may repeat a game
    // state), while route diagnostics are specified in simulation ticks.
    trace_actor_census(ntr::rt_frame());
    // Explicit QA placement only: keep course geometry/ROM and ordinary play
    // intact while exercising the original Power Star pickup and exit path.
    // The host hook runs between game ticks on the same thread.
    static uint64_t power_star_fixture_tick=~uint64_t(0);
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    const char *power_fixture=std::getenv("SM64DS_PC_TEST_POWER_STAR");
    const auto fixture_frame=ntr::rt_frame();
    // Isolated QA encounters. Ordinary mode approaches an original Mario-cap
    // Goomba; fixture mode spawns a native bank-1 Mario-cap Goomba nearby.
    // The normal tongue/contact/reward path must release and collect its cap.
    // No cap actor, hit, transformation or camera state is injected.
    static bool camera_goomba_approached=false;
    if(!camera_goomba_approached && automation && !std::strcmp(automation,"1"))
    if(const char *cap_tick=std::getenv("SM64DS_PC_TEST_CAMERA_GOOMBA"))
    if(fixture_frame==std::strtoull(cap_tick,nullptr,10) && data_0209f2f8==6) {
        camera_goomba_approached=true;
        auto *player=static_cast<Player *>(data_0209f394[0]);
        if(player && player->aliveState==1 && !player->mIsNoControl && !player->mIsControlDisabled) {
            daKrb_c *target=nullptr;double nearest=1e30;
            const char *fixture=std::getenv("SM64DS_PC_TEST_CAMERA_GOOMBA_FIXTURE");
            if(fixture && !std::strcmp(fixture,"1")) {
                Vector3 position={player->mPosX,player->mPosY,player->mPosZ+180*4096};
                target=static_cast<daKrb_c *>(dActor_c::Spawn(200,0xf0f3,position,nullptr,player->mAreaId,-1));
            }
            if(!fixture || std::strcmp(fixture,"1"))
            for(auto *a=dActor_c::FindWithActorID(200,nullptr);a;a=dActor_c::FindWithActorID(200,a)) {
                auto *g=static_cast<daKrb_c *>(a);
                const double distance=std::hypot(double(g->mPosX)-player->mPosX,double(g->mPosZ)-player->mPosZ);
                if(g->aliveState==1 && g->mCapId==0 && distance<nearest) {target=g;nearest=distance;}
            }
            if(target) {
                player->mPosX=player->mPrevPosX=player->mPreClsnPosX=target->mPosX;
                player->mPosY=player->mPrevPosY=player->mPreClsnPosY=target->mPosY;
                player->mPosZ=player->mPrevPosZ=player->mPreClsnPosZ=target->mPosZ-180*4096;
                player->mAngleY=player->mPrevAngleY=0;
                player->mHorzSpeed=player->mVertSpeed=0;
                std::fprintf(stderr,"[native QA camera Goomba] tick=%llu actor=%u cap=%u dormant=%u pos=%d,%d,%d\n",
                    static_cast<unsigned long long>(fixture_frame),target->uniqueID,unsigned(target->mCapId),unsigned(target->mIsDormant),
                    player->mPosX,player->mPosY,player->mPosZ);
            }
        }
    }
    unsigned fixture_param=0;
    const char *fixture_sequence=std::getenv("SM64DS_PC_TEST_POWER_STAR_SEQUENCE");
    if(!fixture_sequence)fixture_sequence="1700:1";
    while(fixture_sequence && *fixture_sequence) {
        char *tick_end=nullptr,*param_end=nullptr;
        const auto tick=std::strtoull(fixture_sequence,&tick_end,10);
        if(tick_end!=fixture_sequence && *tick_end==':') {
            const auto param=std::strtoul(tick_end+1,&param_end,10);
            if(param_end!=tick_end+1 && (!*param_end || *param_end==',') &&
               tick==fixture_frame && param>=1 && param<=7)fixture_param=static_cast<unsigned>(param);
        }
        const char *next=std::strchr(fixture_sequence,',');fixture_sequence=next?next+1:nullptr;
    }
    if(power_star_fixture_tick!=fixture_frame && fixture_param &&
       automation && std::strcmp(automation,"1")==0 &&
       power_fixture && std::strcmp(power_fixture,"1")==0 && data_0209f2f8==6) {
        auto *player=static_cast<Player *>(data_0209f394[0]);
        power_star_fixture_tick=fixture_frame;
        if(player && player->aliveState==1 && !player->mIsNoControl && !player->mIsControlDisabled) {
            Vector3 position;
            position.x=player->mPosX;position.y=player->mPosY+120*4096;
            position.z=player->mPosZ-350*4096;
            auto *star=dActor_c::Spawn(178,fixture_param,position,nullptr,player->mAreaId,-1);
            power_star_fixture_target=star?star->uniqueID:0u;
            std::fprintf(stderr,"[native QA fixture] Power Star level=6 param=%04x actor=%u pos=%d,%d,%d; ordinary native collection\n",
                fixture_param,power_star_fixture_target,position.x,position.y,position.z);
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
    pc_trace_warp_player(static_cast<Player *>(data_0209f394[0]));
    trace_star_completion();
    pc_trace_warp_ground(static_cast<Player *>(data_0209f394[0]));
    pc_probe_koopa_ground(static_cast<Player *>(data_0209f394[0]));
    if(const char *path=std::getenv("SM64DS_PC_MOVING_CONTACT_CAPTURE")) {
        static FILE *file=nullptr;static uint64_t last=~uint64_t(0);
        const auto tick=ntr::rt_frame();
        auto *p=static_cast<Player *>(data_0209f394[0]);
        static const unsigned interval=[](){const char *v=std::getenv("SM64DS_PC_MOVING_CONTACT_INTERVAL");return v?(std::max)(1u,unsigned(std::strtoul(v,nullptr,10))):5u;}();
        if(!file) {file=std::fopen(path,"w");if(file)std::fputs("tick,level,grounded,airborne,owner_kind,owner_uid,x,y,z,ground_y,normal_x,normal_y,normal_z,clsn_flags,wall_x,wall_y,wall_z,wall_owner_kind,wall_owner_uid,wall_triangle\n",file);}
        if(file && p && p->aliveState==1 && tick%interval==0 && tick!=last) {
            last=tick;
            const bool grounded=p->mMeshClsn.IsOnGround()!=0;
            const auto *hit=grounded?static_cast<dBgPi *>(p->mMeshClsn.GetFloorResult()):nullptr;
            const auto *owner=hit && hit->colliderIdx<24 ? hit->owner:nullptr;
            const auto *wall=p->mMeshClsn.IsOnWall()?static_cast<dBgPi *>(p->mMeshClsn.GetWallResult()):nullptr;
            const auto *wallOwner=wall && wall->colliderIdx<24?wall->owner:nullptr;
            std::fprintf(file,"%llu,%d,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%u,%d,%d,%d,%u,%u,%u\n",
                static_cast<unsigned long long>(tick),int(data_0209f2f8),unsigned(grounded),unsigned(p->mIsAirborne),
                owner?unsigned(owner->actorID):0,owner?unsigned(owner->uniqueID):0,
                p->mPosX,p->mPosY,p->mPosZ,p->mGroundY,p->mFloorNormalX,p->mFloorNormalY,p->mFloorNormalZ,
                unsigned(p->mClsnFlags),p->mWallNormalX,p->mWallNormalY,p->mWallNormalZ,
                wallOwner?unsigned(wallOwner->actorID):0,wallOwner?unsigned(wallOwner->uniqueID):0,
                wall?unsigned(wall->triangleID):0);std::fflush(file);
        }
    }
    static uint64_t last_controls_tick=~uint64_t(0);
    if(last_controls_tick!=ntr::rt_frame()) {
        last_controls_tick=ntr::rt_frame();
        if(!pc_replay_koopa(static_cast<Player *>(data_0209f394[0]),replay_course_route_complete) && !pc_replay_lift_ride(static_cast<Player *>(data_0209f394[0]),replay_course_route_complete,replay_lava_bridge_reentered) && !replay_lava_bridge_lifecycle() && !pc_replay_warp_repeat(static_cast<Player *>(data_0209f394[0]),replay_course_route_complete) && !pc_replay_warp_lifecycle(static_cast<Player *>(data_0209f394[0])) && !replay_toad_controls() && !replay_chomp_controls() && !replay_waterbomb_controls() && !replay_cannon_cycle_controls() && !replay_cannon_controls() && !replay_buddy_controls() && !replay_rabbit_controls() && !replay_pillar_controls() && !replay_drain_save_controls() && !replay_star_save_controls() && !replay_course_route() && !replay_power_star_controls())replay_boss_controls();
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
                if(auto *gate_player=static_cast<Player *>(player); gate_player && std::getenv("SM64DS_PC_STAR_GATE_TRACE") &&
                   data_0209f2f8==5 && ntr::rt_frame()%120==0) {
                    std::fprintf(stderr,"[native Star Gate player] tick=%llu area=%d prevarea=%u state=%s step=%u kind=%u arg=%d flags=%u target=%d,%d,%d anchor=%d,%d,%d heading=%d moveheading=%d wall=%d,%d,%d pos=%d,%d,%d\n",
                        static_cast<unsigned long long>(ntr::rt_frame()),gate_player->mAreaId,gate_player->mPrevAreaId,
                        pc_player_state_name(gate_player),gate_player->mStateStep,gate_player->mNoCtrlKind,int(gate_player->mStateArg),
                        gate_player->mIsOpeningBigDoor,gate_player->unk_744,gate_player->unk_748,gate_player->unk_74c,
                        gate_player->unk_750,gate_player->unk_754,gate_player->unk_758,gate_player->mAngleY,gate_player->mPrevAngleY,
                        gate_player->mWallNormalX,gate_player->mWallNormalY,gate_player->mWallNormalZ,
                        gate_player->mPosX,gate_player->mPosY,gate_player->mPosZ);
                    for(auto *a=dActor_c::FindWithActorID(354,nullptr);a;a=dActor_c::FindWithActorID(354,a)) {
                        auto *g=static_cast<daStarGate_c *>(a);
                        if(g->param1!=2)continue;
                        std::fprintf(stderr,"[native Star Gate geometry] tick=%llu state=%d areas=%d,%d speed=%d target=%d offset=%d,%d,%d timer=%u\n",
                            static_cast<unsigned long long>(ntr::rt_frame()),g->mState==&daStarGate_c::ST_WAIT?0:g->mState==&daStarGate_c::ST_OPEN_CLOSE?3:9,
                            g->mAngleX,g->mAngleZ,g->mHorzSpeed,g->mTerminalVelocity,g->unk_0a4,g->mVertSpeed,g->unk_0ac,g->mCloseDoorTimer);
                    }
                }
            }
        }
    }
    // Opt-in diagnostic for the original Secret Slide timed surfaces. The
    // matched Player routine starts/stops this same native Timer on types 15/16.
    if (const char *timer_path = std::getenv("SM64DS_PC_SLIDE_TIMER_TRACE")) {
        static FILE *timer_file = nullptr;
        static unsigned long long last_timer_tick = ~0ull;
        const auto tick = ntr::rt_frame();
        if (tick != last_timer_tick && tick % 10 == 0) {
            last_timer_tick = tick;
            if (!timer_file) {
                timer_file = std::fopen(timer_path, "w");
                if (timer_file)
                    std::fputs("tick,level,player_surface,timer_running,timer_started,key_event,elapsed_sec,player_x,player_y,player_z\n", timer_file);
            }
            auto *p = static_cast<Player *>(data_0209f394[0]);
            if (timer_file && p && p->aliveState == 1) {
                auto *timer = reinterpret_cast<Timer *>(data_0209d4c8);
                const unsigned long long elapsed = (static_cast<unsigned long long>(timer->GetTime()) << 6) / 33514000;
                std::fprintf(timer_file, "%llu,%d,%d,%u,%u,%d,%llu,%d,%d,%d\n",
                    tick, int(data_0209f2f8), int(p->mSurfaceType), unsigned(timer->mIsRunning),
                    unsigned(data_ov002_02111184[0]), Event::GetBit(0x1d), elapsed,
                    p->mPosX, p->mPosY, p->mPosZ);
                std::fflush(timer_file);
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
    capture_coin_actors();
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
    if (!ntr::fs_mount_rom(rom_path, true)) {
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
    ntr::audio_open();

    if(const char *test=std::getenv("SM64DS_PC_TEST_AUDIO_ABI"))if(!std::strcmp(test,"1")){
        int waiting=-1,finished=-1;
        if(ntr::audio_ready()){
            data_0209b490=0;data_0209b49c=0;
            waiting=_ZN5Sound7PlaySubEjjj5Fix12IiEb(0,127,0,0x15666,1);
            data_0209b490=127<<12;
            finished=_ZN5Sound7PlaySubEjjj5Fix12IiEb(0,127,0,0x15666,1);
        }
        const bool ok=waiting==0 && finished==1;
        std::fprintf(stderr,"[native audio ABI] waiting=%d finished=%d expected=0,1 result=%s\n",waiting,finished,ok?"PASS":"FAIL");
        ntr::audio_close();ntr::video_close();ExitProcess(ok?0:10);
    }

    ntr::rt_run(run_game, trace_script_progress, runtime_frame_limit(), pc_stage_graph_prepare);
    trace("frame runtime returned");
    ntr::audio_close();
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



