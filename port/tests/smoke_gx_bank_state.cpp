#include "ntr/mmio.h"
#include <cstdint>
#include <cstdio>
namespace GX { void SetBankForSubBGExtPltt(unsigned short); }
extern "C" {
extern unsigned short data_020a6088[13], data_020a609e;
unsigned short func_02054118();
void Vram__Map(unsigned short) {}
}
#pragma comment(linker, "/alternatename:?data_020a6088@@3UVramReg@@A=_data_020a6088")

int main()
{
    if (!ntr::io_init()) return 2;
    // These external names are aliases supplied by the linker. Prevent the
    // compiler from assuming that distinct declarations imply distinct objects.
    volatile uintptr_t field = reinterpret_cast<uintptr_t>(&data_020a609e);
    volatile uintptr_t expected = reinterpret_cast<uintptr_t>(data_020a6088 + 11);
    if (field != expected) return 3;
    GX::SetBankForSubBGExtPltt(0x80);
    if (data_020a609e != 0x80 || !(NTR_MMIO(uint32_t,0x04001000) & 0x40000000)) return 4;
    const auto bank = func_02054118();
    if (bank != 0x80 || data_020a609e != 0 || (NTR_MMIO(uint32_t,0x04001000) & 0x40000000)) return 5;
    GX::SetBankForSubBGExtPltt(bank);
    if (data_020a609e != 0x80 || !(NTR_MMIO(uint32_t,0x04001000) & 0x40000000)) return 6;
    std::puts("PASS: aggregate and interior VRAM bank state agree; sub BG palette survives unmap/restore");
}
