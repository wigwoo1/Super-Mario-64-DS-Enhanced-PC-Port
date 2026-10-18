#include "ntr/fs.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

static std::vector<unsigned char> read_file(const std::string &path) {
    std::FILE *file = std::fopen(path.c_str(), "rb");
    if (!file) return {};
    std::fseek(file, 0, SEEK_END);
    const long size = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);
    if (size < 0) { std::fclose(file); return {}; }
    std::vector<unsigned char> bytes(static_cast<size_t>(size));
    const bool ok = std::fread(bytes.data(), 1, bytes.size(), file) == bytes.size();
    std::fclose(file);
    return ok ? bytes : std::vector<unsigned char>{};
}

int main(int argc, char **argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: smoke_overlay <rom.nds> <extracted/overlays dir>\n");
        return 2;
    }
    if (!ntr::fs_mount_rom(argv[1])) {
        std::fprintf(stderr, "could not mount ROM: %s\n", argv[1]);
        return 2;
    }

    int checked = 0;
    for (uint32_t id = 1; id < static_cast<uint32_t>(ntr::fs_overlay_count()); ++id) {
        ntr::OverlayInfo info{};
        if (ntr::fs_overlay_info(id, &info) != 0) continue;
        const std::string path = std::string(argv[2]) + "/overlay_" +
            (id < 10 ? "000" : id < 100 ? "00" : "0") + std::to_string(id) + ".bin";
        const auto expected = read_file(path);
        if (expected.empty()) continue;
        const size_t full_size = static_cast<size_t>(info.ram_size) + info.bss_size;
        std::vector<unsigned char> actual(full_size, 0xa5);
        const int32_t loaded = ntr::fs_overlay_load(id, actual.data(),
                                                     static_cast<int32_t>(actual.size()));
        if (loaded != static_cast<int32_t>(full_size) || expected.size() != info.ram_size ||
            std::memcmp(actual.data(), expected.data(), expected.size()) != 0) {
            std::fprintf(stderr,
                "overlay %u mismatch: loaded=%d expected RAM=%u+BSS=%u, extracted=%zu, flags=%02x\n",
                id, loaded, info.ram_size, info.bss_size, expected.size(),
                info.compressed_size >> 24);
            return 1;
        }
        for (size_t i = info.ram_size; i < full_size; ++i) {
            if (actual[i] != 0) {
                std::fprintf(stderr, "overlay %u BSS was not zero-filled\n", id);
                return 1;
            }
        }
        ++checked;
    }
    if (!checked) {
        std::fprintf(stderr, "no extracted overlays were checked\n");
        return 1;
    }
    std::printf("smoke_overlay: %d ROM overlays decompressed and matched extracted images\n",
                checked);
    return 0;
}
