#include "hal/pc_pause_settings_state.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace pc_pause {
static constexpr Page pages[]={Page::Display,Page::Graphics,Page::Map,
    Page::Position,Page::Save,Page::Screens,Page::Camera,Page::Gameplay};
static constexpr unsigned page_count=sizeof(pages)/sizeof(pages[0]);
uint16_t events(uint16_t keys, uint16_t &previous, uint16_t &repeat, unsigned &ticks) {
    uint16_t pressed = keys & ~previous;
    previous = keys;
    uint16_t direction = keys & 0xf0;
    if ((direction & 0xc0) == 0xc0) direction &= ~0xc0;
    if ((direction & 0x30) == 0x30) direction &= ~0x30;
    direction = direction & 0xc0 ? direction & 0xc0 : direction & 0x30;
    pressed = static_cast<uint16_t>((pressed & ~0xf0) | (pressed & direction));
    if (direction != repeat) { repeat = direction; ticks = 0; }
    else if (direction && ++ticks >= 18 && (ticks - 18) % 6 == 0) pressed |= direction;
    return pressed;
}
void Menu::reset(uint16_t held) { *this = Menu{}; previous = held; }
unsigned Menu::rows() const {
    if(page==Page::Display) return 4;
    if(page==Page::Map) return 5;
    if(page==Page::Position || page==Page::Save) return 2;
    if(page==Page::Graphics) return 3;
    if(page==Page::Screens) return 1;
    if(page==Page::Gameplay)return 5;
    if(page==Page::Controls)return 5;
    if(binding_page())return std::min(5u,ntr::binding_count-binding_bank*5);
    return page==Page::Camera ? 4 : 3;
}

