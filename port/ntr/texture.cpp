// DS texture decoding to RGBA.
//
// Formats from GBATEK. Format 5 (4x4-compressed) matters most: 84% of this
// game's 1,408 textures use it, so skipping it would mean texturing almost
// nothing.
//
// Its layout inside a BMD is confirmed by three consecutive texture records --
// each one's main data begins exactly `size + size/2` after the previous, so the
// per-block palette-index table sits immediately after the main block and is
// half its length.

#include "ntr/texture.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <new>
#include <set>
#include <string>

#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <wincodec.h>
#endif

namespace ntr {
namespace {

TextureQuality g_texture_quality = TextureQuality::Original;
TexturePixelWork pixel_work;
TextureBackend backend=TextureBackend::CpuDecode;
uint32_t replacement_epoch=1;
std::set<uint64_t> g_manifest_written;

uint64_t fnv1a(uint64_t hash, const uint8_t *bytes, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 1099511628211ull;
    }
    return hash;
}

size_t encoded_size(const TextureDesc &d) {
    const size_t n = static_cast<size_t>(d.width) * d.height;
    switch (d.format) {
        case 1: case 6: return n;
        case 2: return (n + 3) / 4;
        case 3: return (n + 1) / 2;
        case 4: return n;
        case 5: return n / 4;
        case 7: return n * 2;
        default: return 0;
    }
}

size_t palette_size(const TextureDesc &d) {
    // Hash only entries the texture format reads. Offline files have finite
    // palettes; VRAM continues into neighboring allocations. Including unused
    // padding made the same texture acquire different offline/runtime keys.
    uint32_t maximum=0;
    const size_t pixels=static_cast<size_t>(std::max(0,d.width))*std::max(0,d.height);
    if(d.format!=5 && d.format!=7) {
        for(size_t i=0;i<pixels;++i) {
            const size_t byte=d.format==2?i/4:d.format==3?i/2:i;
            if(!d.data || byte>=static_cast<size_t>(std::max(0,d.data_len))) break;
            const unsigned value=d.data[byte];
            const unsigned index=d.format==1?value&31:d.format==6?value&7:
                d.format==2?(value>>((i&3)*2))&3:d.format==3?(value>>((i&1)*4))&15:value;
            maximum=std::max(maximum,index);
        }
        return pixels?(maximum+1)*2u:0;
    }
    switch (d.format) {
        case 1: return 32u * 2u;
        case 2: return 4u * 2u;
        case 3: return 16u * 2u;
        case 4: return 256u * 2u;
        case 5: {
            uint32_t max_index = 0;
            const size_t blocks = static_cast<size_t>(d.width / 4) * (d.height / 4);
            if (!d.index) return 0;
            for (size_t block = 0; block < blocks && block * 2 + 1 < static_cast<size_t>(d.index_len); ++block) {
                const uint16_t info = static_cast<uint16_t>(d.index[block * 2] | (d.index[block * 2 + 1] << 8));
                const unsigned mode=info>>14;
                const unsigned entries=mode==0?3:mode==2?4:2;
                max_index = std::max(max_index, static_cast<uint32_t>((info & 0x3FFF) * 2u + entries));
            }
            return static_cast<size_t>(max_index) * 2u;
        }
        case 6: return 8u * 2u;
        case 7: return 0;
        default: return 0;
    }
}

#if defined(_WIN32)
template <class T> void release_com(T *&value) {
    if (value) { value->Release(); value = nullptr; }
}

bool load_replacement_png(const std::string &path, std::vector<uint32_t> &out,
                          int &width, int &height,uint64_t max_bytes=256ull*1024*1024) {
    HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool uninit = SUCCEEDED(init);
    if (FAILED(init) && init != RPC_E_CHANGED_MODE) return false;

    IWICImagingFactory *factory = nullptr;
    IWICBitmapDecoder *decoder = nullptr;
    IWICBitmapFrameDecode *frame = nullptr;
    IWICFormatConverter *converter = nullptr;
    bool ok = false;
    do {
        HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
        if (FAILED(hr)) break;
        hr = factory->CreateDecoderFromFilename(std::wstring(path.begin(), path.end()).c_str(),
            nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
        if (FAILED(hr) || FAILED(decoder->GetFrame(0, &frame))) break;
        UINT w = 0, h = 0;
        if (FAILED(frame->GetSize(&w, &h)) || !w || !h || w > 8192 || h > 8192 ||
            static_cast<uint64_t>(w) * h * 4 > max_bytes) break;
        if (FAILED(factory->CreateFormatConverter(&converter))) break;
        if (FAILED(converter->Initialize(frame, GUID_WICPixelFormat32bppBGRA,
                WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom))) break;
        try {out.resize(static_cast<size_t>(w) * h);}catch(const std::bad_alloc &) {break;}
        if (FAILED(converter->CopyPixels(nullptr, w * 4, static_cast<UINT>(out.size() * 4),
                reinterpret_cast<BYTE *>(out.data())))) { out.clear(); break; }
        width = static_cast<int>(w);
        height = static_cast<int>(h);
        ok = true;
    } while (false);

    release_com(converter);
    release_com(frame);
    release_com(decoder);
    release_com(factory);
    if (uninit) CoUninitialize();
    return ok;
}

bool write_png(const std::string &path, const std::vector<uint32_t> &pixels,
               int width, int height) {
    if (width <= 0 || height <= 0 || pixels.size() != static_cast<size_t>(width) * height)
        return false;
    const HRESULT init=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    if(FAILED(init) && init!=RPC_E_CHANGED_MODE)return false;
    const bool uninit=SUCCEEDED(init);
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
        const std::wstring wide_path(path.begin(), path.end());
        if (FAILED(stream->InitializeFromFilename(wide_path.c_str(), GENERIC_WRITE))) break;
        if (FAILED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) ||
            FAILED(encoder->Initialize(stream, WICBitmapEncoderNoCache)) ||
            FAILED(encoder->CreateNewFrame(&frame, &properties)) || FAILED(frame->Initialize(properties))) break;
        if (FAILED(frame->SetSize(static_cast<UINT>(width), static_cast<UINT>(height)))) break;
        WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
        if (FAILED(frame->SetPixelFormat(&format)) || format != GUID_WICPixelFormat32bppBGRA) break;
        if (FAILED(frame->WritePixels(static_cast<UINT>(height), static_cast<UINT>(width * 4),
                static_cast<UINT>(pixels.size() * 4),
                reinterpret_cast<BYTE *>(const_cast<uint32_t *>(pixels.data()))))) break;
        if (FAILED(frame->Commit()) || FAILED(encoder->Commit())) break;
        ok = true;
    } while (false);
    release_com(properties);
    release_com(frame);
    release_com(encoder);
    release_com(stream);
    release_com(factory);
    if(uninit)CoUninitialize();
    return ok;
}

