/* func_020383e4 @ 0x20383e4 (arm9) -- tail-call veneer to _ZN10dBgCh_Actr20UpdateExtraContinousEv (0x20358ac).
 * ldr ip, [pc]; bx ip; .word 0x20358ac
 */
#if defined(SM64DS_PLATFORM_PC)
/* ARM's tail-call veneer preserves r0 without a declared argument. Native
 * callers pass the collision object on the stack, so forward it explicitly. */
extern void _ZN10dBgCh_Actr20UpdateExtraContinousEv(void *self);

void func_020383e4(void *self) {
    _ZN10dBgCh_Actr20UpdateExtraContinousEv(self);
}
#else
extern void _ZN10dBgCh_Actr20UpdateExtraContinousEv(void);

void func_020383e4(void) {
    _ZN10dBgCh_Actr20UpdateExtraContinousEv();
}
#endif
