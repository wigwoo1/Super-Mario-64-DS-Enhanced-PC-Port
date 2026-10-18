//cpp
/* arm9/CP15 -- the coprocessor-15 cache primitives, folded from seven legacy
 * shards at .text 0x02058bb0..0x02058c84. Every member is a whole-function
 * `asm` block: `mcr p15` coprocessor writes are not reachable from C, so the
 * asm is the faithful source rather than a transcription of something lost.
 * The span is bounded on both sides by func_ free functions: func_02058b9c
 * below and func_02058c84 above, so the cache-primitive run is one TU. The
 * other CP15 members (system control, the 0x020593a8 band) live in their own
 * runs and are deliberately out of scope -- the linker places one object's
 * .text contiguously.
 *
 * Whole-function asm members are emitted in source order (the compiler's
 * reverse-emission applies to compiled bodies, not `asm void` blocks), so
 * the definitions below run ROM-ascending.
 *
 *   FlushDataCache() / FlushAndInvalidateDataCache() -- whole-cache clean and
 *   clean+invalidate by set and way (c7,c10,2 / c7,c14,2).
 *   InvalidateDataCache, FlushDataCache, FlushAndInvalidateDataCache,
 *   InvalidateInstructionCache -- the same ops ranged over an MVA window
 *   (c7,c6,1 / c7,c10,1 / c7,c14,1 / c7,c5,1).
 *   DrainWriteBuffer -- wait for the write buffer to empty (c7,c10,4).
 */

namespace CP15 {

// @symbol _ZN4CP1514FlushDataCacheEv
asm void FlushDataCache(void)
{
    mov r1, #0
outer:
    mov r0, #0
seg:
    orr r2, r1, r0
    mcr p15, 0, r2, c7, c10, 2
    add r0, r0, #0x20
    cmp r0, #0x400
    blt seg
    add r1, r1, #0x40000000
    cmp r1, #0
    bne outer
    bx lr
}

/* Clean and invalidate the whole data cache by set and way (c7,c14,2). Same
 * traversal as FlushDataCache: r1 walks the four ways in the top two bits, r0
 * walks the 32 lines of each set; the outer loop ends when r1 wraps to zero. */
// @symbol _ZN4CP1527FlushAndInvalidateDataCacheEv
asm void FlushAndInvalidateDataCache(void)
{
    mov r1, #0
outer:
    mov r0, #0
seg:
    orr r2, r1, r0
    mcr p15, 0, r2, c7, c14, 2
    add r0, r0, #0x20
    cmp r0, #0x400
    blt seg
    add r1, r1, #0x40000000
    cmp r1, #0
    bne outer
    bx lr
}

// @symbol _ZN4CP1519InvalidateDataCacheEjj
asm void InvalidateDataCache(unsigned int addr, unsigned int size)
{
    add r1, r1, r0
    bic r0, r0, #0x1f
loop:
    mcr p15, 0, r0, c7, c6, 1
    add r0, r0, #0x20
    cmp r0, r1
    blt loop
    bx lr
}

// @symbol _ZN4CP1514FlushDataCacheEjj
asm void FlushDataCache(unsigned int addr, unsigned int size)
{
    add r1, r1, r0
    bic r0, r0, #0x1f
loop:
    mcr p15, 0, r0, c7, c10, 1
    add r0, r0, #0x20
    cmp r0, r1
    blt loop
    bx lr
}

// @symbol _ZN4CP1527FlushAndInvalidateDataCacheEjj
asm void FlushAndInvalidateDataCache(unsigned int addr, unsigned int size)
{
    add r1, r1, r0
    bic r0, r0, #0x1f
loop:
    mcr p15, 0, r0, c7, c14, 1
    add r0, r0, #0x20
    cmp r0, r1
    blt loop
    bx lr
}

// @symbol _ZN4CP1516DrainWriteBufferEv
asm void DrainWriteBuffer(void)
{
    mov r0, #0
    mcr p15, 0, r0, c7, c10, 4
    bx lr
}

// @symbol _ZN4CP1526InvalidateInstructionCacheEjj
asm void InvalidateInstructionCache(unsigned int addr, unsigned int size)
{
    add r1, r1, r0
    bic r0, r0, #0x1f
loop:
    mcr p15, 0, r0, c7, c5, 1
    add r0, r0, #0x20
    cmp r0, r1
    blt loop
    bx lr
}

} // namespace CP15
