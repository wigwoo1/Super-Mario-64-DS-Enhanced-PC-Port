#include <cstdio>
extern "C" int func_01ff98f4(unsigned, unsigned);
extern "C" int func_01ff99a4(unsigned, unsigned);

int main()
{
    // Independent IEEE bit patterns: +0, -0, 1, 2, -1, infinities and NaN.
    struct Case { unsigned a, b; int less, greater; };
    const Case cases[] = {
        {0, 0x80000000u, 0, 0}, {0x3f800000, 0x40000000, 1, 0},
        {0x40000000, 0x3f800000, 0, 1}, {0xbf800000, 0, 1, 0},
        {0xff800000, 0x7f800000, 1, 0}, {0x7f800000, 0x7f800000, 0, 0},
        {0x7fc00000, 0, 0, 0}, {0, 0x7fc00000, 0, 0}
    };
    for (auto value : cases) {
        if (func_01ff98f4(value.a, value.b) != value.less ||
            func_01ff99a4(value.a, value.b) != value.greater) return 1;
    }
    std::puts("PASS: recovered float-bit comparisons, signed zero, infinities and NaN");
}
