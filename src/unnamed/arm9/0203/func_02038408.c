/* func_02038408 @ 0x2038408 (arm9) -- tail-call veneer to func_02036acc (0x2036acc).
 * ldr ip, [pc]; bx ip; .word 0x2036acc
 * The collision object in r0 passes straight through.
 */
extern void func_02036acc(char *self);

void func_02038408(void *self) {
    func_02036acc((char *)self);
}
