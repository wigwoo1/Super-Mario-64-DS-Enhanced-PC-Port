#include "hal/stage_graph_state.h"
#include "dScStage_c.h"
#include "ntr/video.h"
#include "ntr/input.h"
#include "ntr/gpu.h"
#include "ntr/rt.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#if defined(SM64DS_GAME_TARGET)
#include "hal/dsmt_scene.h"
#include "hal/wanted_pause_settings.h"
#include "dScMgLuigi_c.h"
extern "C" dScMgBase_c::graphCallback_c data_ov004_020beb74;
extern "C" int pc_pause_settings_page();
#endif

extern "C" {
PcStageGraphState data_0209f3c4 = {};
extern void *data_0209d4a8;
extern unsigned char data_0209f2c4;
#if defined(SM64DS_GAME_TARGET)
// The C IRQ/OAM shards use cdecl tables. The minigame registry is a real
// native C++ object, whose virtual methods require the MSVC thiscall ABI.
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
void pc_stage_graph_prepare()
{
#if defined(SM64DS_GAME_TARGET)
    ntr::gpu_set_menu_scene(pc_dsmt_active());
#endif
    // The fiber runtime does not execute the ARM VBlank IRQ dispatcher.
    // Upload this active Stage callback's latest transform before scanout.
    const bool active = data_0209d4a8 == &data_0209f3c4;
    ntr::video_set_minimap_gameplay(active);
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
    if (active || paused) ntr_input_set_menu_navigation(paused ? 1 : 0);
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
        if (scene && scene->actorID == 366 && std::getenv("SM64DS_PC_TRACE") && ntr::rt_frame()%120==0) {
            auto *wanted = static_cast<dScMgLuigi_c *>(scene);
            std::fprintf(stderr,"[native Wanted] tick=%llu controller=%d timer=%d state=%d time=%d menu=%d\n",
                static_cast<unsigned long long>(ntr::rt_frame()),scene->mStateController.mState,
                scene->mStateController.mTimer,wanted->mState,wanted->mTime,scene->mMenuOpen);
            std::fprintf(stderr,"[Wanted menu] pos=%d,%d;%d,%d;%d,%d cursor=%d phase=%d options=%d/%d bg=%08x,%08x oam=%04x,%04x,%04x\n",
                scene->mMenuItem0X,scene->mMenuItem0Y,scene->mMenuItem1X,scene->mMenuItem1Y,
                scene->mMenuItem2X,scene->mMenuItem2Y,scene->mMenuCursor,scene->mMenuCursorPhase,
                scene->mTouchOptions.mActive,scene->mTouchOptions.mIconCount,
                *reinterpret_cast<volatile unsigned*>(0x04000000),*reinterpret_cast<volatile unsigned*>(0x04001000),
                *reinterpret_cast<volatile unsigned short*>(0x07000400),*reinterpret_cast<volatile unsigned short*>(0x07000402),
                *reinterpret_cast<volatile unsigned short*>(0x07000404));
        }
    }
    else pc_dsmt_graph_prepare();
#endif
}
}
