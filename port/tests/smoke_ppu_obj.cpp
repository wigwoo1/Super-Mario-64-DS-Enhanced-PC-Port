#include "ntr/mmio.h"
#include "ntr/ppu.h"
#include "ntr/texture.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>

namespace {
constexpr uint32_t kSubObj = 0x06600000u;
constexpr uint32_t kSubPalette = 0x05000600u;
constexpr uint32_t kSubOam = 0x07000400u;

void fill_4bpp_tile(uint32_t tile, uint8_t color) {
    auto *bytes = reinterpret_cast<volatile uint8_t *>(kSubObj + tile * 32u);
    for (unsigned i = 0; i < 32; ++i)
        bytes[i] = static_cast<uint8_t>(color | (color << 4));
}

void fill_8bpp_tile(uint32_t tile, uint8_t color) {
    auto *bytes = reinterpret_cast<volatile uint8_t *>(kSubObj + tile * 32u);
    for (unsigned i = 0; i < 64; ++i) bytes[i] = color;
}

void set_obj(uint16_t attr0, uint16_t attr1, uint16_t attr2) {
    auto *oam = reinterpret_cast<volatile uint16_t *>(kSubOam);
    for (unsigned i = 0; i < 128; ++i) {
        oam[i * 4] = 0x0200; // disabled unless explicitly enabled below
        oam[i * 4 + 1] = 0;
        oam[i * 4 + 2] = 0;
        oam[i * 4 + 3] = 0;
    }
    oam[0] = attr0;
    oam[1] = attr1;
    oam[2] = attr2;
}

bool check_16x16(ntr::Framebuffer &frame) {
    return frame.px[8][8] == 0xffff0000u &&
           frame.px[8][16] == 0xff00ff00u &&
           frame.px[16][8] == 0xff0000ffu &&
           frame.px[16][16] == 0xffffffffu;
}
} // namespace

