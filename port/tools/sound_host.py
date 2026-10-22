"""Native sound adaptations; matched cartridge source stays untouched."""
STAGE_SOUND_START=0x02075768
STAGE_SOUND_END=0x02075804
STAGE_SOUND_LABELS=('data_02075768','data_02075769','data_0207576a')

def write_sources(root,directory):
    """Keep all 52 {group,bank,music} rows in their original allocation."""
    image=(root/'extracted/arm9_dec.bin').read_bytes()
    table=image[STAGE_SOUND_START-0x02004000:STAGE_SOUND_END-0x02004000]
    if len(table)!=52*3:raise ValueError('Stage sound table boundary changed')
    symbols=(root/'config/arm9/symbols.txt').read_text()
    for i,name in enumerate(STAGE_SOUND_LABELS):
        if f'{name} kind:data(any) addr:0x{STAGE_SOUND_START+i:08x}' not in symbols:
            raise ValueError('Stage sound interior label changed: '+name)
    lines=['; Generated locally from original level sound records; do not publish.',
        '.386','.model flat','.data','ALIGN 4']
    for i,value in enumerate(table):
        if i<3:lines += ['PUBLIC _'+STAGE_SOUND_LABELS[i],'_'+STAGE_SOUND_LABELS[i]+' LABEL BYTE']
        lines.append(f'BYTE 0{value:02x}h')
    lines.append('END')
    directory.mkdir(parents=True,exist_ok=True)
    path=directory/'stage_sound_data.asm';text='\n'.join(lines)+'\n'
    if not path.exists() or path.read_text()!=text:path.write_text(text)
    return [path.as_posix()]

def transform(text,name):
    if name=='func_ov007_020be098.c':
        anchor='void func_ov007_020be098(int idx){'
        if text.count(anchor)!=1:raise ValueError('Menu sound event anchor changed')
        return ('extern "C++" {\n#include "ntr/audio.h"\n#include "ntr/rt.h"\n#include <cstdio>\n#include <cstdlib>\n}\n'+
            text.replace(anchor,anchor+'''
  if(ntr::audio_ready() && std::getenv("SM64DS_PC_AUDIO_TRACE"))
    std::fprintf(stderr,"[native menu audio] tick=%llu index=%d archive=2 id=%d handle=%p\\n",
      static_cast<unsigned long long>(ntr::rt_frame()),idx,data_ov007_02103134[idx],&data_ov007_02104bbc[idx]);
''',1))
    if name=='SoundEngine.cpp':
        hooks={
            'extern "C" int func_02051f1c(void *a0, u32 a1, int a2)':
                'if(ntr::audio_ready())return ntr::audio_effect(a1,a2,-1,static_cast<uint32_t>(reinterpret_cast<uintptr_t>(a0)))!=0;',
            'extern "C" int func_02051e60(int a, int b, int c, int d, unsigned int arg5, int arg6)':
                'if(ntr::audio_ready())return ntr::audio_effect(arg5,arg6,c,static_cast<uint32_t>(a))!=0;',
            'extern "C" void func_0204f94c(int *p)':
                'if(ntr::audio_ready())ntr::audio_stop(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p)));',
            'extern "C" void func_0204f934(int **p)':
                'if(ntr::audio_ready()){*p=nullptr;return;}',
            'extern "C" void func_0204f924(void **p, unsigned char v)':
                'if(ntr::audio_ready()){ntr::audio_volume(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p)),v,0);return;}',
        }
        import re
        for signature,body in hooks.items():
            pattern=re.escape(signature)+r'\s*\{'
            text,count=re.subn(pattern,lambda m:m[0]+'\n#if defined(SM64DS_PLATFORM_PC)\n    '+body+'\n#endif\n',text,count=1)
            if count!=1:raise ValueError('SDK sound handle anchor changed: '+signature)
        return text
    # The original menu owns real sequence-handle words. Native voices use
    # their addresses as keys; no ARM driver object pointer is fabricated.
    # Every menu liveness check must ask the native player instead of reading
    # the otherwise-empty DS handle word.
    queries={
        'func_ov007_020bd9e4.c':('data_ov007_02104bbc[i] != 0','pc_sound_handle_active(&data_ov007_02104bbc[i])'),
        'func_ov007_020bdbcc.c':('data_ov007_02104bbc[0x35] == 0','!pc_sound_handle_active(&data_ov007_02104bbc[0x35])'),
        'func_ov007_020bde2c.c':('data_ov007_02104bbc[idx] == 0','!pc_sound_handle_active(&data_ov007_02104bbc[idx])'),
        'func_ov007_020bde70.c':('data_ov007_02104bbc[idx] == 0','!pc_sound_handle_active(&data_ov007_02104bbc[idx])'),
        'func_ov007_020bdebc.c':('data_ov007_02104bbc[j] != 0','pc_sound_handle_active(&data_ov007_02104bbc[j])'),
        'func_ov007_020bdf60.c':('((int*)data_ov007_02104bbc)[i] != 0','pc_sound_handle_active(&((int*)data_ov007_02104bbc)[i])'),
    }
    if name in queries:
        old,new=queries[name]
        if text.count(old)!=1:raise ValueError('Menu sound handle query changed: '+name)
        return ('extern "C++" {\n#include "ntr/audio.h"\n}\n'
            'static bool pc_sound_handle_active(const void *slot) {\n'
            '    return ntr::audio_ready()?ntr::audio_active(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(slot))):'
            '*static_cast<const unsigned *>(slot)!=0;\n}\n'+text.replace(old,new,1))
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
