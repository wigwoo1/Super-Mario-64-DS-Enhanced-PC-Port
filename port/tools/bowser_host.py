"""Defer Bowser resource construction until the native runtime is ready."""
import re

from battlefield_platform_host import lazy_handles


def transform_firesea_arena(root, text):
    import struct
    raw = (root / 'extracted/overlays/overlay_0060.bin').read_bytes()
    assert struct.unpack_from('<IHHIiiii', raw, 0x0211a88c-0x02111900) == (
        0x02117cdc, 166, 145, 0, 0, 0x1388000, 0x1388000, 0x1388000)
    arena = (root / 'extracted/overlays/overlay_0046.bin').read_bytes()
    assert struct.unpack_from('<4sHH', arena, 0x021115bc-0x021111a0)[:2] == (b'CLPS', 8)
    text = lazy_handles(text, ('Kpa2BgModelFilePtr', 'Kpa2BgCollisionFilePtr'))
    anchor = 'int daKpa2Bg_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '''
    auto *surface = pc_level_overlay_address(38, 0x021115bc);
    if (!surface) return 0;
    static const bool ready = [] {
        daKpa2Bg_c_ModelFile.Construct(1571);
        daKpa2Bg_c_ClsnFile.Construct(1572);
        return true;
    }();
    (void)ready;''', 1)
    assert text.count('&data_ov046_021115bc') == 1
    text = text.replace('&data_ov046_021115bc', 'surface', 1)
    return '#include "hal/level_overlay.h"\n' + text

def transform(root,text):
    handles=re.findall(r'(KpaModelFilePtr|KpaAnimationFileHandle|KpaTexSequenceFilePtr) (data_ov060_[0-9a-f]+)\((0x[0-9a-f]+)\);',text)
    assert len(handles)==35, len(handles)
    for kind in ('KpaModelFilePtr','KpaAnimationFileHandle','KpaTexSequenceFilePtr'):
        old=f'{kind}(unsigned int fileID) {{ pc_sharedfile_handle_construct(this, fileID); }}'
        assert text.count(old)==1
        text=text.replace(old,f'{kind}(unsigned int fileID) {{ words[0]=fileID;words[1]=0; }}')
        old=f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old)==1
        text=text.replace(old,f'~{kind}() {{}}')
    old='int a;  /* deliberately never assigned: it is animFlags, still in r2 (see above) */'
    assert text.count(old)==1
    text=text.replace(old,'int a = animFlags; // ARM register carry becomes an explicit native argument.')
    for symbol, column in (('02119296', 1), ('02119298', 2)):
        old = f'(char*)data_ov060_{symbol} + off'
        assert text.count(old) == column
        text = text.replace(old, f'(char*)(data_ov060_02119294 + {column}) + off')
    initializer='\nstatic void pc_bowser_files() {\n    static bool ready=false;if(ready)return;\n'
    for kind,name,fid in handles:
        initializer+=f'    pc_sharedfile_handle_construct(&{name}, {fid});\n'
    import struct
    config=(root/'config/arm9/overlays/ov060/delinks.txt').read_text()
    base=int(re.search(r'\.text\s+start:0x([\da-f]+)',config)[1],16)
    raw=(root/'extracted/overlays/overlay_0060.bin').read_bytes()
    known={name for kind,name,fid in handles}
    tables='\n'
    for address,count in ((0x21192dc,28),(0x211927c,6)):
        name=f'data_ov060_{address:08x}'
        entries=struct.unpack_from('<'+'I'*count,raw,address-base)
        targets=[f'data_ov060_{v:08x}' for v in entries]
        assert all(v in known for v in targets),targets
        native='pc_'+name
        text=text.replace(name,native)
        tables+=f'SharedFilePtr *{native}[{count}] = {{'+','.join('&'+v for v in targets)+'};\n'
    initializer+='    pc_sharedfile_handle_construct(data_ov060_0211b208,0x384);\n'
    initializer+='    pc_sharedfile_handle_construct(&data_ov089_02132c50,0x44d);\n'
    initializer+='    ready=true;\n}\n'
    initializer=tables+initializer
    anchor='// @symbol daKpa_c_classInit'
    assert text.count(anchor)==1
    text=text.replace(anchor,'static void pc_bowser_files();\n'+anchor)
    text+=initializer
    anchor='int daKpa_c::InitResources()\n{'
    assert text.count(anchor)==1
    return text.replace(anchor,anchor+'\n    pc_bowser_files();')


def transform_fire(root,text):
    import struct
    anchor='''            do {
                idx = ((u32)RandomIntInternal(&data_0209e650[0]) >> 0x10) % 3;'''
    assert text.count(anchor)==1
    # A diagnostic course entry can reach Bowser with no unlocked characters.
    # The cartridge assumes at least Mario is unlocked and otherwise loops
    # forever choosing a cap. Keep its selection/RNG unchanged for nonempty
    # masks; use the existing coin branch when no eligible cap exists.
    text=text.replace(anchor, '''            if (mask != 0) {
'''+anchor)
    end='''            return;
        }
    }

    {
        dActor_c* r = (dActor_c*)_ZN8dActor_c5SpawnEjjRK7Vector3PK10Vector3_16as(
            ACTOR_COIN'''
    assert text.count(end)==1
    text=text.replace(end, '''            return;
            }
            std::fprintf(stderr,"[Bowser cap selection] no unlocked cap; using coin reward\\n");
        }
    }

    {
        dActor_c* r = (dActor_c*)_ZN8dActor_c5SpawnEjjRK7Vector3PK10Vector3_16as(
            ACTOR_COIN''')
    config=(root/'config/arm9/overlays/ov060/delinks.txt').read_text()
    base=int(re.search(r'\.text\s+start:0x([\da-f]+)',config)[1],16)
    raw=(root/'extracted/overlays/overlay_0060.bin').read_bytes()
    text=text.replace('typedef void (dActor_c::*PMF)();','typedef void (*PMF)(daKpaFire_c *);')
    text=text.replace('typedef void (dActor_c::*ActorFn)();','typedef PMF ActorFn;')
    text=text.replace('(((dActor_c *)this)->*data_ov060_0211af74[this->mVariant])();','data_ov060_0211af74[this->mVariant](this);')
    text=text.replace('(this->*data_ov060_0211afb4[mVariant].pmf)();','data_ov060_0211afb4[mVariant].pmf(this);')
    for addr in range(0x211a734,0x211a7ad,8):
        target,adjust=struct.unpack_from('<II',raw,addr-base)
        assert adjust==0
        fn=f'func_ov060_{target:08x}'
        match=re.search(r'void '+fn+r'\(([^)]*)\)\s*\{',text)
        assert match,fn
        cast='reinterpret_cast<char *>(self)' if match[1].startswith('char') else 'self'
        anchor=f'extern PMF data_ov060_{addr:08x};'
        assert text.count(anchor)==1
        text=text.replace(anchor,f'static PMF data_ov060_{addr:08x} = [](daKpaFire_c *self) {{ {fn}({cast}); }};')
    declarations=(
        'extern void* _ZN8Particle6System12FromUniqueIDEj(u32 id);',
        'extern int _ZNK15dExtFrameCtrl_c13GetFrameCountEv(void *anim);',
        'extern void _ZN6Player4BurnEv(void* p);')
    for declaration in declarations:
        assert text.count(declaration)==1
        text=text.replace(declaration,'')
    text=text.replace('_ZN8Particle6System12FromUniqueIDEj(', 'Particle::System::FromUniqueID(')
    text=text.replace('_ZNK15dExtFrameCtrl_c13GetFrameCountEv((char *)o + 0x124)', 'reinterpret_cast<dExtFrameCtrl_c *>((char *)o + 0x124)->GetFrameCount()')
    text=text.replace('_ZN6Player4BurnEv(p);', 'static_cast<Player *>(p)->Burn();')
    return '#include <cstdio>\n#include "Particle__System.h"\n'+text


def transform_mine(root,text):
    import struct
    config=(root/'config/arm9/overlays/ov060/delinks.txt').read_text()
    base=int(re.search(r'\.text\s+start:0x([\da-f]+)',config)[1],16)
    raw=(root/'extracted/overlays/overlay_0060.bin').read_bytes()
    assert struct.unpack_from('<IHHIiiii',raw,0x0211aa68-base)==(0x02118cbc,284,147,2,0,1638400,98304000,0)
    old='KiraiModelFilePtr(unsigned int fileID) { pc_sharedfile_handle_construct(this, fileID); }'
    assert text.count(old)==1
    text=text.replace(old,'KiraiModelFilePtr(unsigned int fileID) { words[0]=fileID;words[1]=0; }')
    old='~KiraiModelFilePtr() { pc_sharedfile_handle_destroy(this); }'
    assert text.count(old)==1
    text=text.replace(old,'~KiraiModelFilePtr() {}')
    anchor='int daKirai_c::InitResources()\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+"\n    static bool ready=false; if(!ready) { pc_sharedfile_handle_construct(&data_ov060_0211b1c4,0x382);ready=true; }")
    trace=r'''
static void pc_kirai_observe(daKirai_c *mine) {
    const char *path=std::getenv("SM64DS_PC_KIRAI_TRACE");if(!path || !*path)return;
    static FILE *file=std::fopen(path,"w");static bool header=false;
    if(!file)return;
    if(!header){std::fputs("tick,uid,alive,state,x,y,z,home_x,home_y,home_z,home_radius,trigger_distance,slot,hit,other\n",file);header=true;}
    std::fprintf(file,"%llu,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%u\n",
        static_cast<unsigned long long>(ntr::rt_frame()),mine->uniqueID,mine->aliveState,mine->mStateIndex,
        mine->mPosX,mine->mPosY,mine->mPosZ,mine->mHomePosX,mine->mHomePosY,mine->mHomePosZ,
        mine->mHomeHorzDist,mine->mHomeYOffset,mine->mSlotIndex,mine->mdCcAcPos_c.hitFlags,mine->mdCcAcPos_c.otherOwner);
    std::fflush(file);
}
'''
    anchor='int daKirai_c::Behavior()\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,trace+'\n'+anchor+'\n    pc_kirai_observe(this);')
    return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n'+text


def transform_key(root,text):
    import struct
    config=(root/'config/arm9/overlays/ov089/delinks.txt').read_text()
    base=int(re.search(r'\.text\s+start:0x([\da-f]+)',config)[1],16)
    raw=(root/'extracted/overlays/overlay_0089.bin').read_bytes()
    assert struct.unpack_from('<IHHIiiii',raw,0x02132b68-base)==(0x02132828,282,149,536870914,409600,409600,16384000,16384000)
    handles=re.findall(r'(ObjKeyModelFile|ObjKeyAnimationFilePtr) (data_ov089_[0-9a-f]+)\((0x[0-9a-f]+)\);',text)
    assert len(handles)==8
    for kind in ('ObjKeyModelFile','ObjKeyAnimationFilePtr'):
        old=f'{kind}(unsigned int fileID) {{ pc_sharedfile_handle_construct(this, fileID); }}'
        assert text.count(old)==1
        text=text.replace(old,f'{kind}(unsigned int fileID) {{ id=fileID;ptr=nullptr; }}')
        old=f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old)==1
        text=text.replace(old,f'~{kind}() {{}}')
    known={int(name[-8:],16):'&'+name for kind,name,fid in handles}
    externs={0x021305d8:'data_ov085_021305d8',0x0211094c:'data_ov002_0211094c',0x0210da40:'data_ov002_0210da40',0x0210d9a0:'data_ov002_0210d9a0',0x0210d9c0:'data_ov002_0210d9c0'}
    tables='\nextern "C" unsigned char data_ov085_021305d8[],data_ov002_0210da40[],data_ov002_0210d9a0[],data_ov002_0210d9c0[];\n'
    for addr,count in ((0x02132880,5),(0x02132894,8),(0x021328b4,8)):
        original=f'data_ov089_{addr:08x}'
        text=re.sub(r'extern (?:void \*|ObjKeyFile \*)'+original+r'\[\];','',text)
        if addr==0x2132894:
            text=re.sub(r'extern [^;\n]*'+original+r'\[\];','',text)
        native='pc_'+original
        entries=[]
        for value in struct.unpack_from('<'+'I'*count,raw,addr-base):
            assert value==0 or value in known or value in externs,hex(value)
            target=known.get(value,'&'+externs.get(value,''))
            entries.append('nullptr' if value==0 else '(ObjKeyFile *)'+target)
        tables+=f'static ObjKeyFile *{native}[{count}]={{'+','.join(entries)+'};\n'
        text=text.replace(original,native)
    anchor='namespace cstd { s32 sqrt(u64 value); }'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\nstatic void pc_key_files();\n'+tables)
    init='\nstatic void pc_key_files() {\n    static bool ready=false;if(ready)return;\n'
    for kind,name,fid in handles:
        init+=f'    if(!{name}.ptr)pc_sharedfile_handle_construct(&{name},{fid});\n'
    init+='    ready=true;\n}\n'
    text+=init
    anchor='extern "C" void LoadKeyModels(int kind)\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    pc_key_files();')
    # Expose only observation; the retail callback owns key credit and animation.
    anchor='int daObjKey_c::Behavior()\n{'
    assert text.count(anchor)==1
    trace=r"""
    if(const char *path=std::getenv("SM64DS_PC_KEY_TRACE")) {
        static FILE *file=std::fopen(path,"w");static bool header=false;
        if(file){if(!header){std::fputs("tick,uid,kind,step,anim,x,y,z,hit,other,flags1,collector,alive\n",file);header=true;}
        std::fprintf(file,"%llu,%u,%d,%u,%d,%d,%d,%d,%u,%u,%u,%u,%u\n",static_cast<unsigned long long>(ntr::rt_frame()),uniqueID,mState,mStep,mAnimID,mPosX,mPosY,mPosZ,mdCcAcPos_c.hitFlags,mdCcAcPos_c.otherOwner,unsigned(data_0209caa0[1]),mPlayer?mPlayer->uniqueID:0,aliveState);std::fflush(file);}
    }
"""
    text=text.replace(anchor,anchor+trace)
    return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n'+text