void write_pack_manifest(uint64_t key, const TextureDesc &d) {
    if (!g_manifest_written.insert(key).second) return;
    std::error_code ec;
    std::filesystem::create_directories("textures/4k", ec);
    std::ofstream manifest("textures/4k/manifest.tsv", std::ios::app);
    if (manifest)
        manifest << std::hex << key << std::dec << '\t' << d.width << '\t'
                 << d.height << '\t' << d.format << '\n';
}
#endif

inline uint32_t bgr555(uint16_t c, bool opaque = true) {
    const uint32_t r = c & 0x1F, g = (c >> 5) & 0x1F, b = (c >> 10) & 0x1F;
    return (opaque ? 0xFF000000u : 0u) | ((r << 3 | r >> 2) << 16)
           | ((g << 3 | g >> 2) << 8) | (b << 3 | b >> 2);
}

inline uint32_t with_alpha(uint32_t argb, uint32_t a) {
    return (argb & 0x00FFFFFFu) | (a << 24);
}

inline uint16_t pal_at(const uint8_t *pal, int32_t pal_len, uint32_t idx) {
    const uint32_t off = idx * 2u;
    if (off + 1 >= static_cast<uint32_t>(pal_len)) return 0;
    return static_cast<uint16_t>(pal[off] | (pal[off + 1] << 8));
}

// Blend two BGR555 colours in 1/8ths, staying in 5-bit space like the hardware.
uint16_t blend555(uint16_t a, uint16_t b, int wa, int wb) {
    const int ar = a & 0x1F, ag = (a >> 5) & 0x1F, ab = (a >> 10) & 0x1F;
    const int br = b & 0x1F, bg = (b >> 5) & 0x1F, bb = (b >> 10) & 0x1F;
    const int r = (ar * wa + br * wb) / 8;
    const int g = (ag * wa + bg * wb) / 8;
    const int bl = (ab * wa + bb * wb) / 8;
    return static_cast<uint16_t>((r & 0x1F) | ((g & 0x1F) << 5) | ((bl & 0x1F) << 10));
}

}  // namespace