int main() {
    if (!ntr::io_init()) return 2;
    ntr::Framebuffer frame;
    auto *palette = reinterpret_cast<volatile uint16_t *>(kSubPalette);
    palette[0] = 0;
    palette[1] = 0x001f; // red
    palette[2] = 0x03e0; // green
    palette[3] = 0x7c00; // blue
    palette[4] = 0x7fff; // white

    // In 2D 4bpp mapping, row two is 32 tile indices after row one.
    // A 16x16 sprite starting at tile 4 therefore uses 4,5,36,37.
    fill_4bpp_tile(4, 1); fill_4bpp_tile(5, 2);
    fill_4bpp_tile(36, 3); fill_4bpp_tile(37, 4);
    set_obj(8, static_cast<uint16_t>(8 | (1u << 14)), 4);
    NTR_MMIO(uint32_t, 0x04001000) = 0x00001000; // OBJ on, character mapping 2D
    ntr::ppu_scanout_obj(ntr::ENGINE_B, frame);
    if (!check_16x16(frame)) {
        std::fprintf(stderr, "2D 4bpp OBJ tile rows were not addressed correctly\n");
        return 3;
    }

    // 8bpp indices advance by two for adjacent tiles in 2D mapping.
    fill_8bpp_tile(4, 1); fill_8bpp_tile(6, 2);
    fill_8bpp_tile(36, 3); fill_8bpp_tile(38, 4);
    set_obj(static_cast<uint16_t>(8 | (1u << 13)),
            static_cast<uint16_t>(8 | (1u << 14)), 4);
    ntr::ppu_scanout_obj(ntr::ENGINE_B, frame);
    if (!check_16x16(frame)) {
        std::fprintf(stderr, "2D 8bpp OBJ tile rows were not addressed correctly\n");
        return 4;
    }

    // Extended sub OBJ palettes select a 256-color bank via attr2 high bits.
    // Distinct colors ensure this cannot accidentally pass using normal OBJ RAM.
    auto *extended = reinterpret_cast<volatile uint16_t *>(0x068a0000u);
    extended[3*256+1] = 0x7c00;
    NTR_MMIO(uint8_t, 0x04000249) = 0x83;
    NTR_MMIO(uint32_t, 0x04001000) = 0x80001000;
    set_obj(static_cast<uint16_t>(8 | (1u << 13)), 8, 4 | (3u << 12));
    ntr::ppu_scanout_obj(ntr::ENGINE_B,frame);
    if(frame.px[8][8]!=0xff0000ffu) return 7;
    NTR_MMIO(uint32_t, 0x04001000) = 0x00001000;
    ntr::ppu_scanout_obj(ntr::ENGINE_B,frame);
    if(frame.px[8][8]!=0xffff0000u) return 8;
    NTR_MMIO(uint32_t, 0x04001000) = 0x80001000;
    NTR_MMIO(uint8_t, 0x04000249) = 0x80;
    ntr::ppu_scanout_obj(ntr::ENGINE_B,frame);
    if(frame.px[8][8]!=0xffff0000u) return 9;

    // 1D mapping keeps consecutive tile rows and honors the boundary field.
    NTR_MMIO(uint32_t, 0x04001000) = 0x00001010;
    fill_4bpp_tile(4, 1); fill_4bpp_tile(5, 2);
    fill_4bpp_tile(6, 3); fill_4bpp_tile(7, 4);
    set_obj(8, static_cast<uint16_t>(8 | (1u << 14)), 4);
    ntr::ppu_scanout_obj(ntr::ENGINE_B, frame);
    if (!check_16x16(frame)) {
        std::fprintf(stderr, "1D OBJ tile rows regressed\n");
        return 5;
    }

    const unsigned invisible_modes[]={2,3};
    for(unsigned mode:invisible_modes) {
        std::memset(&frame,0x11,sizeof(frame));
        set_obj(static_cast<uint16_t>(8 | (mode<<10)),
                static_cast<uint16_t>(8 | (1u<<14)),4);
        ntr::ppu_scanout_obj(ntr::ENGINE_B,frame);
        if(frame.px[8][8]!=0x11111111u) return 6;
    }
#if defined(_WIN32)
    // A 128-square affine BG3, one transparent tile, and a live OBJ marker.
    // The source key stays stable while the hardware reference matrix rotates.
    using namespace ntr;
    NTR_MMIO(uint32_t,0x04001000)=0x00011813;
    NTR_MMIO(uint16_t,0x0400100e)=0x1f12;
    NTR_MMIO(uint16_t,0x04001030)=256;
    NTR_MMIO(uint16_t,0x04001036)=256;
    auto *tiles=reinterpret_cast<uint8_t *>(0x06210000u);
    std::fill_n(tiles,64,1);
    std::fill_n(tiles+64,64,0);
    auto *map=reinterpret_cast<uint16_t *>(0x0620f800u);
    std::fill_n(map,16*16,0);
    map[0]=1;
    NTR_MMIO(uint16_t,0x05000402)=0x03e0;
    set_obj(32,32,4);
    std::vector<uint32_t> source(128*128,0xff00ff00),png(256*256,0xff2457e0);
    for(unsigned y=0;y<8;++y)std::fill_n(source.begin()+128*y,8,0);
    for(unsigned y=0;y<256;++y)std::fill_n(png.begin()+256*y+128,128,0xfff15a30);
    char filename[24];
    std::snprintf(filename,sizeof(filename),"%016llx.png",
        static_cast<unsigned long long>(texture_pixels_key(source.data(),128,128)));
    namespace fs=std::filesystem;
    const auto old=fs::current_path();
    const auto temp=old/("minimap-pack-smoke-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(temp/"textures/supir/minimap");
    fs::current_path(temp);
    const auto path=fs::path("textures/supir/minimap")/filename;
    int result=0;
    if(!texture_write_png(path.string().c_str(),png,256,256))result=10;
    ppu_minimap_content_changed();
    std::array<uint8_t,SCREEN_W*SCREEN_H> ownership{};
    std::vector<uint32_t> overlay;
    auto view=ppu_minimap_view();
    if(!ppu_minimap_pack_ready(view,true))result=11;
    const auto draw=[&] {
        ppu_scanout(ENGINE_B,frame,true,0x0f,ownership.data());
        ppu_scanout_obj(ENGINE_B,frame,ownership.data());
        ppu_minimap_pack_draw(view,&frame.px[0][0],ownership.data(),512,384,overlay);
    };
    draw();
    if(overlay.size()!=512u*384u || overlay[4*512+4] || overlay[68*512+68]!=0xffff0000u ||
       overlay[40*512+40]!=0xff2457e0u || overlay[40*512+176]!=0xfff15a30u)result=12;
    NTR_MMIO(uint16_t,0x04001030)=0;NTR_MMIO(uint16_t,0x04001032)=256;
    NTR_MMIO(uint16_t,0x04001034)=uint16_t(-256);NTR_MMIO(uint16_t,0x04001036)=0;
    NTR_MMIO(uint32_t,0x0400103c)=127u*256;
    view=ppu_minimap_view();
    if(!ppu_minimap_pack_ready(view,true))result=13;
    draw();
    if(overlay.size()!=512u*384u || overlay[40*512+176]!=0xff2457e0u ||
       overlay[176*512+40]!=0xfff15a30u || overlay[68*512+68]!=0xffff0000u)result=14;
    // Pausing repurposes the hardware background. A saved view can still
    // select the same atlas and paint the saved marker/composite pixels.
    const auto rotated=overlay;
    NTR_MMIO(uint32_t,0x04001000)=0x00010000;
    texture_set_quality(TextureQuality::Upscaled);
    if(ppu_minimap_view().key || !ppu_minimap_pack_ready(view,true))result=16;
    ppu_minimap_pack_draw(view,&frame.px[0][0],ownership.data(),512,384,overlay);
    if(overlay!=rotated)result=17;
    NTR_MMIO(uint32_t,0x04001000)=0x00011813;
    map[0]=0;ppu_minimap_content_changed();
    if(ppu_minimap_pack_ready(ppu_minimap_view(),true))result=15;
    fs::current_path(old);
    fs::remove_all(temp);
    if(result)return result;
#endif
    std::puts("PASS: OBJ tile mapping and non-color window/unsupported bitmap modes");
    return 0;
}
