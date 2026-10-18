// Native build copy of the recovered score-popup owner attachment helper.
#include "daObjNumber_c.h"

extern "C" void func_ov002_020f0918(daObjNumber_c *popup, dActor_c *owner)
{
    if (!owner)
        return;
    popup->mOwnerUniqueID = owner->uniqueID;
    popup->mFollowOffsetY = popup->mPosY - owner->mPosY;
}
