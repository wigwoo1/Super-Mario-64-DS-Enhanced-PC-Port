// Native implementation of the recovered ground-pound camera jitter helper.
#include "dCamera_c.h"

extern "C" int Vec3_Dist(const Vector3 *a, const Vector3 *b);
extern "C" int _ZN4cstd4fdivEii(int numerator, int denominator);

extern "C" void func_0200d8c8(dCamera_c *camera, const Vector3 *target, int strength)
{
    const int distance = Vec3_Dist(&camera->pos, target);
    if (strength <= distance)
        return;

    int offset = static_cast<int>(
        (static_cast<long long>(_ZN4cstd4fdivEii(strength, distance)) * 0x9000 + 0x800) >> 12);
    int *current = reinterpret_cast<int *>(reinterpret_cast<char *>(camera) + 0x134);
    if (offset <= *current)
        return;
    if (offset > 0xc000)
        offset = 0xc000;
    *current = offset;
}
