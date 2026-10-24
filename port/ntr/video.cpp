#include "ntr/video.h"

#include "ntr/gx.h"
#include "ntr/gpu.h"
#include "ntr/input.h"
#include "ntr/mmio.h"
#include "ntr/ppu.h"
#include "ntr/rt.h"
#include "ntr/texture.h"
#include "ntr/minimap_drag.h"
#include "ntr/minimap_hud.h"
#include "ntr/menu_focus.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <chrono>

#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace ntr {
namespace {

VideoSettings g_settings;
struct WindowPreferences {unsigned width=1280,height=720;bool maximized=false;};
WindowPreferences g_window_preferences;
bool g_open = false;
bool g_menu_open = false;
bool g_menu_was_pressed = false;
Framebuffer g_top;
Framebuffer g_foreground;
Framebuffer g_bottom;
Framebuffer g_minimap;
Framebuffer g_editor_map;
Framebuffer g_editor_map_full;
std::vector<uint32_t> g_editor_background;
unsigned g_editor_width=0,g_editor_height=0,g_editor_axis=0;
bool g_editor_active=false,g_editor_dirty=false,g_editor_redraw=false,g_editor_map_ready=false;
bool g_editor_save_failed=false;
bool g_editor_suppress_pointer=false;
uint64_t g_editor_cache_tick=~uint64_t(0);
MinimapDrag g_editor_drag;
MinimapHudBounds g_minimap_bounds;
bool g_minimap_gameplay = false;
struct MinimapCounters {bool active=false;unsigned stars=0,coins=0,lives=0,health=8,character=3;};
MinimapCounters g_minimap_counters;
bool g_cannon_sight=false;
bool g_cutscene_letter = false;
bool g_dialogue_card_active = false;
bool g_pause_screen = false;
bool g_hide_pause_objects = false;
bool g_minigame_active = false;
int g_focus_left=-1,g_focus_top=0,g_focus_right=0,g_focus_bottom=0;
std::vector<uint32_t> g_output;
bool g_gpu = false;
bool g_forced_software = false,g_renderer_failed = false;
VideoRenderer g_active_renderer=VideoRenderer::D3D11;
unsigned g_output_width=SCREEN_W,g_output_height=SCREEN_H;

// The retail dialogue glyphs are a transparent PPU foreground layer. Add a
// small parchment-colored panel only to transparent pixels in that layer, so
// glyphs remain sharp and untouched in both the native GPU and software paths.
uint32_t dialogue_card_color(uint32_t destination, uint32_t source) {
    const unsigned alpha=source>>24;
    const unsigned inverse=255-alpha;
    const unsigned r=(((source>>16)&255)*alpha+((destination>>16)&255)*inverse+127)/255;
    const unsigned g=(((source>>8)&255)*alpha+((destination>>8)&255)*inverse+127)/255;
    const unsigned b=((source&255)*alpha+(destination&255)*inverse+127)/255;
    return 0xff000000u|(r<<16)|(g<<8)|b;
}

struct DialogueCardBounds {int left=54,top=50,right=182,bottom=146;};

DialogueCardBounds dialogue_card_bounds(const Framebuffer &foreground) {
    // Dialogue glyphs are in the PPU foreground. Their anchor differs between
    // cutscene/Lakitu messages and ordinary NPC boxes, so center the same-size
    // card on the actual visible text rather than assuming one fixed anchor.
    int min_x=SCREEN_W,min_y=SCREEN_H,max_x=-1,max_y=-1;
    for(int y=40;y<168;++y) for(int x=20;x<196;++x) {
        if(foreground.px[y][x]>>24) {
            min_x=std::min(min_x,x); min_y=std::min(min_y,y);
            max_x=std::max(max_x,x); max_y=std::max(max_y,y);
        }
    }
    if(max_x<min_x || max_y<min_y) return {};
    constexpr int min_width=128,min_height=96,padding_x=10,padding_y=10;
    const int width=std::min(SCREEN_W-24,std::max(min_width,max_x-min_x+1+padding_x*2));
    const int height=std::min(SCREEN_H-24,std::max(min_height,max_y-min_y+1+padding_y*2));
    const int center_x=(min_x+max_x+1)/2,center_y=(min_y+max_y+1)/2;
    const int left=std::clamp(center_x-width/2,12,SCREEN_W-12-width);
    const int top=std::clamp(center_y-height/2,12,SCREEN_H-12-height);
    return {left,top,left+width,top+height};
}

void dialogue_card_on_scene(Framebuffer &frame,const DialogueCardBounds &bounds) {
    const int left=bounds.left,top=bounds.top,right=bounds.right,bottom=bounds.bottom;
    for(int y=top;y<bottom;++y) for(int x=left;x<right;++x) {
        const bool border=x==left || x==right-1 || y==top || y==bottom-1;
        const bool inner_border=x==left+2 || x==right-3 || y==top+2 || y==bottom-3;
        const bool corner=(x<left+4 || x>=right-4) && (y<top+4 || y>=bottom-4);
        if(corner && !border && !inner_border) continue;
        const uint32_t color=border?0xe8e8b850u:inner_border?0xffedcf86u:0xb8172033u;
        auto &pixel=frame.px[y][x];
        pixel=dialogue_card_color(pixel,color);
    }
}

void dialogue_card_on_foreground(Framebuffer &frame,const DialogueCardBounds &bounds) {
    const int left=bounds.left,top=bounds.top,right=bounds.right,bottom=bounds.bottom;
    for(int y=top;y<bottom;++y) for(int x=left;x<right;++x) {
        const bool border=x==left || x==right-1 || y==top || y==bottom-1;
        const bool inner_border=x==left+2 || x==right-3 || y==top+2 || y==bottom-3;
        const bool corner=(x<left+4 || x>=right-4) && (y<top+4 || y>=bottom-4);
        if(corner && !border && !inner_border) continue;
        auto &pixel=frame.px[y][x];
        if(pixel>>24) continue; // retain every original text and UI pixel
        if(border) pixel=0xe8e8b850u;
        else if(inner_border) pixel=0xffedcf86u;
        else pixel=0xb8172033u;
    }
}

constexpr std::array<std::array<uint32_t, 2>, 15> kResolutionPresets{{
    {{1280, 720}}, {{1920, 1080}}, {{2560, 1440}}, {{3840, 2160}},
    {{1280, 800}}, {{1920, 1200}}, {{2560, 1600}}, {{3456, 2160}},
    {{1280, 549}}, {{2560, 1097}}, {{3440, 1474}}, {{3840, 1646}},
    {{854, 480}}, {{640, 360}},
    {{256,192}},
}};

const char *aspect_name(uint32_t width, uint32_t height) {
    if (height == 0) return "unknown";
    const double ratio = static_cast<double>(width) / height;
    if(std::abs(ratio-4.0/3.0)<0.035)return "4:3";
    if (std::abs(ratio - 16.0 / 10.0) < 0.035) return "16:10";
    if (std::abs(ratio - 21.0 / 9.0) < 0.035) return "21:9";
    return "16:9";
}

#if defined(_WIN32)
HWND g_window = nullptr;
const char *kWindowClass = "SM64DSNativeVideoWindow";

LRESULT CALLBACK video_window_proc(HWND window, UINT message, WPARAM wparam,
                                   LPARAM lparam) {
    if(message==WM_SIZE && g_open) {
        if(wparam==SIZE_MAXIMIZED)g_window_preferences.maximized=true;
        else if(wparam==SIZE_RESTORED && LOWORD(lparam) && HIWORD(lparam)) {
            g_window_preferences.width=LOWORD(lparam);g_window_preferences.height=HIWORD(lparam);
            g_window_preferences.maximized=false;
        }
    }
    if(message==WM_EXITSIZEMOVE && g_open)video_save_settings();
    if (message == WM_CLOSE) {
        if(g_open)video_save_settings();
        DestroyWindow(window);
        return 0;
    }
    if (message == WM_DESTROY) {
        g_window = nullptr;
        g_open = false;
        return 0;
    }
    return DefWindowProcA(window, message, wparam, lparam);
}

bool create_window(const char *title) {
    HINSTANCE instance = GetModuleHandleA(nullptr);
    WNDCLASSA wc{};
    wc.lpfnWndProc = video_window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = kWindowClass;
    RegisterClassA(&wc);

    RECT rect{0, 0, static_cast<LONG>(g_window_preferences.width),
              static_cast<LONG>(g_window_preferences.height)};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    g_window = CreateWindowA(kWindowClass, title, WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             rect.right - rect.left, rect.bottom - rect.top,
                             nullptr, nullptr, instance, nullptr);
    if (!g_window) return false;
    ShowWindow(g_window, g_window_preferences.maximized?SW_SHOWMAXIMIZED:SW_SHOW);
    UpdateWindow(g_window);
    g_open = true;
    return true;
}

void blend_minimap(uint32_t *dst, uint32_t src, uint8_t opacity) {
    const uint32_t effective_opacity = (((src >> 24) & 255u) * opacity) / 255u;
    if (effective_opacity == 0) return;
    const uint32_t inv = 255u - effective_opacity;
    const uint32_t r = (((src >> 16) & 255u) * effective_opacity + ((*dst >> 16) & 255u) * inv) / 255u;
    const uint32_t g = (((src >> 8) & 255u) * effective_opacity + ((*dst >> 8) & 255u) * inv) / 255u;
    const uint32_t b = ((src & 255u) * effective_opacity + (*dst & 255u) * inv) / 255u;
    *dst = 0xFF000000u | (r << 16) | (g << 8) | b;
}

void draw_menu_text(HDC dc, int x, int y, const char *text, COLORREF color) {
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    TextOutA(dc, x, y, text, static_cast<int>(lstrlenA(text)));
}

void draw_settings_menu(HDC dc) {
    if (!g_menu_open) return;
    RECT area{};
    GetClientRect(g_window, &area);
    RECT panel{12, 12, std::max(272L, std::min(532L, area.right - 12)), 400};
    HBRUSH brush = CreateSolidBrush(RGB(16, 22, 34));
    FillRect(dc, &panel, brush);
    DeleteObject(brush);
    HFONT font = CreateFontA(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                            DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    HGDIOBJ old_font = SelectObject(dc, font);
    VideoSettings s = video_get_settings();
    char line[160];
    draw_menu_text(dc, 28, 23, "Native PC settings  (F1 closes)", RGB(255, 220, 92));
    wsprintfA(line, "Resolution: %lu x %lu (%s)   [R cycles presets]",
              s.width, s.height, aspect_name(s.width, s.height));
    draw_menu_text(dc, 28, 51, line, RGB(245, 248, 255));
    wsprintfA(line, "Frame pacing: %lu FPS   [F changes 60 / 120 / 240]", rt_target_fps());
    draw_menu_text(dc, 28, 78, line, RGB(245, 248, 255));
    wsprintfA(line, "Minimap: %s   [M toggles]", s.minimap ? "on" : "off");
    draw_menu_text(dc, 28, 105, line, RGB(245, 248, 255));
    wsprintfA(line, "Map opacity: %u%%   [- / + adjusts]", (unsigned)(s.minimap_opacity * 100u / 255u));
    draw_menu_text(dc, 28, 132, line, RGB(245, 248, 255));
    wsprintfA(line, "Map size: %u%%   [ [ / ] adjusts]", s.minimap_scale / 10);
    draw_menu_text(dc, 28, 159, line, RGB(245, 248, 255));
    wsprintfA(line, "Map position: %u%%, %u%%   [arrow keys move]",
              s.minimap_x / 10, s.minimap_y / 10);
    draw_menu_text(dc, 28, 187, line, RGB(245, 248, 255));
    wsprintfA(line, "2D/HUD scaling: %s   [H toggles crisp pixels]",
              s.upscale_2d ? "pixel-crisp" : "smooth");
    draw_menu_text(dc, 28, 215, line, RGB(245, 248, 255));
    wsprintfA(line, "Texture quality: %s%s   [T cycles]",
              texture_quality_name(s.texture_quality),
              s.texture_quality==TextureQuality::Upscaled?" - no pack installed":"");
    draw_menu_text(dc, 28, 245, line, RGB(245, 248, 255));
    wsprintfA(line, "Map orientation: %s   [O toggles]",
              s.minimap_rotate_with_player ? "Rotate with camera" : "Upright");
    draw_menu_text(dc, 28, 274, line, RGB(245, 248, 255));
    wsprintfA(line, "Touch target: %s   [V toggles]",
              s.minimap_hide_touch_target ? "hidden" : "shown");
    draw_menu_text(dc, 28, 302, line, RGB(245, 248, 255));
    wsprintfA(line,"Minigame screens: %s   [L cycles 75/25 layout]",screen_layout_name(s.minigame_layout));
    draw_menu_text(dc,28,332,line,RGB(245,248,255));
    draw_menu_text(dc, 28, 362, "Mouse: touch input   |   Xbox: left stick moves", RGB(180, 196, 220));
    SelectObject(dc, old_font);
    DeleteObject(font);
}

const char *capture_name(const char *base, uint64_t tick, char (&buffer)[1024]) {
    if (!base || !base[0] || !tick) return base;
    size_t length = std::strlen(base);
    if (length>=4 && std::strcmp(base+length-4,".bmp")==0) length-=4;
    const int written=std::snprintf(buffer,sizeof(buffer),"%.*s-%llu.bmp",
        static_cast<int>(length),base,static_cast<unsigned long long>(tick));
    return written>0 && static_cast<size_t>(written)<sizeof(buffer) ? buffer : nullptr;
}
void capture_framebuffer_if_requested() {
    static bool captured = false;
    static uint64_t presentation_count = 0;
    static uint64_t last_scheduled_tick = 0;
    const char *schedule = std::getenv("SM64DS_PC_FRAME_CAPTURE_TICKS");
    uint64_t shot_tick=0;
    if (schedule && schedule[0]) {
        while (*schedule) {
            char *end=nullptr;
            const auto candidate=std::strtoull(schedule,&end,10);
            if (end!=schedule && (!*end || *end==',') && candidate>last_scheduled_tick &&
                candidate<=rt_frame() && (!shot_tick || candidate<shot_tick)) shot_tick=candidate;
            const char *next=std::strchr(schedule,',');
            if (!next) break;
            schedule=next+1;
        }
        if (!shot_tick) return;
    }
    const char *latest = std::getenv("SM64DS_PC_FRAME_CAPTURE_LATEST");
    const bool keep_latest = latest && latest[0] && latest[0] != '0';
    if (captured && !keep_latest && !shot_tick) return;
    char filename[1024];
    const char *path = capture_name(std::getenv("SM64DS_PC_FRAME_CAPTURE"),shot_tick,filename);
    if (!path || !path[0]) return;
    ++presentation_count;
    const char *after_value = std::getenv("SM64DS_PC_FRAME_CAPTURE_AFTER");
    const unsigned long long after = after_value && after_value[0]
        ? std::strtoull(after_value, nullptr, 10) : 1;
    if (!shot_tick && presentation_count < after) return;
    const char *tick_value = std::getenv("SM64DS_PC_FRAME_CAPTURE_AFTER_TICK");
    if (!shot_tick && tick_value && rt_frame() < std::strtoull(tick_value, nullptr, 10)) return;

    // Read back the actual native render target only for requested captures.
    // Ordinary GPU presentation never copies the 4K frame through the CPU.
    if (g_gpu && !g_editor_active && !gpu_readback(g_output,g_output_width,g_output_height)) return;
    FILE *file = std::fopen(path, "wb");
    if (!file) return;
    const uint32_t pixel_bytes = static_cast<uint32_t>(g_output.size() * sizeof(uint32_t));
    BITMAPFILEHEADER file_header{};
    BITMAPINFOHEADER info_header{};
    file_header.bfType = 0x4d42;
    file_header.bfOffBits = sizeof(file_header) + sizeof(info_header);
    file_header.bfSize = file_header.bfOffBits + pixel_bytes;
    info_header.biSize = sizeof(info_header);
    info_header.biWidth = g_output_width;
    info_header.biHeight = -static_cast<LONG>(g_output_height);
    info_header.biPlanes = 1;
    info_header.biBitCount = 32;
    info_header.biCompression = BI_RGB;
    info_header.biSizeImage = pixel_bytes;
    const bool ok = std::fwrite(&file_header, sizeof(file_header), 1, file) == 1 &&
                    std::fwrite(&info_header, sizeof(info_header), 1, file) == 1 &&
                    std::fwrite(g_output.data(), sizeof(uint32_t), g_output.size(), file) ==
                        g_output.size();
    std::fclose(file);
    if (ok) {
        std::fprintf(stderr,"[native capture] tick=%llu raster=%ux%u triangles=%u adapter=%s\n",
            static_cast<unsigned long long>(rt_frame()),g_output_width,g_output_height,
            g_gpu?gpu_triangle_count():0,g_gpu?gpu_adapter():"software 256x192");
        captured = true;
        if (shot_tick) last_scheduled_tick=shot_tick;
        char additional[1024];
        if (const char *sub = std::getenv("SM64DS_PC_SUB_CAPTURE"))
            if (const char *name=capture_name(sub,shot_tick,additional)) ppu_write_bmp(name, g_bottom);
        if (const char *map = std::getenv("SM64DS_PC_MAP_CAPTURE"))
            if (const char *name=capture_name(map,shot_tick,additional))
                ppu_write_bmp(name, g_settings.minimap && !g_pause_screen && g_minimap_gameplay &&
                    g_settings.minimap_hide_touch_target ? g_minimap : g_bottom);
    }
}
// Editor uses a clean frozen render target. GDI captions are rendered into
// the same CPU pixels as the map so saved screenshots match the actual page.
void render_minimap_editor() {
    g_output=g_editor_background;
    g_output_width=g_editor_width;g_output_height=g_editor_height;
    const int w=int(g_output_width),h=int(g_output_height);
    BITMAPINFO bmi{};bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth=w;bmi.bmiHeader.biHeight=-h;
    bmi.bmiHeader.biPlanes=1;bmi.bmiHeader.biBitCount=32;bmi.bmiHeader.biCompression=BI_RGB;
    void *bits=nullptr;HDC dc=CreateCompatibleDC(nullptr);
    HBITMAP bitmap=CreateDIBSection(dc,&bmi,DIB_RGB_COLORS,&bits,nullptr,0);
    if(dc && bitmap && bits) {
        const auto previous=SelectObject(dc,bitmap);
        std::memcpy(bits,g_output.data(),g_output.size()*4);
        const int font_size=std::max(8,h/43),margin=std::max(4,w/80);
        RECT footer{0,std::max(0,h-font_size*5),w,h};
        FillRect(dc,&footer,static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        HFONT font=CreateFontA(-font_size,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,
            ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,"Segoe UI");
        const auto old_font=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);
        SetTextColor(dc,RGB(255,225,95));
        const char *title="MINIMAP POSITION - drag the map";
        TextOutA(dc,margin,footer.top+font_size/3,title,int(std::strlen(title)));
        char value[160];
        std::snprintf(value,sizeof(value),"%sX %.1f%%    %sY %.1f%%    %s",
            g_editor_axis==0?"> ":"",g_settings.minimap_x/10.0,
            g_editor_axis==1?"> ":"",g_settings.minimap_y/10.0,
            g_editor_save_failed?"SAVE FAILED":g_editor_drag.active?"Release to save":"Saved");
        SetTextColor(dc,g_editor_save_failed?RGB(255,100,100):RGB(255,255,255));
        TextOutA(dc,margin,footer.top+font_size*2,value,int(std::strlen(value)));
        const char *help="D-pad: select/adjust   L/R: pages   B: back";
        TextOutA(dc,margin,footer.top+font_size*7/2,help,int(std::strlen(help)));
        GdiFlush();std::memcpy(g_output.data(),bits,g_output.size()*4);
        SelectObject(dc,old_font);SelectObject(dc,previous);DeleteObject(font);
    }
    if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);
    const auto &map=g_settings.minimap_hide_touch_target?g_editor_map:g_editor_map_full;
    std::array<uint8_t,SCREEN_W*SCREEN_H> transparent{};
    video_minimap_transparency_mask(&map.px[0][0],transparent.data(),SCREEN_W,SCREEN_H);
    const auto bounds=minimap_visible_bounds(transparent.data(),SCREEN_W,SCREEN_H);
    const auto placement=minimap_visible_placement(g_settings,bounds);
    const int left=w*placement.minimap_x/1000,top=h*placement.minimap_y/1000;
    const int mw=std::max(1,w*g_settings.minimap_scale/1000),mh=std::max(1,h*g_settings.minimap_scale/1000);
    for(int y=0;y<mh && top+y<h;++y)for(int x=0;x<mw && left+x<w;++x) {
        const int sy=y*SCREEN_H/mh,sx=x*SCREEN_W/mw;
        if(!transparent[sy*SCREEN_W+sx])
            blend_minimap(&g_output[size_t(top+y)*w+left+x],map.px[sy][sx],g_settings.minimap_opacity);
    }
    const uint32_t border=g_editor_drag.active?0xff80ffff:0xffffdc40;
    const int edge_left=std::min(w-1,left+mw*int(bounds.left)/1000);
    const int edge_top=std::min(h-1,top+mh*int(bounds.top)/1000);
    const int right=std::min(w-1,left+(mw*int(bounds.right)+999)/1000-1);
    const int bottom=std::min(h-1,top+(mh*int(bounds.bottom)+999)/1000-1);
    for(int x=edge_left;x<=right;++x) {g_output[size_t(edge_top)*w+x]=border;g_output[size_t(bottom)*w+x]=border;}
    for(int y=edge_top;y<=bottom;++y) {g_output[size_t(y)*w+edge_left]=border;g_output[size_t(y)*w+right]=border;}
    g_editor_redraw=false;
}

void update_minimap_drag(bool down,int x,int y) {
    const bool was_active=g_editor_drag.active;
    const auto &map=g_settings.minimap_hide_touch_target?g_editor_map:g_editor_map_full;
    std::array<uint8_t,SCREEN_W*SCREEN_H> transparent{};
    video_minimap_transparency_mask(&map.px[0][0],transparent.data(),SCREEN_W,SCREEN_H);
    const auto bounds=minimap_visible_bounds(transparent.data(),SCREEN_W,SCREEN_H);
    if(g_editor_drag.update(down,x,y,g_settings.minimap_scale,g_settings.minimap_x,g_settings.minimap_y,bounds)) {
        g_editor_dirty=true;g_editor_redraw=true;
    }
    if(was_active!=g_editor_drag.active)g_editor_redraw=true;
    if(was_active && !down && g_editor_dirty) {
        g_editor_save_failed=!video_save_settings();
        if(!g_editor_save_failed)g_editor_dirty=false;
        std::fprintf(stderr,"[minimap editor] release tick=%llu pos=%u,%u saved=%d\n",
            static_cast<unsigned long long>(rt_frame()),g_settings.minimap_x,g_settings.minimap_y,int(!g_editor_save_failed));
    }
}
#endif

void clamp_settings(VideoSettings &s) {
    if(static_cast<unsigned>(s.renderer)>1)s.renderer=VideoRenderer::D3D11;
    if(static_cast<unsigned>(s.minigame_layout)>2) s.minigame_layout=MinigameLayout::TopBottom;
    if(s.camera_zoom>3) s.camera_zoom=0;
    s.camera_sensitivity=std::clamp<uint16_t>(s.camera_sensitivity,25,300);
    if(s.camera_acceleration>2)s.camera_acceleration=1;
    const InputBindings defaults;
    for(unsigned i=0;i<binding_count;++i) {
        if(!valid_key_binding(s.bindings.keys[i]))s.bindings.keys[i]=defaults.keys[i];
        if(!valid_pad_binding(s.bindings.pad[i]))s.bindings.pad[i]=defaults.pad[i];
    }
    s.left_stick_deadzone=std::min<uint8_t>(s.left_stick_deadzone,50);
    s.right_stick_deadzone=std::min<uint8_t>(s.right_stick_deadzone,50);
    if(static_cast<unsigned>(s.render_distance)>3) s.render_distance=RenderDistance::Minimum;
    if(static_cast<unsigned>(s.texture_quality)>5) s.texture_quality=TextureQuality::Original;
    if(s.anisotropic_filtering!=2 && s.anisotropic_filtering!=4 &&
       s.anisotropic_filtering!=8 && s.anisotropic_filtering!=16) s.anisotropic_filtering=0;
    s.width = std::max(256u, std::min(s.width, 3840u));
    s.height = std::max(192u, std::min(s.height, 2160u));
    s.minimap_scale = std::max<uint16_t>(100, std::min<uint16_t>(s.minimap_scale, 500));
    // Source-texture padding may be outside the screen. Placement is limited
    // using the actual map pixels once a gameplay map is available.
    s.minimap_x = std::min<uint16_t>(s.minimap_x, 1000);
    s.minimap_y = std::min<uint16_t>(s.minimap_y, 1000);
}

}  // namespace

