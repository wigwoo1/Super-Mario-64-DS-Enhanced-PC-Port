// Host slice of recovered func_ov002_020b18f0 from src/actors/daCoin_c.cpp.
// The original symbol is nested in the large daCoin_c aggregate translation unit.
#include "common.h"
#include "Player.h"
#include "daStar_c.h"

namespace Event {
int GetBit(u32 bit);
void SetBit(u32 bit);
}

extern "C" {
extern signed char data_0209f2f8;
extern short data_0209f358[4];
int SublevelToLevel(int level);
}

extern "C" void func_ov002_020b18f0(char *self)
{
    if (Event::GetBit(0x1f)) return;
    if (SublevelToLevel(data_0209f2f8) >= 0xf) return;
    if (self == nullptr) return;
    Player *player = reinterpret_cast<Player *>(self);
    if (data_0209f358[player->mPlayerNo] < 0x64) return;

    Vector3 spawnPos;
    spawnPos.x = player->mPosX;
    spawnPos.y = player->mPosY + 0x12c000;
    spawnPos.z = player->mPosZ;
    daStar_c *star = static_cast<daStar_c *>(dActor_c::Spawn(
        0xb2, 0x20, spawnPos, 0, player->mAreaId, -1));
    if (star == nullptr) return;
    Event::SetBit(0x1f);
    star->AddStarMarker();
}
