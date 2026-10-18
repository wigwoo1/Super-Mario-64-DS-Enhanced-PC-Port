// Exact host extraction of the recovered Player::IncMegaKillCount routine.
// The ROM version lives inside actors/Player.cpp's large overlay aggregate.
#include "Player.h"
#include "Sound.h"

extern "C" {
void GiveLives(int delta);
void func_ov002_020f0e54(char *actor, char *owner);
}

void Player::IncMegaKillCount()
{
    if (*(unsigned char *)&mIsMega == 0)
        return;
    ++mMegaKillCount;

    if (*(unsigned short *)((char *)this + 0x6d0) >= 8) {
        *(unsigned short *)((char *)this + 0x6d0) = 8;
        Sound::PlayBank3(0x6e, *(Vector3 *)((char *)this + 0x74));
        GiveLives(1);
    }

    Vector3 pos;
    pos.x = mPosX;
    pos.y = mPosY + 0x190000;
    pos.z = mPosZ;
    dActor_c *logo = dActor_c::Spawn(
        0x14b, mMegaKillCount, pos, nullptr, mAreaId, -1);
    if (!logo)
        return;
    *(unsigned short *)((char *)logo + 0x100 + 0x4c) = 0;
    func_ov002_020f0e54((char *)logo, nullptr);
}
