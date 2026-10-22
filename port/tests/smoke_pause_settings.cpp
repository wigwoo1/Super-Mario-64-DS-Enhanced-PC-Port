#include "hal/pc_pause_settings_state.h"
#include "hal/star_collection_policy.h"
#include "hal/touch_menu_navigation.h"
#include "hal/course_clear_input.h"
#include <array>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>

int main() {
    {
        pc_course_clear::State nav;
        auto step=[&](bool ready,uint16_t keys,unsigned selected=0,bool touch=false){return nav.poll(ready,keys,selected,touch);};
        step(false,1);if(step(true,1).activate!=-1)return 220;
        step(true,0);if(step(true,1,1).activate!=1 || step(true,1,1).activate!=-1)return 221;
        step(false,1);if(step(true,2).activate!=-1)return 222; // A/B mapping swap while held.
        step(true,0);if(step(true,2).activate!=2)return 223;
        step(true,0);if(step(true,0x40).selection!=2)return 224;
        step(true,0);if(step(true,0x80,2).selection!=0)return 225;
        step(true,0);if(step(true,1,0,true).activate!=-1)return 226; // Real touch wins.
        step(true,0);if(step(true,8,2).activate!=2)return 227; // Original Start confirm.
        step(false,0);if(step(true,0,99).selection!=0)return 228;
        step(true,0x80);
        for(int i=0;i<17;++i)if(step(true,0x80,1).selection!=1)return 229;
        if(step(true,0x80,1).selection!=2)return 230;
    }
    {
        pc_touch_menu::ReleaseLatch latch;latch.activate();
        if(latch.filter(1)!=0 || latch.filter(2)!=0 || latch.filter(0x81)!=0x80)return 210;
        if(latch.filter(0)!=0 || latch.blocked || latch.filter(1)!=1)return 211;
        pc_touch_menu::State nav;int owner=0;
        const ntr::MenuTarget items[]={{0,32,20,224,60},{1,64,80,192,112},{2,64,132,192,168}};
        auto poll=[&](int mode,uint16_t keys,bool touch=false,int x=0,int y=0) {
            return nav.poll(&owner,mode,keys,items,3,2,touch,x,y);
        };
        if(poll(1,1)!=-1 || poll(1,1)!=-1 || nav.focused!=0)return 200; // Opening A is consumed.
        poll(1,0);
        if(poll(1,0x80)!=-1 || nav.focused!=1)return 201;
        poll(1,0);
        if(poll(1,1)!=1 || poll(1,1)!=-1)return 202; // One action per press.
        if(poll(2,1)!=-1 || nav.focused!=0)return 203; // No carry into another menu.
        poll(2,0);
        if(poll(2,2)!=2 || poll(2,2)!=-1)return 204;
        poll(2,0);
        if(poll(2,1,true,128,96)!=-1 || nav.focused!=1 || nav.visible)return 205; // Mouse wins.
        poll(2,0);
        if(poll(2,1)!=1 || !nav.visible)return 206;
        poll(3,0);
        poll(3,0x80);
        for(int i=0;i<17;++i)poll(3,0x80);
        if(nav.focused!=1)return 207;
        poll(3,0x80);
        if(nav.focused!=2)return 208;
    }
    using namespace pc_pause;
    Menu menu;
    ntr::VideoSettings settings;
    if(settings.stay_in_stage_after_star) return 60;
    if(!pc_star::can_stay(true,178,0,0,1,0,4) ||
       !pc_star::can_stay(true,178,0,14,7,4,0)) return 61;
    for(int course:{-1,15,29}) if(pc_star::can_stay(true,178,0,course,1,0,4))return 62;
    for(unsigned mode:{1u,2u})if(pc_star::can_stay(true,178,mode,0,1,0,4))return 63;
    for(unsigned star:{0u,8u,15u})if(pc_star::can_stay(true,178,0,0,star,0,4))return 64;
    for(unsigned kind:{3u,5u,8u,9u})if(pc_star::can_stay(true,178,0,0,1,kind,4))return 65;
    if(pc_star::can_stay(false,178,0,0,1,0,4) ||
       pc_star::can_stay(true,179,0,0,1,0,4) ||
       pc_star::can_stay(true,178,0,0,1,0,9))return 66;
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
    press(0x80);
    if(menu.rows()!=4 || menu.row!=3)return 10;
    press(1);
    if(!settings.upscale_2d)return 11;
    press(0x100);
    if(menu.page!=Page::Graphics || menu.rows()!=3)return 43;
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
    press(0x80); // Render-distance selector.
    const char *distance_names[]={"MINIMUM","LOW","MEDIUM","ENTIRE MAP"};
    for(unsigned i=0;i<4;++i) {
        if(static_cast<unsigned>(settings.render_distance)!=i)return 79;
        char label[64];menu.line(2,settings,label,sizeof(label));
        if(!std::strstr(label,distance_names[i]))return 80;
        if(press(0x10)!=Action::Changed)return 81;
    }
    if(settings.render_distance!=ntr::RenderDistance::Minimum)return 82;
    if(press(0x20)!=Action::Changed || settings.render_distance!=ntr::RenderDistance::EntireMap)return 83;
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
    press(0x80);press(1);press(0x80);press(1);
    if(menu.rows()!=5 || settings.minimap || !settings.minimap_rotate_with_player)return 84;
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
    if(menu.page!=Page::Gameplay || menu.rows()!=5)return 67;
    char gameplay_label[64];menu.line(0,settings,gameplay_label,sizeof(gameplay_label));
    if(std::strcmp(gameplay_label,"STAY AFTER STAR OFF"))return 68;
    if(press(1)!=Action::Changed || !settings.stay_in_stage_after_star)return 69;
    menu.line(0,settings,gameplay_label,sizeof(gameplay_label));
    if(std::strcmp(gameplay_label,"STAY AFTER STAR ON"))return 70;
    if(press(0x20)!=Action::Changed || settings.stay_in_stage_after_star)return 71;
    press(0x80);
    for(unsigned i=1;i<=4;++i) if(press(1)!=Action::Changed || settings.camera_zoom!=i%4)return 73;
    if(press(0x20)!=Action::Changed || settings.camera_zoom!=3)return 74;
    menu.line(1,settings,gameplay_label,sizeof(gameplay_label));
    if(std::strcmp(gameplay_label,"CAMERA ZOOM 0.25X"))return 75;
    while(menu.row<3){press(0x80);}
    auto free_action=press(1);
    if(free_action!=Action::Changed)return 80;
    char camera_mode_label[64]; menu.line(3,settings,camera_mode_label,sizeof(camera_mode_label));
    if(settings.camera_mode!=1 || std::strcmp(camera_mode_label,"CAMERA FREE"))return 81;
    press(0x80);
    if(menu.row!=4 || press(1)!=Action::Changed || !settings.pause_music_rec_room)return 94;
    menu.line(4,settings,camera_mode_label,sizeof(camera_mode_label));
    if(std::strcmp(camera_mode_label,"PAUSE MUSIC REC ROOM"))return 95;
    if(press(0x20)!=Action::Changed || settings.pause_music_rec_room)return 96;
    while(menu.row>2)press(0x40);
    while(menu.row<2)press(0x80);
    auto run_action=press(1);
    if(run_action!=Action::Changed || !settings.always_run)return 76;
    menu.line(2,settings,gameplay_label,sizeof(gameplay_label));
    if(std::strcmp(gameplay_label,"ALWAYS RUN ON"))return 77;
    if(press(0x20)!=Action::Changed || settings.always_run)return 78;
    press(0x200);
    if(menu.page!=Page::Camera || menu.rows()!=4)return 82;
    char free_label[64];menu.line(0,settings,free_label,sizeof(free_label));
    if(std::strcmp(free_label,"SENSITIVITY 100%"))return 83;
    if(press(1)!=Action::Changed || settings.camera_sensitivity!=125)return 84;
    for(unsigned i=0;i<12;++i)press(0x10);
    if(settings.camera_sensitivity!=300)return 85;
    for(unsigned i=0;i<15;++i)press(0x20);
    if(settings.camera_sensitivity!=25)return 86;
    press(0x80);if(press(1)!=Action::Changed || !settings.camera_invert_x)return 87;
    press(0x80);if(press(1)!=Action::Changed || !settings.camera_invert_y)return 88;
    press(0x80);if(press(1)!=Action::Changed || settings.camera_acceleration!=2)return 89;
    if(press(1)!=Action::Changed || settings.camera_acceleration!=0)return 90;
    if(press(0x20)!=Action::Changed || settings.camera_acceleration!=2)return 91;
    menu.line(3,settings,free_label,sizeof(free_label));
    if(std::strcmp(free_label,"ACCELERATION SMOOTH"))return 92;
    menu.page=Page::Controls;menu.row=0;
    if(menu.page!=Page::Controls || menu.rows()!=5)return 100;
    if(press(1)!=Action::Changed || settings.left_stick_deadzone!=26)return 101;
    press(0x80);if(press(1)!=Action::Changed || settings.right_stick_deadzone!=22)return 102;
    menu.line(0,settings,free_label,sizeof(free_label));
    if(std::strcmp(free_label,"LEFT DEADZONE 26%"))return 103;
    menu.line(1,settings,free_label,sizeof(free_label));
    if(std::strcmp(free_label,"RIGHT DEADZONE 22%"))return 104;
    if(press(2)!=Action::BackPause)return 105;
    menu.page=Page::Camera;menu.row=3;
    press(0x200);
    if(menu.page!=Page::Screens)return 41;
    const ntr::MinigameLayout layouts[]={ntr::MinigameLayout::LeftRight,ntr::MinigameLayout::RightLeft,ntr::MinigameLayout::TopBottom};
    for(auto layout:layouts) {if(press(1)!=Action::Changed || settings.minigame_layout!=layout)return 49;}
    if(press(0x20)!=Action::Changed || settings.minigame_layout!=ntr::MinigameLayout::RightLeft)return 50;
    press(0x100);
    if(menu.page!=Page::Camera)return 93;
    press(0x100);
    if(menu.page!=Page::Gameplay)return 72;
    press(0x100);
    if (menu.page!=Page::Display) return 20;
    press(0x200); if (menu.page!=Page::Gameplay) return 21;
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
    {
        Menu native_menu;native_menu.page=Page::Display;
        auto native_settings=settings;native_settings.width=256;native_settings.height=192;
        char label[64];native_menu.line(0,native_settings,label,sizeof(label));
        if(std::strcmp(label,"RESOLUTION NATIVE DS"))return 97;
        native_menu.reset(0);native_menu.page=Page::Display;
        if(native_menu.update(0x10,native_settings)!=Action::Changed || native_settings.width!=1280)return 98;
        native_menu.update(0,native_settings);
        if(native_menu.update(0x20,native_settings)!=Action::Changed || native_settings.width!=256)return 99;
    }
    std::array<unsigned char,0x6020> chars;
    std::array<uint16_t,1024> text,panels;
    std::array<uint16_t,128> panel;
    for (size_t i=0;i<font.size();++i) font[i]=static_cast<unsigned char>(i*7);
    chars.fill(0xa5); panel.fill(0x1123);
    draw(menu,settings,chars.data(),text.data(),panels.data(),font.data(),panel.data(),0x3ff);
    bool ink_visible=false;
    for(unsigned i=0x4000;i<0x6000;++i)ink_visible|=chars[i]!=0;
    if(!ink_visible)return 28;
    for (unsigned i=0x6000;i<chars.size();++i) if (chars[i]!=0xa5) return 29;
    if (panels[3*32]!=0x2123 || panels[6*32]!=0x1123 || text[0]!=0x2ff) return 30;
    // Native pause layouts put their first button on different tile rows.
    // Capture its declared position, then keep both original bevel edges.
    std::array<uint16_t,1024> original_map;
    for(unsigned offset:{0x60u,0x80u}) {
        original_map.fill(0x3ff);
        for(unsigned y=0;y<4;++y)for(unsigned x=0;x<32;++x)
            original_map[offset+y*32+x]=static_cast<uint16_t>(0x100+y);
        if(!cache_button_panel(original_map.data(),offset,panel.data()))return 88;
        draw(menu,settings,chars.data(),text.data(),panels.data(),font.data(),panel.data(),0x3ff);
        if(panels[3*32]!=0x2100 || panels[4*32]!=0x2103 || panels[5*32]!=0x3ff)return 89;
    }
    if(cache_button_panel(original_map.data(),1,panel.data()) ||
       cache_button_panel(original_map.data(),928,panel.data()))return 90;
    for (int page=0;page<=10;++page) {
        menu.page=static_cast<Page>(page); menu.row=0;
        draw(menu,settings,chars.data(),text.data(),panels.data(),font.data(),panel.data(),0x3ff);
        for (auto entry:text) if (entry<0x200 || entry>0x2ff) return 31;
        char line[64];
        for (unsigned row=0;row<menu.rows();++row) {
            menu.line(row,settings,line,sizeof(line));
            if (std::strlen(line)>30) return 32;
            const unsigned y=row_top/8+row*row_height/8;
            int left=256,right=-1,top=16,bottom=-1;
            for(unsigned py=0;py<16;++py)for(unsigned x=0;x<256;++x) {
                const unsigned tile=text[(y+py/8)*32+x/8]&0x3ff;
                const auto pair=chars[tile*32+(py%8)*4+(x%8)/2];
                if((pair>>((x&1)*4))&15) {
                    left=std::min(left,int(x));right=std::max(right,int(x));
                    top=std::min(top,int(py));bottom=std::max(bottom,int(py));
                }
            }
            if(right<left || std::abs(left+right-255)>1 || std::abs(top+bottom-15)>1)return 85;
            if(hit_row(row_top+row*row_height,menu.rows())!=static_cast<int>(row) ||
               hit_row(row_top+row*row_height+15,menu.rows())!=static_cast<int>(row) ||
               hit_row(row_top+row*row_height+16,menu.rows())!=-1)return 86;
        }
        if(hit_row(row_top-1,menu.rows())!=-1 || hit_row(row_top+menu.rows()*row_height,menu.rows())!=-1)return 87;
    }
    // The title font/metrics are copied before its scene frees the resource.
    std::array<unsigned char,0x4f> widths; widths.fill(6);
    const auto original_font=font;
    if(cache_options_font(nullptr,0,widths.data()) || cache_options_font(font.data(),0x101,widths.data()))return 91;
    if(!cache_options_font(font.data(),0,widths.data()) || !options_font_ready())return 92;
    menu.page=Page::Display;
    draw(menu,settings,chars.data(),text.data(),panels.data(),font.data(),panel.data(),0x3ff);
    const auto proportional_chars=chars;const auto proportional_text=text;
    for(int page=0;page<=10;++page) {
        menu.page=static_cast<Page>(page);menu.row=0;
        draw(menu,settings,chars.data(),text.data(),panels.data(),font.data(),panel.data(),0x3ff);
        for(unsigned row=0;row<menu.rows();++row) {
            const unsigned y=row_top/8+row*row_height/8;
            int left=256,right=-1,top=16,bottom=-1;
            for(unsigned py=0;py<16;++py)for(unsigned x=0;x<256;++x) {
                const unsigned tile=text[(y+py/8)*32+x/8]&0x3ff;
                const auto pair=chars[tile*32+(py%8)*4+(x%8)/2];
                if((pair>>((x&1)*4))&15) {
                    left=std::min(left,int(x));right=std::max(right,int(x));
                    top=std::min(top,int(py));bottom=std::max(bottom,int(py));
                }
            }
            if(right<left || std::abs(left+right-255)>1 || std::abs(top+bottom-15)>1)return 96;
        }
    }
    menu.page=Page::Display;
    widths.fill(1);font.fill(0); // source lifetime and metrics changes are isolated
    // The destination scene's ink palette is independent of the cached source.
    draw(menu,settings,chars.data(),text.data(),panels.data(),original_font.data(),panel.data(),0x3ff);
    if(chars!=proportional_chars || text!=proportional_text)return 93;
    widths[0]=0;
    if(cache_options_font(font.data(),0,widths.data()) || !options_font_ready())return 94;
    clear_options_font();
    if(options_font_ready())return 95;
    std::puts("PASS: native pause settings navigation, font cache lifetime, repeat, every option, bounds and page layout");
}
