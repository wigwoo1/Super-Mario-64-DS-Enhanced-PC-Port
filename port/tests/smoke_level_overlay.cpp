#include "hal/level_overlay.h"
#include "ntr/fs.h"
#include <cstdio>
#include <cstring>
#include <vector>

int main(int argc, char **argv) {
    if (argc != 3 || !ntr::fs_mount_rom(argv[1], false)) return 1;
    size_t checked_relocations = 0, checked_bytes = 0;
    for (size_t level = 0; level < PC_LEVEL_COUNT; ++level) {
        const auto &spec = pc_level_overlay_specs[level];
        char path[2048];
        std::snprintf(path, sizeof(path), "%s/extracted/overlays/overlay_%04u.bin", argv[2], spec.id);
        FILE *f = std::fopen(path, "rb");
        if (!f) return 2;
        std::vector<unsigned char> expected(spec.image_size + spec.bss_size, 0);
        const size_t got = std::fread(expected.data(), 1, spec.image_size, f);
        std::fclose(f);
        if (got != spec.image_size) return 3;
        auto *actual = static_cast<unsigned char *>(pc_level_overlay_address(static_cast<int>(level), spec.base));
        if (!actual) { std::fprintf(stderr, "failed loading level %zu\n", level); return 4; }
        for (size_t i = 0; i < spec.relocation_count; ++i) {
            const auto offset = spec.relocations[i];
            uint32_t raw;
            std::memcpy(&raw, expected.data() + offset, sizeof(raw));
            const auto target = reinterpret_cast<uintptr_t>(pc_level_overlay_address(static_cast<int>(level), raw));
            if (!target || target > UINT32_MAX) return 5;
            const auto host = static_cast<uint32_t>(target);
            std::memcpy(expected.data() + offset, &host, sizeof(host));
            ++checked_relocations;
        }
        if (std::memcmp(actual, expected.data(), expected.size()) != 0) {
            std::fprintf(stderr, "ROM bytes/fixups differ at level %zu\n", level); return 6;
        }
        checked_bytes += expected.size();
        if (!pc_level_overlay_address(static_cast<int>(level), spec.record_address) ||
            pc_level_overlay_address(static_cast<int>(level), spec.base - 1) ||
            pc_level_overlay_address(static_cast<int>(level), spec.base + static_cast<uint32_t>(expected.size()))) return 7;
    }
    // Regression: these levels reuse the SAME DS address, with different data.
    void *test_map = pc_level_overlay_address(0, 0x02111210u);
    void *castle = pc_level_overlay_address(1, 0x02111210u);
    if (!test_map || !castle || test_map == castle ||
        std::memcmp(test_map, castle, 16) == 0 ||
        pc_level_overlay_address(-1, 0x02111210u) ||
        pc_level_overlay_address(52, 0x02111210u) || pc_load_level_overlay(81)) return 8;
    std::printf("course overlays: 52 images, %zu bytes and %zu self-references match extracted ROM; overlapping addresses stay distinct\n",
                checked_bytes, checked_relocations);
    return 0;
}
