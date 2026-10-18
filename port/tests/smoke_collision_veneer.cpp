#include <cstdio>

extern "C" void func_020383e4(void *self);
static void *received;
static unsigned calls;

// Stand in for the collision worker while executing the real compiled veneer.
extern "C" void _ZN10dBgCh_Actr20UpdateExtraContinousEv(void *self)
{
    received = self;
    ++calls;
}

int main()
{
    unsigned collisionObject[4] = {0x12345678, 0, 0, 0};
    func_020383e4(collisionObject);
    if (received != collisionObject || calls != 1) {
        std::fprintf(stderr, "FAIL: collision veneer changed the object pointer\n");
        return 1;
    }
    std::puts("PASS: collision veneer preserves its object argument");
}
