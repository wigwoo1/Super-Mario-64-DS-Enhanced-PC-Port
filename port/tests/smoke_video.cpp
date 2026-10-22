#include "ntr/video.h"
#include "ntr/texture.h"
#include "ntr/rt.h"
#include "ntr/minimap_drag.h"
#include "ntr/menu_focus.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <wincodec.h>

static bool write_test_png(const char *path) {
    HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool uninit = SUCCEEDED(init);
    if (FAILED(init) && init != RPC_E_CHANGED_MODE) return false;
    IWICImagingFactory *factory = nullptr;
    IWICStream *stream = nullptr;
    IWICBitmapEncoder *encoder = nullptr;
    IWICBitmapFrameEncode *frame = nullptr;
    IPropertyBag2 *properties = nullptr;
    bool ok = false;
    do {
        HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
        if (FAILED(hr) || FAILED(factory->CreateStream(&stream))) break;
        const std::wstring wide_path(path, path + strlen(path));
        if (FAILED(stream->InitializeFromFilename(wide_path.c_str(), GENERIC_WRITE))) break;
        if (FAILED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) ||
            FAILED(encoder->Initialize(stream, WICBitmapEncoderNoCache)) ||
            FAILED(encoder->CreateNewFrame(&frame, &properties)) ||
            FAILED(frame->Initialize(properties)) || FAILED(frame->SetSize(4, 4))) break;
        WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
        if (FAILED(frame->SetPixelFormat(&format)) || format != GUID_WICPixelFormat32bppBGRA) break;
        unsigned char pixels[4 * 4 * 4];
        for (int i = 0; i < 16; ++i) {
            pixels[i * 4 + 0] = 0x30; // B
            pixels[i * 4 + 1] = 0x20; // G
            pixels[i * 4 + 2] = 0x10; // R
            pixels[i * 4 + 3] = 0xFF; // A
        }
        if (FAILED(frame->WritePixels(4, 16, sizeof(pixels), pixels)) ||
            FAILED(frame->Commit()) || FAILED(encoder->Commit())) break;
        ok = true;
    } while (false);
    if (properties) properties->Release();
    if (frame) frame->Release();
    if (encoder) encoder->Release();
    if (stream) stream->Release();
    if (factory) factory->Release();
    if (uninit) CoUninitialize();
    return ok;
}
#endif

