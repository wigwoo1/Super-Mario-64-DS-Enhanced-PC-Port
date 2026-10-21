#include <cstring>
#include <limits>

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

// ITCM 01ffa344 converts a raw IEEE single to an integer, without scaling.
// Match the recovered primitive's default truncation and signed saturation;
// avoid undefined host casts for infinities, NaNs and out-of-range inputs.
extern "C" int func_01ffa344(unsigned bits)
{
    if ((bits & 0x7fffffffu) >= 0x4f000000u)
        return (bits & 0x80000000u) ? std::numeric_limits<int>::min()
                                   : std::numeric_limits<int>::max();
    return static_cast<int>(unpack(bits));
}
