/* func_0203c178 @ 0x203c178 (arm9) -- tail-call veneer to func_020527e8 (0x20527e8).
 * ldr ip, [pc]; bx ip; .word 0x20527e9 (Thumb bit set on the BX target; the
 * real symbol, config/arm9/symbols.txt, is func_020527e8 thumb size 0x16)
 */
#ifdef SM64DS_PLATFORM_PC
/* ARM's tail jump preserves r0-r3. A host C call must forward those
 * arguments explicitly; a void(void) call loses the destination matrix. */
extern void func_020527e8(int *m, int sx, int sy, int sz);

void func_0203c178(int *m, int sx, int sy, int sz) {
    func_020527e8(m, sx, sy, sz);
}
#else
extern void func_020527e8(void);

void func_0203c178(void) {
    func_020527e8();
}
#endif