bool video_open(const char *title) {
#if defined(_WIN32)
    if (g_window) return true;
    video_load_settings();
    if (!create_window(title ? title : "Super Mario 64 DS")) return false;
    const char *software=std::getenv("SM64DS_PC_SOFTWARE_VIDEO");
    g_forced_software=software && software[0]=='1';
    g_renderer_failed=false;
    g_gpu = !g_forced_software && gpu_open(g_window,false,g_settings.renderer);
    if(!g_forced_software&&!g_gpu&&g_settings.renderer==VideoRenderer::OpenGLSoftware) {
        g_renderer_failed=true;g_settings.renderer=VideoRenderer::D3D11;
        g_gpu=gpu_open(g_window);video_save_settings();
        std::fprintf(stderr,"[video] OpenGL CPU unavailable; restored D3D11\n");
    }
    g_active_renderer=g_settings.renderer;
    gpu_set_anisotropic_filtering(g_settings.anisotropic_filtering);
    texture_set_backend(g_gpu?TextureBackend::NativeGpu:TextureBackend::SoftwareReference);
    if (!g_gpu) std::fprintf(stderr,"[video] using original 256x192 software renderer; native 4K unavailable in this mode\n");
    return true;
#else
    (void)title;
    return false;
#endif
}

void video_close() {
    g_cannon_sight=false;
    video_set_minimap_editor(false);
    g_editor_background.clear();g_editor_map_ready=false;g_editor_cache_tick=~uint64_t(0);
    gpu_close(); g_gpu=false;
    texture_set_backend(TextureBackend::CpuDecode);
#if defined(_WIN32)
    if (g_window) DestroyWindow(g_window);
#endif
    g_open = false;
    g_minigame_active = false;
    video_set_cutscene_letter(false);
    g_output.clear();
}

