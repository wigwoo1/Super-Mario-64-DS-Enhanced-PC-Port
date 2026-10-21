#include <cstddef>
#include <cstdint>
#include <cstdio>

extern "C" void pc_load_overlay9_image();
extern "C" bool pc_validate_overlay9_image();
extern "C" void *pc_overlay9_address(uint32_t address);
extern "C" size_t pc_overlay9_relocation_count();

int main()
{
    pc_load_overlay9_image();
    if (!pc_validate_overlay9_image()) {
        std::fprintf(stderr, "overlay 9 self-relocations point outside the native image\n");
        return 1;
    }
    if (pc_overlay9_relocation_count() == 0) {
        std::fprintf(stderr, "overlay 9 relocation table is unexpectedly empty\n");
        return 1;
    }
    auto *clps = static_cast<const unsigned char *>(pc_overlay9_address(0x02112bf8u));
    if (!clps || pc_overlay9_address(0x02113e90u) == nullptr ||
        pc_overlay9_address(0x02113ee0u) != nullptr) {
        std::fprintf(stderr, "metal-net collision/model address translation has invalid bounds\n");
        return 1;
    }
    // CLPS_Block starts with its format signature/version and collision data;
    // its own fields are not pointers. Check the exact net material record.
    if (clps[0] != 'C' || clps[1] != 'L' || clps[2] != 'P' || clps[3] != 'S' ||
        clps[4] != 0x08 || clps[5] != 0x00 || clps[6] != 0x01 || clps[7] != 0x00 ||
        clps[8] != 0xc0 || clps[9] != 0x0f || clps[12] != 0xff) {
        std::fprintf(stderr, "metal-net CLPS record does not match the extracted overlay\n");
        return 1;
    }
    std::printf("overlay 9 smoke: %zu self-relocations and metal-net CLPS/model bounds pass\n",
                pc_overlay9_relocation_count());
    return 0;
}
