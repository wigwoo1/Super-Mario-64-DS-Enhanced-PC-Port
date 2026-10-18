// Host slice of the recovered daStar_c::AddStarMarker method from
// src/actors/daStar_c.cpp. Keep the complete actor TU out of this smoke slice.
#include "common.h"
#include "daStar_c.h"

extern "C" {
extern int data_0209f40c[12];
extern unsigned char data_0209f208;
void SetStarMarker(int slot, void *actor, int type);
int IsStarCollectedInCurLevel(int starId);
void FUN_0202a130(void);
}

void daStar_c::AddStarMarker()
{
    s8 i;
    if (mMarkerSlot >= 0) return;
    for (i = 0; i < 0xc; ++i) {
        if (data_0209f40c[(int)i] != 0) continue;
        if (mMarkerType == 0) {
            SetStarMarker((int)i, this, mBits.collectedModel ? 3 : 2);
        } else {
            if (mMarkerType == 2) {
                if (mBits.collectedModel) {
                    mMarkerType = 3;
                } else if ((mKind == 5 || mKind == 7) &&
                           IsStarCollectedInCurLevel(mStarID) != 0) {
                    mMarkerType = 3;
                }
            }
            SetStarMarker((int)i, this, mMarkerType);
        }
        mMarkerSlot = i;
        if (mState == STATE_WAIT_MARKER && data_0209f208 == 0) return;
        FUN_0202a130();
        return;
    }
}
