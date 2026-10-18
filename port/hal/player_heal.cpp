// Host slice of the byte-recovered Player::Heal method from src/actors/Player.cpp.
// The monolithic Player TU contains block-scope class declarations rejected by
// MSVC; keep this method's production behavior while leaving src/ unchanged.
#include "Player.h"

extern "C" {
int _ZN6Player7IsStateERNS_5StateE(void *self, void *state);
int GiveHealth(int player, int amount);
extern unsigned char data_ov002_0211010c[0x18];
extern unsigned char data_ov002_02110124[0x18];
}

int Player::Heal(int amount)
{
    int result = _ZN6Player7IsStateERNS_5StateE(this, data_ov002_0211010c);
    if (result) return result;
    result = _ZN6Player7IsStateERNS_5StateE(this, data_ov002_02110124);
    if (result) return result;
    return GiveHealth(mPlayerNo, amount);
}
