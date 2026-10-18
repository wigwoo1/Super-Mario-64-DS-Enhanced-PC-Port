// Portable host implementations for the small ARM assembly primitives that
// the standalone game reaches during save, copy, and integer math paths.
#include <climits>
#include <cstring>

static unsigned int g_pc_ipc_event_bits;
extern "C" void pc_ipc_set_event_bits(unsigned int mask) { g_pc_ipc_event_bits |= mask; }
extern "C" int pc_ipc_event_bit(unsigned int bit) { return (g_pc_ipc_event_bits >> bit) & 1u; }

extern "C" {

void CpuCopy8(const void *src, void *dst, unsigned int count)
{
    std::memmove(dst, src, count);
}

// Replacement for the ARM stmia helper used by G3X::SetFogTable. The PC
// runtime maps the DS I/O window, so the real 32-byte register upload remains
// observable by the native GX adapter.
void Copy32Bytes(const void *src, void *dst)
{
    std::memcpy(dst, src, 32);
}

void MultiStore32Bytes(unsigned int value, int *dst, int length)
{
    if (!dst || length <= 0) return;
    unsigned char *out = reinterpret_cast<unsigned char *>(dst);
    while (length >= 4) {
        std::memcpy(out, &value, sizeof(value));
        out += 4;
        length -= 4;
    }
    for (int i = 0; i < length; ++i)
        out[i] = static_cast<unsigned char>(value >> (i * 8));
}

void func_0205a588(void *dst, int value, int length)
{
    if (dst && length > 0)
        std::memset(dst, value, static_cast<unsigned int>(length));
}

int __aeabi_idiv(int numerator, int denominator)
{
    if (denominator == 0) return numerator < 0 ? INT_MIN : INT_MAX;
    if (numerator == INT_MIN && denominator == -1) return INT_MIN;
    return numerator / denominator;
}

unsigned int __aeabi_uidiv(unsigned int numerator, unsigned int denominator)
{
    if (denominator == 0) return UINT_MAX;
    return numerator / denominator;
}

}
