#include "ntr/video.h"

#include "ntr/gx.h"
#include "ntr/gpu.h"
#include "ntr/input.h"
#include "ntr/mmio.h"
#include "ntr/ppu.h"
#include "ntr/rt.h"
#include "ntr/texture.h"

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
bool g_open = false;
bool g_menu_open = false;
bool g_menu_was_pressed = false;
Framebuffer g_top;
Framebuffer g_foreground;
Framebuffer g_bottom;
Framebuffer g_minimap;
bool g_minimap_gameplay = false;
bool g_pause_screen = false;
bool g_hide_pause_objects = false;
bool g_minigame_active = false;
int g_focus_left=-1,g_focus_top=0,g_focus_right=0,g_focus_bottom=0;
std::vector<uint32_t> g_output;
bool g_gpu = false;
bool g_forced_software = false,g_renderer_failed = false;
VideoRenderer g_active_renderer=VideoRenderer::D3D11;
unsigned g_output_width=SCREEN_W,g_output_height=SCREEN_H;

constexpr std::array<std::array<uint32_t, 2>, 12> kResolutionPresets{{
    {{1280, 720}}, {{1920, 1080}}, {{2560, 1440}}, {{3840, 2160}},
    {{1280, 800}}, {{1920, 1200}}, {{2560, 1600}}, {{3456, 2160}},
    {{1280, 549}}, {{2560, 1097}}, {{3440, 1474}}, {{3840, 1646}},
}};

const char *aspect_name(uint32_t width, uint32_t height) {
    if (height == 0) return "unknown";
    const double ratio = static_cast<double>(width) / height;
    if (std::abs(ratio - 16.0 / 10.0) < 0.035) return "16:10";
    if (std::abs(ratio - 21.0 / 9.0) < 0.035) return "21:9";
    return "16:9";
}

#if defined(_WIN32)
HWND g_window = nullptr;
const char *kWindowClass = "SM64DSNativeVideoWindow";

