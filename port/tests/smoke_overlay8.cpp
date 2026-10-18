#include <cstddef>
#include <cstdint>
#include <cstdio>

extern "C" void pc_load_overlay8_image();
extern "C" bool pc_validate_overlay8_image();
extern "C" void *pc_overlay8_address(uint32_t address);
extern "C" size_t pc_overlay8_relocation_count();

int main()
{
    pc_load_overlay8_image();
    if (!pc_validate_overlay8_image()) {
        std::fprintf(stderr, "overlay 8 self-relocations point outside the native image\n");
        return 1;
    }
    if (pc_overlay8_relocation_count() != 25) {
        std::fprintf(stderr, "overlay 8 relocation count changed unexpectedly\n");
        return 1;
    }
    if (!pc_overlay8_address(0x021111c0u) ||
        !pc_overlay8_address(0x02111be0u) ||
        pc_overlay8_address(0x02111c20u)) {
        std::fprintf(stderr, "overlay 8 address translation has invalid bounds\n");
        return 1;
    }
    std::puts("overlay 8 smoke: data image, 25 self-relocations, and address bounds pass");
    return 0;
}