Action Menu::update(uint16_t keys, ntr::VideoSettings &s) {
    if(capture)return Action::None;
    const auto edge = events(keys, previous, repeat, ticks);
    if(binding_page()) {
        if(edge&(2|8)){page=Page::Controls;row=0;return Action::None;}
        if(edge&0x300){binding_bank=(binding_bank+(edge&0x100?1:2))%3;row=0;return Action::None;}
        if(edge&0x40)row=(row+rows()-1)%rows();
        if(edge&0x80)row=(row+1)%rows();
        if(edge&1){capture=1;capture_ticks=0;}
        return Action::None;
    }
    if(page==Page::Controls && row==4 && reset_confirm && (edge&(2|8))) {reset_confirm=false;return Action::None;}
    if(edge&0xf0)reset_confirm=false;
    if (edge & (2 | 8)) {
        if(page==Page::Controls)return Action::BackPause;
        if (page == Page::Options) return Action::BackPause;
        page = Page::Options; row = 0; return Action::None;
    }
    if (edge & 0x40) row = (row + rows() - 1) % rows();
    if (edge & 0x80) row = (row + 1) % rows();
    if (page == Page::Options) {
        if (!(edge & 1)) return Action::None;
        if (row == 1) return Action::GameOptions;
        if (row == 2) return Action::BackPause;
        page = Page::Display; row = 0; return Action::None;
    }
    if (edge & 0x300) {
        if(page==Page::Controls)return Action::None;
        unsigned current=0;
        while(current<page_count && pages[current]!=page) ++current;
        page=pages[(current+(edge & 0x100 ? 1 : page_count-1))%page_count];
        row = 0; return Action::None;
    }
    if(page==Page::Save) {
        if(!(edge&1))return Action::None;
        if(row==0)return Action::SaveProgress;
        page=Page::Options;row=0;return Action::None;
    }
    const int direction = edge & (0x10 | 1) ? 1 : edge & 0x20 ? -1 : 0;
    if (!direction) return Action::None;
    auto step = [direction](int value, int amount, int low, int high) {
        return std::max(low, std::min(high, value + direction * amount));
    };
    switch (page) {
    case Page::Display:
        if (row == 0) ntr::video_cycle_resolution(s,direction);
        if (row == 1) s.target_fps = direction > 0
            ? (s.target_fps == 60 ? 120 : s.target_fps == 120 ? 240 : 60)
            : (s.target_fps == 60 ? 240 : s.target_fps == 240 ? 120 : 60);
        if (row == 2) s.texture_quality = ntr::texture_cycle_quality(s.texture_quality,direction);
        if (row == 3) s.upscale_2d = !s.upscale_2d;
        break;
    case Page::Visual:
        if (row == 0) s.upscale_2d = !s.upscale_2d;
        if (row == 1) s.minimap = !s.minimap;
        if (row == 2) s.minimap_rotate_with_player = !s.minimap_rotate_with_player;
        break;
    case Page::Graphics: {
        if(row==1) {
            s.renderer=s.renderer==ntr::VideoRenderer::D3D11?ntr::VideoRenderer::OpenGLSoftware:ntr::VideoRenderer::D3D11;
            break;
        }
        if(row==2) {
            s.render_distance=static_cast<ntr::RenderDistance>((static_cast<unsigned>(s.render_distance)+(direction>0?1:3))%4);
            break;
        }
        static constexpr uint8_t levels[]={0,2,4,8,16};
        unsigned current=0;
        while(current<4 && levels[current]!=s.anisotropic_filtering) ++current;
        s.anisotropic_filtering=levels[(current+(direction>0?1:4))%5];
        break;
    }
    case Page::Map:
        if (row == 0) s.minimap_opacity = static_cast<uint8_t>(step(s.minimap_opacity, 16, 0, 255));
        if (row == 1) s.minimap_scale = static_cast<uint16_t>(step(s.minimap_scale, 10, 100, 500));
        if (row == 2) s.minimap_hide_touch_target = !s.minimap_hide_touch_target;
        if (row == 3) s.minimap = !s.minimap;
        if (row == 4) s.minimap_rotate_with_player = !s.minimap_rotate_with_player;
        break;
    case Page::Screens:
        s.minigame_layout=static_cast<ntr::MinigameLayout>((static_cast<unsigned>(s.minigame_layout)+(direction>0?1:2))%3);
        break;
    case Page::Gameplay:
        if(row==0) s.stay_in_stage_after_star = !s.stay_in_stage_after_star;
        else if(row==1) s.camera_zoom=static_cast<uint8_t>((s.camera_zoom+(direction>0?1:3))%4);
        else if(row==2) s.always_run=!s.always_run;
        else if(row==3)s.camera_mode^=1;
        else s.pause_music_rec_room=!s.pause_music_rec_room;
        break;
    case Page::Camera:
        if(row==0)s.camera_sensitivity=static_cast<uint16_t>(step(s.camera_sensitivity,25,25,300));
        else if(row==1)s.camera_invert_x=!s.camera_invert_x;
        else if(row==2)s.camera_invert_y=!s.camera_invert_y;
        else s.camera_acceleration=static_cast<uint8_t>((s.camera_acceleration+(direction>0?1:2))%3);
        break;
    case Page::Controls:
        if(row==2 || row==3) {page=row==2?Page::Keyboard:Page::Gamepad;binding_bank=row=0;return Action::None;}
        if(row==4) {
            if(!(edge&1))return Action::None;
            if(!reset_confirm){reset_confirm=true;return Action::None;}
            reset_confirm=false;s.bindings=ntr::InputBindings{};s.left_stick_deadzone=24;s.right_stick_deadzone=20;break;
        }
        if(row==0)s.left_stick_deadzone=static_cast<uint8_t>(step(s.left_stick_deadzone,2,0,50));
        else s.right_stick_deadzone=static_cast<uint8_t>(step(s.right_stick_deadzone,2,0,50));
        break;
    case Page::Position:
        if (row == 0) s.minimap_x = static_cast<uint16_t>(step(s.minimap_x, 10, 0, 1000));
        if (row == 1) s.minimap_y = static_cast<uint16_t>(step(s.minimap_y, 10, 0, 1000));
        break;
    default: break;
    }
    s.minimap_x = std::min<uint16_t>(s.minimap_x, 1000);
    s.minimap_y = std::min<uint16_t>(s.minimap_y, 1000);
    return Action::Changed;
}

Action Menu::binding_sample(uint32_t value,bool cancel,ntr::VideoSettings &s) {
    if(!capture)return Action::None;
    if(cancel || ++capture_ticks>600) {capture=0;previous=0xffff;return Action::None;}
    if(capture==1){if(!value)capture=2;return Action::None;}
    const unsigned index=binding_bank*5+row;
    if(page==Page::Gamepad) {
        if(!ntr::valid_pad_binding(value))return Action::None;
        ntr::swap_binding(s.bindings.pad,index,value);
    } else {
        if(!ntr::valid_key_binding(value))return Action::None;
        ntr::swap_binding(s.bindings.keys,index,static_cast<uint16_t>(value));
    }
    capture=0;previous=0xffff;return Action::Changed;
}

