//cpp
extern "C" void *_ZN2G212GetBG0ScrPtrEv(void);
extern "C" void func_ov003_020adc74(signed char* s, int r6, int r5) {
    short* p = (short *)_ZN2G212GetBG0ScrPtrEv() + (r5 + (r6 << 5));
    if (*s == 0) return;
    do {
        *p++ = (short)*s++;
    } while (*s != 0);
}
