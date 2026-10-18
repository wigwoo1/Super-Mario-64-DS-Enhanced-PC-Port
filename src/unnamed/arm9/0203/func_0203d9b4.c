extern unsigned char data_020a0f10;

int func_0203d9b4(void){
#if defined(SM64DS_PLATFORM_PC)
    /* The boot-mode mailbox is ARM9/ARM7 shared WRAM on DS and is not part of
       the native address space. Retain the game's save-state selection. */
    return *(unsigned char*)&data_020a0f10 == 0;
#else
    int b = (int)(*(unsigned short*)0x27ffc40 == 2);
    if (b != 0) return 0;
    return *(unsigned char*)&data_020a0f10 == 0;
#endif
}
