struct Bits96 {
    unsigned char pad[0x388];
    unsigned bits[1];
};

int func_0205ba3c(int bit, int word) {
#if defined(SM64DS_PLATFORM_PC)
    extern int pc_ipc_event_bit(unsigned int bit);
    (void)word;
    return pc_ipc_event_bit((unsigned int)bit);
#else
    struct Bits96 *p = (struct Bits96 *)0x27ffc00;
    return (((1 << bit) & p->bits[word]) != 0);
#endif
}
