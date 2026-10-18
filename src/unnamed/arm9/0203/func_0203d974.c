extern unsigned char data_020a0f10;

int func_0203d974(void){
#if defined(SM64DS_PLATFORM_PC)
    /* The host has no DS boot-info halfword at 0x027FFC40. Match the normal
       retail branch used by func_0201a054, then retain the runtime mode flag. */
    return *(unsigned char*)&data_020a0f10 != 0;
#else
    int b = (int)(*(unsigned short*)0x27ffc40 == 2);
    if (b != 0) return 1;
    return *(unsigned char*)&data_020a0f10 != 0;
#endif
}