void Menu::line(unsigned i, const ntr::VideoSettings &s, char *out, size_t size) const {
    const char *on = "ON", *off = "OFF";
    switch (page) {
    case Page::Options: std::snprintf(out,size,"%s", i==0?"PC SETTINGS":i==1?"GAME OPTIONS":"BACK"); break;
    case Page::Display:
        if (i==0) {
            if(s.width==256 && s.height==192)std::snprintf(out,size,"RESOLUTION NATIVE DS");
            else std::snprintf(out,size,"RESOLUTION %uX%u",s.width,s.height);
        }
        if (i==1) std::snprintf(out,size,"FRAME RATE %u FPS",s.target_fps);
        if (i==2) std::snprintf(out,size,"TEXTURES %s%s",ntr::texture_quality_name(s.texture_quality),
            s.texture_quality==ntr::TextureQuality::Upscaled?" - NO PACK":"");
        if (i==3) std::snprintf(out,size,"2D SCALING %s",s.upscale_2d?"CRISP":"SMOOTH");
        break;
    case Page::Visual:
        if (i==0) std::snprintf(out,size,"2D SCALING %s",s.upscale_2d?"CRISP":"SMOOTH");
        if (i==1) std::snprintf(out,size,"MINIMAP %s",s.minimap?on:off);
        if (i==2) std::snprintf(out,size,"MAP ROTATION %s",s.minimap_rotate_with_player?"CAMERA":"UPRIGHT");
        break;
    case Page::Graphics:
        if(i==1) {std::snprintf(out,size,"RENDERER %s",s.renderer==ntr::VideoRenderer::OpenGLSoftware?"OPENGL CPU":"D3D11");break;}
        if(i==2) { static const char *names[]={"MINIMUM","LOW","MEDIUM","ENTIRE MAP"};
            std::snprintf(out,size,"RENDER DISTANCE %s",names[static_cast<unsigned>(s.render_distance)]); break; }
        if(i==0) std::snprintf(out,size,"ANISOTROPIC %s",s.anisotropic_filtering==0?"OFF":
            s.anisotropic_filtering==2?"2X":s.anisotropic_filtering==4?"4X":
            s.anisotropic_filtering==8?"8X":"16X");
        break;
    case Page::Map:
        if (i==0) std::snprintf(out,size,"OPACITY %u%%",(unsigned(s.minimap_opacity)*100+127)/255);
        if (i==1) std::snprintf(out,size,"MAP SIZE %u%%",s.minimap_scale/10);
        if (i==2) std::snprintf(out,size,"HIDE TOUCH TARGET %s",s.minimap_hide_touch_target?on:off);
        if (i==3) std::snprintf(out,size,"MINIMAP %s",s.minimap?on:off);
        if (i==4) std::snprintf(out,size,"MAP ROTATION %s",s.minimap_rotate_with_player?"CAMERA":"UPRIGHT");
        break;
    case Page::Position:
        std::snprintf(out,size,"MAP %s POSITION %u%%",i==0?"X":"Y",
            (i==0?s.minimap_x:s.minimap_y)/10); break;
    case Page::Save:
        std::snprintf(out,size,"%s",i==0?"SAVE CURRENT FILE":"BACK");break;
    case Page::Screens:
        std::snprintf(out,size,"MINIGAME VIEW %s",ntr::screen_layout_name(s.minigame_layout));break;
    case Page::Gameplay:
        if(i==0) std::snprintf(out,size,"STAY AFTER STAR %s",s.stay_in_stage_after_star?on:off);
        else if(i==1) std::snprintf(out,size,"CAMERA ZOOM %s",s.camera_zoom==1?"0.75X":s.camera_zoom==2?"0.5X":s.camera_zoom==3?"0.25X":"ORIGINAL");
        else if(i==2) std::snprintf(out,size,"ALWAYS RUN %s",s.always_run?on:off);
        else if(i==3)std::snprintf(out,size,"CAMERA %s",s.camera_mode?"FREE":"PRESET");
        else std::snprintf(out,size,"PAUSE MUSIC %s",s.pause_music_rec_room?"REC ROOM":"ORIGINAL");break;
    case Page::Camera:
        if(i==0)std::snprintf(out,size,"SENSITIVITY %u%%",unsigned(s.camera_sensitivity));
        else if(i==1)std::snprintf(out,size,"INVERT X %s",s.camera_invert_x?on:off);
        else if(i==2)std::snprintf(out,size,"INVERT Y %s",s.camera_invert_y?on:off);
        else std::snprintf(out,size,"ACCELERATION %s",s.camera_acceleration==0?"OFF":s.camera_acceleration==2?"SMOOTH":"GENTLE");
        break;
    case Page::Keyboard: case Page::Gamepad: {
        if(capture && i==row){std::snprintf(out,size,"%s",capture==1?"RELEASE BUTTONS":page==Page::Keyboard?"PRESS NEW KEY":"PRESS NEW BUTTON");break;}
        const unsigned index=binding_bank*5+i;char value[24];
        ntr::binding_name(page==Page::Gamepad,page==Page::Gamepad?s.bindings.pad[index]:s.bindings.keys[index],value,sizeof(value));
        std::snprintf(out,size,"%s  %s",ntr::binding_action(index),value);break;
    }
    case Page::Controls:
        if(i==2){std::snprintf(out,size,"KEYBOARD BINDINGS");break;}
        if(i==3){std::snprintf(out,size,"CONTROLLER BINDINGS");break;}
        if(i==4){std::snprintf(out,size,"%s",reset_confirm?"CONFIRM RESET DEFAULTS":"RESET CONTROL DEFAULTS");break;}
        if(i==0)std::snprintf(out,size,"LEFT DEADZONE %u%%",unsigned(s.left_stick_deadzone));
        else std::snprintf(out,size,"RIGHT DEADZONE %u%%",unsigned(s.right_stick_deadzone));
        break;
    }
}

