#include "hal/stage_graph_state.h"
#include "dScStage_c.h"
#include "ntr/video.h"
#include "ntr/input.h"
#include "ntr/gpu.h"
#include "ntr/rt.h"
#include "ntr/audio.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#if defined(SM64DS_GAME_TARGET)
#include "Timer.h"
#include "Event.h"
#include <algorithm>
#include "hal/dsmt_scene.h"
#include "hal/wanted_pause_settings.h"
#include "hal/touch_menu_navigation.h"
#include "hal/pc_pause_settings_state.h"
#include "dScMgLuigi_c.h"
#include "Player.h"
extern "C" dScMgBase_c::graphCallback_c data_ov004_020beb74;
extern "C" int pc_pause_settings_page();
extern "C" int pc_pause_settings_row();
#endif

#if defined(SM64DS_GAME_TARGET)
bool pc_minimap_hud_art_prepare();
#endif

extern "C" {
PcStageGraphState data_0209f3c4 = {};
extern void *data_0209d4a8;
extern unsigned char data_0209f2c4;
#if defined(SM64DS_GAME_TARGET)
extern void *data_0209f394[];
extern unsigned char data_0209f294,data_0209f20c;
extern int data_0209fc48;
extern unsigned char data_0209f250;
extern signed char data_0209f2f4;
extern short data_0209f358[];
extern unsigned char NumStars(void);
extern short data_02092144[];
extern unsigned char data_0209d4c8[],data_ov002_02111184[];
extern signed char data_0209f2f8;
// The C IRQ/OAM shards use cdecl tables. The minigame registry is a real
// native C++ object, whose virtual methods require the MSVC thiscall ABI.
extern signed char data_0208e42c;
extern int data_0209b490;
int pc_minigame_graph_dispatch(void *object, int slot, int *result)
{
    if(object != &data_ov004_020beb74) return 0;
    switch(slot) {
    case 0: *result=data_ov004_020beb74.GraphCallback0();break;
    case 1: *result=data_ov004_020beb74.GraphCallback1();break;
    case 2: *result=data_ov004_020beb74.GraphCallback2();break;
    case 3: *result=data_ov004_020beb74.GraphCallback3();break;
    default: return 0;
    }
    return 1;
}
#endif
int pc_stage_graph_apply(void *self)
{
    return static_cast<dScStage_c *>(self)->GraphCallback2();
}
#if defined(SM64DS_GAME_TARGET)
int pc_stage_graph_draw_particles(void *self)
{
    // Restore the original slot1 worker in the game's graphics phase, before
    // SWAP_BUFFERS. before_present() can repeat at120/240Hz and is too late.
    return static_cast<dScStage_c *>(self)->GraphCallback1();
}
#endif
void pc_stage_graph_prepare()
{
#if defined(SM64DS_GAME_TARGET)
    ntr::gpu_set_menu_scene(pc_dsmt_active());
#endif
    // The fiber runtime does not execute the ARM VBlank IRQ dispatcher.
    // Upload this active Stage callback's latest transform before scanout.
    const bool active = data_0209d4a8 == &data_0209f3c4;
    if(!active)ntr::video_set_cannon_sight(false);
    ntr::video_set_minimap_gameplay(active);
#if defined(SM64DS_GAME_TARGET)
    const unsigned player_slot=data_0209f250<4?data_0209f250:0;
    const int coin_count=data_0209f358[player_slot];
    const int life_count=data_0209f2f4;
    const auto *hud_player=static_cast<const Player *>(data_0209f394[player_slot]);
    const bool hud_ready=active && hud_player && pc_minimap_hud_art_prepare();
    ntr::video_set_minimap_counters(hud_ready,NumStars(),
        unsigned(coin_count<0?0:coin_count),unsigned(life_count<0?0:life_count),
        unsigned((data_02092144[player_slot]>>8)&255),hud_player?hud_player->mCharacter:3);
    unsigned timer_hundredths=0;
    const bool slide_timer=active && data_0209f2f8==29 &&
        data_ov002_02111184[0]!=0 && Event::GetBit(0x1d)==0;
    if(slide_timer) {
        auto *timer=reinterpret_cast<Timer *>(data_0209d4c8);
        const unsigned long long ds_ticks=static_cast<unsigned long long>(timer->GetTime())<<6;
        const unsigned long long seconds=ds_ticks/33514000;
        const unsigned long long centiseconds=(ds_ticks/33514)%1000/10;
        timer_hundredths=unsigned(seconds*100+centiseconds);
        timer_hundredths=std::min(timer_hundredths,599999u);
    }
    ntr::video_set_minimap_timer(slide_timer,timer_hundredths);
#else
    ntr::video_set_minimap_counters(false,0,0,0);
    ntr::video_set_minimap_timer(false,0);
#endif
    const bool paused = active && data_0209f2c4 != 0;
    bool custom_settings = false;
#if defined(SM64DS_GAME_TARGET)
    custom_settings = pc_pause_settings_page() != 0;
#endif
    bool wanted_settings=false;
#if defined(SM64DS_GAME_TARGET)
    wanted_settings=pc_wanted_settings_page()!=0;
#endif
    ntr::video_set_pause_screen(paused || wanted_settings,custom_settings);
    bool music_paused=paused || wanted_settings;
#if defined(SM64DS_GAME_TARGET)
    if(data_0209d4a8==&data_ov004_020beb74) {
        const auto *scene=data_ov004_020beb74.mScene;
        music_paused=scene && scene->mMenuOpen;
    }
#endif
    ntr::audio_set_pause_menu(music_paused,ntr::video_get_settings().pause_music_rec_room);
#if defined(SM64DS_GAME_TARGET)
    ntr::video_set_minimap_editor(paused && pc_pause_settings_page()==6,pc_pause_settings_row());
#endif
    if (active || paused) {
        bool menu_context=paused;
#if defined(SM64DS_GAME_TARGET)
        const auto *player=static_cast<const Player *>(data_0209f394[0]);
        menu_context=menu_context || data_0209f294 || data_0209f20c || data_0209fc48 ||
            (player && (player->mIsNoControl || player->mIsControlDisabled));
#endif
        ntr_input_set_menu_navigation(menu_context ? 1 : 0);
    }
    if (active) {
        pc_stage_graph_apply(&data_0209f3c4);
#if defined(SM64DS_GAME_TARGET)
        if(const char *prefix=std::getenv("SM64DS_PC_DSMT_DUMP")) if(ntr::rt_frame()==2400) {
            extern unsigned char data_0209e660;
            extern unsigned int data_0209e674[],data_0209ea74[];
            extern int data_0209e664,data_0209e670;
            char path[2048];static ntr::Framebuffer frame;
            ntr::ppu_scanout(ntr::ENGINE_B,frame,true,0x0b);
            std::snprintf(path,sizeof(path),"%s-game-bg.bmp",prefix);ntr::ppu_write_bmp(path,frame);
            ntr::ppu_scanout_obj(ntr::ENGINE_B,frame);
            std::snprintf(path,sizeof(path),"%s-game-obj.bmp",prefix);ntr::ppu_write_bmp(path,frame);
            std::snprintf(path,sizeof(path),"%s-game-oam.bin",prefix);
            if(FILE *out=std::fopen(path,"wb")) {
                std::fwrite(data_0209e674,1,0x400,out);std::fwrite(data_0209ea74,1,0x400,out);
                std::fwrite(reinterpret_cast<void*>(0x07000000),1,0x800,out);std::fclose(out);
            }
            std::fprintf(stderr,"[game OAM compare] suppress=%u fill=%d,%d capture=%04x shadow0=%08x,%08x hw0=%08x,%08x\n",
                data_0209e660,data_0209e664,data_0209e670,*reinterpret_cast<volatile unsigned short*>(0x04000304),
                data_0209e674[0],data_0209ea74[0],*reinterpret_cast<volatile unsigned*>(0x07000000),*reinterpret_cast<volatile unsigned*>(0x07000400));
        }
#endif
    }
#if defined(SM64DS_GAME_TARGET)
    else if (data_0209d4a8 == &data_ov004_020beb74) {
        data_ov004_020beb74.GraphCallback2();
        pc_wanted_settings_prepare();
        auto *scene = data_ov004_020beb74.mScene;
        ntr_input_set_menu_navigation(scene && (scene->mMenuOpen || scene->mStateController.mState==18) ? 1 : 0);
        if (scene && scene->actorID == 366 &&
            (std::getenv("SM64DS_PC_TRACE") || std::getenv("SM64DS_PC_WANTED_TRACE")) && ntr::rt_frame()%120==0) {
            auto *wanted = static_cast<dScMgLuigi_c *>(scene);
            std::fprintf(stderr,"[native Wanted] tick=%llu controller=%d timer=%d state=%d time=%d menu=%d music_volume=%d music_target=%d\n",
                static_cast<unsigned long long>(ntr::rt_frame()),scene->mStateController.mState,
                scene->mStateController.mTimer,wanted->mState,wanted->mTime,scene->mMenuOpen,
                data_0209b490 >> 12,int(data_0208e42c));
            std::fprintf(stderr,"[Wanted board] tick=%llu board=%u target=%u wanted=%u found=%u slots=",
                static_cast<unsigned long long>(ntr::rt_frame()),wanted->mBoard,wanted->mTarget,
                wanted->mWanted,wanted->mFound);
            for(int i=0;i<4;i++) if(wanted->mActive[i])
                std::fprintf(stderr,"%s%d:%d,%d:c%d:k%d",i?";":"",i,
                    int(wanted->mPosX[i])>>12,int(wanted->mPosY[i])>>12,
                    wanted->mCharacter[i],wanted->mKind[i]);
            std::fputc('\n',stderr);
            std::fprintf(stderr,"[Wanted menu] pos=%d,%d;%d,%d;%d,%d cursor=%d phase=%d options=%d/%d bg=%08x,%08x oam=%04x,%04x,%04x\n",
                scene->mMenuItem0X,scene->mMenuItem0Y,scene->mMenuItem1X,scene->mMenuItem1Y,
                scene->mMenuItem2X,scene->mMenuItem2Y,scene->mMenuCursor,scene->mMenuCursorPhase,
                scene->mTouchOptions.mActive,scene->mTouchOptions.mIconCount,
                *reinterpret_cast<volatile unsigned*>(0x04000000),*reinterpret_cast<volatile unsigned*>(0x04001000),
                *reinterpret_cast<volatile unsigned short*>(0x07000400),*reinterpret_cast<volatile unsigned short*>(0x07000402),
                *reinterpret_cast<volatile unsigned short*>(0x07000404));
        }
    }
    else {ntr_input_set_menu_navigation(1);pc_dsmt_graph_prepare();}
    pc_touch_menus_prepare();
    static bool settings_focus=false;
    const int settings_row=pc_pause_settings_row();
    if(pc_pause_settings_page()>0 && settings_row>=0) {
        const int y=pc_pause::row_top+settings_row*pc_pause::row_height;
        ntr::video_set_menu_focus(8,y,248,y+16);
        settings_focus=true;
    } else if(settings_focus) {
        ntr::video_set_menu_focus(-1,0,0,0);
        settings_focus=false;
    }
#endif
}
}
