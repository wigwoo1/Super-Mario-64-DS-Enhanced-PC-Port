#include "hal/pc_pause_settings_state.h"
#include "hal/wanted_pause_settings.h"
#include "dScStage_c.h"
#include "SaveData.h"
#include "ntr/input.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "ntr/rt.h"

namespace G2S {
unsigned int GetBG0CharPtr();
unsigned int GetBG0ScrPtr();
unsigned int GetBG1CharPtr();
unsigned short *GetBG1ScrPtr();
}
extern "C" {
extern unsigned char data_0209f2c4, data_0209f248, data_0209f1ec, data_0209f2e0;
extern unsigned char data_0209f2b4, data_0209f22c;
// local extern: the original pause button offsets are TU-only globals.
extern unsigned short data_0209f360[];
extern unsigned short data_0209f300;
extern signed char data_0209f2f8;
extern int data_0208ee44;
extern unsigned char data_0209d698;
extern unsigned char data_0208ee3c;
extern void *data_0209d6f8;
int GetSoundMode();
int SublevelToLevel(int);
int func_02054d88();
}

namespace {
pc_pause::Menu menu;
bool active, legacy, saved_panel, touch_was_down;
uint16_t old_panels[1024], button_panel[128];
uint16_t nav_previous, nav_repeat;
unsigned nav_ticks;
unsigned stage_options_focus;
int stage_options_action;
int stage_options_last_action;
int stage_pause_action=-1;

static void trace_stage_options(uint16_t keys,int edge,const ntr::InputState &input) {
    const char *path=std::getenv("SM64DS_PC_STAGE_OPTIONS_TRACE");
    if(!path || !*path)return;
    static FILE *file=nullptr;
    if(!file) { file=std::fopen(path,"w"); if(file)std::fputs("tick,state,focus,keys,edge,action,sound,backlight,touch\n",file); }
    if(file && (edge || stage_options_action || stage_options_last_action || ntr::rt_frame()%60==0)) {
        std::fprintf(file,"%llu,%u,%u,%u,%u,%d,%d,%u,%u\n",static_cast<unsigned long long>(ntr::rt_frame()),unsigned(data_0209f1ec),stage_options_focus,unsigned(keys),unsigned(edge),stage_options_last_action,GetSoundMode(),unsigned(data_0208ee3c),unsigned(input.touch));
        std::fflush(file);
    }
    stage_options_last_action=0;
}

static void queue_stage_options_action(unsigned focus, bool forward) {
    stage_options_action = focus == 0 ? (forward ? 1 : -1) : 2;
}

void restore_panels() {
    if (saved_panel) std::memcpy(G2S::GetBG1ScrPtr(),old_panels,sizeof(old_panels));
    saved_panel = false;
}
void back_to_pause() {
    active = legacy = false;
    restore_panels();
    data_0209f22c = 0;
    data_0209f1ec = SublevelToLevel(data_0209f2f8)==0x1d ? 0 : 2;
}
void draw() {
    auto *panels = G2S::GetBG1ScrPtr();
    if (!saved_panel) {
        std::memcpy(old_panels,panels,sizeof(old_panels));
        if(!pc_pause::cache_button_panel(panels,data_0209f360[0],button_panel)) {
            std::fprintf(stderr,"[settings artwork] invalid native button offset %u\n",data_0209f360[0]);
            back_to_pause();return;
        }
        saved_panel = true;
    }
    // Follow Message::AddChar's active-font selection. Pause can use the
    // sub-engine font even though the gameplay scene originally loaded mode 0.
    const void *font = data_0209d698==2 ? data_0209d6f8 : reinterpret_cast<void *>(
        data_0209d698==1 ? G2S::GetBG1CharPtr() : func_02054d88());
    if(const char *prefix=std::getenv("SM64DS_PC_SETTINGS_ART_DUMP")) {
        static bool dumped=false;
        if(!dumped && *prefix) {
            dumped=true;
            auto save=[prefix](const char *kind,const void *p,size_t bytes) {
                char path[2048];std::snprintf(path,sizeof(path),"%s-%s.bin",prefix,kind);
                if(FILE *f=std::fopen(path,"wb")){std::fwrite(p,1,bytes,f);std::fclose(f);}
            };
            save("regs",reinterpret_cast<const void *>(0x04001000),0x70);
            save("vram",reinterpret_cast<const void *>(0x06200000),0x20000);
            save("palette",reinterpret_cast<const void *>(0x05000400),0x200);
            save("panels",old_panels,sizeof(old_panels));
            save("button",button_panel,sizeof(button_panel));
            std::fprintf(stderr,"[settings artwork] char0=%08x char1=%08x map0=%08x map1=%p font=%p blank=%04x button_offset=%04x\n",
                G2S::GetBG0CharPtr(),G2S::GetBG1CharPtr(),G2S::GetBG0ScrPtr(),panels,font,old_panels[0],data_0209f360[0]);
        }
    }
    pc_pause::draw(menu,ntr::video_get_settings(),
        reinterpret_cast<unsigned char *>(G2S::GetBG0CharPtr()),
        reinterpret_cast<uint16_t *>(G2S::GetBG0ScrPtr()),panels,
        static_cast<const unsigned char *>(font),button_panel,old_panels[0]);
}
}

