#ifndef PC_LEVEL_OVERLAY_H
#define PC_LEVEL_OVERLAY_H

#include <cstddef>
#include <cstdint>

// The DS reuses one address range for all course overlays. Each host image
// needs its own storage; an address alone cannot identify its owning course.
struct PcLevelOverlaySpec {
    uint32_t id, base, image_size, bss_size, record_address;
    const uint32_t *relocations;
    size_t relocation_count;
};

constexpr size_t PC_LEVEL_COUNT = 52;
extern const PcLevelOverlaySpec pc_level_overlay_specs[PC_LEVEL_COUNT];

// These load course DATA only. Recovered actor code still runs as compiled
// native C++; the ARM instructions in a ROM image are never executed here.
extern "C" int pc_load_level_overlay(int overlay_id);
extern "C" void *pc_level_overlay_address(int level, uint32_t address);

#endif
