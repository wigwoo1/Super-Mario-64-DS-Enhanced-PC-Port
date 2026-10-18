// Host counterpart of cstd::atan2(Fix12, Fix12). It follows the game's
// ROM-table quadrant logic while replacing the DS soft-float sign test with
// an integer sign check. The table itself is generated from the supplied ROM.
#include <cstdint>

extern "C" int _ZN4cstd4fdivEii(int numerator, int denominator);
extern "C" unsigned short data_020994e0[];

extern "C" short _ZN4cstd5atan2E5Fix12IiES1_(int y, int x)
{
    int angle;
    if (y == 0) {
        angle = x >= 0 ? 0 : 0x8000;
    } else if (x == 0) {
        angle = y >= 0 ? 0x4000 : 0xc000;
    } else if (y >= 0) {
        if (x >= 0) {
            angle = x >= y ? data_020994e0[_ZN4cstd4fdivEii(y, x) >> 2]
                           : 0x4000 - data_020994e0[_ZN4cstd4fdivEii(x, y) >> 2];
        } else {
            angle = -x < y ? data_020994e0[_ZN4cstd4fdivEii(-x, y) >> 2] + 0x4000
                           : 0x8000 - data_020994e0[_ZN4cstd4fdivEii(y, -x) >> 2];
        }
    } else if (x < 0) {
        angle = x <= y ? data_020994e0[_ZN4cstd4fdivEii(-y, -x) >> 2] + 0x8000
                       : 0xc000 - data_020994e0[_ZN4cstd4fdivEii(-x, -y) >> 2];
    } else {
        angle = x < -y ? data_020994e0[_ZN4cstd4fdivEii(x, -y) >> 2] + 0xc000
                       : -data_020994e0[_ZN4cstd4fdivEii(-y, x) >> 2];
    }
    return static_cast<short>(angle);
}
