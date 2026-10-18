#include "OverlayID.h"

#include "types.h"
extern void UnloadOverlay(int id);
extern int overlay_6;
extern int overlay_4;

void func_0201a754(int id) {
    if (id == -1) return;
    UnloadOverlay(id);
    if (id != OVERLAY_ID(6)) return;
    UnloadOverlay(OVERLAY_ID(4));
}
