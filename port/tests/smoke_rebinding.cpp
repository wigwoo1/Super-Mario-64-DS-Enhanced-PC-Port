#include "hal/pc_pause_settings_state.h"
#include "ntr/input.h"
#include <cstdio>
#include <cstdlib>

int main() {
    using namespace pc_pause;
    for(unsigned state:{1u,3u,13u}) {
        const unsigned y=state==1?72:state==3?64:80;
        if(controller_modes_hit(state,7,y) || !controller_modes_hit(state,8,y) ||
           !controller_modes_hit(state,247,y+31) || controller_modes_hit(state,248,y) ||
           controller_modes_hit(state,128,y-1) || controller_modes_hit(state,128,y+32))return 24;
    }
    ntr::VideoSettings settings;Menu menu;menu.page=Page::Controls;menu.row=3;
    auto press=[&](unsigned key){menu.update(0,settings);return menu.update(uint16_t(key),settings);};
    press(1);if(menu.page!=Page::Gamepad)return 1;
    menu.row=1;press(1);
    if(menu.capture!=1)return 2;
    menu.binding_sample(0x1000,false,settings);
    if(menu.capture!=1 || settings.bindings.pad[1]!=0x1000)return 3;
    menu.binding_sample(0,false,settings);
    if(menu.binding_sample(0x3000,false,settings)!=Action::None || menu.capture!=2)return 4;
    if(menu.binding_sample(0x4000,false,settings)!=Action::Changed || menu.capture)return 5;
    if(settings.bindings.pad[1]!=0x4000 || settings.bindings.pad[11]!=0x1000)return 6;
    menu.row=0;press(1);menu.binding_sample(0,true,settings);
    if(menu.capture || settings.bindings.pad[0]!=0x2000)return 7;
    press(0x100);press(0x100);if(menu.binding_bank!=2 || menu.rows()!=4)return 8;
    press(2);if(menu.page!=Page::Controls)return 9;
    menu.row=2;press(1);if(menu.page!=Page::Keyboard)return 10;
    menu.row=1;press(1);menu.binding_sample(0,false,settings);
    menu.binding_sample('J',false,settings);
    if(settings.bindings.keys[1]!='J')return 11;
    menu.binding_bank=2;menu.row=2;press(1);menu.binding_sample(0,false,settings);
    menu.binding_sample('K',false,settings);
    settings.left_stick_deadzone=0;settings.right_stick_deadzone=50;
    ntr::video_set_settings(settings);
    if(!ntr::video_save_settings("smoke-rebinding.ini"))return 12;
    ntr::video_set_settings(ntr::VideoSettings{});
    if(!ntr::video_load_settings("smoke-rebinding.ini"))return 13;
    const auto loaded=ntr::video_get_settings();
    if(loaded.bindings.keys!=settings.bindings.keys || loaded.bindings.pad!=settings.bindings.pad || loaded.left_stick_deadzone!=0 || loaded.right_stick_deadzone!=50)return 14;
    std::remove("smoke-rebinding.ini");
    // Invalid serialized values must not truncate into another valid binding.
    if(FILE *f=std::fopen("smoke-rebinding.ini","w")) {
        std::fputs("key_bind_1=65568\npad_bind_1=12288\nkey_bind_14=74\nleft_stick_deadzone=999\nright_stick_deadzone=0\n",f);std::fclose(f);
    } else return 20;
    if(!ntr::video_load_settings("smoke-rebinding.ini"))return 21;
    auto invalid=ntr::video_get_settings();std::remove("smoke-rebinding.ini");
    if(invalid.bindings.keys[1]!=160 || invalid.bindings.pad[1]!=0x1000 || invalid.left_stick_deadzone!=50 || invalid.right_stick_deadzone!=0)return 22;
    std::array<bool,256> no_keys{};
    auto trigger=settings.bindings;trigger.pad[1]=0x10000;
    if(ntr::mapped_buttons(trigger,no_keys,0x10000)!=2 || ntr::mapped_buttons(trigger,no_keys,0))return 23;
    ntr::video_set_settings(settings);
#ifdef _WIN32
    _putenv_s("SM64DS_PC_AUTOMATION_ONLY","1");
    _putenv_s("SM64DS_PC_TEST_PAD_BUTTONS","1:4000");
    _putenv_s("SM64DS_PC_TEST_HOST_KEYS","8:4b,15:4a");
    ntr_input_set_menu_navigation(0);
    for(unsigned poll=1;poll<=20;++poll) {
        ntr::input_poll();unsigned short ui=0;ntr_input_get_ui_keys(&ui);
        const unsigned expected=(poll<6 || (poll>=15 && poll<20))?2:0;
        if(ntr::input_keyinput()!=(0xfff^expected))return 15;
        if(poll==1 && (ui!=0x400 || ntr::input_binding_candidate(true)!=0x4000))return 16;
        if(ntr::input_take_camera_zoom()!=(poll==8))return 17;
    }
#endif
    menu.page=Page::Controls;menu.row=4;press(1);
    if(!menu.reset_confirm || settings.bindings.keys[1]!='J')return 18;
    press(1);
    if(settings.bindings.keys!=ntr::InputBindings{}.keys || settings.bindings.pad!=ntr::InputBindings{}.pad || settings.left_stick_deadzone!=24 || settings.right_stick_deadzone!=20)return 19;
    std::puts("PASS rebinding capture/release/cancel/conflict swap, paging, defaults, persistence, real keyboard/pad dispatch and fixed UI navigation");
}
