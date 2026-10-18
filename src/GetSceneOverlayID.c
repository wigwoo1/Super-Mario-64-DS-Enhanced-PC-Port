#include "OverlayID.h"

extern int IsMinigameActorID(unsigned int id);

extern int overlay_2;
extern int overlay_3;
extern int overlay_5;
extern int overlay_6;
extern int overlay_7;

int GetSceneOverlayID(int id)
{
    if (IsMinigameActorID(id))
        return OVERLAY_ID(6);

    switch (id) {
    case 3:
    case 6:
    case 7:
        return OVERLAY_ID(2);
    case 5:
        return OVERLAY_ID(5);
    case 1:
        return OVERLAY_ID(7);
    case 2:
    case 4:
    case 8:
        return OVERLAY_ID(3);
    case 0x168:
        break;
    }
    return -1;
}