int main() {
    {
        ntr::Framebuffer frame{};
        ntr::draw_menu_focus(frame,32,48,224,64);
        if(frame.px[47][100]!=0xffffef9c || frame.px[65][100]!=0xffffbd32 ||
           frame.px[56][20]!=0xff704016 || frame.px[56][128]!=0)return 240;
        // All sides clip safely, and hidden/invalid selections leave it alone.
        ntr::draw_menu_focus(frame,0,0,256,192);
        const auto corner=frame.px[0][0];
        ntr::draw_menu_focus(frame,-1,0,100,100);
        ntr::draw_menu_focus(frame,100,100,50,50);
        if(frame.px[0][0]!=corner)return 241;
    }
    {
        ntr::MinimapDrag drag;uint16_t x=710,y=35;
        if(drag.update(true,800,100,260,x,y) || !drag.active)return 200;
        if(!drag.update(true,300,400,260,x,y) || x!=210 || y!=335)return 201;
        if(!drag.update(true,-400,1500,260,x,y) || x!=0 || y!=740)return 202;
        if(drag.update(false,0,0,260,x,y) || drag.active || x!=0 || y!=740)return 203;
        if(drag.update(true,900,900,260,x,y) || drag.active)return 204;
        if(drag.update(true,100,800,260,x,y) || drag.active)return 205; // No press inside: no drag.
        drag.reset();x=y=100;
        if(drag.update(true,600,600,500,x,y) || drag.active)return 206; // Outside right/bottom edge.
        drag.reset();
        if(drag.update(true,599,599,500,x,y) || !drag.active)return 207;
        if(!drag.update(true,2000,-2000,500,x,y) || x!=500 || y!=0)return 208;
        for(unsigned scale=100;scale<=500;scale+=10)for(int delta: {-2000,0,150,2000}) {
            drag.reset();x=y=100;
            drag.update(true,100+scale/3,100+scale/2,uint16_t(scale),x,y);
            drag.update(true,delta,delta,uint16_t(scale),x,y);
            if(x!=std::clamp(delta-int(scale/3),0,1000-int(scale)) ||
               y!=std::clamp(delta-int(scale/2),0,1000-int(scale)))return 209;
        }
    }
    if (ntr::video_pause_screen()) return 22;
    for(unsigned paused=0;paused<2;++paused)for(unsigned custom=0;custom<2;++custom) {
        ntr::video_set_pause_screen(paused!=0,custom!=0);
        if(ntr::video_bottom_objects_visible()==bool(paused && custom))return 90;
    }
    ntr::video_set_pause_screen(true);
    if (!ntr::video_pause_screen()) return 23;
    ntr::video_set_pause_screen(false);
    uint32_t map_pixels[25];
    uint8_t map_transparent[25];
    for (uint32_t &pixel : map_pixels) pixel = 0xFF5AEF18u;
    for (int y = 1; y < 4; ++y)
        for (int x = 1; x < 4; ++x)
            map_pixels[y * 5 + x] = 0xFF234567u;
    map_pixels[7] = 0x00000000u; // A genuinely transparent source texel.
    map_pixels[12] = 0xFF5AEF18u; // Same color, enclosed inside the map.
    ntr::video_minimap_transparency_mask(map_pixels, map_transparent, 5, 5);
    if (!map_transparent[0] || !map_transparent[24] || !map_transparent[7] ||
        map_transparent[12] || map_transparent[6]) return 18;

    ntr::VideoSettings settings;
    // Title/file screens are not maps; pausing also suppresses the overlay.
    for (int enabled=0; enabled<2; ++enabled)
        for (int gameplay=0; gameplay<2; ++gameplay)
            for (int paused=0; paused<2; ++paused) {
                settings.minimap=enabled!=0;
                ntr::video_set_settings(settings);
                ntr::video_set_minimap_gameplay(gameplay!=0);
                ntr::video_set_pause_screen(paused!=0);
                if(ntr::video_minimap_visible() != (enabled && gameplay && !paused)) return 30;
            }
    ntr::video_set_pause_screen(false);
    ntr::video_set_minimap_gameplay(true);
    ntr::video_set_cutscene_letter(true);
    if(ntr::video_minimap_visible())return 40;
    ntr::video_set_cutscene_letter(false);
    settings=ntr::VideoSettings{};
    ntr::video_set_minimap_gameplay(false);
    ntr::video_set_pause_screen(false);
    if (settings.minimap_rotate_with_player || !settings.minimap_hide_touch_target) return 19;
    ntr::video_set_settings(settings);
    if (ntr::video_minimap_angle(0x4567) != 0) return 20;
    settings.minimap_rotate_with_player = true;
    ntr::video_set_settings(settings);
    if (ntr::video_minimap_angle(0x4567) != 0x4567 ||
        ntr::video_minimap_angle(0xffff) != 0xffff ||
        ntr::video_minimap_angle(0x8000) != 0x8000 ||
        ntr::video_minimap_angle(0) != 0) return 21;
    settings.minimap_scale = 500;
    settings.minimap_x = settings.minimap_y = 1000;
    ntr::video_set_settings(settings);
    const ntr::VideoSettings clamped = ntr::video_get_settings();
    if (clamped.minimap_x != 500 || clamped.minimap_y != 500) return 14;
    settings = clamped;
    settings.minimap_x = 250;
    settings.minimap_y = 350;
    ntr::video_set_settings(settings);
    if (ntr::video_get_settings().minimap_x != 250 ||
        ntr::video_get_settings().minimap_y != 350) return 15;

    settings.upscale_2d = true;
    ntr::video_set_settings(settings);
    if (!ntr::video_get_settings().upscale_2d) return 4;
    settings.upscale_2d = false;
    settings.texture_quality = ntr::TextureQuality::Upscaled;
    ntr::video_set_settings(settings);
    if (ntr::video_get_settings().upscale_2d || !ntr::texture_upscaled()) return 5;
    settings.texture_quality = ntr::TextureQuality::Original;
    ntr::video_set_settings(settings);
    if (ntr::texture_upscaled()) return 6;

    const uint8_t texel[] = {0x00, 0x80};
    const uint8_t palette[] = {0, 0};
    ntr::TextureDesc desc;
    desc.data = texel; desc.data_len = sizeof(texel);
    desc.pal = palette; desc.pal_len = sizeof(palette);
    desc.width = desc.height = 1; desc.format = 7;
    std::vector<uint32_t> decoded;
    int decoded_width = 0, decoded_height = 0;
    if (ntr::texture_replacement_key(desc) == 0 ||
        !ntr::texture_decode(desc, decoded, &decoded_width, &decoded_height) ||
        decoded_width != 1 || decoded_height != 1 || decoded.size() != 1) return 7;
#if defined(_WIN32)
    char old_directory[MAX_PATH] = {};
    char temp_directory[MAX_PATH] = {};
    if (!GetCurrentDirectoryA(MAX_PATH, old_directory)) return 10;
    wsprintfA(temp_directory, "%s\\sm64ds_texture_smoke_%lu", old_directory,
              static_cast<unsigned long>(GetCurrentProcessId()));
    RemoveDirectoryA(temp_directory);
    if (!CreateDirectoryA(temp_directory, nullptr)) {
        std::fprintf(stderr, "CreateDirectoryA failed for '%s': %lu\n", temp_directory,
                     static_cast<unsigned long>(GetLastError()));
        return 11;
    }
    if (!SetCurrentDirectoryA(temp_directory)) return 13;
    CreateDirectoryA("textures", nullptr);
    CreateDirectoryA("textures\\4k", nullptr);
    const std::string replacement = ntr::texture_replacement_filename(desc);
    if (!write_test_png(replacement.c_str())) return 12;
    ntr::texture_set_upscaled(true);
    // A matching PNG must not replace unknown/shared or character textures.
    for(auto role:{ntr::TextureRole::Unknown,ntr::TextureRole::Character}) {
        desc.role=role;
        if(!ntr::texture_decode(desc,decoded,&decoded_width,&decoded_height) ||
            decoded_width!=1 || decoded_height!=1 || decoded[0]!=0xff000000) return 24;
    }
    desc.role=ntr::TextureRole::Object;
    if (!ntr::texture_decode(desc, decoded, &decoded_width, &decoded_height) ||
        decoded_width != 4 || decoded_height != 4 || decoded.size() != 16 ||
        decoded.front() != 0xFF102030u) return 9;
    ntr::texture_set_upscaled(false);
    DeleteFileA(replacement.c_str());
    DeleteFileA("textures\\4k\\manifest.tsv");
    RemoveDirectoryA("textures\\4k");
    RemoveDirectoryA("textures");
    SetCurrentDirectoryA(old_directory);
    RemoveDirectoryA(temp_directory);
#endif

    const double expected[] = {16.0 / 9.0, 16.0 / 10.0, 21.0 / 9.0};
    for (int i = 0; i < 13; ++i) {
        if (settings.width > 3840 || settings.height > 2160 ||
            settings.width < 256 || settings.height < 192) return 1;
        const double ratio = static_cast<double>(settings.width) / settings.height;
        const int group = i / 4;
        if (i<12 && std::abs(ratio - expected[group]) > 0.035) return 2;
        if(i==12 && (settings.width!=256 || settings.height!=192))return 28;
        ntr::video_cycle_resolution(settings);
    }
    if (settings.width != 1280 || settings.height != 720) return 3;
    ntr::video_cycle_resolution(settings,-1);
    if(settings.width!=256 || settings.height!=192)return 29;
    ntr::video_cycle_resolution(settings);

    const char *settings_path = "sm64ds-video-settings-smoke.ini";
    settings.width=256;settings.height=192;ntr::video_set_settings(settings);
    if(!ntr::video_save_settings(settings_path))return 33;
    ntr::video_set_settings(ntr::VideoSettings{});
    if(!ntr::video_load_settings(settings_path) || ntr::video_get_settings().width!=256 || ntr::video_get_settings().height!=192)return 34;
    settings.width = 3840;
    settings.height = 2160;
    settings.target_fps = 240;
    settings.anisotropic_filtering = 16;
    settings.renderer = ntr::VideoRenderer::OpenGLSoftware;
    settings.minimap = false;
    settings.minimap_opacity = 137;
    settings.minimap_scale = 310;
    settings.minimap_x = 420;
    settings.minimap_y = 170;
    settings.upscale_2d = true;
    settings.texture_quality = ntr::TextureQuality::Upscaled;
    settings.minimap_rotate_with_player = true;
    settings.minimap_hide_touch_target = false;
    settings.stay_in_stage_after_star = true; settings.brain_rot_text=true; settings.camera_zoom=3; settings.camera_mode=1; settings.always_run=true;
    settings.camera_sensitivity=175;settings.camera_invert_x=true;settings.camera_invert_y=true;settings.camera_acceleration=2;
    settings.pause_music_rec_room=true;
    settings.render_distance=ntr::RenderDistance::EntireMap;
    ntr::video_set_settings(settings);
    if (!ntr::video_save_settings(settings_path)) return 16;
    settings.width = 1280;
    settings.height = 720;
    settings.target_fps = 60;
    settings.renderer = ntr::VideoRenderer::D3D11;
    settings.minimap_rotate_with_player = false;
    settings.minimap_hide_touch_target = true;
    settings.stay_in_stage_after_star = false; settings.brain_rot_text=false; settings.camera_zoom=0; settings.always_run=false;
    settings.camera_sensitivity=100;settings.camera_invert_x=false;settings.camera_invert_y=false;settings.camera_acceleration=1;
    settings.pause_music_rec_room=false;
    settings.render_distance=ntr::RenderDistance::Minimum;
    ntr::video_set_settings(settings);
    if (!ntr::video_load_settings(settings_path)) return 17;
    const ntr::VideoSettings loaded = ntr::video_get_settings();
    if(loaded.camera_sensitivity!=175 || !loaded.camera_invert_x || !loaded.camera_invert_y || loaded.camera_acceleration!=2)return 34;
    if(!loaded.pause_music_rec_room)return 35;
    if (loaded.width != 3840 || loaded.height != 2160 || loaded.target_fps != 240 ||
        ntr::rt_target_fps() != 240 || loaded.anisotropic_filtering!=16 ||
        loaded.renderer!=ntr::VideoRenderer::OpenGLSoftware || loaded.minimap || loaded.minimap_opacity != 137 ||
        loaded.minimap_scale != 310 || loaded.minimap_x != 420 || loaded.minimap_y != 170 ||
        !loaded.upscale_2d || loaded.texture_quality!=ntr::TextureQuality::Upscaled || !ntr::texture_upscaled() ||
        !loaded.minimap_rotate_with_player || loaded.minimap_hide_touch_target || !loaded.stay_in_stage_after_star || !loaded.brain_rot_text || loaded.camera_zoom!=3 || loaded.camera_mode!=1 || !loaded.always_run || loaded.render_distance!=ntr::RenderDistance::EntireMap) return 18;
    // Every saved quality overrides the historical toggle, in either key order.
    for(unsigned quality=0;quality<6;++quality) {
        settings.texture_quality=static_cast<ntr::TextureQuality>(quality);
        ntr::video_set_settings(settings);
        if(!ntr::video_save_settings(settings_path)) return 25;
        ntr::video_set_settings(ntr::VideoSettings{});
        if(!ntr::video_load_settings(settings_path) ||
            ntr::video_get_settings().texture_quality!=static_cast<ntr::TextureQuality>(quality) ||
            ntr::texture_quality()!=static_cast<ntr::TextureQuality>(quality)) return 26;
    }
    const char *legacy[]={"hd_textures=1\n","hd_textures=0\n",
        "texture_quality=2\nhd_textures=1\n","hd_textures=1\ntexture_quality=3\n",
        "hd_textures=1\ntexture_quality=999\n"};
    const unsigned expected_quality[]={5,0,2,3,0};
    for(unsigned i=0;i<5;++i) {
        FILE *f=std::fopen(settings_path,"wb");if(!f)return 27;
        std::fputs(legacy[i],f);std::fclose(f);
        if(!ntr::video_load_settings(settings_path) ||
            static_cast<unsigned>(ntr::video_get_settings().texture_quality)!=expected_quality[i]) return 28;
    }
    settings.texture_quality=static_cast<ntr::TextureQuality>(255);ntr::video_set_settings(settings);
    if(ntr::video_get_settings().texture_quality!=ntr::TextureQuality::Original) return 29;
    settings.anisotropic_filtering=3;ntr::video_set_settings(settings);
    if(ntr::video_get_settings().anisotropic_filtering!=0) return 30;
    settings.renderer=static_cast<ntr::VideoRenderer>(255);ntr::video_set_settings(settings);
    if(ntr::video_get_settings().renderer!=ntr::VideoRenderer::D3D11)return 31;
    settings.render_distance=static_cast<ntr::RenderDistance>(255);ntr::video_set_settings(settings);
    if(ntr::video_get_settings().render_distance!=ntr::RenderDistance::Minimum)return 32;
    std::remove(settings_path);
#if defined(_WIN32)
    _putenv_s("SM64DS_PC_SETTINGS_FILE","smoke-isolated-settings.ini");
    if(!ntr::video_save_settings() || !ntr::video_load_settings())return 242;
    FILE *isolated=std::fopen("smoke-isolated-settings.ini","rb");
    if(!isolated)return 243;
    std::fclose(isolated);std::remove("smoke-isolated-settings.ini");
    _putenv_s("SM64DS_PC_SETTINGS_FILE","");
#endif
    std::puts("video smoke: Native DS, 16:9, 16:10, 21:9 presets through 4K pass");
    return 0;
}
