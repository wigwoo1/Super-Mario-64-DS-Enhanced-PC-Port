// DS 2D engine, text-mode backgrounds.
//
// Register layout from GBATEK. The address arithmetic is cross-checked against
// the decomp's own G2::GetBG0ScrPtr, which computes
//
//     0x6000000 + ((DISPCNT & 0x38000000) >> 27 << 16) + ((BG0CNT & 0x1f00) >> 8 << 11)
//
// i.e. DISPCNT bits 27-29 select a 64K screen-base offset and BGxCNT bits 8-12 a
// 2K screen block. Those two shifts are the ones most likely to be wrong, so they
// are taken from code that is byte-verified against the ROM rather than from docs.

#include "ntr/ppu.h"

#include "ntr/mmio.h"
#include "ntr/rt.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace ntr {
namespace {

// Engine A registers start at 0x4000000, engine B at 0x4001000. Engine B has no
// DISPCNT char/screen base offset -- those bits are engine A only -- and its BG
// VRAM and palette live elsewhere.
struct EngineMap {
    uint32_t reg_base;
    uint32_t vram_base;
    uint32_t pltt_base;
    bool has_base_offset;
};

constexpr EngineMap kEngines[2] = {
    {0x04000000u, 0x06000000u, 0x05000000u, true},
    {0x04001000u, 0x06200000u, 0x05000400u, false},
};

inline uint16_t rd16(uint32_t a) { return *reinterpret_cast<volatile uint16_t *>(a); }
inline uint32_t rd32(uint32_t a) { return *reinterpret_cast<volatile uint32_t *>(a); }

// DS palettes are BGR555.
inline uint32_t bgr555(uint16_t c) {
    const uint32_t r = (c >> 0) & 0x1F, g = (c >> 5) & 0x1F, b = (c >> 10) & 0x1F;
    return 0xFF000000u | ((r << 3 | r >> 2) << 16) | ((g << 3 | g >> 2) << 8)
           | (b << 3 | b >> 2);
}

// Text-mode screen sizes, in tiles.
void text_size(unsigned sz, int &tw, int &th) {
    tw = (sz & 1) ? 64 : 32;
    th = (sz & 2) ? 64 : 32;
}

struct BgConfig {
    bool enabled;
    int priority;
    uint32_t screen;      // tilemap address
    uint32_t chars;       // tile data address
    bool bpp8;
    int tiles_w, tiles_h;
    int hofs, vofs;
    bool affine, map16, wrap;
    bool extended_palette;
    uint32_t palette;
    int pa, pb, pc, pd, ref_x, ref_y;
};

BgConfig read_bg(const EngineMap &em, int bg, uint32_t dispcnt) {
    BgConfig c{};
    c.enabled = (dispcnt >> (8 + bg)) & 1;
    if (!c.enabled) return c;

    const uint16_t cnt = rd16(em.reg_base + 0x08 + bg * 2);
    c.priority = cnt & 3;
    c.bpp8 = (cnt >> 7) & 1;

    const uint32_t char_off = em.has_base_offset ? ((dispcnt >> 24) & 7) << 16 : 0;
    const uint32_t scr_off = em.has_base_offset ? ((dispcnt >> 27) & 7) << 16 : 0;

    c.screen = em.vram_base + scr_off + (((cnt >> 8) & 0x1F) << 11);
    c.chars = em.vram_base + char_off + (((cnt >> 2) & 15) << 14);

    text_size((cnt >> 14) & 3, c.tiles_w, c.tiles_h);
    const unsigned mode = dispcnt & 7;
    const bool plain_affine = (bg == 3 && (mode == 1 || mode == 2)) ||
                              (bg == 2 && (mode == 2 || mode == 4));
    const bool extended = (bg == 3 && mode >= 3 && mode <= 5) ||
                          (bg == 2 && mode == 5);
    c.affine = plain_affine || (extended && !(cnt & 0x80));
    c.map16 = !plain_affine;
    if (c.affine) {
        c.bpp8 = true;
        c.tiles_w = c.tiles_h = 16 << ((cnt >> 14) & 3);
        c.wrap = (cnt & 0x2000) != 0;
        const uint32_t affine_regs = em.reg_base + 0x20 + (bg - 2) * 16;
        c.pa = (int16_t)rd16(affine_regs);
        c.pb = (int16_t)rd16(affine_regs + 2);
        c.pc = (int16_t)rd16(affine_regs + 4);
        c.pd = (int16_t)rd16(affine_regs + 6);
        c.ref_x = (int32_t)(rd32(affine_regs + 8) << 4) >> 4;
        c.ref_y = (int32_t)(rd32(affine_regs + 12) << 4) >> 4;
    }
    c.palette = em.pltt_base;
    // The sub-engine's extended BG palette occupies VRAM H in LCDC space.
    // The game's GXS uploader writes this same memory before mapping H back.
    c.extended_palette = em.reg_base == 0x04001000 && (dispcnt & (1u << 30)) &&
                         c.bpp8 && !plain_affine;
    if (c.extended_palette) {
        const int slot = bg < 2 && (cnt & 0x2000) ? bg + 2 : bg;
        c.palette = 0x06898000 + slot * 0x2000;
    }

    // BGxHOFS/BGxVOFS are write-only 9-bit scroll registers, 4 bytes per BG.
    c.hofs = rd16(em.reg_base + 0x10 + bg * 4) & 0x1FF;
    c.vofs = rd16(em.reg_base + 0x12 + bg * 4) & 0x1FF;
    return c;
}

// One text-mode screen entry: tile 0-9, hflip 10, vflip 11, palette 12-15.
inline uint32_t screen_entry_addr(const BgConfig &c, int tx, int ty) {
    // 512-wide/tall maps are stored as consecutive 32x32 blocks of 2KB.
    const int bx = tx >> 5, by = ty >> 5;
    const int blocks_w = c.tiles_w >> 5;
    const int block = by * blocks_w + bx;
    return c.screen + block * 0x800 + (((ty & 31) * 32 + (tx & 31)) * 2);
}

// Returns false where the background is transparent at this pixel.
bool sample_bg(const EngineMap &em, const BgConfig &c, int x, int y, uint32_t &out) {
    int px, py;
    if (c.affine) {
        px = (c.ref_x + c.pa * x + c.pb * y) >> 8;
        py = (c.ref_y + c.pc * x + c.pd * y) >> 8;
        if (!c.wrap && (px < 0 || py < 0 || px >= c.tiles_w * 8 || py >= c.tiles_h * 8))
            return false;
        px &= c.tiles_w * 8 - 1;
        py &= c.tiles_h * 8 - 1;
    } else {
        px = (x + c.hofs) & (c.tiles_w * 8 - 1);
        py = (y + c.vofs) & (c.tiles_h * 8 - 1);
    }

    const uint32_t entry = c.affine ? c.screen + ((py >> 3) * c.tiles_w + (px >> 3)) * (c.map16 ? 2 : 1)
                                    : screen_entry_addr(c, px >> 3, py >> 3);
    const uint16_t se = c.map16 || !c.affine ? rd16(entry) :
        *reinterpret_cast<volatile uint8_t *>(entry);
    const int tile = se & 0x3FF;
    int fx = px & 7, fy = py & 7;
    if (se & 0x400) fx = 7 - fx;
    if (se & 0x800) fy = 7 - fy;

    uint32_t index;
    if (c.bpp8) {
        index = *reinterpret_cast<volatile uint8_t *>(c.chars + tile * 64 + fy * 8 + fx);
        if (index == 0) return false;
        if (c.extended_palette) index += ((se >> 12) & 15) * 256;
    } else {
        const uint8_t pair =
            *reinterpret_cast<volatile uint8_t *>(c.chars + tile * 32 + fy * 4 + (fx >> 1));
        index = (fx & 1) ? (pair >> 4) : (pair & 0xF);
        if (index == 0) return false;
        index += ((se >> 12) & 0xF) * 16;
    }
    out = bgr555(rd16(c.palette + index * 2));
    return true;
}

// Window controls are latched per visible line because HBlank handlers can
// change WIN0H/WIN1H between lines (Wanted's expanding iris does this).
struct WindowState {
    uint32_t control;
    uint16_t h[2], v[2], inside, outside;
};
WindowState window_lines[2][SCREEN_H];
bool window_line_valid[SCREEN_H]{};
uint64_t window_frame = ~uint64_t(0);
const Framebuffer *last_bg_frame[2]{};
uint8_t last_bg_mask[2]{0x0f,0x0f};

WindowState read_windows(Engine eng) {
    const uint32_t base=kEngines[eng].reg_base;
    return {rd32(base), {rd16(base+0x40),rd16(base+0x42)},
            {rd16(base+0x44),rd16(base+0x46)},rd16(base+0x48),rd16(base+0x4a)};
}
WindowState windows_at(Engine eng,int y) {
    return window_frame==rt_frame() && window_line_valid[y]
        ? window_lines[eng][y] : read_windows(eng);
}
bool window_contains(uint16_t range,int coordinate) {
    const int start=range>>8, end=range&255;
    return start<=end ? coordinate>=start && coordinate<end
                      : coordinate>=start || coordinate<end;
}
uint8_t window_mask(const WindowState &w,int x,int y,bool object_window) {
    if(!(w.control&0xe000)) return 0x3f;
    for(int i=0;i<2;++i)
        if((w.control&(0x2000u<<i)) && window_contains(w.h[i],x) &&
           window_contains(w.v[i],y)) return (w.inside>>(8*i))&0x3f;
    if((w.control&0x9000)==0x9000 && object_window) return (w.outside>>8)&0x3f;
    return w.outside&0x3f;
}
struct ObjPlane {
    uint32_t color[SCREEN_H][SCREEN_W];
    uint8_t priority[SCREEN_H][SCREEN_W];
    bool window[SCREEN_H][SCREEN_W];
};
void raster_objects(Engine eng,ObjPlane &out);

}  // namespace

