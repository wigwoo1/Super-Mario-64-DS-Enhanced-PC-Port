// Runtime BSS owned by the ROM. Sized through the next named ARM9 symbol;
// used by the actor smoke's recovered overlay functions.
extern "C" {
unsigned char data_0209ee90[0x348] = {};
#if !defined(SM64DS_GAME_TARGET)
unsigned char data_0209caa0[0x400] = {};
#endif
// Raw input/history, including key and camera-angle interior labels, is owned
// by input_state.asm so writes and reads share the original record strides.
}
