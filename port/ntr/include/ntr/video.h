// Native host display for the two DS engines.
#ifndef NTR_VIDEO_H
#define NTR_VIDEO_H

#include <stdint.h>
#include "ntr/texture.h"
#include "ntr/screen_layout.h"

namespace ntr {
enum class VideoRenderer : uint8_t { D3D11, OpenGLSoftware };

struct VideoSettings {
    uint32_t width = 1280;
    uint32_t height = 720;
    uint32_t target_fps = 60;
    uint8_t anisotropic_filtering = 0; // 0=off, otherwise 2/4/8/16x
    VideoRenderer renderer = VideoRenderer::D3D11;
    MinigameLayout minigame_layout = MinigameLayout::TopBottom;
    bool minimap = true;
    uint8_t minimap_opacity = 190;  // 0 transparent, 255 opaque
    uint16_t minimap_scale = 260;   // width as tenths of a percent of output width
    uint16_t minimap_x = 710;       // position as tenths of a percent
    uint16_t minimap_y = 35;
    bool minimap_rotate_with_player = false; // otherwise lock map upright
    bool minimap_hide_touch_target = true;
    bool upscale_2d = false;        // crisp pixel-preserving window upscaling
    TextureQuality texture_quality = TextureQuality::Original;
};

// Create a resizable native window. Resolution and minimap layout are changed
// through video_set_settings, so a game settings menu can bind to this API.
bool video_open(const char *title = "Super Mario 64 DS");
void video_close();
bool video_is_open();
VideoSettings video_get_settings();
void video_set_settings(const VideoSettings &settings);
const char *video_renderer_notice();
// Load/save the user settings file. A null path uses sm64ds_pc.ini in the
// application's working directory (the executable folder in the PC build).
bool video_load_settings(const char *path = nullptr);
bool video_save_settings(const char *path = nullptr);
// Cycle through the supported 16:9, 16:10, and 21:9 output presets up to 4K.
void video_cycle_resolution(VideoSettings &settings);
uint16_t video_minimap_angle(uint16_t player_heading);
void video_set_minimap_gameplay(bool active);
bool video_minimap_visible();
// Present the original bottom-screen pause/options interface as the main view.
void video_set_pause_screen(bool active, bool custom_settings = false);
// Custom settings use background tiles; gameplay map sprites obscure their text.
bool video_bottom_objects_visible();
bool video_pause_screen();
void video_set_minigame_active(bool active);
bool video_dual_screen_active();
// Controller focus on the original touch menu; negative left hides it.
void video_set_menu_focus(int left,int top,int right,int bottom);

// Marks transparent source pixels and the edge-connected dominant border
// color, so a flat DS map fill can be omitted from the PC minimap overlay.
void video_minimap_transparency_mask(const uint32_t *pixels, uint8_t *mask,
                                     uint32_t width, uint32_t height);

// Poll native window events and rasterize 3D at the selected native resolution.
// Retail 2D layers are scaled separately; the bottom engine is the minimap or
// full pause view. D3D11 is the default; software remains a diagnostic fallback.
bool video_present_frame();

}  // namespace ntr

#endif
