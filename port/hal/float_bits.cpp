#include <cstring>

static float unpack(unsigned bits)
{
    float value;
    static_assert(sizeof(value) == sizeof(bits), "ARM single-value bit interface");
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

// ITCM 01ff98f4 / 01ff99a4 compare IEEE single values passed as integer bits.
// The recovered entries use signed ordering after sign/magnitude conversion;
// equality (including signed zero) and unordered NaN inputs return false.
extern "C" int func_01ff98f4(unsigned lhs, unsigned rhs) { return unpack(lhs) < unpack(rhs); }
extern "C" int func_01ff99a4(unsigned lhs, unsigned rhs) { return unpack(lhs) > unpack(rhs); }