uint64_t texture_replacement_key(const TextureDesc &d) {
    if (d.width <= 0 || d.height <= 0 || !d.data || (d.format!=7 && !d.pal)) return 0;
    uint64_t hash = 14695981039346656037ull;
    const uint32_t fields[] = {static_cast<uint32_t>(d.width), static_cast<uint32_t>(d.height),
        static_cast<uint32_t>(d.format), static_cast<uint32_t>(d.color0_transparent)};
    hash = fnv1a(hash, reinterpret_cast<const uint8_t *>(fields), sizeof(fields));
    const size_t data_bytes = std::min(encoded_size(d), static_cast<size_t>(std::max(0, d.data_len)));
    hash = fnv1a(hash, d.data, data_bytes);
    const size_t index_bytes = d.format == 5 ? std::min(static_cast<size_t>(std::max(0, d.index_len)),
        static_cast<size_t>(d.width / 4) * (d.height / 4) * 2) : 0;
    if (index_bytes && d.index) hash = fnv1a(hash, d.index, index_bytes);
    const size_t pal_bytes = std::min(palette_size(d), static_cast<size_t>(std::max(0, d.pal_len)));
    hash = fnv1a(hash, d.pal, pal_bytes);
    return hash;
}

TexturePixelsInfo texture_pixels_info(const uint32_t *pixels,int width,int height) {
    TexturePixelsInfo info;
    if(!pixels || width<=0 || height<=0)return info;
    info.key=14695981039346656037ull;info.immutable=true;
    auto mix=[&info](uint32_t value) {info.key^=value;info.key*=1099511628211ull;};
    mix(static_cast<uint32_t>(width));mix(static_cast<uint32_t>(height));
    const size_t count=static_cast<size_t>(width)*height;
    for(size_t i=0;i<count;++i) {
        const auto pixel=pixels[i];mix(pixel);
        info.partial_alpha|=(pixel>>24)>0 && (pixel>>24)<255;
    }
    ++pixel_work.calls;pixel_work.texels+=count;
    return info;
}
uint64_t texture_pixels_key(const uint32_t *pixels,int width,int height) {
    if(!pixels || width<=0 || height<=0)return 0;
    uint64_t key=14695981039346656037ull;
    auto mix=[&key](uint32_t value) {key^=value;key*=1099511628211ull;};
    mix(static_cast<uint32_t>(width));mix(static_cast<uint32_t>(height));
    const size_t count=static_cast<size_t>(width)*height;
    for(size_t i=0;i<count;++i)mix(pixels[i]);
    ++pixel_work.calls;pixel_work.texels+=count;
    return key;
}
TexturePixelWork texture_pixel_work_reset() {const auto work=pixel_work;pixel_work={};return work;}

std::string texture_replacement_filename(const TextureDesc &d) {
    char name[64];
    std::snprintf(name, sizeof(name), "textures/4k/%016llx.png",
                  static_cast<unsigned long long>(texture_replacement_key(d)));
    return name;
}

TextureQuality texture_quality() { return g_texture_quality; }
void texture_set_backend(TextureBackend value) {if(backend!=value){backend=value;++replacement_epoch;}}
TextureBackend texture_backend() {return backend;}
uint32_t texture_replacement_epoch() {return replacement_epoch;}
void texture_set_quality(TextureQuality quality) {
    const auto value = static_cast<unsigned>(quality) <= static_cast<unsigned>(TextureQuality::Upscaled)
        ? quality : TextureQuality::Original;
    if(value!=g_texture_quality){g_texture_quality=value;++replacement_epoch;}
}
TextureQuality texture_cycle_quality(TextureQuality quality, int direction) {
    const unsigned value=static_cast<unsigned>(quality);
    return static_cast<TextureQuality>(((value<6?value:0)+(direction<0?5:1))%6);
}
const char *texture_quality_name(TextureQuality quality) {
    static const char *names[]={"ORIGINAL","75%","50%","25%","NO TEXTURES","UPSCALED"};
    const unsigned index=static_cast<unsigned>(quality);
    return names[index<6?index:0];
}
void texture_set_upscaled(bool enabled) {
    texture_set_quality(enabled?TextureQuality::Upscaled:TextureQuality::Original);
}
bool texture_upscaled() { return g_texture_quality==TextureQuality::Upscaled; }
bool texture_load_replacement_pixels(uint64_t key,std::vector<uint32_t> &pixels,int &width,int &height,uint64_t max_bytes) {
#if defined(_WIN32)
    char name[64];std::snprintf(name,sizeof(name),"textures/4k/%016llx.png",static_cast<unsigned long long>(key));
    return load_replacement_png(name,pixels,width,height,max_bytes);
#else
    return false;
#endif
}
bool texture_write_png(const char *path,const std::vector<uint32_t> &pixels,int width,int height) {
    if(!path || width<=0 || height<=0 || pixels.size()!=static_cast<size_t>(width)*height)return false;
#if defined(_WIN32)
    return write_png(path,pixels,width,height);
#else
    return false;
#endif
}