void ppu_begin_window_frame() {
    window_frame=rt_frame();
    std::memset(window_line_valid,0,sizeof(window_line_valid));
}
void ppu_capture_window_line(int y) {
    if(y<0 || y>=SCREEN_H) return;
    window_lines[0][y]=read_windows(ENGINE_A);
    window_lines[1][y]=read_windows(ENGINE_B);
    window_line_valid[y]=true;
}

uint8_t ppu_backgrounds_above_3d() {
    const uint32_t control=rd32(0x04000000);
    if((control & 0x00010108u)!=0x00010108u) return 0;
    if((control & 0x80u) || ((control>>16)&3u)!=1) return 0;
    const unsigned priority=rd16(0x04000008)&3;
    uint8_t mask=0;
    for(unsigned bg=1;bg<4;++bg)
        if((control & (1u<<(8+bg))) && (rd16(0x04000008+bg*2)&3)<priority)
            mask|=static_cast<uint8_t>(1u<<bg);
    return mask;
}

bool ppu_geometry_active() {
    const uint32_t control=rd32(0x04000000);
    return !(control&0x80u) && ((control>>16)&3u)==1 &&
           (control&0x108u)==0x108u;
}

void ppu_scanout(Engine eng, Framebuffer &fb, bool transparent_backdrop,
                 uint8_t background_mask) {
    const EngineMap &em = kEngines[eng];
    const uint32_t dispcnt = rd32(em.reg_base);

    // Display mode 0 is "display off" -- the DS shows white, not black.
    const unsigned disp_mode = (dispcnt >> 16) & 3;
    const bool forced_blank = (dispcnt >> 7) & 1;
    if (disp_mode == 0 || forced_blank) {
        const uint32_t blank = transparent_backdrop ? 0x00000000u : 0xFFFFFFFFu;
        for (int y = 0; y < SCREEN_H; ++y)
            for (int x = 0; x < SCREEN_W; ++x) fb.px[y][x] = blank;
        return;
    }

    // The opaque 3D rear-plane belongs to BG0, ahead of lower-priority
    // backgrounds and the palette backdrop. It is not palette entry zero.
    // Keep transparent clears unchanged; translucent/bitmap rear-planes need
    // their own 3D compositing support rather than an opaque replacement.
    const uint32_t clear = eng == ENGINE_A ? rd32(0x04000350) : 0;
    const bool opaque_rear_plane = eng == ENGINE_A && disp_mode == 1 &&
        (dispcnt & 0x108u) == 0x108u && (background_mask & 1u) &&
        !(rd16(0x04000060) & (1u << 14)) && ((clear >> 16) & 31u) == 31u;
    const uint32_t rear_color = bgr555(static_cast<uint16_t>(clear));
    const unsigned rear_priority = rd16(em.reg_base + 8) & 3u;
    const unsigned rear_scroll = rd16(em.reg_base + 0x10) & 511u;
    BgConfig bgs[4];
    for (int i = 0; i < 4; ++i) {
        bgs[i] = read_bg(em, i, dispcnt);
        bgs[i].enabled = bgs[i].enabled && (background_mask & (1u << i));
        // Main BG0 selects the geometry engine when DISPCNT bit 3 is set.
        // Its tile VRAM is then unrelated data, not a second 2D background.
        if (eng == ENGINE_A && i == 0 && (dispcnt & (1u << 3)))
            bgs[i].enabled = false;
    }
    if (eng == ENGINE_B && std::getenv("SM64DS_PC_TRACE")) {
        static uint32_t last_mode = ~0u;
        static uint16_t last_map = 0xffff;
        const uint16_t map_cnt = rd16(em.reg_base + 0x0e);
        if (dispcnt != last_mode || map_cnt != last_map) {
            std::fprintf(stderr,
                "[map screen] DISPCNT=%08x BG3CNT=%04x matrix=%d,%d,%d,%d origin=%08x,%08x screen=%08x chars=%08x\n",
                dispcnt, map_cnt,
                (int16_t)rd16(em.reg_base + 0x30), (int16_t)rd16(em.reg_base + 0x32),
                (int16_t)rd16(em.reg_base + 0x34), (int16_t)rd16(em.reg_base + 0x36),
                rd32(em.reg_base + 0x38), rd32(em.reg_base + 0x3c),
                bgs[3].screen, bgs[3].chars);
            last_mode = dispcnt;
            last_map = map_cnt;
            for (int bg = 0; bg < 4; ++bg)
                std::fprintf(stderr, "[map layer] bg=%d enabled=%d priority=%d cnt=%04x screen=%08x chars=%08x entry0=%04x\n",
                    bg, bgs[bg].enabled, bgs[bg].priority, rd16(em.reg_base+8+bg*2),
                    bgs[bg].screen, bgs[bg].chars, bgs[bg].enabled ? rd16(bgs[bg].screen) : 0);
        }
    }
#if defined(_WIN32)
    if (eng == ENGINE_A) {
        static uint32_t last_dispcnt = ~0u;
        static uint16_t last_bg0cnt = 0xffffu;
        const char *trace = std::getenv("SM64DS_PC_TRACE");
        const uint16_t bg0cnt = rd16(em.reg_base + 0x08);
        if (trace && trace[0] && trace[0] != '0' &&
            (dispcnt != last_dispcnt || bg0cnt != last_bg0cnt)) {
            const BgConfig &c = bgs[0];
            if(c.enabled) {
            std::fprintf(stderr,
                "[sm64ds] PPU BG0 enabled=%d DISPCNT=%08x CNT=%04x screen=%08x chars=%08x bpp8=%d size=%dx%d map00=%04x map-row3=%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x tile31=%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x tileff=%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x pal=%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x\n",
                c.enabled, dispcnt, bg0cnt, c.screen, c.chars,
                c.bpp8, c.tiles_w, c.tiles_h,
                rd16(screen_entry_addr(c, 0, 0)),
                rd16(screen_entry_addr(c, 0, 3)), rd16(screen_entry_addr(c, 1, 3)),
                rd16(screen_entry_addr(c, 2, 3)), rd16(screen_entry_addr(c, 3, 3)),
                rd16(screen_entry_addr(c, 4, 3)), rd16(screen_entry_addr(c, 5, 3)),
                rd16(screen_entry_addr(c, 6, 3)), rd16(screen_entry_addr(c, 7, 3)),
                *reinterpret_cast<volatile uint8_t *>(c.chars + 0x31 * 32),
                *reinterpret_cast<volatile uint8_t *>(c.chars + 0x31 * 32 + 1),
                *reinterpret_cast<volatile uint8_t *>(c.chars + 0x31 * 32 + 2),
                *reinterpret_cast<volatile uint8_t *>(c.chars + 0x31 * 32 + 3),
                *reinterpret_cast<volatile uint8_t *>(c.chars + 0x31 * 32 + 4),
                *reinterpret_cast<volatile uint8_t *>(c.chars + 0x31 * 32 + 5),
                *reinterpret_cast<volatile uint8_t *>(c.chars + 0x31 * 32 + 6),
                *reinterpret_cast<volatile uint8_t *>(c.chars + 0x31 * 32 + 7),
                *reinterpret_cast<volatile uint8_t *>(c.chars + 0xFF * 32),
                *reinterpret_cast<volatile uint8_t *>(c.chars + 0xFF * 32 + 1),
                *reinterpret_cast<volatile uint8_t *>(c.chars + 0xFF * 32 + 2),
                *reinterpret_cast<volatile uint8_t *>(c.chars + 0xFF * 32 + 3),
                *reinterpret_cast<volatile uint8_t *>(c.chars + 0xFF * 32 + 4),
                *reinterpret_cast<volatile uint8_t *>(c.chars + 0xFF * 32 + 5),
                *reinterpret_cast<volatile uint8_t *>(c.chars + 0xFF * 32 + 6),
                *reinterpret_cast<volatile uint8_t *>(c.chars + 0xFF * 32 + 7),
                rd16(em.pltt_base + 2), rd16(em.pltt_base + 4),
                rd16(em.pltt_base + 6), rd16(em.pltt_base + 8),
                rd16(em.pltt_base + 10), rd16(em.pltt_base + 12),
                rd16(em.pltt_base + 14), rd16(em.pltt_base + 16),
                rd16(em.pltt_base + 18), rd16(em.pltt_base + 20),
                rd16(em.pltt_base + 22), rd16(em.pltt_base + 24),
                rd16(em.pltt_base + 26), rd16(em.pltt_base + 28),
                rd16(em.pltt_base + 30));
            } else {
                std::fprintf(stderr,"[sm64ds] PPU BG0 disabled DISPCNT=%08x CNT=%04x\n",dispcnt,bg0cnt);
            }
            for (int bg = 1; bg < 4; ++bg) {
                const BgConfig &other = bgs[bg];
                std::fprintf(stderr,
                    "[sm64ds] PPU BG%d enabled=%d CNT=%04x screen=%08x chars=%08x map00=%04x\n",
                    bg, other.enabled, rd16(em.reg_base + 0x08 + bg * 2),
                    other.screen, other.chars,
                    other.enabled ? rd16(screen_entry_addr(other, 0, 0)) : 0);
            }
            if (c.enabled && c.screen == 0x0600F800u && c.tiles_w == 32) {
                for (int row = 0; row < 24; ++row) {
                    std::fprintf(stderr, "[sm64ds] PPU title row %02d: ", row);
                    for (int col = 0; col < 32; ++col) {
                        const unsigned tile = rd16(screen_entry_addr(c, col, row)) & 0x3ff;
                        const char ch = tile == 0xff ? ' '
                            : tile >= 32 && tile < 127 ? static_cast<char>(tile) : '#';
                        std::fputc(ch, stderr);
                    }
                    std::fputc('\n', stderr);
                }
            }
            std::fflush(stderr);
            last_dispcnt = dispcnt;
            last_bg0cnt = bg0cnt;
        }
    }
#endif

    ObjPlane objects;
    raster_objects(eng,objects);
    last_bg_frame[eng]=&fb;
    last_bg_mask[eng]=background_mask;
    const uint32_t backdrop = bgr555(rd16(em.pltt_base));
    const uint32_t uncovered = transparent_backdrop ? (backdrop & 0x00FFFFFFu) : backdrop;

    for (int y = 0; y < SCREEN_H; ++y) {
        const WindowState windows=windows_at(eng,y);
        for (int x = 0; x < SCREEN_W; ++x) {
            const uint8_t layers=window_mask(windows,x,y,objects.window[y][x]);
            uint32_t c = uncovered;
            // Lower priority value wins; ties break toward the lower BG number,
            // so walk priorities 3..0 and let later writes overwrite.
            for (int prio = 3; prio >= 0; --prio) {
                for (int bg = 3; bg >= 0; --bg) {
                    if (bg == 0 && opaque_rear_plane && (layers & 1u) &&
                        rear_priority == static_cast<unsigned>(prio) &&
                        ((x + rear_scroll) & 511u) < SCREEN_W) c = rear_color;
                    if (!(layers & (1u<<bg)) || !bgs[bg].enabled || bgs[bg].priority != prio) continue;
                    uint32_t s;
                    if (sample_bg(em, bgs[bg], x, y, s)) c = s;
                }
            }
            fb.px[y][x] = c;
        }
    }
    if (eng == ENGINE_B && (dispcnt & 7) == 3) {
        static bool sub_layers_captured = false;
        const char *prefix = std::getenv("SM64DS_PC_SUB_LAYERS");
        const char *after_text = std::getenv("SM64DS_PC_SUB_LAYERS_AFTER");
        const uint64_t after = after_text && after_text[0]
            ? std::strtoull(after_text, nullptr, 10) : 0;
        if (!sub_layers_captured && prefix && prefix[0] && rt_frame() >= after) {
            Framebuffer layer;
            for (int bg = 0; bg < 4; ++bg) {
                for (int y = 0; y < SCREEN_H; ++y)
                    for (int x = 0; x < SCREEN_W; ++x) {
                        uint32_t pixel = backdrop;
                        if (bgs[bg].enabled) sample_bg(em, bgs[bg], x, y, pixel);
                        layer.px[y][x] = pixel;
                    }
                char path[1024];
                std::snprintf(path, sizeof(path), "%s-bg%d.bmp", prefix, bg);
                ppu_write_bmp(path, layer);
            }
            Framebuffer coverage;
            for (int y = 0; y < SCREEN_H; ++y)
                for (int x = 0; x < SCREEN_W; ++x)
                    coverage.px[y][x] = (fb.px[y][x] >> 24)
                        ? 0xFFFFFFFFu : 0xFF000000u;
            char coverage_path[1024];
            std::snprintf(coverage_path, sizeof(coverage_path), "%s-coverage.bmp", prefix);
            ppu_write_bmp(coverage_path, coverage);
            sub_layers_captured = true;
        }
    }
}