LRESULT CALLBACK video_window_proc(HWND window, UINT message, WPARAM wparam,
                                   LPARAM lparam) {
    if (message == WM_CLOSE) {
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

    RECT rect{0, 0, static_cast<LONG>(g_settings.width),
              static_cast<LONG>(g_settings.height)};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    g_window = CreateWindowA(kWindowClass, title, WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             rect.right - rect.left, rect.bottom - rect.top,
                             nullptr, nullptr, instance, nullptr);
    if (!g_window) return false;
    ShowWindow(g_window, SW_SHOW);
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
              s.minimap_rotate_with_player ? "Rotate with player" : "Upright");
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
    if (g_gpu && !gpu_readback(g_output,g_output_width,g_output_height)) return;
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
#endif

void clamp_settings(VideoSettings &s) {
    if(static_cast<unsigned>(s.renderer)>1)s.renderer=VideoRenderer::D3D11;
    if(static_cast<unsigned>(s.minigame_layout)>2) s.minigame_layout=MinigameLayout::TopBottom;
    if(static_cast<unsigned>(s.texture_quality)>5) s.texture_quality=TextureQuality::Original;
    if(s.anisotropic_filtering!=2 && s.anisotropic_filtering!=4 &&
       s.anisotropic_filtering!=8 && s.anisotropic_filtering!=16) s.anisotropic_filtering=0;
    s.width = std::max(256u, std::min(s.width, 3840u));
    s.height = std::max(192u, std::min(s.height, 2160u));
    s.minimap_scale = std::max<uint16_t>(100, std::min<uint16_t>(s.minimap_scale, 500));
    const uint16_t max_position = static_cast<uint16_t>(1000 - s.minimap_scale);
    s.minimap_x = std::min<uint16_t>(s.minimap_x, max_position);
    s.minimap_y = std::min<uint16_t>(s.minimap_y, max_position);
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
    gpu_close(); g_gpu=false;
    texture_set_backend(TextureBackend::CpuDecode);
#if defined(_WIN32)
    if (g_window) DestroyWindow(g_window);
#endif
    g_open = false;
    g_minigame_active = false;
    g_output.clear();
}

bool video_is_open() { return g_open; }
VideoSettings video_get_settings() { return g_settings; }
const char *video_renderer_notice() {return g_renderer_failed?"OPENGL CPU UNAVAILABLE":"";}

bool video_load_settings(const char *path) {
    const char *filename = path && path[0] ? path : "sm64ds_pc.ini";
    FILE *file = std::fopen(filename, "rb");
    if (!file) return false;

    VideoSettings loaded;
    bool has_quality=false,legacy_upscaled=false;
    char line[160];
    while (std::fgets(line, sizeof(line), file)) {
        char *equals = std::strchr(line, '=');
        if (!equals) continue;
        *equals++ = '\0';
        char *end = nullptr;
        const unsigned long value = std::strtoul(equals, &end, 10);
        if (end == equals) continue;
        if (std::strcmp(line, "width") == 0) loaded.width = static_cast<uint32_t>(value);
        else if (std::strcmp(line, "height") == 0) loaded.height = static_cast<uint32_t>(value);
        else if (std::strcmp(line, "target_fps") == 0) loaded.target_fps = static_cast<uint32_t>(value);
        else if (std::strcmp(line, "anisotropic_filtering") == 0) loaded.anisotropic_filtering=static_cast<uint8_t>(std::min(value,255ul));
        else if (std::strcmp(line, "renderer") == 0) loaded.renderer=value==1?VideoRenderer::OpenGLSoftware:VideoRenderer::D3D11;
        else if (std::strcmp(line, "minigame_layout") == 0) loaded.minigame_layout=value<=2?static_cast<MinigameLayout>(value):MinigameLayout::TopBottom;
        else if (std::strcmp(line, "minimap") == 0) loaded.minimap = value != 0;
        else if (std::strcmp(line, "minimap_opacity") == 0) loaded.minimap_opacity = static_cast<uint8_t>(std::min(value, 255ul));
        else if (std::strcmp(line, "minimap_scale") == 0) loaded.minimap_scale = static_cast<uint16_t>(std::min(value, 65535ul));
        else if (std::strcmp(line, "minimap_x") == 0) loaded.minimap_x = static_cast<uint16_t>(std::min(value, 65535ul));
        else if (std::strcmp(line, "minimap_y") == 0) loaded.minimap_y = static_cast<uint16_t>(std::min(value, 65535ul));
        else if (std::strcmp(line, "minimap_rotate_with_player") == 0) loaded.minimap_rotate_with_player = value != 0;
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
    rt_set_target_fps(g_settings.target_fps);
    texture_set_quality(g_settings.texture_quality);
    gpu_set_anisotropic_filtering(g_settings.anisotropic_filtering);
    return true;
}

bool video_save_settings(const char *path) {
    const char *filename = path && path[0] ? path : "sm64ds_pc.ini";
    char temporary[1024];
    const int length = std::snprintf(temporary, sizeof(temporary), "%s.tmp", filename);
    if (length < 0 || static_cast<size_t>(length) >= sizeof(temporary)) return false;
    FILE *file = std::fopen(temporary, "wb");
    if (!file) return false;
    const bool write_ok = std::fprintf(file,
        "width=%u\nheight=%u\ntarget_fps=%u\nminimap=%u\nminimap_opacity=%u\n"
        "minimap_scale=%u\nminimap_x=%u\nminimap_y=%u\nupscale_2d=%u\nhd_textures=%u\n"
        "minimap_rotate_with_player=%u\nminimap_hide_touch_target=%u\ntexture_quality=%u\nanisotropic_filtering=%u\nminigame_layout=%u\nrenderer=%u\n",
        g_settings.width, g_settings.height, rt_target_fps(), g_settings.minimap ? 1u : 0u,
        static_cast<unsigned>(g_settings.minimap_opacity),
        static_cast<unsigned>(g_settings.minimap_scale),
        static_cast<unsigned>(g_settings.minimap_x), static_cast<unsigned>(g_settings.minimap_y),
        g_settings.upscale_2d ? 1u : 0u, g_settings.texture_quality==TextureQuality::Upscaled ? 1u : 0u,
        g_settings.minimap_rotate_with_player ? 1u : 0u,
        g_settings.minimap_hide_touch_target ? 1u : 0u,
        static_cast<unsigned>(g_settings.texture_quality),
        static_cast<unsigned>(g_settings.anisotropic_filtering),static_cast<unsigned>(g_settings.minigame_layout),
        static_cast<unsigned>(g_settings.renderer)) >= 0;
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
    clamp_settings(g_settings);
    if (g_settings.target_fps != 60 && g_settings.target_fps != 120 &&
        g_settings.target_fps != 240) g_settings.target_fps = 60;
    rt_set_target_fps(g_settings.target_fps);
    texture_set_quality(g_settings.texture_quality);
    gpu_set_anisotropic_filtering(g_settings.anisotropic_filtering);
#if defined(_WIN32)
    if (g_window) {
        RECT rect{0, 0, static_cast<LONG>(g_settings.width),
                  static_cast<LONG>(g_settings.height)};
        AdjustWindowRect(&rect, static_cast<DWORD>(GetWindowLongA(g_window, GWL_STYLE)), FALSE);
        SetWindowPos(g_window, nullptr, 0, 0, rect.right - rect.left,
                     rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);
    }
#endif
    if (g_open) video_save_settings();
}

void video_cycle_resolution(VideoSettings &settings) {
    size_t index = 0;
    for (size_t i = 0; i < kResolutionPresets.size(); ++i) {
        if (settings.width == kResolutionPresets[i][0] &&
            settings.height == kResolutionPresets[i][1]) {
            index = (i + 1) % kResolutionPresets.size();
            goto found;
        }
    }
found:
    settings.width = kResolutionPresets[index][0];
    settings.height = kResolutionPresets[index][1];
}

uint16_t video_minimap_angle(uint16_t player_heading) {
    // The map's +Z projects downward; player-forward must project upward.
    return g_settings.minimap_rotate_with_player
        ? static_cast<uint16_t>(player_heading + 0x8000u) : 0;
}

void video_set_minimap_gameplay(bool active) { g_minimap_gameplay = active; }
void video_set_menu_focus(int left,int top,int right,int bottom) {
    g_focus_left=left;g_focus_top=top;g_focus_right=right;g_focus_bottom=bottom;
}
bool video_minimap_visible() {
    return g_settings.minimap && g_minimap_gameplay && !g_pause_screen && !g_minigame_active;
}
void video_set_pause_screen(bool active, bool custom_settings) {
    g_pause_screen = active;g_hide_pause_objects = active && custom_settings;
}
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
        const uint16_t max_position = static_cast<uint16_t>(1000 - s.minimap_scale);
        if (GetAsyncKeyState(VK_LEFT) & 1) {
            s.minimap_x = static_cast<uint16_t>(s.minimap_x > 10 ? s.minimap_x - 10 : 0);
            video_set_settings(s);
        }
        if (GetAsyncKeyState(VK_RIGHT) & 1) {
            s.minimap_x = static_cast<uint16_t>(std::min<unsigned>(max_position, s.minimap_x + 10));
            video_set_settings(s);
        }
        if (GetAsyncKeyState(VK_UP) & 1) {
            s.minimap_y = static_cast<uint16_t>(s.minimap_y > 10 ? s.minimap_y - 10 : 0);
            video_set_settings(s);
        }
        if (GetAsyncKeyState(VK_DOWN) & 1) {
            s.minimap_y = static_cast<uint16_t>(std::min<unsigned>(max_position, s.minimap_y + 10));
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
    input_set_touch(!g_menu_open && have_cursor && touch_inside && (GetAsyncKeyState(VK_LBUTTON)&0x8000),touch_x,touch_y);

    ppu_scanout(ENGINE_A, g_top);
    ppu_scanout_obj(ENGINE_A, g_top);
    const bool geometry_visible=!g_pause_screen && ppu_geometry_active();
    uint8_t foreground_mask=ppu_backgrounds_above_3d();
    ppu_scanout(ENGINE_A,g_foreground,true,foreground_mask);
    if (!g_gpu && geometry_visible) {
        gx_render(g_top);
        for(int y=0;y<SCREEN_H;++y)for(int x=0;x<SCREEN_W;++x)
            if(g_foreground.px[y][x]>>24)g_top.px[y][x]=g_foreground.px[y][x];
    }
    ppu_scanout(ENGINE_B, g_bottom, true);
    if(video_bottom_objects_visible()) ppu_scanout_obj(ENGINE_B, g_bottom);
    if(g_focus_left>=0) {
        auto draw_focus=[&](Framebuffer &frame) {
            const int l=std::clamp(g_focus_left-2,0,255),r=std::clamp(g_focus_right+2,0,255);
            const int t=std::clamp(g_focus_top-2,0,191),b=std::clamp(g_focus_bottom+2,0,191);
            for(int x=l;x<=r;++x){frame.px[t][x]=0xffffdc40;frame.px[b][x]=0xffffdc40;}
            for(int y=t;y<=b;++y){frame.px[y][l]=0xffffdc40;frame.px[y][r]=0xffffdc40;}
        };
        if(engine_a_on_top)draw_focus(g_bottom);
        else {draw_focus(g_top);draw_focus(g_foreground);foreground_mask|=1;}
    }

    RECT client{};
    GetClientRect(g_window, &client);
    const int width = std::max(1L, client.right - client.left);
    const int height = std::max(1L, client.bottom - client.top);
    const Framebuffer &main_view = g_pause_screen && !dual ? g_bottom : g_top;
    g_output.assign(&main_view.px[0][0], &main_view.px[0][0] + SCREEN_W * SCREEN_H);
    g_output_width=SCREEN_W;g_output_height=SCREEN_H;
    if(dual && !g_gpu) {
        screen_compose(engine_a_on_top?g_top:g_bottom,engine_a_on_top?g_bottom:g_top,
                       screens,g_settings.width,g_settings.height,g_output);
        g_output_width=g_settings.width;g_output_height=g_settings.height;
    }
    std::array<uint32_t,SCREEN_W*SCREEN_H> gpu_map_pixels{};

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
        if (g_gpu) for (size_t i=0;i<gpu_map_pixels.size();++i) {
            const uint32_t pixel=(&map->px[0][0])[i];
            const uint32_t alpha=transparent[i]?0:((pixel>>24)*g_settings.minimap_opacity/255);
            gpu_map_pixels[i]=(pixel&0x00ffffffu)|(alpha<<24);
        }
        const int map_w = std::max(1, SCREEN_W * g_settings.minimap_scale / 1000);
        const int map_h = std::max(1, map_w * SCREEN_H / SCREEN_W);
        const int left = std::min(SCREEN_W - 1, SCREEN_W * g_settings.minimap_x / 1000);
        const int top = std::min(SCREEN_H - 1, SCREEN_H * g_settings.minimap_y / 1000);
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

    if (g_gpu) {
        const auto submit_start=std::chrono::steady_clock::now();
        if (!gpu_render(main_view,video_minimap_visible()?gpu_map_pixels.data():nullptr,
                        g_settings,geometry_visible,foreground_mask?&g_foreground.px[0][0]:nullptr,
                        dual?&screens:nullptr,dual?&g_bottom:nullptr,engine_a_on_top) || !gpu_present(width,height,dual)) {
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
