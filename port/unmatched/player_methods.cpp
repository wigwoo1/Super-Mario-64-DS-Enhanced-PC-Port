// Small host methods extracted from the recovered Player aggregate TU. Keep
// the original decision logic and provide the Itanium spelling used by the
// neighboring decompiled C call sites.
#include "Player.h"

unsigned int Player::GetBodyModelID(unsigned int character, bool otherPlayer) const
{
    if (!otherPlayer) {
        if (mIsMetal)
            character = 4;
    } else if (unk_6fa) {
        character = 4;
    }
    return character;
}

extern "C" unsigned int _ZNK6Player14GetBodyModelIDEjb(
    const Player *self, unsigned int character, bool otherPlayer)
{
    return self->Player::GetBodyModelID(character, otherPlayer);
}
