#include "OverlayID.h"

#include "types.h"
extern void LoadOverlay(int id);
extern int overlay_6;
extern int overlay_4;

void func_0201a798(int id) {
    if (id == -1) return;
    if (id == OVERLAY_ID(6)) LoadOverlay(OVERLAY_ID(4));
    LoadOverlay(id);
}
