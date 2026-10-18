typedef int (*Fn)(void *);
extern Fn data_020a4b58;

int func_02043060(void *a)
{
#if defined(SM64DS_PLATFORM_PC)
    /* The retail validator checks the scene overlay and returns 2 when the
       scene can spawn. Native scene factories are statically linked; accept
       the recovered PC profiles without dereferencing cartridge addresses. */
    if ((int)a == 0 || (int)a == 2 || (int)a == 3 || (int)a == 4)
        return 2;
#endif
    if (data_020a4b58 == 0)
        return 2;
    return data_020a4b58(a);
}
