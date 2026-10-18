// Runtime BSS owned by the ROM. Sized through the next named ARM9 symbol;
// used by the actor smoke's recovered overlay functions.
extern "C" {
unsigned char data_0209ee90[0x348] = {};
#if !defined(SM64DS_GAME_TARGET)
unsigned char data_0209caa0[0x400] = {};
#endif
// Four 0x24-byte player input records are live BSS. The PC input shim fills
// the key halfword before the normal game's input-history conversion reads it.
unsigned char data_020a1162[4 * 0x24] = {};
}