static unsigned char options_glyphs[0x4f*64], options_widths[0x4f];
static bool have_options_font=false;
void clear_options_font() { have_options_font=false; }
bool options_font_ready() { return have_options_font; }
bool cache_options_font(const unsigned char *font,unsigned base_tile,const unsigned char *widths) {
    if(!font || !widths || base_tile>0x100)return false;
    for(unsigned c=0;c<0x4f;++c)if(widths[c]<1 || widths[c]>8)return false;
    for(unsigned c=0;c<0x4f;++c) {
        const unsigned offset=(base_tile+(c&31)+(c>>5)*64)*32;
        std::memcpy(options_glyphs+c*64,font+offset,32);
        std::memcpy(options_glyphs+c*64+32,font+offset+0x400,32);
        options_widths[c]=c==0x4d?4:widths[c];
    }
    std::memset(options_glyphs+0x4d*64,0,64); // native title skips the space tile
    have_options_font=true;return true;
}
static unsigned glyph(char c) {
    if (c>='0' && c<='9') return c-'0';
    if (c>='A' && c<='Z') return c-'A'+0x0a;
    if (c>='a' && c<='z') return c-'a'+0x2d;
    return c=='.' ? 0x50 : c=='%' ? 0x4f : c=='-' ? 0x47 : c==':' ? 0x4a : c=='/' ? 0x4e : 0x4d;
}
bool draw_controls_label(unsigned char *chars,uint16_t *text,const unsigned char *font,unsigned row) {
    if(!chars || !text || !font || !have_options_font || row>=31)return false;
    bool used[256]{};
    for(unsigned i=0;i<1024;++i)if(i/32!=row && i/32!=row+1) {
        const unsigned t=text[i]&0x3ff;
        if(t>=0x200 && t<0x300)used[t-0x200]=true;
    }
    // The last tile is Message's transparent tile. Only borrow pairs which
    // no other visible label references; custom settings own this same pool.
    unsigned slots[127],count=0;
    for(unsigned i=0;i<254;i+=2)if(!used[i]&&!used[i+1])slots[count++]=0x200+i;
    unsigned char raster[16][128]{};
    constexpr char label[]="CONTROLS";
    unsigned width=0;
    for(char c:label)if(c)width+=options_widths[glyph(c)];
    unsigned cursor=(256-width)/2;
    unsigned char ink=1;
    for(unsigned i=0;i<64;++i)if(font[i]){ink=font[i]&15?font[i]&15:font[i]>>4;break;}
    for(char c:label)if(c) {
        const unsigned g=glyph(c);
        for(unsigned y=0;y<16;++y)for(unsigned x=0;x<8;++x)
            if((options_glyphs[g*64+y*4+x/2]>>((x&1)*4))&15) {
                const unsigned dx=cursor+x;
                if(dx<256)raster[y][dx/2]|=ink<<((dx&1)*4);
            }
        cursor+=options_widths[g];
    }
    unsigned required=0;
    for(unsigned x=0;x<32;++x) {
        bool visible=false;
        for(unsigned y=0;y<16;++y)for(unsigned b=0;b<4;++b)visible|=raster[y][x*4+b]!=0;
        required+=visible;
    }
    if(required>count)return false; // failure must leave the retail label intact
    std::fill(text+row*32,text+(row+2)*32,0x2ff);
    unsigned next=0;
    for(unsigned x=0;x<32;++x) {
        unsigned char cell[64];bool visible=false;
        for(unsigned y=0;y<16;++y)for(unsigned b=0;b<4;++b) {
            cell[y*4+b]=raster[y][x*4+b];visible|=cell[y*4+b]!=0;
        }
        if(!visible)continue;
        const unsigned tile=slots[next++];
        std::memcpy(chars+tile*32,cell,64);
        text[row*32+x]=static_cast<uint16_t>(tile);
        text[(row+1)*32+x]=static_cast<uint16_t>(tile+1);
    }
    return true;
}
void draw(const Menu &m, const ntr::VideoSettings &s, unsigned char *chars,
          uint16_t *text, uint16_t *panels, const unsigned char *font,
          const uint16_t *panel, uint16_t blank_panel, bool touch_navigation) {
    // Dynamic glyph area is the same 0x200..0x2ff tiles used by Message.
    std::memset(chars+0x4000,0,0x2000);
    for (unsigned c=0;c<=0x4e;++c) {
        const unsigned offset = ((c&0x1f)+((c&0xe0)<<1))*32;
        std::memcpy(chars+0x4000+c*64,font+offset,32);
        std::memcpy(chars+0x4000+c*64+32,font+offset+0x400,32);
    }
    // The retail Latin font has no percent glyph. Use the same 8x16 tile
    // format and ink palette as its zero glyph for the three new percentages.
    static const unsigned char percent[16]={0,0x60,0x90,0x90,0x60,0x02,0x04,0x08,
        0x10,0x20,0x46,0x09,0x09,0x06,0,0};
    unsigned char ink=1;
    const auto *zero=chars+0x4000;
    for(unsigned i=0;i<64;++i) if(zero[i]) {ink=zero[i]&15?zero[i]&15:zero[i]>>4;break;}
    if(have_options_font)for(unsigned c=0;c<0x4f;++c)for(unsigned b=0;b<64;++b) {
        const unsigned source=options_glyphs[c*64+b];
        chars[0x4000+c*64+b]=static_cast<unsigned char>((source&15?ink:0)|(source&0xf0?ink<<4:0));
    }
    auto *percent_tiles=chars+0x4000+0x4f*64;
    for(unsigned y=0;y<16;++y) for(unsigned x=0;x<8;++x)
        if(percent[y]&(0x80>>x)) percent_tiles[y*4+x/2]|=ink<<((x&1)*4);
    auto *dot_tiles=chars+0x4000+0x50*64;
    for(unsigned y=12;y<14;++y) for(unsigned x=3;x<5;++x)
        dot_tiles[y*4+x/2]|=ink<<((x&1)*4);
    // Center visible ink at pixel precision, including odd-length labels and
    // the font's asymmetric blank bearings. Deduplicate composed8x16 cells
    // within Message's existing256-tile scratch area (127cells + blank tiles).
    unsigned char glyphs[0x51*64];
    std::memcpy(glyphs,chars+0x4000,sizeof(glyphs));
    std::memset(chars+0x4000,0,0x2000);
    std::fill(text,text+1024,0x2ff);
    std::fill(panels,panels+1024,blank_panel);
    unsigned cells=0;
    auto line = [text,chars,&glyphs,&cells](const char *label, unsigned y) {
        const unsigned count = std::min<size_t>(std::strlen(label),30);
        unsigned positions[30]{},position=0;
        for(unsigned i=0;i<count;++i) {
            const unsigned g=glyph(label[i]);
            positions[i]=position+(have_options_font && g==1?2:0); // original numeral-1 bearing
            position+=have_options_font && g<0x4f?options_widths[g]:8;
        }
        auto ink=[&](unsigned i,unsigned x,unsigned py) {
            const auto pair=glyphs[glyph(label[i])*64+py*4+x/2];
            return unsigned((pair>>((x&1)*4))&15);
        };
        int left=256,right=-1,top=16,bottom=-1;
        for(unsigned i=0;i<count;++i)for(unsigned py=0;py<16;++py)for(unsigned x=0;x<8;++x)
            if(ink(i,x,py)) {
                left=std::min(left,int(positions[i]+x));right=std::max(right,int(positions[i]+x));
                top=std::min(top,int(py));bottom=std::max(bottom,int(py));
            }
        if(right<left)return;
        const int offset_x=(256-(right-left+1))/2-left;
        const int offset_y=(16-(bottom-top+1))/2-top;
        unsigned char raster[16][128]{};
        for(unsigned i=0;i<count;++i)for(unsigned py=0;py<16;++py)for(unsigned x=0;x<8;++x)
            if(const auto color=ink(i,x,py)) {
                const int dx=int(positions[i]+x)+offset_x,dy=int(py)+offset_y;
                if(dx>=0 && dx<256 && dy>=0 && dy<16)
                    raster[dy][dx/2]|=static_cast<unsigned char>(color<<((dx&1)*4));
            }
        for(unsigned x=0;x<32;++x) {
            unsigned char cell[64];bool visible=false;
            for(unsigned py=0;py<16;++py)for(unsigned bx=0;bx<4;++bx) {
                cell[py*4+bx]=raster[py][x*4+bx];visible|=cell[py*4+bx]!=0;
            }
            if(!visible)continue;
            unsigned slot=0;
            while(slot<cells && std::memcmp(chars+0x4000+slot*64,cell,64))++slot;
            if(slot==cells) {
                if(cells==127)continue; // never overwrite scratch/blank tiles
                std::memcpy(chars+0x4000+cells++*64,cell,64);
            }
            text[y*32+x]=static_cast<uint16_t>(0x200+slot*2);
            text[(y+1)*32+x]=static_cast<uint16_t>(0x201+slot*2);
        }
    };
    char label[64];
    if (m.page==Page::Options) line("OPTIONS",0);
    else {
        unsigned index=0;while(index<page_count && pages[index]!=m.page)++index;
        const char *names[]={"DISPLAY","GRAPHICS","MINIMAP","MAP POSITION","SAVE","SCREENS","FREE CAMERA","GAMEPLAY"};
        if(m.binding_page())std::snprintf(label,sizeof(label),"%s %u/3",m.page==Page::Keyboard?"KEYBOARD":"CONTROLLER",m.binding_bank+1);
        else if(m.page==Page::Controls)std::snprintf(label,sizeof(label),"CONTROLS");
        else std::snprintf(label,sizeof(label),"%s %u/%u",index<page_count?names[index]:"VISUAL",index+1,page_count);
        line(label,0);
    }
    if(m.page==Page::Camera) line("L3 FREE / R3 ZOOM",15);
    if(m.page==Page::Screens) line("TOP 75%  BOTTOM 25%",15);
    if(m.page==Page::Graphics && ntr::video_renderer_notice()[0])line(ntr::video_renderer_notice(),15);
    // The caller caches a complete four-row button from its native position.
    // Keep its top and bottom bevels; omit the two middle rows to fit16px.
    for (unsigned i=0;i<m.rows();++i) {
        const unsigned y=row_top/8+i*row_height/8;
        for (unsigned dy=0;dy<2;++dy) for(unsigned x=0;x<32;++x)
            panels[(y+dy)*32+x] = static_cast<uint16_t>((panel[(dy?3:0)*32+x]&0x0fff)|(i==m.row?0x2000:0x1000));
        m.line(i,s,label,sizeof(label)); line(label,y);
    }
    if(m.binding_page()) {
        line(m.capture?"ESC / BACK CANCEL":"GAMEPLAY - MENU KEYS FIXED",18);
        line(m.capture?"WAITING FOR INPUT":"L/R PAGE  A BIND  B BACK",21);
    }
    else if(m.page==Page::Controls) {line("LEFT/RIGHT DEADZONE",18);line("A SELECT  B BACK",21);}
    else if(m.page==Page::Save) {
        if(m.save_status)line(m.save_status>0?"PROGRESS SAVED":"SAVE FAILED - TRY AGAIN",16);
        line(touch_navigation?"L PAGE    BACK    R PAGE":"L/R PAGE  A SELECT  B BACK",21);
    }
    else if (m.page==Page::Options) line(touch_navigation?"TAP ROW    BACK    B BACK":"A SELECT  B BACK",21);
    else {
        line(touch_navigation?"TAP VALUE OR LEFT/RIGHT":"L/R PAGE  LEFT/RIGHT CHANGE",18);
        line(touch_navigation?"L PAGE    BACK    R PAGE":"A CHANGE  B BACK",21);
    }
}
}