// --- OBJ layer --------------------------------------------------------------
// GBATEK "LCD OBJ": 128 OAM entries per engine; tiles in the engine's OBJ
// VRAM region, palette at pltt+0x200. Sizes come from the shape/size table.
// Affine params sit in the filler word of entries group*4+0..3 (pa,pb,pc,pd).
namespace {
void raster_objects(Engine eng, ObjPlane &out) {
    std::memset(&out,0,sizeof(out));
    static const int kSizes[3][4][2] = {
        {{8, 8}, {16, 16}, {32, 32}, {64, 64}},    // square
        {{16, 8}, {32, 8}, {32, 16}, {64, 32}},    // wide
        {{8, 16}, {8, 32}, {16, 32}, {32, 64}},    // tall
    };
    const uint32_t oam_base = eng == ENGINE_A ? 0x07000000u : 0x07000400u;
    const uint32_t obj_vram = eng == ENGINE_A ? 0x06400000u : 0x06600000u;
    const uint32_t obj_pltt = kEngines[eng].pltt_base + 0x200u;
    // DISPCNT bit 4: OBJ 1D mapping; bits 20-21: tile-index boundary shift.
    const uint32_t dispcnt = rd32(kEngines[eng].reg_base);
    if(!(dispcnt & (1u<<12))) return;
    // Sub OBJ extended palettes live in VRAM I (LCDC 068a0000).
    // GX::SetBankForSubOBJExtPltt selects I with VRAMCNT_I=83 and bit31.
    const bool sub_obj_extended = eng == ENGINE_B && (dispcnt & (1u << 31)) &&
        ((*reinterpret_cast<volatile uint8_t *>(0x04000249u) & 0x87u) == 0x83u);
    const bool mapping_1d = (dispcnt & (1u << 4)) != 0;
    const uint32_t boundary = 32u << ((dispcnt >> 20) & 3);
    unsigned onscreen_objects = 0;
    unsigned drawn_pixels = 0;
    uint32_t oam_signature = 2166136261u;
    for (unsigned byte = 0; byte < 0x400; ++byte) {
        oam_signature = (oam_signature ^ *reinterpret_cast<volatile uint8_t *>(oam_base + byte)) * 16777619u;
    }

    for (int i = 127; i >= 0; --i) {           // low index on top: draw high first
        const uint16_t a0 = rd16(oam_base + i * 8u);
        const uint16_t a1 = rd16(oam_base + i * 8u + 2);
        const uint16_t a2 = rd16(oam_base + i * 8u + 4);
        const bool affine = a0 & 0x100;
        if (!affine && (a0 & 0x200)) continue;             // disabled
        const unsigned mode=(a0>>10)&3;
        if(mode==3) continue; // Bitmap OBJ format is not implemented.
        const bool is_window=mode==2;
        if(is_window && !(dispcnt&0x8000)) continue;
        const unsigned priority=(a2>>10)&3;
        const int shape = (a0 >> 14) & 3;
        if (shape == 3) continue;
        const int size = (a1 >> 14) & 3;
        const int w = kSizes[shape][size][0], h = kSizes[shape][size][1];
        const bool dbl = affine && (a0 & 0x200);
        const int bw = dbl ? w * 2 : w, bh = dbl ? h * 2 : h;
        int x = a1 & 0x1FF, y = a0 & 0xFF;
        if (x >= 256) x -= 512;
        if (y >= 192 && y >= 256 - bh) y -= 256;
        if (x < SCREEN_W && x + bw > 0 && y < SCREEN_H && y + bh > 0)
            ++onscreen_objects;
        const bool c256 = a0 & 0x2000;
        const bool hflip = !affine && (a1 & 0x1000);
        const bool vflip = !affine && (a1 & 0x2000);
        const uint32_t tile = a2 & 0x3FF;
        const uint32_t pal = (a2 >> 12) & 0xF;

        int pa = 256, pb = 0, pc = 0, pd = 256;
        if (affine) {
            const int grp = (a1 >> 9) & 0x1F;
            pa = (int16_t)rd16(oam_base + (grp * 4 + 0) * 8u + 6);
            pb = (int16_t)rd16(oam_base + (grp * 4 + 1) * 8u + 6);
            pc = (int16_t)rd16(oam_base + (grp * 4 + 2) * 8u + 6);
            pd = (int16_t)rd16(oam_base + (grp * 4 + 3) * 8u + 6);
        }

        for (int sy = 0; sy < bh; ++sy) {
            const int py = y + sy;
            if (py < 0 || py >= SCREEN_H) continue;
            for (int sx = 0; sx < bw; ++sx) {
                const int px = x + sx;
                if (px < 0 || px >= SCREEN_W) continue;
                // texel coordinate: affine maps through the matrix around the
                // box center; plain is direct with optional flips.
                int tx, ty;
                if (affine) {
                    const int cx = sx - bw / 2, cy = sy - bh / 2;
                    tx = ((pa * cx + pb * cy) >> 8) + w / 2;
                    ty = ((pc * cx + pd * cy) >> 8) + h / 2;
                    if (tx < 0 || tx >= w || ty < 0 || ty >= h) continue;
                } else {
                    tx = hflip ? w - 1 - sx : sx;
                    ty = vflip ? h - 1 - sy : sy;
                }
                const int tcol = tx >> 3, trow = ty >> 3;
                const int fx = tx & 7, fy = ty & 7;
                // 1D mapping: tiles of one sprite are consecutive.
                const int tiles_per_row = w / 8;
                const uint32_t tno = trow * tiles_per_row + tcol;
                const uint32_t tile_bytes = c256 ? 64u : 32u;
                uint32_t index;
                if (c256) {
                    const uint32_t addr = mapping_1d
                        ? obj_vram + tile * boundary + tno * tile_bytes + fy * 8u + fx
                        : obj_vram + (tile >> 5) * 1024u + (tile & 31u) * 32u
                            + trow * 1024u + tcol * tile_bytes + fy * 8u + fx;
                    index = *reinterpret_cast<volatile uint8_t *>(addr);
                    if (!index) continue;
                    if(is_window) out.window[py][px]=true;
                    else if(!out.color[py][px] || priority<=out.priority[py][px]) {
                        const uint32_t palette_base = sub_obj_extended
                            ? 0x068a0000u + pal * 512u : obj_pltt;
                        out.color[py][px]=bgr555(rd16(palette_base+index*2u));
                        out.priority[py][px]=static_cast<uint8_t>(priority);
                        ++drawn_pixels;
                    }
                } else {
                    const uint32_t addr = mapping_1d
                        ? obj_vram + tile * boundary + tno * tile_bytes + fy * 4u + fx / 2
                        : obj_vram + (tile >> 5) * 1024u + (tile & 31u) * 32u
                            + trow * 1024u + tcol * tile_bytes + fy * 4u + fx / 2;
                    const uint8_t b = *reinterpret_cast<volatile uint8_t *>(addr);
                    index = (fx & 1) ? (b >> 4) : (b & 0xF);
                    if (!index) continue;
                    if(is_window) out.window[py][px]=true;
                    else if(!out.color[py][px] || priority<=out.priority[py][px]) {
                        out.color[py][px]=bgr555(rd16(obj_pltt+(pal*16u+index)*2u));
                        out.priority[py][px]=static_cast<uint8_t>(priority);
                        ++drawn_pixels;
                    }
                }
            }
        }
    }

#if defined(_WIN32)
    if (eng == ENGINE_B) {
        static uint64_t scans = 0;
        ++scans;
        const char *trace = std::getenv("SM64DS_PC_TRACE_OAM");
        if (trace && trace[0] && trace[0] != '0' && scans % 300 == 0) {
            int first = -1;
            for (int i = 0; i < 128; ++i) {
                const uint16_t a0 = rd16(oam_base + i * 8u);
                if ((a0 & 0x200u) == 0 && ((a0 >> 14) & 3u) != 3u) {
                    first = i;
                    break;
                }
            }
            const uint16_t a0 = first >= 0 ? rd16(oam_base + first * 8u) : 0;
            const uint16_t a1 = first >= 0 ? rd16(oam_base + first * 8u + 2) : 0;
            const uint16_t a2 = first >= 0 ? rd16(oam_base + first * 8u + 4) : 0;
            const uint32_t tile_base = obj_vram + (a2 & 0x3ffu) * boundary;
            const uint32_t selected_palette = obj_pltt + ((a2 >> 12) & 0xfu) * 32u;
            std::fprintf(stderr,
                "[sub OBJ detail] scan=%llu index=%d attr=%04x,%04x,%04x boundary=%u dispcnt=%08x tile=%08x bytes=%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x pal10=%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x\n",
                static_cast<unsigned long long>(scans), first, a0, a1, a2, boundary,
                dispcnt, tile_base,
                *reinterpret_cast<volatile uint8_t *>(tile_base + 0),
                *reinterpret_cast<volatile uint8_t *>(tile_base + 1),
                *reinterpret_cast<volatile uint8_t *>(tile_base + 2),
                *reinterpret_cast<volatile uint8_t *>(tile_base + 3),
                *reinterpret_cast<volatile uint8_t *>(tile_base + 4),
                *reinterpret_cast<volatile uint8_t *>(tile_base + 5),
                *reinterpret_cast<volatile uint8_t *>(tile_base + 6),
                *reinterpret_cast<volatile uint8_t *>(tile_base + 7),
                rd16(selected_palette), rd16(selected_palette + 2),
                rd16(selected_palette + 4), rd16(selected_palette + 6),
                rd16(selected_palette + 8), rd16(selected_palette + 10),
                rd16(selected_palette + 12), rd16(selected_palette + 14),
                rd16(selected_palette + 16), rd16(selected_palette + 18),
                rd16(selected_palette + 20), rd16(selected_palette + 22),
                rd16(selected_palette + 24), rd16(selected_palette + 26),
                rd16(selected_palette + 28), rd16(selected_palette + 30));
            std::fflush(stderr);
        }
        if (trace && trace[0] && trace[0] != '0' && scans % 300 == 0) {
            std::fprintf(stderr,
                "[sub OBJ] scan=%llu signature=%08x onscreen_objects=%u drawn_pixels=%u first_attr=%04x,%04x,%04x\n",
                static_cast<unsigned long long>(scans), oam_signature,
                onscreen_objects, drawn_pixels,
                rd16(oam_base), rd16(oam_base + 2), rd16(oam_base + 4));
            std::fflush(stderr);
        }
    }
#endif
}

} // namespace

