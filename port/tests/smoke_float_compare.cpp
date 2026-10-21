#include <cstdio>
extern "C" int func_01ff98f4(unsigned, unsigned);
extern "C" int func_01ff99a4(unsigned, unsigned);
extern "C" int func_01ffa344(unsigned);

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
    struct Conversion { unsigned bits; int expected; };
    const Conversion conversions[] = {
        {0,0},{0x80000000u,0},{0x3f800000,1},{0xbf800000u,-1},
        {0x3fe00000,1},{0xbfe00000u,-1},{1,0},{0x80000001u,0},
        {0x48400000,196608}, // Original airborne slide speed target.
        {0x46400000,12288}, // Kuri King scale is already in fixed-point units.
        {0x4effffff,2147483520},{0x4f000000,2147483647},
        {0xcf000000u,(-2147483647-1)},
        {0x7f800000,2147483647},{0xff800000u,(-2147483647-1)},
        {0x7fc00000,2147483647},{0xffc00000u,(-2147483647-1)}
    };
    for(auto value:conversions)if(func_01ffa344(value.bits)!=value.expected) {
        std::fprintf(stderr,"float conversion %08x: got %d expected %d\n",
            value.bits,func_01ffa344(value.bits),value.expected);return 1;
    }
    std::puts("PASS: recovered float-bit comparisons and unscaled truncating/saturating conversion");
}