extern "C" void pc_pause_settings_reset() {
    restore_panels();
    active = legacy = touch_was_down = false;
    nav_previous = nav_repeat = 0; nav_ticks = 0;
    stage_pause_action=-1;
}
extern "C" int pc_pause_settings_page() { return active ? static_cast<int>(menu.page)+1 : pc_wanted_settings_page(); }
extern "C" int pc_pause_settings_row() { return active ? static_cast<int>(menu.row) : pc_wanted_settings_row(); }
extern "C" int pc_pause_save_status() { return menu.save_status; }
extern "C" int pc_stage_options_take_action() {
    const int action=stage_options_action; stage_options_action=0; return action;
}
extern "C" int pc_stage_pause_take_action() {
    const int action=stage_pause_action;stage_pause_action=-1;return action;
}
extern "C" int pc_pause_settings_render() { if (!active) return 0; draw(); return 1; }

extern "C" int pc_pause_settings_update() {
    stage_pause_action=-1;
    uint16_t keys = 0;
    ntr_input_get_ui_keys(&keys);
    const auto edge = pc_pause::events(keys,nav_previous,nav_repeat,nav_ticks);
    const auto input = ntr::input_state();
    if(const char *path=std::getenv("SM64DS_PC_SETTINGS_NAV_TRACE")) {
        static FILE *trace=nullptr;
        if(!trace) {trace=std::fopen(path,"w");if(trace)std::fputs("tick,state,active,page,row,keys,edge,pad\n",trace);}
        if(trace && (active || keys || edge)) {
            std::fprintf(trace,"%llu,%u,%u,%u,%u,%u,%u,%u\n",
                static_cast<unsigned long long>(ntr::rt_frame()),unsigned(data_0209f1ec),
                unsigned(active),unsigned(menu.page),menu.row,unsigned(keys),unsigned(edge),
                unsigned(input.buttons));std::fflush(trace);
        }
    }
    const bool tapped = input.touch && !touch_was_down;
    touch_was_down = input.touch;
    if (data_0209f2c4 != 1) return 0;
    const unsigned state = data_0209f1ec;
    // Add ordinary button navigation to the recovered touch-oriented menu.
    if (!active && (state==1 || state==3 || state==13) && !data_0209f300) {
        legacy = false;
        const unsigned count = std::max(1u,static_cast<unsigned>(data_0209f2b4));
        if (edge & 0x40) data_0209f2e0 = static_cast<unsigned char>((data_0209f2e0+count-1)%count);
        if (edge & 0x80) data_0209f2e0 = static_cast<unsigned char>((data_0209f2e0+1)%count);
        if (edge & 0xc0) dScStage_c::UpdateMenuButtons(false);
        if (!input.touch && !data_0209f22c && (edge & (1|8))) {
            // Scope confirmation as a native touch on the focused button.
            // Retail hit tests also examine released stylus coordinates;
            // leaving those stale can activate a different row on pad A.
            if(edge & 8)data_0209f2e0=0;
            stage_pause_action=data_0209f2e0;
        }
        if (edge & 2) {
            data_0209f2e0 = 0;
            data_0209f22c = static_cast<unsigned char>(data_0208ee44<<3);
            data_0209f2c4 = 2;
            return 1;
        }
    }
    if (legacy) {
        if (state!=9 && state!=10) legacy=false;
        else if (edge & 2) back_to_pause();
        else if (state==10 && !input.touch) {
            if (edge & (0x40|0x80)) {
                stage_options_focus ^= 1;
                data_0209f2e0 = static_cast<unsigned char>(stage_options_focus);
                dScStage_c::PS_UpdateOptionsMenu();
            }
            if (edge & 1) queue_stage_options_action(stage_options_focus,true);
            else if (stage_options_focus==0 && (edge & 0x30))
                queue_stage_options_action(stage_options_focus,(edge & 0x10)!=0);
        }
        if(state==10) {
            stage_options_last_action=stage_options_action;
            trace_stage_options(keys,edge,input);
        }
        return 0;
    }
    if (!active && state==9) {
        active = true;
        menu.reset(keys);
        data_0209f248 = data_0209f1ec = 10;
        data_0209f22c = 0;
        draw();
        return 1;
    }
    if (!active) return 0;
    auto settings = ntr::video_get_settings();
    uint16_t menu_keys = keys;
    if (tapped && !ntr::video_minimap_editor_active() && input.touch_x>=8 && input.touch_x<248) {
        for (unsigned row=0;row<menu.rows();++row) {
            if (pc_pause::hit_row(input.touch_y,menu.rows())==static_cast<int>(row)) {
                menu.row = row;
                menu_keys |= menu.page==pc_pause::Page::Options || menu.page==pc_pause::Page::Save ? 1 : input.touch_x<128 ? 0x20 : 0x10;
            }
        }
    }
    const auto action = menu.update(menu_keys,settings);
    if (action==pc_pause::Action::SaveProgress) menu.save_status=SaveData::SaveCurrentFile()?1:-1;
    if (action==pc_pause::Action::Changed) ntr::video_set_settings(settings);
    if (action==pc_pause::Action::GameOptions) {
        active = false; legacy = true; restore_panels();
        stage_options_focus = 0;
        stage_options_action = 0;
        data_0209f248 = data_0209f1ec = 9;
        return 0; // Run the original Options initializer and update path.
    }
    if (action==pc_pause::Action::BackPause) { back_to_pause(); return 0; }
    draw();
    return 1;
}
