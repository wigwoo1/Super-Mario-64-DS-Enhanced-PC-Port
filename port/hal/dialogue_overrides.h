#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace pc_dialogue {
uint32_t crc32(const uint8_t *data,size_t size,uint32_t previous=0);
struct Entry { uint32_t id,original_crc;std::vector<uint8_t> text; };
class Overrides {
    std::vector<Entry> entries;
public:
    bool load(const uint8_t *file,size_t size,unsigned language);
    const uint8_t *resolve(unsigned id,const uint8_t *original,size_t size,
                           const uint8_t *info,const uint8_t *widths) const;
    size_t count() const {return entries.size();}
};
}

extern "C" void pc_dialogue_begin();
