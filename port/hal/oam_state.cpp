// Host storage for OAM globals and the small CPU-copy primitives the OAM
// upload path shares with GX. The ROM labels point into fixed memory; the host
// build gives them storage here and aliases decorated references onto it.
#include <cstring>

extern "C" {
// Original ARM9 BSS starts with both sprite buffers enabled.
// Single-buffer mode is selected explicitly by the scenes that need it.
unsigned char data_0209e660 = 0;
int data_0209e664, data_0209e668, data_0209e66c, data_0209e670;
// oam_buffers.asm owns both buffers and their original interior labels.
int data_02099fd0 = -1;

void MultiCopy_Int(const void *src, void *dst, int len)
{
    // The ARM primitive loads/stores one word at a time, advancing forward.
    // Reset deliberately overlaps source/destination to repeat a blank entry.
    auto *out = static_cast<unsigned int *>(dst);
    auto *in = static_cast<const unsigned int *>(src);
    for (int offset = 0; offset < len; offset += 4) *out++ = *in++;
}

void MultiCopy32Bytes(const int *src, int *dst, int len)
{
    // Match the original eight-register load/store groups, then word tail.
    // Neither memcpy nor memmove implements its overlapping-copy contract.
    int offset = 0;
    for (; offset + 32 <= len; offset += 32) {
        int words[8];
        for (int i = 0; i < 8; ++i) words[i] = src[i];
        for (int i = 0; i < 8; ++i) dst[i] = words[i];
        src += 8; dst += 8;
    }
    for (; offset < len; offset += 4) *dst++ = *src++;
}

void DMAStartTransfer(int channel, int src, int dst, int ctrl);
void DMAStartTransferFB(unsigned char channel, unsigned int src,
                        unsigned int dst, unsigned int ctrl)
{ DMAStartTransfer(channel, static_cast<int>(src), static_cast<int>(dst), static_cast<int>(ctrl)); }

}

#pragma comment(linker, "/alternatename:?data_0209e660@@3EA=_data_0209e660")
#pragma comment(linker, "/alternatename:?data_0209e664@@3HA=_data_0209e664")
#pragma comment(linker, "/alternatename:?data_0209e668@@3HA=_data_0209e668")
#pragma comment(linker, "/alternatename:?data_0209e66c@@3HA=_data_0209e66c")
#pragma comment(linker, "/alternatename:?data_0209e670@@3HA=_data_0209e670")
#pragma comment(linker, "/alternatename:?data_0209e674@@3PAUOamAttr@@A=_data_0209e674")
#pragma comment(linker, "/alternatename:?data_0209ea74@@3PAUOamAttr@@A=_data_0209ea74")
#pragma comment(linker, "/alternatename:?data_0209e67c@@3PAHA=_data_0209e67c")
#pragma comment(linker, "/alternatename:?data_0209e694@@3PAHA=_data_0209e694")
#pragma comment(linker, "/alternatename:?data_02099fd0@@3HA=_data_02099fd0")
#pragma comment(linker, "/alternatename:?data_0209e664@@3HA=_data_0209e664")
#pragma comment(linker, "/alternatename:?data_0209e668@@3HA=_data_0209e668")
#pragma comment(linker, "/alternatename:?data_0209e66c@@3HA=_data_0209e66c")
#pragma comment(linker, "/alternatename:?data_0209e670@@3HA=_data_0209e670")
#pragma comment(linker, "/alternatename:?data_0209e674@@3PAUOamAttr@@A=_data_0209e674")
#pragma comment(linker, "/alternatename:?data_0209ea74@@3PAUOamAttr@@A=_data_0209ea74")
