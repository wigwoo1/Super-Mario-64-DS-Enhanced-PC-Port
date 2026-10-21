"""Bind recovered ov098 cannon/water-bomb callbacks to native functions."""
import re
import struct

def native_sound_lifetime(text):
    old='''#if defined(SM64DS_PLATFORM_PC)
        /* The cannon-rise loop spans the load/aim sequence in the host audio
           player. End it when the original launch cue fires. */
        ntr::audio_stop_effect(3,0x14c,4);
#endif
'''
    if text.count(old)!=1:raise ValueError('Legacy cannon sound-stop anchor changed')
    # Original player15 has one sequence slot. Its new cue replaces the motor
    # through archive metadata, without stopping other cues by effect number.
    return text.replace(old,'',1)


def inventory(root, bomb=False):
    bounds=(root/'config/arm9/overlays/ov098/delinks.txt').read_text()
    base=int(re.search(r'\.text\s+start:0x([\da-f]+)',bounds)[1],16)
    rom=(root/'extracted/overlays/overlay_0098.bin').read_bytes()
    order=(0x0213c724,0x0213c72c,0x0213c71c) if bomb else (0x0213c64c,0x0213c65c,0x0213c654,0x0213c644)
    expected=(0x0213b9d8,0x0213bb1c,0x0213b7e8) if bomb else (0x0213b0a4,0x0213ade8,0x0213ad08,0x0213aa28)
    pairs=[struct.unpack_from('<II',rom,address-base) for address in order]
    if pairs!=[(target,0) for target in expected]:
        raise ValueError('Cannon callback inventory differs from recovered initializer')
    return list(expected)


def transform(root,text,name):
    if name=='func_0201ffcc.c':
        # PC startup skips the DS mailbox/wireless initializer. Its zero ready
        # flag otherwise skips the offline camera/input publication as well.
        old='if (data_0209e64c == 0) return;'
        assert text.count(old)==1
        return 'extern void func_0203e0ac(void);\n'+text.replace(old,
            'if (data_0209e64c == 0) { func_0203e0ac(); return; }')
    if name not in ('d_a_cnn.cpp','daWbm_c.cpp'):return text
    bomb=name=='daWbm_c.cpp'
    if not bomb:text=native_sound_lifetime(text)
    cls='daWbm_c' if bomb else 'daCnn_c'
    wrapper='WbmModelFilePtr' if bomb else 'CnnModelFilePtr'
    table='data_ov098_0213c930' if bomb else 'data_ov098_0213c8fc'
    handle='data_ov098_0213c91c' if bomb else 'data_ov098_0213c8e8'
    file_id=0x42c if bomb else 0x45e
    targets=inventory(root,bomb)
    text=text.replace(f'typedef void ({cls}::*State)();',f'struct State {{ void (*call)({cls} *); int adjustment; }};')
    text,count=re.subn(r'extern State data_ov098_[\da-f]+;\s*','',text)
    assert count==len(targets)
    methods=['StateDrop','StateFall','StateBounce'] if bomb else [f'func_ov098_{target:08x}' for target in targets]
    funcs='\n'.join(f'static void pc_cannon_{target:08x}({cls} *self) {{ self->{method}(); }}' for target,method in zip(targets,methods))
    entries=',\n'.join(f'{{pc_cannon_{target:08x},0}}' for target in targets)
    text,count=re.subn(r'State '+table+r'\[\d\] = \{[\s\S]*?\};',funcs+'\nextern "C" { State '+table+'['+str(len(targets))+'] = {\n'+entries+'\n}; }',text)
    assert count==1
    old=f'(this->*{table}[mState])();'
    assert text.count(old)==1
    text=text.replace(old,f'{table}[mState].call(this);')
    # File constructors run before the game's allocator is ready on Windows.
    old=f'{wrapper}(unsigned int id) {{ pc_sharedfile_handle_construct(this, id); }}'
    assert text.count(old)==1
    text=text.replace(old,f'{wrapper}(unsigned int id) {{ unk0=id;file=0; }}')
    text=text.replace(f'~{wrapper}() {{ pc_sharedfile_handle_destroy(this); }}',f'~{wrapper}() {{}}')
    text=text.replace(f'{wrapper} {handle}(0x{file_id:x});',f'extern "C" {{ {wrapper} {handle}(0x{file_id:x}); }}')
    text+='\nstatic_assert(sizeof(State)==8,"native cannon callback stride");\n'
    text+=f'static_assert(sizeof({cls})==0x'+('3cc' if bomb else '198')+',"native cannon actor layout");\n'
    text+=f'extern "C" void pc_{"waterbomb" if bomb else "cannon"}_init_resources() {{\n'
    text+='    static bool ready=false;if(ready)return;ready=true;\n'
    text+=f'    {handle}.Construct(0x{file_id:x});\n}}\n'
    return text
