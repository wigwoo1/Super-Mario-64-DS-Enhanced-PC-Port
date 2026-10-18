#include "hal/level_overlay.h"
#include "ntr/fs.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
unsigned char *images[PC_LEVEL_COUNT] = {};

bool load(size_t level) {
    if (images[level]) return true;
    const PcLevelOverlaySpec &spec = pc_level_overlay_specs[level];
    ntr::OverlayInfo info{};
    if (ntr::fs_overlay_info(spec.id, &info) != 0 ||
        info.ram_address != spec.base || info.ram_size != spec.image_size ||
        info.bss_size != spec.bss_size) {
        std::fprintf(stderr, "[level overlay] metadata mismatch for level %zu, overlay %u\n", level, spec.id);
        return false;
    }
    const size_t size = static_cast<size_t>(spec.image_size) + spec.bss_size;
    // Keep these cached immutable images on the Windows heap. Game allocators
    // are torn down on scene changes, while course data can still be referenced.
    auto *image = static_cast<unsigned char *>(HeapAlloc(GetProcessHeap(), 0, size));
    if (!image) return false;
    if (ntr::fs_overlay_load(spec.id, image, static_cast<int32_t>(size)) != size) {
        std::fprintf(stderr, "[level overlay] ROM read failed for overlay %u\n", spec.id);
        HeapFree(GetProcessHeap(), 0, image);
        return false;
    }
    for (size_t i = 0; i < spec.relocation_count; ++i) {
        const uint32_t offset = spec.relocations[i];
        uint32_t target;
        std::memcpy(&target, image + offset, sizeof(target));
        if (target < spec.base || static_cast<uint64_t>(target) >= spec.base + size) {
            std::fprintf(stderr, "[level overlay] invalid self-reference in overlay %u at +%x\n", spec.id, offset);
            HeapFree(GetProcessHeap(), 0, image);
            return false;
        }
        const uint32_t host = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(image + (target - spec.base)));
        std::memcpy(image + offset, &host, sizeof(host));
    }
    images[level] = image;
    const char *trace = std::getenv("SM64DS_PC_TRACE");
    if (trace && trace[0] && trace[0] != '0') {
        std::fprintf(stderr, "[level overlay] level=%zu id=%u image=%p bytes=%zu self_references=%zu\n",
                     level, spec.id, image, size, spec.relocation_count);
        std::fflush(stderr);
    }
    return true;
}
} // namespace

extern "C" int pc_load_level_overlay(int overlay_id) {
    for (size_t level = 0; level < PC_LEVEL_COUNT; ++level)
        if (pc_level_overlay_specs[level].id == static_cast<uint32_t>(overlay_id))
            return load(level) ? 1 : 0;
    return 0;
}

extern "C" void *pc_level_overlay_address(int level, uint32_t address) {
    if (level < 0 || static_cast<size_t>(level) >= PC_LEVEL_COUNT) return nullptr;
    const PcLevelOverlaySpec &spec = pc_level_overlay_specs[level];
    const uint64_t end = static_cast<uint64_t>(spec.base) + spec.image_size + spec.bss_size;
    if (address < spec.base || address >= end || !load(static_cast<size_t>(level))) return nullptr;
    return images[level] + (address - spec.base);
}
