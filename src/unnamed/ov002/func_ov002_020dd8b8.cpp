//cpp
#if defined(SM64DS_PLATFORM_PC)
#include "Player.h"

extern "C" Player::State data_ov002_021105a4;

extern "C" int func_ov002_020dd8b8(char* c)
{
    const auto *player = reinterpret_cast<const Player *>(c);
    return player->mState == &data_ov002_021105a4 && player->mStateStep == 0;
}
#else
extern "C" {
struct State;
extern State data_ov002_021105a4;
extern int _ZN6Player7IsStateERNS_5StateE(void* c, State* st);
int func_ov002_020dd8b8(char* c){
  if(!_ZN6Player7IsStateERNS_5StateE(c, &data_ov002_021105a4)) return 0;
  return *(unsigned char*)(c+0x6e3)==0 ? 1 : 0;
}
}
#endif
