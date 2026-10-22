#include "hal/wanted_pause_settings.h"
#include "hal/wanted_settings_surface.h"
#include "dScMgBase_c.h"
#include "SaveData.h"
#include "ntr/input.h"
#include <cstdio>
extern "C" {
extern void *data_0209d6f8;
extern unsigned char data_0209d454;
void func_ov004_020b9220(char *);
}
namespace {
pc_pause::Menu menu;
pc_pause::WantedSurface surface;
bool active=false,touch_down=false;
unsigned char saved_layers=0;
void close() {
    surface.close();
    data_0209d454=saved_layers;
    active=false;
    ntr_input_set_menu_navigation(0);
}
}
extern "C" int pc_wanted_settings_open(void *raw) {
    auto *scene=static_cast<dScMgBase_c *>(raw);
    if(scene->actorID!=366 || !data_0209d6f8) return 0;
    uint16_t keys=0;ntr_input_get_ui_keys(&keys);
    menu.reset(keys);
    touch_down=ntr::input_state().touch;
    saved_layers=data_0209d454;
    surface.open(data_0209d6f8);
    active=true;
    ntr_input_set_menu_navigation(1);
    std::fprintf(stderr,"[Wanted PC settings] opened\n");
    return 1;
}
extern "C" int pc_wanted_settings_update(void *raw) {
    if(!active) return 0;
    auto *scene=static_cast<dScMgBase_c *>(raw);
    if(!scene->mMenuOpen) {close();return 0;}
    ntr_input_set_menu_navigation(1);
    uint16_t keys=0;ntr_input_get_ui_keys(&keys);
    const auto input=ntr::input_state();
    const bool tapped=input.touch && !touch_down;
    touch_down=input.touch;
    if(tapped) {
        if(input.touch_y>=168) keys|=input.touch_x<64?0x200:input.touch_x>=192?0x100:2;
        else for(unsigned row=0;row<menu.rows();++row)
            if(pc_pause::hit_row(input.touch_y,menu.rows())==static_cast<int>(row)) {
                menu.row=row;
                keys|=menu.page==pc_pause::Page::Options || menu.page==pc_pause::Page::Save?
                    1:input.touch_x<128?0x20:0x10;
            }
    }
    auto settings=ntr::video_get_settings();
    const auto action=menu.update(keys,settings);
    if(action==pc_pause::Action::Changed) {
        ntr::video_set_settings(settings);
        std::fprintf(stderr,"[Wanted PC settings] changed page=%u row=%u fps=%u layout=%u\n",
            unsigned(menu.page),menu.row,settings.target_fps,unsigned(settings.minigame_layout));
    }
    if(action==pc_pause::Action::SaveProgress) menu.save_status=SaveData::SaveCurrentFile()?1:-1;
    if(action==pc_pause::Action::GameOptions) {
        close();func_ov004_020b9220(reinterpret_cast<char *>(&scene->mTouchOptions));
        std::fprintf(stderr,"[Wanted PC settings] original game options\n");
    } else if(action==pc_pause::Action::BackPause) {
        close();std::fprintf(stderr,"[Wanted PC settings] back to pause\n");
    }
    return 1; // consume the closing key/touch before retail resume handling
}
extern "C" int pc_wanted_settings_render() {return active?1:0;}
extern "C" void pc_wanted_settings_prepare() {
    if(!active)return;
    data_0209d454=3;
    surface.draw(menu,ntr::video_get_settings());
}
extern "C" void pc_wanted_settings_reset() {if(active)close();}
extern "C" int pc_wanted_settings_page() {return active?int(menu.page)+1:0;}
extern "C" int pc_wanted_settings_row() {return active?int(menu.row):-1;}