bool video_is_open() { return g_open; }
VideoSettings video_get_settings() { return g_settings; }
const char *video_renderer_notice() {return g_renderer_failed?"OPENGL CPU UNAVAILABLE":"";}

bool video_load_settings(const char *path) {
    const char *filename = path && path[0] ? path : std::getenv("SM64DS_PC_SETTINGS_FILE");
    if(!filename || !*filename)filename="sm64ds_pc.ini";
    FILE *file = std::fopen(filename, "rb");
    if (!file) return false;

    VideoSettings loaded;
    WindowPreferences loaded_window;
    bool has_quality=false,legacy_upscaled=false;
    char line[160];
    while (std::fgets(line, sizeof(line), file)) {
        char *equals = std::strchr(line, '=');
        if (!equals) continue;
        *equals++ = '\0';
        char *end = nullptr;
        const unsigned long value = std::strtoul(equals, &end, 10);
        if (end == equals) continue;
        unsigned binding_index=0;int consumed=0;
        if(std::sscanf(line,"key_bind_%u%n",&binding_index,&consumed)==1 && line[consumed]==0 && binding_index<binding_count) {
            if(valid_key_binding(value))loaded.bindings.keys[binding_index]=static_cast<uint16_t>(value);
            continue;
        }
        consumed=0;
        if(std::sscanf(line,"pad_bind_%u%n",&binding_index,&consumed)==1 && line[consumed]==0 && binding_index<binding_count) {
            if(value<=0x3ffff && valid_pad_binding(value))loaded.bindings.pad[binding_index]=static_cast<uint32_t>(value);
            continue;
        }
        if (std::strcmp(line, "width") == 0) loaded.width = static_cast<uint32_t>(value);
        else if (std::strcmp(line, "height") == 0) loaded.height = static_cast<uint32_t>(value);
        else if(std::strcmp(line,"window_width")==0)loaded_window.width=static_cast<unsigned>(std::min(value,3840ul));
        else if(std::strcmp(line,"window_height")==0)loaded_window.height=static_cast<unsigned>(std::min(value,2160ul));
        else if(std::strcmp(line,"window_maximized")==0)loaded_window.maximized=value==1;
        else if (std::strcmp(line, "target_fps") == 0) loaded.target_fps = static_cast<uint32_t>(value);
        else if (std::strcmp(line, "camera_zoom") == 0) loaded.camera_zoom = value<=3 ? static_cast<uint8_t>(value) : 0;
        else if (std::strcmp(line, "camera_mode") == 0) loaded.camera_mode = value<=1 ? static_cast<uint8_t>(value) : 0;
        else if (std::strcmp(line,"camera_sensitivity")==0) loaded.camera_sensitivity=static_cast<uint16_t>(std::clamp(value,25ul,300ul));
        else if (std::strcmp(line,"camera_invert_x")==0) loaded.camera_invert_x=value==1;
        else if (std::strcmp(line,"camera_invert_y")==0) loaded.camera_invert_y=value==1;
        else if (std::strcmp(line,"camera_acceleration")==0) loaded.camera_acceleration=value<=2?static_cast<uint8_t>(value):1;
        else if (std::strcmp(line,"left_stick_deadzone")==0) loaded.left_stick_deadzone=static_cast<uint8_t>(std::min(value,50ul));
        else if (std::strcmp(line,"right_stick_deadzone")==0) loaded.right_stick_deadzone=static_cast<uint8_t>(std::min(value,50ul));
        else if (std::strcmp(line, "always_run") == 0) loaded.always_run = value == 1;
        else if (std::strcmp(line, "render_distance") == 0) loaded.render_distance = value<=3 ? static_cast<RenderDistance>(value) : RenderDistance::Minimum;
        else if (std::strcmp(line, "stay_in_stage_after_star") == 0) loaded.stay_in_stage_after_star = value == 1;
        else if (std::strcmp(line, "brain_rot_text") == 0) loaded.brain_rot_text = value == 1;
        else if (std::strcmp(line,"pause_music_rec_room")==0) loaded.pause_music_rec_room=value==1;
        else if (std::strcmp(line, "anisotropic_filtering") == 0) loaded.anisotropic_filtering=static_cast<uint8_t>(std::min(value,255ul));
        else if (std::strcmp(line, "renderer") == 0) loaded.renderer=value==1?VideoRenderer::OpenGLSoftware:VideoRenderer::D3D11;
        else if (std::strcmp(line, "minigame_layout") == 0) loaded.minigame_layout=value<=2?static_cast<MinigameLayout>(value):MinigameLayout::TopBottom;
        else if (std::strcmp(line, "minimap") == 0) loaded.minimap = value != 0;
        else if (std::strcmp(line, "minimap_opacity") == 0) loaded.minimap_opacity = static_cast<uint8_t>(std::min(value, 255ul));
        else if (std::strcmp(line, "minimap_scale") == 0) loaded.minimap_scale = static_cast<uint16_t>(std::min(value, 65535ul));
        else if (std::strcmp(line, "minimap_x") == 0) loaded.minimap_x = static_cast<uint16_t>(std::min(value, 65535ul));
        else if (std::strcmp(line, "minimap_y") == 0) loaded.minimap_y = static_cast<uint16_t>(std::min(value, 65535ul));
        else if (std::strcmp(line, "minimap_rotate_with_player") == 0 || std::strcmp(line, "minimap_rotate_with_camera") == 0) loaded.minimap_rotate_with_player = value != 0;
        else if (std::strcmp(line, "minimap_hide_touch_target") == 0) loaded.minimap_hide_touch_target = value != 0;
        else if (std::strcmp(line, "upscale_2d") == 0) loaded.upscale_2d = value != 0;
        else if (std::strcmp(line, "hd_textures") == 0) legacy_upscaled = value != 0;
        else if (std::strcmp(line, "texture_quality") == 0) {
            has_quality=true;
            loaded.texture_quality=value<=5?static_cast<TextureQuality>(value):TextureQuality::Original;
        }
    }
    const bool read_ok = !std::ferror(file);
    std::fclose(file);
    if (!read_ok) return false;

    if(!has_quality && legacy_upscaled) loaded.texture_quality=TextureQuality::Upscaled;
    clamp_settings(loaded);
    if (loaded.target_fps != 60 && loaded.target_fps != 120 && loaded.target_fps != 240)
        loaded.target_fps = 60;
    g_settings = loaded;
    loaded_window.width=std::max(320u,loaded_window.width);
    loaded_window.height=std::max(240u,loaded_window.height);
    g_window_preferences=loaded_window;
    rt_set_target_fps(g_settings.target_fps);
    texture_set_quality(g_settings.texture_quality);
    gpu_set_anisotropic_filtering(g_settings.anisotropic_filtering);
    return true;
}

