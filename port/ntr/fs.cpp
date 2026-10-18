// DS card filesystem: FNT/FAT parsing and a directory backend.
//
// Header layout from GBATEK:
//   0x40 FNT offset, 0x44 FNT size, 0x48 FAT offset, 0x4C FAT size
// FAT is (start, end) u32 pairs, one per file, indexed by file ID.
// FNT is a main table of 8-byte directory records plus name subtables.

#include "ntr/fs.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace ntr {
namespace {

struct Entry {
    uint32_t start = 0, end = 0;
    std::string path;
};

struct Fs {
    bool mounted = false;
    bool from_dir = false;
    std::string root;                 // directory backend
    std::vector<uint8_t> rom;         // ROM backend
    std::vector<Entry> files;
    uint32_t ovt9_offset = 0, ovt9_size = 0;
    std::unordered_map<std::string, int32_t> by_path;
    int32_t overlays = 0;
};

Fs g;
uint32_t g_walk_directory_count = 0;
uint32_t g_walk_entry_count = 0;

bool trace_enabled() {
    const char *value = std::getenv("SM64DS_PC_TRACE");
    return value && value[0] && value[0] != '0';
}

void trace(const char *message) {
    if (trace_enabled()) {
        std::fprintf(stderr, "[sm64ds] fs: %s\n", message);
        std::fflush(stderr);
    }
}

uint16_t rd16(const uint8_t *p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
uint32_t rd32(const uint8_t *p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8)
           | (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

// Walk one FNT directory subtable, naming its files and recursing into subdirs.
void walk_dir(const uint8_t *fnt, uint32_t fnt_size, uint16_t dir_id,
              const std::string &prefix, int depth,
              std::vector<uint16_t> &visited) {
    if (depth > 32) return;                       // malformed FNT guard
    for (uint16_t seen : visited)
        if (seen == dir_id) return;                // avoid malformed directory cycles
    visited.push_back(dir_id);
    ++g_walk_directory_count;
    if (trace_enabled() && g_walk_directory_count <= 12) {
        std::fprintf(stderr, "[sm64ds] fs: dir %u id=%04x depth=%d prefix=%zu\n",
                     g_walk_directory_count, dir_id, depth, prefix.size());
        std::fflush(stderr);
    }
    const uint32_t rec = (dir_id & 0x0FFFu) * 8u;
    if (rec + 8 > fnt_size) return;

    uint32_t off = rd32(fnt + rec);
    uint16_t file_id = rd16(fnt + rec + 4);

    while (off < fnt_size) {
        const uint8_t type = fnt[off++];
        ++g_walk_entry_count;
        if (trace_enabled() && (g_walk_entry_count % 2048) == 0) {
            std::fprintf(stderr, "[sm64ds] fs: parsed %u FNT entries across %u dirs\n",
                         g_walk_entry_count, g_walk_directory_count);
            std::fflush(stderr);
        }
        if (type == 0) break;                     // end of this subtable
        const uint32_t len = type & 0x7Fu;
        if (off + len > fnt_size) break;
        std::string name(reinterpret_cast<const char *>(fnt + off), len);
        off += len;

        if (type & 0x80u) {                       // subdirectory
            if (off + 2 > fnt_size) break;
            const uint16_t sub = rd16(fnt + off);
            off += 2;
            walk_dir(fnt, fnt_size, sub, prefix + name + "/", depth + 1, visited);
        } else {                                  // file
            if (file_id < g.files.size()) {
                g.files[file_id].path = prefix + name;
                g.by_path[g.files[file_id].path] = file_id;
            }
            ++file_id;
        }
    }
}

}  // namespace

bool fs_mount_rom(const char *rom_path, bool build_path_index) {
    trace("mount entry");
    // The process starts with a default-constructed empty Fs. Avoid assigning
    // a fresh aggregate to its STL containers on first mount: the native game
    // also supplies custom global allocation hooks, and that teardown path can
    // enter them before the game heap has been initialized.
    if (g.mounted || g.from_dir || !g.rom.empty() || !g.files.empty() ||
        !g.by_path.empty())
        fs_unmount();
    trace("previous mount cleared");

    trace("opening ROM file");
    std::FILE *f = std::fopen(rom_path, "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    const long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (size < 0x200) { std::fclose(f); return false; }

    if (trace_enabled()) {
        std::fprintf(stderr, "[sm64ds] fs: reading %ld ROM bytes\n", size);
        std::fflush(stderr);
    }
    g.rom.resize(static_cast<size_t>(size));
    const size_t got = std::fread(g.rom.data(), 1, g.rom.size(), f);
    std::fclose(f);
    if (got != g.rom.size()) { g.rom.clear(); return false; }
    trace("ROM bytes read");

    const uint32_t fnt_off = rd32(&g.rom[0x40]), fnt_size = rd32(&g.rom[0x44]);
    const uint32_t fat_off = rd32(&g.rom[0x48]), fat_size = rd32(&g.rom[0x4C]);
    g.ovt9_offset = rd32(&g.rom[0x50]);
    g.ovt9_size = rd32(&g.rom[0x54]);
    if (trace_enabled()) {
        std::fprintf(stderr, "[sm64ds] fs: header FNT=%08x+%08x FAT=%08x+%08x OVT9=%08x+%08x\n",
                     fnt_off, fnt_size, fat_off, fat_size,
                     g.ovt9_offset, g.ovt9_size);
        std::fflush(stderr);
    }
    if (fat_off + fat_size > g.rom.size() || fnt_off + fnt_size > g.rom.size()) {
        g.rom.clear();
        return false;
    }
    if ((g.ovt9_size % 32) != 0 ||
        (g.ovt9_size && (g.ovt9_offset + g.ovt9_size > g.rom.size()))) {
        g.rom.clear();
        return false;
    }

    g.files.resize(fat_size / 8);
    for (size_t i = 0; i < g.files.size(); ++i) {
        g.files[i].start = rd32(&g.rom[fat_off + i * 8]);
        g.files[i].end = rd32(&g.rom[fat_off + i * 8 + 4]);
    }

    // The root record's "first file ID" is where the named filesystem begins;
    // everything below it is an overlay.
    g.overlays = rd16(&g.rom[fnt_off + 4]);
    if (build_path_index) {
        trace("walking ROM file-name table");
        g_walk_directory_count = 0;
        g_walk_entry_count = 0;
        std::vector<uint16_t> visited_directories;
        walk_dir(&g.rom[fnt_off], fnt_size, 0xF000, "", 0, visited_directories);
        trace("file-name table parsed");
    } else {
        trace("skipping optional ROM path index");
    }

    g.mounted = true;
    g.from_dir = false;
    return true;
}

bool fs_mount_dir(const char *dir_path) {
    fs_unmount();
    // The manifest extract_rom.py writes alongside the files: one
    // "<id> <size> <path>" line per entry, so IDs survive extraction.
    std::string manifest = std::string(dir_path) + "/files.txt";
    std::FILE *f = std::fopen(manifest.c_str(), "rb");
    if (!f) return false;

    char line[1024];
    while (std::fgets(line, sizeof line, f)) {
        int32_t id = 0, size = 0;
        char path[768] = {};
        if (std::sscanf(line, "%d %d %767[^\r\n]", &id, &size, path) < 2) continue;
        if (id < 0) continue;
        if (static_cast<size_t>(id) >= g.files.size()) g.files.resize(id + 1);
        g.files[id].start = 0;
        g.files[id].end = static_cast<uint32_t>(size);
        g.files[id].path = path;
        if (path[0]) g.by_path[path] = id;
    }
    std::fclose(f);

    g.root = dir_path;
    g.mounted = !g.files.empty();
    g.from_dir = true;
    // Overlays are the leading run of unnamed entries.
    g.overlays = 0;
    for (size_t i = 0; i < g.files.size() && g.files[i].path.empty(); ++i)
        g.overlays = static_cast<int32_t>(i + 1);
    return g.mounted;
}

void fs_unmount() {
    g = Fs{};
}

bool fs_mounted() { return g.mounted; }
int32_t fs_file_count() { return static_cast<int32_t>(g.files.size()); }
int32_t fs_overlay_count() { return g.overlays; }

int32_t fs_size(int32_t id) {
    if (id < 0 || static_cast<size_t>(id) >= g.files.size()) return -1;
    const Entry &e = g.files[id];
    return static_cast<int32_t>(e.end - e.start);
}

const char *fs_path(int32_t id) {
    if (id < 0 || static_cast<size_t>(id) >= g.files.size()) return "";
    return g.files[id].path.c_str();
}

int32_t fs_id_by_path(const char *path) {
    if (!path) return -1;
    /* unordered_map::find(const char*) materializes a temporary std::string
       with the CRT allocator; global delete is routed through the game heap.
       Compare keys in place so this ABI seam does not cross allocators. */
    for (const auto &entry : g.by_path)
        if (entry.first.compare(path) == 0)
            return entry.second;
    return -1;
}

int32_t fs_read(int32_t id, void *dst, int32_t capacity) {
    const int32_t size = fs_size(id);
    if (size < 0 || !dst) return -1;
    const int32_t n = size < capacity ? size : capacity;

    if (g.from_dir) {
        const std::string &p = g.files[id].path;
        if (p.empty()) return -1;
        const std::string full = g.root + "/" + p;
        std::FILE *f = std::fopen(full.c_str(), "rb");
        if (!f) return -1;
        const size_t got = std::fread(dst, 1, static_cast<size_t>(n), f);
        std::fclose(f);
        return static_cast<int32_t>(got);
    }

    const Entry &e = g.files[id];
    if (e.start + static_cast<uint32_t>(n) > g.rom.size()) return -1;
    std::memcpy(dst, &g.rom[e.start], static_cast<size_t>(n));
    return n;
}

int32_t fs_overlay_info(uint32_t overlay_id, OverlayInfo *out) {
    if (!out || !g.mounted || g.from_dir || overlay_id == 0) return -1;
    const uint64_t offset = static_cast<uint64_t>(g.ovt9_offset) +
                            static_cast<uint64_t>(overlay_id) * 32;
    if (offset + 32 > static_cast<uint64_t>(g.ovt9_offset) + g.ovt9_size ||
        offset + 32 > g.rom.size()) return -1;
    const uint8_t *p = &g.rom[static_cast<size_t>(offset)];
    out->overlay_id = rd32(p + 0);
    out->ram_address = rd32(p + 4);
    out->ram_size = rd32(p + 8);
    out->bss_size = rd32(p + 12);
    out->static_init_start = rd32(p + 16);
    out->static_init_end = rd32(p + 20);
    out->file_id = rd32(p + 24);
    out->compressed_size = rd32(p + 28);
    if (out->overlay_id != overlay_id) return -1;
    if (out->file_id >= g.files.size()) return -1;
    return 0;
}

int32_t fs_overlay_read(uint32_t overlay_id, void *dst, int32_t capacity) {
    OverlayInfo info{};
    if (fs_overlay_info(overlay_id, &info) != 0 || !dst || capacity < 0)
        return -1;
    const int32_t size = fs_size(static_cast<int32_t>(info.file_id));
    if (size < 0 || size > capacity) return -1;
    return fs_read(static_cast<int32_t>(info.file_id), dst, capacity);
}

namespace {

bool blz_decompress(const uint8_t *input, size_t input_size,
                    std::vector<uint8_t> *output) {
    if (!input || !output || input_size < 8) return false;

    // BLZ permits up to 28 bytes of unrelated data after its 8-byte footer.
    size_t appended = 0;
    uint32_t packed = 0, extra_size = 0;
    size_t header_len = 0, compressed_len = 0;
    bool found = false;
    for (; appended < 0x20 && appended + 8 <= input_size; appended += 4) {
        const size_t footer = input_size - appended - 8;
        packed = rd32(input + footer);
        extra_size = rd32(input + footer + 4);
        header_len = packed >> 24;
        compressed_len = packed & 0x00ffffffu;
        if (header_len < 8 || compressed_len > footer + 8 || header_len > footer + 8)
            continue;
        bool padding_ok = true;
        for (size_t i = footer + 8 - header_len; i < footer; ++i)
            padding_ok = padding_ok && input[i] == 0xff;
        if (padding_ok) { found = true; break; }
    }
    if (!found || extra_size == 0) return false;

    const size_t base_size = input_size - appended;
    if (compressed_len > base_size || compressed_len < header_len) return false;
    const size_t passthrough = base_size - compressed_len;
    const size_t stream_size = compressed_len - header_len;
    if (passthrough > base_size || extra_size > 64u * 1024u * 1024u) return false;

    std::vector<uint8_t> decoded(base_size - passthrough + extra_size);
    size_t produced = 0, consumed = 0;
    unsigned flags = 0, mask = 1;
    const uint8_t *stream = input + passthrough;
    while (produced < decoded.size()) {
        if (mask == 1) {
            if (consumed >= stream_size) return false;
            flags = stream[stream_size - 1 - consumed++];
            mask = 0x80;
        } else {
            mask >>= 1;
        }
        if (flags & mask) {
            if (consumed + 2 > stream_size) return false;
            const uint8_t first = stream[stream_size - 1 - consumed++];
            const uint8_t second = stream[stream_size - 1 - consumed++];
            const size_t length = (first >> 4) + 3;
            size_t displacement = (((first & 0x0f) << 8) | second) + 3;
            if (displacement > produced) {
                if (produced < 2) return false;
                displacement = 2;
            }
            if (length > decoded.size() - produced) return false;
            size_t source = produced - displacement;
            for (size_t i = 0; i < length; ++i)
                decoded[decoded.size() - 1 - produced++] =
                    decoded[decoded.size() - 1 - source++];
        } else {
            if (consumed >= stream_size) return false;
            decoded[decoded.size() - 1 - produced++] =
                stream[stream_size - 1 - consumed++];
        }
    }

    output->assign(input, input + passthrough);
    output->insert(output->end(), decoded.begin(), decoded.end());
    if (appended)
        output->insert(output->end(), input + base_size, input + input_size);
    return true;
}

}  // namespace

int32_t fs_overlay_load(uint32_t overlay_id, void *dst, int32_t capacity) {
    OverlayInfo info{};
    if (!dst || fs_overlay_info(overlay_id, &info) != 0) return -1;
    const uint64_t full_size = static_cast<uint64_t>(info.ram_size) + info.bss_size;
    if (full_size > static_cast<uint64_t>(capacity) || full_size > 64u * 1024u * 1024u)
        return -1;
    const int32_t stored_size = fs_size(static_cast<int32_t>(info.file_id));
    if (stored_size < 0) return -1;
    std::vector<uint8_t> stored(static_cast<size_t>(stored_size));
    if (fs_read(static_cast<int32_t>(info.file_id), stored.data(), stored_size) != stored_size)
        return -1;

    std::vector<uint8_t> image;
    const uint32_t flags = info.compressed_size >> 24;
    if (flags & 1u) {
        if (!blz_decompress(stored.data(), stored.size(), &image)) return -1;
    } else {
        image = std::move(stored);
    }
    if (image.size() != info.ram_size) return -1;
    std::memcpy(dst, image.data(), image.size());
    std::memset(static_cast<uint8_t *>(dst) + image.size(), 0, info.bss_size);
    return static_cast<int32_t>(full_size);
}

}  // namespace ntr
