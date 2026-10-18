// DS texture formats -> RGBA.
#ifndef NTR_TEXTURE_H
#define NTR_TEXTURE_H

#include <stdint.h>
#include <string>
#include <vector>

namespace ntr {

enum class TextureQuality : uint8_t { Original, Percent75, Percent50, Percent25, None, Upscaled };
enum class TextureRole : uint8_t { Unknown, Level, Object, Character };
enum class TextureBackend : uint8_t { CpuDecode, NativeGpu, SoftwareReference };
void texture_set_backend(TextureBackend backend);
TextureBackend texture_backend();
uint32_t texture_replacement_epoch();
struct TextureReplacementRef {
    uint64_t key=0;
    TextureRole role=TextureRole::Unknown;
    bool eligible=false;
    uint8_t reserved[6]={};
};
static_assert(sizeof(TextureReplacementRef)==16,"Replacement reference layout");
struct TexturePixelsInfo {
    uint64_t key = 0;
    bool partial_alpha = false;
    bool immutable = false;
    // Explicit zero bytes keep the geometry pump's strict byte comparison
    // deterministic; otherwise MSVC copies six uninitialized padding bytes.
    uint8_t reserved[6] = {};
};
static_assert(sizeof(TexturePixelsInfo)==16,"Texture pixel metadata layout");
// Compute once for owned, immutable decoded pixels. Mutable bindings must use
// the default empty info so GPU uploads continue to detect pixel changes.
TexturePixelsInfo texture_pixels_info(const uint32_t *pixels,int width,int height);
uint64_t texture_pixels_key(const uint32_t *pixels,int width,int height);
struct TexturePixelWork {uint64_t calls=0,texels=0;};
TexturePixelWork texture_pixel_work_reset();
TextureQuality texture_quality();
void texture_set_quality(TextureQuality quality);
TextureQuality texture_cycle_quality(TextureQuality quality, int direction = 1);
const char *texture_quality_name(TextureQuality quality);

struct TextureDesc {
    const uint8_t *data = nullptr;   int32_t data_len = 0;
    const uint8_t *index = nullptr;  int32_t index_len = 0;   // format 5 only
    const uint8_t *pal = nullptr;    int32_t pal_len = 0;
    int width = 0, height = 0;
    int format = 0;                  // TEXIMAGE_PARAM bits 26..28
    bool color0_transparent = false;
    // Unknown/shared ownership must retain original art in Upscaled mode.
    // Native model scopes supply roles; unresolved assets remain Unknown.
    TextureRole role = TextureRole::Unknown;
    bool defer_replacement = false;
};

// Decode into `out` (width*height, 0xAARRGGBB). False if the format is
// unsupported or the buffers are too short for the stated dimensions.
bool texture_decode(const TextureDesc &d, std::vector<uint32_t> &out,
                    int *out_width = nullptr, int *out_height = nullptr);

// Optional user texture replacements live at textures/4k/<key>.png relative
// to the game working directory. The key is deterministic for the DS texture.
uint64_t texture_replacement_key(const TextureDesc &d);
std::string texture_replacement_filename(const TextureDesc &d);
void texture_set_upscaled(bool enabled);
bool texture_upscaled();
// Tooling exports original references/test fixtures through the same PNG codec.
bool texture_write_png(const char *path,const std::vector<uint32_t> &pixels,int width,int height);
bool texture_load_replacement_pixels(uint64_t key,std::vector<uint32_t> &pixels,int &width,int &height,
                                     uint64_t max_bytes=64ull*1024*1024);

}  // namespace ntr

#endif