bool video_save_settings(const char *path) {
    const char *filename = path && path[0] ? path : std::getenv("SM64DS_PC_SETTINGS_FILE");
    if(!filename || !*filename)filename="sm64ds_pc.ini";
    char temporary[1024];
    const int length = std::snprintf(temporary, sizeof(temporary), "%s.tmp", filename);
    if (length < 0 || static_cast<size_t>(length) >= sizeof(temporary)) return false;
    FILE *file = std::fopen(temporary, "wb");
    if (!file) return false;
    bool write_ok = std::fprintf(file,
        "window_width=%u\nwindow_height=%u\nwindow_maximized=%u\nwidth=%u\nheight=%u\ntarget_fps=%u\nminimap=%u\nminimap_opacity=%u\n"
        "minimap_scale=%u\nminimap_x=%u\nminimap_y=%u\nupscale_2d=%u\nhd_textures=%u\n"
        "minimap_rotate_with_player=%u\nminimap_hide_touch_target=%u\ntexture_quality=%u\nanisotropic_filtering=%u\nminigame_layout=%u\nrenderer=%u\nstay_in_stage_after_star=%u\nbrain_rot_text=%u\ncamera_zoom=%u\ncamera_mode=%u\nalways_run=%u\nrender_distance=%u\n"
        "camera_sensitivity=%u\ncamera_invert_x=%u\ncamera_invert_y=%u\ncamera_acceleration=%u\nleft_stick_deadzone=%u\nright_stick_deadzone=%u\npause_music_rec_room=%u\n",
        g_window_preferences.width,g_window_preferences.height,g_window_preferences.maximized?1u:0u,
        g_settings.width, g_settings.height, rt_target_fps(), g_settings.minimap ? 1u : 0u,
        static_cast<unsigned>(g_settings.minimap_opacity),
        static_cast<unsigned>(g_settings.minimap_scale),
        static_cast<unsigned>(g_settings.minimap_x), static_cast<unsigned>(g_settings.minimap_y),
        g_settings.upscale_2d ? 1u : 0u, g_settings.texture_quality==TextureQuality::Upscaled ? 1u : 0u,
        g_settings.minimap_rotate_with_player ? 1u : 0u,
        g_settings.minimap_hide_touch_target ? 1u : 0u,
        static_cast<unsigned>(g_settings.texture_quality),
        static_cast<unsigned>(g_settings.anisotropic_filtering),static_cast<unsigned>(g_settings.minigame_layout),
        static_cast<unsigned>(g_settings.renderer),g_settings.stay_in_stage_after_star ? 1u : 0u,g_settings.brain_rot_text ? 1u : 0u,static_cast<unsigned>(g_settings.camera_zoom),static_cast<unsigned>(g_settings.camera_mode),g_settings.always_run ? 1u : 0u,static_cast<unsigned>(g_settings.render_distance),
        unsigned(g_settings.camera_sensitivity),unsigned(g_settings.camera_invert_x),unsigned(g_settings.camera_invert_y),unsigned(g_settings.camera_acceleration),unsigned(g_settings.left_stick_deadzone),unsigned(g_settings.right_stick_deadzone),unsigned(g_settings.pause_music_rec_room)) >= 0;
    for(unsigned i=0;i<binding_count;++i)
        write_ok=(std::fprintf(file,"key_bind_%u=%u\npad_bind_%u=%u\n",i,unsigned(g_settings.bindings.keys[i]),i,unsigned(g_settings.bindings.pad[i]))>=0) && write_ok;
    const bool close_ok = std::fclose(file) == 0;
    if (!write_ok || !close_ok) { std::remove(temporary); return false; }
#if defined(_WIN32)
    if (!MoveFileExA(temporary, filename, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::remove(temporary);
        return false;
    }
#else
    if (std::rename(temporary, filename) != 0) { std::remove(temporary); return false; }
#endif
    return true;
}

void video_set_settings(const VideoSettings &settings) {
    g_settings = settings;
    if(g_editor_active)g_editor_redraw=true;
    clamp_settings(g_settings);
    if(g_editor_map_ready)g_settings=minimap_visible_placement(g_settings,g_minimap_bounds);
    if (g_settings.target_fps != 60 && g_settings.target_fps != 120 &&
        g_settings.target_fps != 240) g_settings.target_fps = 60;
    rt_set_target_fps(g_settings.target_fps);
    texture_set_quality(g_settings.texture_quality);
    gpu_set_anisotropic_filtering(g_settings.anisotropic_filtering);
    // Render-target changes must never resize, move or restore the user's window.
    if (g_open) video_save_settings();
}

void video_cycle_resolution(VideoSettings &settings,int direction) {
    size_t index = 0;
    for (size_t i = 0; i < kResolutionPresets.size(); ++i) {
        if (settings.width == kResolutionPresets[i][0] &&
            settings.height == kResolutionPresets[i][1]) {
            index = (i + (direction<0?kResolutionPresets.size()-1:1)) % kResolutionPresets.size();
            goto found;
        }
    }
found:
    settings.width = kResolutionPresets[index][0];
    settings.height = kResolutionPresets[index][1];
}

uint16_t video_minimap_angle(uint16_t camera_heading) {
    // Use the original map convention: camera look-at -> eye angle. This
    // places the world in the same orientation as the current camera view.
    return g_settings.minimap_rotate_with_player
        ? camera_heading : 0;
}

void video_set_minimap_gameplay(bool active) {
    if (!active) video_set_cutscene_letter(false);
    if(!active && g_minimap_gameplay) {
        g_editor_map_ready=false;g_editor_background.clear();g_editor_cache_tick=~uint64_t(0);
    }
    g_minimap_gameplay = active;
}
void video_set_cannon_sight(bool active) {g_cannon_sight=active;}

void video_set_minimap_counters(bool active,unsigned stars,unsigned coins,unsigned lives,unsigned health,unsigned character) {
    g_minimap_counters={active,stars,coins,lives,health,character};
    if(const char *path=std::getenv("SM64DS_PC_HUD_CAPTURE")) {
        static FILE *file=nullptr;static MinimapCounters previous;static uint64_t tick=~uint64_t(0);
        if(!file && *path){file=std::fopen(path,"w");if(file)std::fputs("tick,active,stars,coins,lives,health,character,scale,map\n",file);}
        const auto now=rt_frame();
        if(file && (now>=tick+60 || tick==~uint64_t(0) || previous.active!=active || previous.stars!=stars || previous.coins!=coins || previous.lives!=lives || previous.health!=health || previous.character!=character)) {
            std::fprintf(file,"%llu,%u,%u,%u,%u,%u,%u,%u,%u\n",static_cast<unsigned long long>(now),unsigned(active),stars,coins,lives,health,character,g_settings.minimap_scale,unsigned(g_settings.minimap));std::fflush(file);
            previous=g_minimap_counters;tick=now;
        }
    }
}
void video_set_menu_focus(int left,int top,int right,int bottom) {
    g_focus_left=left;g_focus_top=top;g_focus_right=right;g_focus_bottom=bottom;
}
bool video_minimap_visible() {
    return g_settings.minimap && g_minimap_gameplay && !g_pause_screen && !g_minigame_active && !g_cutscene_letter;
}
void video_set_cutscene_letter(bool active) {
    if(g_cutscene_letter!=active && std::getenv("SM64DS_PC_INTRO_TRACE"))
        std::fprintf(stderr,"[intro letter] tick=%llu active=%u\n",
            static_cast<unsigned long long>(rt_frame()),active?1u:0u);
    g_cutscene_letter = active;
    gpu_set_letter_overlay(active);
}
void video_set_dialogue_card_active(bool active) {
    g_dialogue_card_active = active;
}
void video_set_pause_screen(bool active, bool custom_settings) {
    if(active && !g_pause_screen && g_minimap_gameplay && g_gpu) {
        if(!gpu_readback_gameplay(g_editor_background,g_editor_width,g_editor_height))
            g_editor_background.clear();
    }
    g_pause_screen = active;g_hide_pause_objects = active && custom_settings;
}
void video_set_minimap_editor(bool enabled,unsigned selected_axis) {
    enabled=enabled && g_pause_screen && g_minimap_gameplay &&
        !g_editor_background.empty() && g_editor_map_ready;
    if(enabled!=g_editor_active) {
        if(enabled)std::fprintf(stderr,"[minimap editor] enter tick=%llu preview=%ux%u pos=%u,%u\n",
            static_cast<unsigned long long>(rt_frame()),g_editor_width,g_editor_height,g_settings.minimap_x,g_settings.minimap_y);
        if(!enabled && g_editor_drag.held)g_editor_suppress_pointer=true;
        if(g_editor_dirty) {
            g_editor_save_failed=!video_save_settings();
            if(!g_editor_save_failed)g_editor_dirty=false;
        }
        g_editor_drag.reset();g_editor_redraw=true;
    }
    if(g_editor_axis!=selected_axis)g_editor_redraw=true;
    g_editor_axis=selected_axis;g_editor_active=enabled;
}
bool video_minimap_editor_active() {return g_editor_active;}
bool video_bottom_objects_visible() { return !g_hide_pause_objects; }
bool video_pause_screen() { return g_pause_screen; }
void video_set_minigame_active(bool active) {
    g_minigame_active=active;
    if(active) { g_pause_screen=false;g_hide_pause_objects=false;g_minimap_gameplay=false; }
    const char *trace=std::getenv("SM64DS_PC_TRACE");
    if(trace && trace[0]=='1') { std::fprintf(stderr,"[video] minigame active=%d tick=%llu\n",active,static_cast<unsigned long long>(rt_frame()));std::fflush(stderr); }
}
bool video_dual_screen_active() { return g_minigame_active && !g_pause_screen; }

void video_minimap_transparency_mask(const uint32_t *pixels, uint8_t *mask,
                                     uint32_t width, uint32_t height) {
    if (!pixels || !mask || width == 0 || height == 0) return;
    const size_t count = static_cast<size_t>(width) * height;
    std::fill(mask, mask + count, 0);
    for (size_t i = 0; i < count; ++i)
        if ((pixels[i] >> 24) == 0) mask[i] = 1;

    std::vector<uint32_t> border_colors;
    border_colors.reserve(static_cast<size_t>(width) * 2 + static_cast<size_t>(height) * 2);
    auto collect = [&](uint32_t x, uint32_t y) {
        const size_t i = static_cast<size_t>(y) * width + x;
        if ((pixels[i] >> 24) != 0) border_colors.push_back(pixels[i] & 0x00FFFFFFu);
    };
    for (uint32_t x = 0; x < width; ++x) {
        collect(x, 0);
        if (height > 1) collect(x, height - 1);
    }
    for (uint32_t y = 1; y + 1 < height; ++y) {
        collect(0, y);
        if (width > 1) collect(width - 1, y);
    }
    if (border_colors.empty()) return;

    std::sort(border_colors.begin(), border_colors.end());
    uint32_t border_color = border_colors[0];
    size_t best_count = 0;
    for (size_t begin = 0; begin < border_colors.size();) {
        size_t end = begin + 1;
        while (end < border_colors.size() && border_colors[end] == border_colors[begin]) ++end;
        if (end - begin > best_count) {
            border_color = border_colors[begin];
            best_count = end - begin;
        }
        begin = end;
    }
    // Avoid removing a background from ordinary imagery whose borders have
    // no clear, uniform fill color.
    if (best_count * 5 < border_colors.size() * 3) return;

    std::vector<uint32_t> pending;
    pending.reserve(count);
    auto enqueue = [&](uint32_t x, uint32_t y) {
        const size_t i = static_cast<size_t>(y) * width + x;
        if (mask[i] || (pixels[i] >> 24) == 0 ||
            (pixels[i] & 0x00FFFFFFu) != border_color) return;
        mask[i] = 1;
        pending.push_back(static_cast<uint32_t>(i));
    };
    for (uint32_t x = 0; x < width; ++x) {
        enqueue(x, 0);
        if (height > 1) enqueue(x, height - 1);
    }
    for (uint32_t y = 1; y + 1 < height; ++y) {
        enqueue(0, y);
        if (width > 1) enqueue(width - 1, y);
    }
    for (size_t cursor = 0; cursor < pending.size(); ++cursor) {
        const uint32_t i = pending[cursor];
        const uint32_t x = i % width, y = i / width;
        if (x > 0) enqueue(x - 1, y);
        if (x + 1 < width) enqueue(x + 1, y);
        if (y > 0) enqueue(x, y - 1);
        if (y + 1 < height) enqueue(x, y + 1);
    }
}

bool video_present_frame() {
#if defined(_WIN32)
    if (!g_window || !g_open) return false;
    if(!g_forced_software && g_active_renderer!=g_settings.renderer) {
        gpu_close();g_renderer_failed=false;
        g_gpu=gpu_open(g_window,false,g_settings.renderer);
        if(!g_gpu&&g_settings.renderer==VideoRenderer::OpenGLSoftware) {
            g_renderer_failed=true;g_settings.renderer=VideoRenderer::D3D11;
            g_gpu=gpu_open(g_window);video_save_settings();
            std::fprintf(stderr,"[video] OpenGL CPU unavailable; restored D3D11\n");
        }
        g_active_renderer=g_settings.renderer;
        gpu_set_anisotropic_filtering(g_settings.anisotropic_filtering);
        texture_set_backend(g_gpu?TextureBackend::NativeGpu:TextureBackend::SoftwareReference);
    }

    MSG message{};
    while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) {
            g_open = false;
            return false;
        }
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    if (!g_open) return false;

    const bool menu_pressed = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;
    if (menu_pressed && !g_menu_was_pressed) g_menu_open = !g_menu_open;
    g_menu_was_pressed = menu_pressed;
    if (g_menu_open) {
        VideoSettings s = video_get_settings();
        if (GetAsyncKeyState('M') & 1) { s.minimap = !s.minimap; video_set_settings(s); }
        if (GetAsyncKeyState('O') & 1) { s.minimap_rotate_with_player = !s.minimap_rotate_with_player; video_set_settings(s); }
        if (GetAsyncKeyState('V') & 1) { s.minimap_hide_touch_target = !s.minimap_hide_touch_target; video_set_settings(s); }
        if (GetAsyncKeyState('R') & 1) {
            video_cycle_resolution(s);
            video_set_settings(s);
        }
        if (GetAsyncKeyState('F') & 1) {
            s.target_fps = s.target_fps == 60 ? 120 : s.target_fps == 120 ? 240 : 60;
            video_set_settings(s);
        }
        if (GetAsyncKeyState('H') & 1) {
            s.upscale_2d = !s.upscale_2d;
            video_set_settings(s);
        }
        if (GetAsyncKeyState('L') & 1) {
            s.minigame_layout=static_cast<MinigameLayout>((static_cast<unsigned>(s.minigame_layout)+1)%3);
            video_set_settings(s);
        }
        if (GetAsyncKeyState('T') & 1) {
            s.texture_quality = texture_cycle_quality(s.texture_quality);
            video_set_settings(s);
        }
        if (GetAsyncKeyState(VK_OEM_MINUS) & 1) {
            s.minimap_opacity = static_cast<uint8_t>(s.minimap_opacity > 16 ? s.minimap_opacity - 16 : 0);
            video_set_settings(s);
        }
        if (GetAsyncKeyState(VK_OEM_PLUS) & 1) {
            s.minimap_opacity = static_cast<uint8_t>(std::min(255, s.minimap_opacity + 16));
            video_set_settings(s);
        }
        if (GetAsyncKeyState(VK_OEM_4) & 1) {
            s.minimap_scale = static_cast<uint16_t>(s.minimap_scale > 10 ? s.minimap_scale - 10 : 100);
            video_set_settings(s);
        }
        if (GetAsyncKeyState(VK_OEM_6) & 1) {
            s.minimap_scale = static_cast<uint16_t>(std::min(500, s.minimap_scale + 10));
            video_set_settings(s);
        }
        const uint16_t max_x=minimap_position_limit(s.minimap_scale,g_minimap_bounds.right);
        const uint16_t max_y=minimap_position_limit(s.minimap_scale,g_minimap_bounds.bottom);
        if (GetAsyncKeyState(VK_LEFT) & 1) {
            s.minimap_x = static_cast<uint16_t>(s.minimap_x > 10 ? s.minimap_x - 10 : 0);
            video_set_settings(s);
        }
        if (GetAsyncKeyState(VK_RIGHT) & 1) {
            s.minimap_x = static_cast<uint16_t>(std::min<unsigned>(max_x, s.minimap_x + 10));
            video_set_settings(s);
        }
        if (GetAsyncKeyState(VK_UP) & 1) {
            s.minimap_y = static_cast<uint16_t>(s.minimap_y > 10 ? s.minimap_y - 10 : 0);
            video_set_settings(s);
        }
        if (GetAsyncKeyState(VK_DOWN) & 1) {
            s.minimap_y = static_cast<uint16_t>(std::min<unsigned>(max_y, s.minimap_y + 10));
            video_set_settings(s);
        }
    }

    POINT cursor{};
    const bool have_cursor = GetCursorPos(&cursor) && ScreenToClient(g_window, &cursor);
    RECT touch_area{};
    GetClientRect(g_window, &touch_area);
    const int touch_w = std::max(1L, touch_area.right - touch_area.left);
    const int touch_h = std::max(1L, touch_area.bottom - touch_area.top);
    const bool dual=video_dual_screen_active();
    const auto screens=screen_layout(g_settings.minigame_layout,g_settings.width,g_settings.height);
    const auto output_area=dual?screen_fit(g_settings.width,g_settings.height,{0,0,touch_w,touch_h}):ScreenRect{0,0,touch_w,touch_h};
    // POWCNT1 bit15 routes engine A to the physical top screen; minigame
    // pause interfaces can swap it onto the touch screen.
    const bool engine_a_on_top=(io_read(0x04000304,2)&0x8000)!=0;
    uint16_t touch_x=0,touch_y=0;
    const ScreenRect touch_rect=dual?screens.bottom:ScreenRect{0,0,int(g_settings.width),int(g_settings.height)};
    const bool touch_inside=output_area.width>0 && output_area.height>0 &&
        cursor.x>=output_area.x && cursor.y>=output_area.y &&
        cursor.x<output_area.x+output_area.width && cursor.y<output_area.y+output_area.height &&
        screen_touch(touch_rect,int(int64_t(cursor.x-output_area.x)*g_settings.width/output_area.width),
                     int(int64_t(cursor.y-output_area.y)*g_settings.height/output_area.height),touch_x,touch_y);
    if(!(GetAsyncKeyState(VK_LBUTTON)&0x8000))g_editor_suppress_pointer=false;
    input_set_touch(!g_menu_open && !g_editor_suppress_pointer && have_cursor && touch_inside && (GetAsyncKeyState(VK_LBUTTON)&0x8000),touch_x,touch_y);

    if(g_editor_active) {
        input_set_touch(false,0,0);
        bool down=have_cursor && GetForegroundWindow()==g_window && (GetAsyncKeyState(VK_LBUTTON)&0x8000);
        int x=int(int64_t(cursor.x)*1000/touch_w),y=int(int64_t(cursor.y)*1000/touch_h);
        if(const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY"))if(automation[0]=='1') {
            // Normalized pointer segments exercise the production drag logic;
            // release outside each interval, independent of host presentation.
            down=false;
            const char *segment=std::getenv("SM64DS_PC_TEST_MINIMAP_DRAG");
            while(segment && *segment) {
                unsigned long long begin=0,end=0;int x0=0,y0=0,x1=0,y1=0,count=0;
                if(std::sscanf(segment,"%llu:%llu:%d:%d:%d:%d%n",&begin,&end,&x0,&y0,&x1,&y1,&count)==6 &&
                   count>0 && (!segment[count] || segment[count]==',') && end>begin+1 &&
                   rt_frame()>=begin && rt_frame()<end &&
                   x0>=-3000 && x0<=3000 && y0>=-3000 && y0<=3000 &&
                   x1>=-3000 && x1<=3000 && y1>=-3000 && y1<=3000) {
                    down=true;
                    x=x0+int((int64_t(x1)-x0)*int64_t(rt_frame()-begin)/int64_t(end-begin-1));
                    y=y0+int((int64_t(y1)-y0)*int64_t(rt_frame()-begin)/int64_t(end-begin-1));
                    break;
                }
                const char *next=std::strchr(segment,',');segment=next?next+1:nullptr;
            }
        }
        update_minimap_drag(down,x,y);
        if(g_editor_redraw)render_minimap_editor();
        HDC dc=GetDC(g_window);if(!dc)return false;
        BITMAPINFO bmi{};bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth=g_output_width;bmi.bmiHeader.biHeight=-LONG(g_output_height);
        bmi.bmiHeader.biPlanes=1;bmi.bmiHeader.biBitCount=32;bmi.bmiHeader.biCompression=BI_RGB;
        SetStretchBltMode(dc,COLORONCOLOR);
        StretchDIBits(dc,0,0,touch_w,touch_h,0,0,g_output_width,g_output_height,g_output.data(),&bmi,DIB_RGB_COLORS,SRCCOPY);
        ReleaseDC(g_window,dc);capture_framebuffer_if_requested();
        return true;
    }

    ppu_scanout(ENGINE_A, g_top);
    ppu_scanout_obj(ENGINE_A, g_top);
    const bool geometry_visible=!g_pause_screen && ppu_geometry_active();
    uint8_t foreground_mask=ppu_backgrounds_above_3d();
    ppu_scanout(ENGINE_A,g_foreground,true,foreground_mask);
    const bool dialogue_card=g_dialogue_card_active && !g_pause_screen && !g_minigame_active &&
        !g_cutscene_letter && !g_menu_open && !dual;
    const auto dialogue_bounds=dialogue_card?dialogue_card_bounds(g_foreground):DialogueCardBounds{};
    if(dialogue_card && g_gpu) dialogue_card_on_foreground(g_foreground,dialogue_bounds);
    if (!g_gpu && geometry_visible) {
        gx_render(g_top);
        if(dialogue_card) dialogue_card_on_scene(g_top,dialogue_bounds);
        for(int y=0;y<SCREEN_H;++y)for(int x=0;x<SCREEN_W;++x)
            if(g_foreground.px[y][x]>>24)g_top.px[y][x]=g_foreground.px[y][x];
    }
    ppu_scanout(ENGINE_B, g_bottom, true);
    if(video_bottom_objects_visible()) ppu_scanout_obj(ENGINE_B, g_bottom);
    if(g_focus_left>=0) {
        auto draw_focus=[&](Framebuffer &frame) {
            draw_menu_focus(frame,g_focus_left,g_focus_top,g_focus_right,g_focus_bottom);
        };
        if(engine_a_on_top)draw_focus(g_bottom);
        else {draw_focus(g_top);draw_focus(g_foreground);foreground_mask|=1;}
    }

    RECT client{};
    GetClientRect(g_window, &client);
    const int width = std::max(1L, client.right - client.left);
    const int height = std::max(1L, client.bottom - client.top);
    if(const char *path=std::getenv("SM64DS_PC_WINDOW_CAPTURE")) {
        static FILE *file=nullptr;static long previous[11]{};static bool observed=false;
        RECT bounds{};GetWindowRect(g_window,&bounds);
        const long state[]={long(g_settings.width),long(g_settings.height),width,height,
            bounds.left,bounds.top,bounds.right,bounds.bottom,IsZoomed(g_window)?1L:0L,dual?1L:0L,long(g_settings.minigame_layout)};
        if(!observed || std::memcmp(state,previous,sizeof(state))) {
            observed=true;std::memcpy(previous,state,sizeof(state));
            if(!file){file=std::fopen(path,"w");if(file)std::fputs("tick,internal_width,internal_height,client_width,client_height,left,top,right,bottom,maximized,dual,layout\n",file);}
            if(file){std::fprintf(file,"%llu",static_cast<unsigned long long>(rt_frame()));for(long value:state)std::fprintf(file,",%ld",value);std::fputc('\n',file);std::fflush(file);}
        }
    }
    const Framebuffer &main_view = g_pause_screen && !dual ? g_bottom : g_top;
    g_output.assign(&main_view.px[0][0], &main_view.px[0][0] + SCREEN_W * SCREEN_H);
    g_output_width=SCREEN_W;g_output_height=SCREEN_H;
    const bool cache_gameplay=g_minimap_gameplay && !g_pause_screen && !dual && geometry_visible && g_editor_cache_tick!=rt_frame();
    if(cache_gameplay) {
        g_editor_map_full=g_bottom;
        ppu_scanout(ENGINE_B,g_editor_map,true,0x0b);ppu_scanout_obj(ENGINE_B,g_editor_map);
        g_editor_map_ready=true;
        if(!g_gpu) {
            g_editor_background=g_output;g_editor_width=SCREEN_W;g_editor_height=SCREEN_H;
        }
        g_editor_cache_tick=rt_frame();
    }
    if(dual && !g_gpu) {
        screen_compose(engine_a_on_top?g_top:g_bottom,engine_a_on_top?g_bottom:g_top,
                       screens,g_settings.width,g_settings.height,g_output);
        g_output_width=g_settings.width;g_output_height=g_settings.height;
    }
    std::array<uint32_t,SCREEN_W*SCREEN_H> gpu_map_pixels{};
    static std::vector<uint32_t> gpu_hud_pixels;
    MinimapHudBounds hud_map_bounds{};
    VideoSettings map_settings=minimap_visible_placement(g_settings,g_minimap_bounds);

    if (video_minimap_visible()) {
        const Framebuffer *map = &g_bottom;
        if (g_minimap_gameplay && g_settings.minimap_hide_touch_target) {
            // Gameplay BG2 is the independent touch-control target; BG3 is
            // the map. Filter the overlay scanout only, leaving raw DS output
            // and the input registers untouched. Other screens keep all layers.
            ppu_scanout(ENGINE_B, g_minimap, true, 0x0b);
            ppu_scanout_obj(ENGINE_B, g_minimap);
            map = &g_minimap;
        }
        std::array<uint8_t, SCREEN_W * SCREEN_H> transparent{};
        video_minimap_transparency_mask(&map->px[0][0], transparent.data(),
                                       SCREEN_W, SCREEN_H);
        hud_map_bounds=minimap_visible_bounds(transparent.data(),SCREEN_W,SCREEN_H);
        g_minimap_bounds=hud_map_bounds;
        map_settings=minimap_visible_placement(g_settings,hud_map_bounds);
        if (g_gpu) for (size_t i=0;i<gpu_map_pixels.size();++i) {
            const uint32_t pixel=(&map->px[0][0])[i];
            const uint32_t alpha=transparent[i]?0:((pixel>>24)*g_settings.minimap_opacity/255);
            gpu_map_pixels[i]=(pixel&0x00ffffffu)|(alpha<<24);
        }
        const int map_w = std::max(1, SCREEN_W * g_settings.minimap_scale / 1000);
        const int map_h = std::max(1, map_w * SCREEN_H / SCREEN_W);
        const int left = std::min(SCREEN_W - 1, SCREEN_W * map_settings.minimap_x / 1000);
        const int top = std::min(SCREEN_H - 1, SCREEN_H * map_settings.minimap_y / 1000);
        const int draw_w = std::min(map_w, SCREEN_W - left);
        const int draw_h = std::min(map_h, SCREEN_H - top);
        for (int y = 0; y < draw_h; ++y) {
            const int sy = y * SCREEN_H / map_h;
            for (int x = 0; x < draw_w; ++x) {
                const int sx = x * SCREEN_W / map_w;
                if (transparent[sy * SCREEN_W + sx]) continue;
                uint32_t &dst = g_output[static_cast<size_t>(top + y) * SCREEN_W + left + x];
                blend_minimap(&dst, map->px[sy][sx], g_settings.minimap_opacity);
            }
        }
        if (const char *trace = std::getenv("SM64DS_PC_TRACE_MINIMAP")) {
            static bool reported = false;
            if (!reported && trace[0] && trace[0] != '0' && rt_frame() >= 5000) {
                size_t transparent_pixels = 0;
                for (uint8_t value : transparent) transparent_pixels += value != 0;
                const int sample_x = std::min(SCREEN_W - 1, left + draw_w / 2);
                const int sample_y = std::min(SCREEN_H - 1, top + draw_h / 2);
                const int source_x = (sample_x - left) * SCREEN_W / map_w;
                const int source_y = (sample_y - top) * SCREEN_H / map_h;
                std::fprintf(stderr,
                    "[minimap trace] tick=%llu enabled=%d opacity=%u scale=%u pos=%u,%u size=%dx%d transparent=%zu sample_mask=%u src=%08x dst=%08x\n",
                    static_cast<unsigned long long>(rt_frame()),
                    g_settings.minimap, g_settings.minimap_opacity,
                    g_settings.minimap_scale, g_settings.minimap_x, g_settings.minimap_y,
                    draw_w, draw_h, transparent_pixels,
                    transparent[source_y * SCREEN_W + source_x],
                    map->px[source_y][source_x],
                    g_output[static_cast<size_t>(sample_y) * SCREEN_W + sample_x]);
                std::fflush(stderr);
                reported = true;
            }
        }
    }

    const bool hud_visible=g_minimap_counters.active && !g_pause_screen && !dual &&
        !g_minigame_active && !g_cutscene_letter;
    const bool sight_visible=g_cannon_sight && g_minimap_gameplay && !g_pause_screen && !dual &&
        !g_minigame_active && !g_cutscene_letter;
    const bool overlay_visible=hud_visible || sight_visible;
    const unsigned hud_width=g_gpu?g_settings.width:SCREEN_W;
    const unsigned hud_height=g_gpu?g_settings.height:SCREEN_H;
    static std::array<uint32_t,16> previous_hud_key{};
    static bool have_hud_key=false;
    static uint64_t hud_revision=0;
    const std::array<uint32_t,16> hud_key{{
        unsigned(hud_visible),unsigned(sight_visible),hud_width,hud_height,map_settings.minimap_x,map_settings.minimap_y,
        g_settings.minimap_scale,g_minimap_counters.stars,g_minimap_counters.coins,
        g_minimap_counters.lives,g_minimap_counters.health,g_minimap_counters.character,
        hud_map_bounds.left,hud_map_bounds.top,hud_map_bounds.right,hud_map_bounds.bottom}};
    if(!have_hud_key||hud_key!=previous_hud_key) {
        previous_hud_key=hud_key;have_hud_key=true;
        if(++hud_revision==0)++hud_revision;
        if(overlay_visible) {
            gpu_hud_pixels.resize(size_t(hud_width)*hud_height);
            minimap_hud_build(gpu_hud_pixels.data(),hud_width,hud_height,map_settings,hud_visible,
                g_minimap_counters.stars,g_minimap_counters.coins,g_minimap_counters.lives,
                g_minimap_counters.health,g_minimap_counters.character,hud_map_bounds);
            if(sight_visible)cannon_sight_build(gpu_hud_pixels.data(),hud_width,hud_height);
        } else gpu_hud_pixels.clear();
    }
    if(!g_gpu&&overlay_visible)
        minimap_hud_blend(g_output.data(),gpu_hud_pixels.data(),SCREEN_W,SCREEN_H);
    if(const char *path=std::getenv("SM64DS_PC_CANNON_SIGHT_TRACE")) {
        static FILE *file=nullptr;static int previous=-1;static unsigned pw=0,ph=0;
        if(!file){file=std::fopen(path,"w");if(file)std::fputs("tick,visible,width,height,center_x,center_y\n",file);}
        if(file && (previous!=int(sight_visible)||pw!=hud_width||ph!=hud_height)){
            std::fprintf(file,"%llu,%u,%u,%u,%u,%u\n",static_cast<unsigned long long>(rt_frame()),
                unsigned(sight_visible),hud_width,hud_height,hud_width/2,hud_height/2);std::fflush(file);
            previous=int(sight_visible);pw=hud_width;ph=hud_height;
        }
    }
    if(const char *trace=std::getenv("SM64DS_PC_MINIMAP_COUNTERS_TRACE")) {
        static bool reported=false;
        if(!reported&&trace[0]&&trace[0]!='0'&&rt_frame()>=1800&&hud_visible) {
            std::fprintf(stderr,"[minimap counters] tick=%llu stars=%u coins=%u lives=%u scale=%u enabled=%d\n",
                static_cast<unsigned long long>(rt_frame()),g_minimap_counters.stars,
                g_minimap_counters.coins,g_minimap_counters.lives,g_settings.minimap_scale,int(g_settings.minimap));
            std::fflush(stderr);reported=true;
        }
    }

    if (g_gpu) {
        const auto submit_start=std::chrono::steady_clock::now();
        if (!gpu_render(main_view,video_minimap_visible()?gpu_map_pixels.data():nullptr,
                        map_settings,geometry_visible,foreground_mask?&g_foreground.px[0][0]:nullptr,
                        dual?&screens:nullptr,dual?&g_bottom:nullptr,engine_a_on_top,cache_gameplay,
                        overlay_visible?gpu_hud_pixels.data():nullptr,hud_width,hud_height,hud_revision) ||
            !gpu_present(width,height,dual)) {
            std::fprintf(stderr,"[video] native GPU presentation failed; reverting to software on the next frame\n");
            gpu_close();g_gpu=false;
            texture_set_backend(TextureBackend::SoftwareReference);
            return true;
        }
        if(const char *path=std::getenv("SM64DS_PC_GPU_PROFILE")) {
            static FILE *profile=nullptr;
            if(!profile && path[0]) {
                profile=std::fopen(path,"wb");
                if(profile)std::fputs("tick,render_cpu_us,render_present_us,hashed_texels,upload_bytes,uploads,identity_uses,cache_hits,metadata_calls,metadata_texels,cache_bytes,cache_budget,evictions,replacement_attempts,replacement_uploads,staging_peak_bytes,gx_cache_bytes\n",profile);
            }
            if(profile) {
                const auto stats=gpu_frame_stats();const auto work=texture_pixel_work_reset();
                const auto total=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-submit_start).count();
                std::fprintf(profile,"%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu\n",
                    static_cast<unsigned long long>(rt_frame()),stats.cpu_us,static_cast<unsigned long long>(total),
                    stats.hash_texels,stats.upload_bytes,stats.uploads,stats.identity_uses,stats.cache_hits,work.calls,work.texels,stats.cache_bytes,stats.cache_budget,stats.evictions,
                    stats.replacement_attempts,stats.replacement_uploads,stats.staging_peak_bytes,gx_texture_cache_bytes());
                if(rt_frame()%120==0)std::fflush(profile);
            }
        }
        if (g_menu_open) {
            HDC menu_dc=GetDC(g_window);
            if (menu_dc) { draw_settings_menu(menu_dc);ReleaseDC(g_window,menu_dc); }
        }
        capture_framebuffer_if_requested();
        return true;
    }

    HDC dc = GetDC(g_window);
    if (!dc) return false;
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = g_output_width;
    bmi.bmiHeader.biHeight = -static_cast<LONG>(g_output_height);
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    // COLORONCOLOR maps source pixels directly to integer-sized output blocks,
    // keeping DS HUD and minigame sprite edges crisp. HALFTONE remains the
    // default smooth scaler for the combined 3D/2D frame.
    SetStretchBltMode(dc, g_settings.upscale_2d ? COLORONCOLOR : HALFTONE);
    if(dual) { RECT black{0,0,width,height};FillRect(dc,&black,static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH))); }
    StretchDIBits(dc, output_area.x, output_area.y, output_area.width, output_area.height, 0, 0, g_output_width, g_output_height,
                  g_output.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
    draw_settings_menu(dc);
    ReleaseDC(g_window, dc);
    capture_framebuffer_if_requested();
    return true;
#else
    return false;
#endif
}

}  // namespace ntr

extern "C" void pc_cutscene_letter_state(int active) {
    ntr::video_set_cutscene_letter(active != 0);
}
