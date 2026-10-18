"""Bind recovered MIP/key callbacks to native functions without changing game logic."""
import re
import struct


def inventory(root):
    symbols=(root/'config/arm9/overlays/ov085/symbols.txt').read_text()
    bounds=(root/'config/arm9/overlays/ov085/delinks.txt').read_text()
    base=int(re.search(r'\.text\s+start:0x([\da-f]+)',bounds)[1],16)
    rom=(root/'extracted/overlays/overlay_0085.bin').read_bytes()
    methods={int(a,16):m for m,a in re.findall(
        r'^_ZN7daMip_c\d+(State\w+)Ev kind:function[^\n]*addr:0x([\da-f]+)',symbols,re.M)}
    bindings={}
    for address in range(0x0213003c,0x021300bc,8):
        target,adjustment=struct.unpack_from('<II',rom,address-base)
        if adjustment or target not in methods:raise ValueError(f'MIP callback {address:08x} changed')
        bindings[f'data_ov085_{address:08x}']=methods[target]
    keys={}
    for address in range(0x02130174,0x02130194,8):
        target,adjustment=struct.unpack_from('<II',rom,address-base)
        if adjustment:raise ValueError('MIP key callback adjustment changed')
        if target not in (0x0212cd80,0x0212d038,0x0212d108,0x0212d24c):raise ValueError('MIP key callback target changed')
        keys[f'data_ov085_{address:08x}']=f'func_ov085_{target:08x}'
    return bindings,keys


def transform(root,text,name):
    bindings,keys=inventory(root)
    if name=='daObj_Mip_Key_c.cpp':
        text=re.sub(r'^extern MipKeyPMF data_ov085_[\da-f]+;\s*$','',text,flags=re.M)
        text=text.replace('(c->*c->mState->enter)()', 'c->mState->enter(c)')
        text=text.replace('(this->*mState->execute)()', 'mState->execute(this)')
        for constant,target in keys.items():text=text.replace(constant,target)
        # Definitions may be after the first callback reference.
        declarations='\n'.join(f'extern "C" int {target}(daObj_Mip_Key_c *);' for target in sorted(set(keys.values())))
        text=text.replace('extern "C" {',declarations+'\nextern "C" {',1)
        return '#include <cstddef>\n'+text+'\nstatic_assert(sizeof(daObj_Mip_Key_c)==0x1a0 && offsetof(daObj_Mip_Key_c,mState)==0x188,"native rabbit key layout");\n'
    if name!='daMip_c.cpp':return text
    old='typedef int (daMip_cSelf::*daMip_cStateFn)();'
    if text.count(old)!=1:raise ValueError('MIP callback typedef changed')
    text=text.replace(old,'struct daMip_cStateFn { int (*fn)(daMip_c *); int adjustment; };')
    text=re.sub(r'^extern daMip_cStateFn data_ov085_[\da-f]+;\s*$','',text,flags=re.M)
    wrappers='\n'.join(f'static int pc_mip_{m}(daMip_c *self) {{ return self->{m}(); }}' for m in sorted(set(bindings.values())))
    text=text.replace('struct MipStatePair {',wrappers+'\nstruct MipStatePair {',1)
    for constant,method in bindings.items():
        text=text.replace(constant,'{pc_mip_'+method+', 0}')
    text,count=re.subn(r'int daMip_c::SetState\(void \*record\)\s*\{.*?\n\}',
        '''int daMip_c::SetState(void *record) {
    mState = reinterpret_cast<int>(record);
    auto *state = static_cast<MipStatePair *>(record);
    return state->a.fn ? state->a.fn(this) : 1;
}''',text,count=1,flags=re.S)
    if count!=1:raise ValueError('MIP SetState changed')
    start=text.index('    {\n        int* p = *(int**)&mState;')
    end=text.index('    mModelAnim.Advance();',start)
    text=text[:start]+'''    auto *state = reinterpret_cast<MipStatePair *>(mState);
    if (state->b.fn) state->b.fn(this);

'''+text[end:]
    # hostgen has already supplied the eight-byte resource wrapper constructors.
    handles=re.findall(r'(DaMip(?:ModelFile|AnimationFilePtr)) (data_ov085_[\da-f]+)\((0x[\da-f]+)\);',text)
    if len(handles)!=7:raise ValueError('MIP resource roster changed')
    for kind in ('DaMipModelFile','DaMipAnimationFilePtr'):
        text=text.replace(f'{kind}(unsigned int fileID) {{ pc_sharedfile_handle_construct(this, fileID); }}',
                          f'{kind}(unsigned int fileID) {{ words[0]=fileID; words[1]=0; }}')
        text=text.replace(f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}',f'~{kind}() {{}}')
    text+='\nextern "C" void pc_rabbit_init_resources() {\nstatic bool ready=false;if(ready)return;ready=true;\n'
    for _,symbol,file_id in handles:text+=f'pc_sharedfile_handle_construct(&{symbol},{file_id});\n'
    text+='}\n'
    records=re.findall(r'MipStatePair (data_ov085_[\da-f]+) =',text)
    if len(records)!=8:raise ValueError('MIP state roster changed')
    text+='''
static_assert(sizeof(daMip_c)==0x474 && offsetof(daMip_c,mState)==0x364,"native rabbit layout");
static_assert(sizeof(daMip_cStateFn)==8 && sizeof(MipStatePair)==16,"rabbit callback stride");
extern "C" int pc_rabbit_state_id(const void *object) {
    if(!object)return -1;
    const auto *rabbit=static_cast<const daMip_c *>(object);
'''
    for record in records:text+=f'    if(rabbit->mState==reinterpret_cast<int>(&{record}))return 0x{record[-8:]};\n'
    text+='    return -2;\n}\n'
    return '#include <cstddef>\n'+text
