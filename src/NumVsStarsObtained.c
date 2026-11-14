extern unsigned char data_0209f21c;
#if defined(SM64DS_PLATFORM_PC)
extern signed char data_0209f310[];

/* Native callers read an int. The DS build returns a signed byte using
   the ARM calling convention. Keep the native tally in its player array. */
int NumVsStarsObtained(void)
{
    signed char sum = data_0209f310[0];
    for (int i = 1; i < data_0209f21c; ++i)
        sum = (signed char)(sum + data_0209f310[i]);
    return sum;
}
#else
extern signed char data_0209f310;
extern signed char data_0209f311;

signed char NumVsStarsObtained(void)
{
    int i = 1;
    signed char sum = data_0209f310;
    signed char *p;
    if (i >= (int)data_0209f21c) return sum;
    p = &data_0209f311;
    do {
        sum = (signed char)(sum + *p);
        i++;
        p++;
    } while (i < (int)data_0209f21c);
    return sum;
}
#endif
