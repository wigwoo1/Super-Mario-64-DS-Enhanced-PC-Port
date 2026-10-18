#include "ntr/mmio.h"
#include "ntr/ppu.h"

#include <cstdio>
#include <cstring>

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
    std::puts("PASS: OBJ tile mapping and non-color window/unsupported bitmap modes");
    return 0;
}
