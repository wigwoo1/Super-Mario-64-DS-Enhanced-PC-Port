"""Native sound adaptations; matched cartridge source stays untouched."""
def transform(text,name):
    if name!='Sound.cpp':return text
    # The original coin cue ends after tied notes. The sequencer now releases
    # that held voice at END; remove the older arbitrary pickup timeout.
    start=text.index('        /* SSAR event 0x11 is the red/yellow coin pickup.')
    end=text.index('        else ntr::audio_effect(j1,j2,bank);',start)+len('        else ntr::audio_effect(j1,j2,bank);')
    text=text[:start]+'        ntr::audio_effect(j1,j2,bank);'+text[end:]
    anchor='extern "C" void func_0201251c(int a, int b, int c, int d)\n{'
    if text.count(anchor)!=1:raise ValueError('Motion sound helper anchor changed')
    # Original func_02048a18 is a four-byte BX LR identity: sound ID b is
    # unchanged by speed d. The remaining retail path uses the DS driver.
    branch='''
#if defined(SM64DS_PLATFORM_PC)
    if(ntr::audio_ready()) {
        ntr::audio_effect(a,b,a==3?data_0208e428:-1);
        return;
    }
#endif
'''
    return text.replace(anchor,anchor+branch,1)
