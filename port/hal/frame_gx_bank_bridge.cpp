// smoke_frames retains only the display-frame slice, not the full GX bank
// controller. In the host model the texture and palette windows are fixed, so
// these setup calls intentionally have no additional mapping side effect.
namespace GX {
void SetBankForTex(unsigned short) {}
void SetBankForTexPltt(unsigned short) {}
}
