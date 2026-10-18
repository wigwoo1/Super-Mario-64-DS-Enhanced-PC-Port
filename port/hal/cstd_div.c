/* Preserve cstd::fdiv's value AND divider-register side effects. Camera
 * projection reuses DIVCNT and reads later quotients through fdiv_result /
 * ldiv_result; pure host arithmetic leaves those reads in the wrong mode. */
typedef int s32;
extern s32 ntr_fixed_divide(s32 numerator, s32 denominator);

s32 _ZN4cstd4fdivEii(s32 a, s32 b)
{
    return ntr_fixed_divide(a, b);
}
