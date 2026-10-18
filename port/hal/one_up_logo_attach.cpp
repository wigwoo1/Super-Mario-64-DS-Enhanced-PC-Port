// Exact helper body used by Player::IncMegaKillCount. With the null owner
// passed by that caller it intentionally performs no attachment.
#include "dActor_c.h"
#include "daObj1UpLogo_c.h"

extern "C" void func_ov002_020f0e54(char *actor, char *owner)
{
    if (!owner)
        return;
    daObj1UpLogo_c *self = (daObj1UpLogo_c *)actor;
    dActor_c *target = (dActor_c *)owner;
    self->mOwnerUniqueID = target->uniqueID;
    self->mFollowOffsetY = self->mPosY - target->mPosY;
}