void ppu_scanout_obj(Engine eng,Framebuffer &fb) {
    ObjPlane objects;
    raster_objects(eng,objects);
    const EngineMap &em=kEngines[eng];
    const uint32_t control=rd32(em.reg_base);
    const uint8_t backgrounds=last_bg_frame[eng]==&fb ? last_bg_mask[eng] : 0x0f;
    BgConfig bgs[4];
    for(int i=0;i<4;++i) {
        bgs[i]=read_bg(em,i,control);
        bgs[i].enabled=bgs[i].enabled && (backgrounds&(1u<<i)) &&
            !(eng==ENGINE_A && i==0 && (control&8));
    }
    for(int y=0;y<SCREEN_H;++y) {
        const WindowState windows=windows_at(eng,y);
        for(int x=0;x<SCREEN_W;++x) {
            if(!objects.color[y][x]) continue;
            const uint8_t layers=window_mask(windows,x,y,objects.window[y][x]);
            if(!(layers&0x10)) continue;
            bool covered=false;
            for(int bg=0;bg<4;++bg) {
                uint32_t ignored;
                // OBJ wins equal-priority BG ties. Transparent BG texels do not occlude.
                if((layers&(1u<<bg)) && bgs[bg].enabled &&
                   bgs[bg].priority<objects.priority[y][x] &&
                   sample_bg(em,bgs[bg],x,y,ignored)) { covered=true;break; }
            }
            if(!covered) fb.px[y][x]=objects.color[y][x];
        }
    }
}

