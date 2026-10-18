#include "hal/pc_pause_settings_state.h"
#include <array>
#include <cstdio>
#include <cstring>

int main() {
    using namespace pc_pause;
    Menu menu;
    ntr::VideoSettings settings;
    auto press = [&](uint16_t keys) { menu.update(0,settings); return menu.update(keys,settings); };
    menu.reset(1);
    menu.update(1,settings); // Held opening confirm cannot enter a page twice.
    if (menu.page!=Page::Options) return 1;
    press(1);
    if (menu.page!=Page::Display) return 2;
    press(0x10);
    if (settings.width!=1920 || settings.height!=1080) return 3;
    menu.update(0x10,settings);
    if (settings.width!=1920) return 4;
    press(0x20);
    if (settings.width!=1280) return 5;
    press(0x80); press(0x10);
    if (settings.target_fps!=120) return 6;
    press(0x10); if (settings.target_fps!=240) return 7;
    press(0x20); if (settings.target_fps!=120) return 8;
    press(0x80); press(1);
    if (settings.texture_quality!=ntr::TextureQuality::Percent75) return 9;
    // All six values wrap both directions and fit the original menu.
    const char *names[]={"ORIGINAL","75%","50%","25%","NO TEXTURES","UPSCALED"};
    press(0x20);
    for(unsigned i=0;i<6;++i) {
        if(static_cast<unsigned>(settings.texture_quality)!=i) return 33;
        char label[64];menu.line(2,settings,label,sizeof(label));
        if(!std::strstr(label,names[i]) || std::strlen(label)>30) return 34;
        press(1);
    }
    if(settings.texture_quality!=ntr::TextureQuality::Original) return 35;
    press(0x20);
    if(settings.texture_quality!=ntr::TextureQuality::Upscaled) return 36;
    press(0x100);
    if (menu.page!=Page::Visual || menu.row!=0) return 10;
    press(1); press(0x80); press(1); press(0x80); press(1);
    if (!settings.upscale_2d || settings.minimap || !settings.minimap_rotate_with_player) return 11;
    press(0x100);
    if(menu.page!=Page::Graphics || menu.rows()!=2)return 43;
    const uint8_t filters[]={2,4,8,16,0};
    for(uint8_t filter:filters) {
        if(press(0x10)!=Action::Changed || settings.anisotropic_filtering!=filter)return 44;
    }
    if(press(0x20)!=Action::Changed || settings.anisotropic_filtering!=16)return 45;
    char filter_label[64];menu.line(0,settings,filter_label,sizeof(filter_label));
    if(std::strcmp(filter_label,"ANISOTROPIC 16X"))return 46;
    press(0x80);press(1);
    if(settings.renderer!=ntr::VideoRenderer::OpenGLSoftware)return 56;
    menu.line(1,settings,filter_label,sizeof(filter_label));
    if(std::strcmp(filter_label,"RENDERER OPENGL CPU"))return 57;
    press(0x20);
    if(settings.renderer!=ntr::VideoRenderer::D3D11)return 58;
    press(0x100);
    if(menu.page!=Page::Map)return 47;
    settings.minimap_opacity=250; press(0x10);
    if (settings.minimap_opacity!=255) return 12;
    settings.minimap_opacity=3; press(0x20);
    if (settings.minimap_opacity!=0) return 13;
    char map_label[64];menu.line(0,settings,map_label,sizeof(map_label));
    if(std::strcmp(map_label,"OPACITY 0%"))return 51;
    press(0x80); settings.minimap_scale=495; settings.minimap_x=740; press(0x10);
    if (settings.minimap_scale!=500 || settings.minimap_x!=500) return 14;
    menu.line(1,settings,map_label,sizeof(map_label));
    if(std::strcmp(map_label,"MAP SIZE 50%"))return 52;
    settings.minimap_scale=100; press(0x20);
    if (settings.minimap_scale!=100) return 15;
    press(0x80); press(1);
    if (settings.minimap_hide_touch_target) return 16;
    menu.line(2,settings,map_label,sizeof(map_label));
    if(std::strcmp(map_label,"HIDE TOUCH TARGET OFF"))return 53;
    press(0x100);
    if(menu.page!=Page::Position)return 48;
    settings.minimap_x=0; press(0x20);
    if (settings.minimap_x!=0) return 17;
    settings.minimap_x=900; press(0x10);
    if (settings.minimap_x!=900) return 18;
    menu.line(0,settings,map_label,sizeof(map_label));
    if(std::strcmp(map_label,"MAP X POSITION 90%"))return 54;
    press(0x80); settings.minimap_y=5; press(0x20);
    if (settings.minimap_y!=0) return 19;
    menu.line(1,settings,map_label,sizeof(map_label));
    if(std::strcmp(map_label,"MAP Y POSITION 0%"))return 55;
    press(0x100);
    if(menu.page!=Page::Save || menu.rows()!=2 || press(0x10)!=Action::None || press(0x20)!=Action::None)return 37;
    char save_label[64]={};menu.line(0,settings,save_label,sizeof(save_label));
    if(std::strcmp(save_label,"SAVE CURRENT FILE"))return 42;
    if(press(1)!=Action::SaveProgress)return 38;
    if(menu.update(1,settings)!=Action::None)return 39; // Held A must not write repeatedly.
    press(0x80);if(press(1)!=Action::None || menu.page!=Page::Options)return 40;
    press(1);press(0x200);
    if(menu.page!=Page::Screens)return 41;
    const ntr::MinigameLayout layouts[]={ntr::MinigameLayout::LeftRight,ntr::MinigameLayout::RightLeft,ntr::MinigameLayout::TopBottom};
    for(auto layout:layouts) {if(press(1)!=Action::Changed || settings.minigame_layout!=layout)return 49;}
    if(press(0x20)!=Action::Changed || settings.minigame_layout!=ntr::MinigameLayout::RightLeft)return 50;
    press(0x100);
    if (menu.page!=Page::Display) return 20;
    press(0x200); if (menu.page!=Page::Screens) return 21;
    press(2); if (menu.page!=Page::Options) return 22;
    press(0x80);
    if (press(1)!=Action::GameOptions) return 23;
    press(0x80); if (press(1)!=Action::BackPause) return 24;

    menu.reset(0);
    menu.update(0x80,settings);
    for (int i=0;i<17;++i) menu.update(0x80,settings);
    if (menu.row!=1) return 25;
    menu.update(0x80,settings); if (menu.row!=2) return 26;
    for (int i=0;i<6;++i) menu.update(0x80,settings);
    if (menu.row!=0) return 27;

    std::array<unsigned char,0x4000> font;
    std::array<unsigned char,0x6020> chars;
    std::array<uint16_t,1024> text,panels;
    std::array<uint16_t,128> panel;
    for (size_t i=0;i<font.size();++i) font[i]=static_cast<unsigned char>(i*7);
    chars.fill(0xa5); panel.fill(0x1123);
    draw(menu,settings,chars.data(),text.data(),panels.data(),font.data(),panel.data(),0x3ff);
    if (chars[0x4000+0xa*64]!=font[0xa*32] ||
        chars[0x4000+0xa*64+32]!=font[0xa*32+0x400]) return 28;
    for (unsigned i=0x6000;i<chars.size();++i) if (chars[i]!=0xa5) return 29;
    if (panels[4*32]!=0x2123 || panels[9*32]!=0x1123 || text[0]!=0x2ff) return 30;
    for (int page=0;page<=7;++page) {
        menu.page=static_cast<Page>(page); menu.row=0;
        draw(menu,settings,chars.data(),text.data(),panels.data(),font.data(),panel.data(),0x3ff);
        for (auto entry:text) if (entry<0x200 || entry>0x2ff) return 31;
        char line[64];
        for (unsigned row=0;row<menu.rows();++row) {
            menu.line(row,settings,line,sizeof(line));
            if (std::strlen(line)>30) return 32;
        }
    }
    std::puts("PASS: native pause settings navigation, repeat, every option, bounds, font tiles and page layout");
}