static bool decode_native(const TextureDesc &d, std::vector<uint32_t> &out) {
    if (d.width <= 0 || d.height <= 0) return false;
    const size_t n = static_cast<size_t>(d.width) * d.height;
    out.assign(n, 0);

    switch (d.format) {
        case 1: {  // A3I5: 5-bit index, 3-bit alpha
            if (d.data_len < static_cast<int32_t>(n)) return false;
            for (size_t i = 0; i < n; ++i) {
                const uint8_t b = d.data[i];
                const uint32_t a = (b >> 5) & 7;
                out[i] = with_alpha(bgr555(pal_at(d.pal, d.pal_len, b & 0x1F)),
                                    (a * 255) / 7);
            }
            return true;
        }
        case 2: {  // 4-colour palette, 2bpp
            for (size_t i = 0; i < n; ++i) {
                const size_t byte = i >> 2;
                if (byte >= static_cast<size_t>(d.data_len)) break;
                const uint32_t idx = (d.data[byte] >> ((i & 3) * 2)) & 3;
                out[i] = (idx == 0 && d.color0_transparent)
                             ? 0u : bgr555(pal_at(d.pal, d.pal_len, idx));
            }
            return true;
        }
        case 3: {  // 16-colour, 4bpp
            for (size_t i = 0; i < n; ++i) {
                const size_t byte = i >> 1;
                if (byte >= static_cast<size_t>(d.data_len)) break;
                const uint32_t idx = (i & 1) ? (d.data[byte] >> 4) : (d.data[byte] & 0xF);
                out[i] = (idx == 0 && d.color0_transparent)
                             ? 0u : bgr555(pal_at(d.pal, d.pal_len, idx));
            }
            return true;
        }
        case 4: {  // 256-colour, 8bpp
            for (size_t i = 0; i < n; ++i) {
                if (i >= static_cast<size_t>(d.data_len)) break;
                const uint32_t idx = d.data[i];
                out[i] = (idx == 0 && d.color0_transparent)
                             ? 0u : bgr555(pal_at(d.pal, d.pal_len, idx));
            }
            return true;
        }
        case 5: {  // 4x4-compressed
            const int bw = d.width / 4, bh = d.height / 4;
            const int32_t need = static_cast<int32_t>(bw) * bh * 4;
            if (d.data_len < need || !d.index || d.index_len < need / 2) return false;

            for (int by = 0; by < bh; ++by) {
                for (int bx = 0; bx < bw; ++bx) {
                    const int block = by * bw + bx;
                    const uint32_t texels =
                        static_cast<uint32_t>(d.data[block * 4])
                        | (static_cast<uint32_t>(d.data[block * 4 + 1]) << 8)
                        | (static_cast<uint32_t>(d.data[block * 4 + 2]) << 16)
                        | (static_cast<uint32_t>(d.data[block * 4 + 3]) << 24);
                    const uint16_t info = static_cast<uint16_t>(
                        d.index[block * 2] | (d.index[block * 2 + 1] << 8));

                    // Palette offset is in 4-byte units, i.e. two colours.
                    const uint32_t base = (info & 0x3FFF) * 2u;
                    const uint32_t mode = (info >> 14) & 3;

                    const uint16_t c0 = pal_at(d.pal, d.pal_len, base);
                    const uint16_t c1 = pal_at(d.pal, d.pal_len, base + 1);
                    uint16_t c2 = 0, c3 = 0;
                    bool a2 = true, a3 = true;
                    switch (mode) {
                        case 0: c2 = pal_at(d.pal, d.pal_len, base + 2); c3 = 0; a3 = false; break;
                        case 1: c2 = blend555(c0, c1, 4, 4); c3 = 0; a3 = false; break;
                        case 2: c2 = pal_at(d.pal, d.pal_len, base + 2);
                                c3 = pal_at(d.pal, d.pal_len, base + 3); break;
                        case 3: c2 = blend555(c0, c1, 5, 3);
                                c3 = blend555(c0, c1, 3, 5); break;
                    }
                    const uint16_t cols[4] = {c0, c1, c2, c3};
                    const bool op[4] = {true, true, a2, a3};

                    for (int ty = 0; ty < 4; ++ty) {
                        for (int tx = 0; tx < 4; ++tx) {
                            const int bit = (ty * 4 + tx) * 2;
                            const uint32_t sel = (texels >> bit) & 3;
                            const int px = bx * 4 + tx, py = by * 4 + ty;
                            if (px >= d.width || py >= d.height) continue;
                            out[static_cast<size_t>(py) * d.width + px] =
                                op[sel] ? bgr555(cols[sel]) : 0u;
                        }
                    }
                }
            }
            return true;
        }
        case 6: {  // A5I3: 3-bit index, 5-bit alpha
            if (d.data_len < static_cast<int32_t>(n)) return false;
            for (size_t i = 0; i < n; ++i) {
                const uint8_t b = d.data[i];
                const uint32_t a = (b >> 3) & 0x1F;
                out[i] = with_alpha(bgr555(pal_at(d.pal, d.pal_len, b & 7)),
                                    (a * 255) / 31);
            }
            return true;
        }
        case 7: {  // direct 16-bit colour; bit 15 is the alpha flag
            for (size_t i = 0; i < n; ++i) {
                if ((i * 2 + 1) >= static_cast<size_t>(d.data_len)) break;
                const uint16_t c = static_cast<uint16_t>(d.data[i * 2] | (d.data[i * 2 + 1] << 8));
                out[i] = (c & 0x8000) ? bgr555(c) : 0u;
            }
            return true;
        }
        default:
            return false;
    }
}