bool ppu_write_bmp(const char *path, const Framebuffer &fb) {
    std::FILE *f = std::fopen(path, "wb");
    if (!f) return false;

    const uint32_t stride = SCREEN_W * 3;
    const uint32_t pad = (4 - (stride & 3)) & 3;
    const uint32_t img = (stride + pad) * SCREEN_H;
    const uint32_t off = 54;

    uint8_t hdr[54] = {};
    hdr[0] = 'B'; hdr[1] = 'M';
    const uint32_t total = off + img;
    std::memcpy(hdr + 2, &total, 4);
    std::memcpy(hdr + 10, &off, 4);
    const uint32_t dib = 40;
    std::memcpy(hdr + 14, &dib, 4);
    const int32_t w = SCREEN_W, h = SCREEN_H;
    std::memcpy(hdr + 18, &w, 4);
    std::memcpy(hdr + 22, &h, 4);
    const uint16_t planes = 1, bpp = 24;
    std::memcpy(hdr + 26, &planes, 2);
    std::memcpy(hdr + 28, &bpp, 2);
    std::memcpy(hdr + 34, &img, 4);
    std::fwrite(hdr, 1, sizeof hdr, f);

    const uint8_t zero[3] = {0, 0, 0};
    for (int y = SCREEN_H - 1; y >= 0; --y) {          // BMP rows run bottom-up
        for (int x = 0; x < SCREEN_W; ++x) {
            const uint32_t c = fb.px[y][x];
            const uint8_t bgr[3] = {static_cast<uint8_t>(c), static_cast<uint8_t>(c >> 8),
                                    static_cast<uint8_t>(c >> 16)};
            std::fwrite(bgr, 1, 3, f);
        }
        if (pad) std::fwrite(zero, 1, pad, f);
    }
    std::fclose(f);
    return true;
}

}  // namespace ntr