bool texture_decode(const TextureDesc &d, std::vector<uint32_t> &out,
                    int *out_width, int *out_height) {
    if (d.width <= 0 || d.height <= 0) return false;
    if (out_width) *out_width = d.width;
    if (out_height) *out_height = d.height;
#if defined(_WIN32)
    if (!d.defer_replacement && texture_upscaled() && (d.role==TextureRole::Level || d.role==TextureRole::Object)) {
        const uint64_t key = texture_replacement_key(d);
        write_pack_manifest(key, d);
        int replacement_width = 0, replacement_height = 0;
        if (load_replacement_png(texture_replacement_filename(d), out,
                                 replacement_width, replacement_height)) {
            if (out_width) *out_width = replacement_width;
            if (out_height) *out_height = replacement_height;
            return true;
        }
        if (!decode_native(d, out)) return false;
        std::error_code ec;
        std::filesystem::create_directories("textures/source", ec);
        char name[64];
        std::snprintf(name, sizeof(name), "textures/source/%016llx.png",
                      static_cast<unsigned long long>(key));
        if (!std::filesystem::exists(name, ec))
            write_png(name, out, d.width, d.height);
        return true;
    }
#endif
    if (!decode_native(d, out)) return false;
    if (g_texture_quality==TextureQuality::None) {
        // A white material preserves vertex lighting and the original mask,
        // including partially transparent effects and transparent texels.
        for (auto &pixel:out) pixel=(pixel&0xff000000u)|0x00ffffffu;
    } else {
        int percent=100;
        if(g_texture_quality==TextureQuality::Percent75) percent=75;
        if(g_texture_quality==TextureQuality::Percent50) percent=50;
        if(g_texture_quality==TextureQuality::Percent25) percent=25;
        if(percent!=100) {
            const int width=std::max(1,d.width*percent/100);
            const int height=std::max(1,d.height*percent/100);
            std::vector<uint32_t> reduced(static_cast<size_t>(width)*height);
            // Center-sampled nearest texels keep palette colors and alpha
            // classes intact, without color bleeding across transparent edges.
            for(int y=0;y<height;++y) for(int x=0;x<width;++x) {
                const int sx=std::min(d.width-1,(2*x+1)*d.width/(2*width));
                const int sy=std::min(d.height-1,(2*y+1)*d.height/(2*height));
                reduced[static_cast<size_t>(y)*width+x]=out[static_cast<size_t>(sy)*d.width+sx];
            }
            out.swap(reduced);
            if(out_width)*out_width=width;
            if(out_height)*out_height=height;
        }
    }
    return true;
}

}  // namespace ntr
