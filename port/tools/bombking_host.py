"""Bind King Bob-omb's original overlay callbacks/resources to native code."""
from functools import lru_cache
from pathlib import Path
import re
import struct

ROOT=Path(__file__).resolve().parents[2]

@lru_cache(None)
def inventory():
    sinit=(ROOT/'src/unnamed/ov078/__sinit_ov078_02126660.c').read_text()
    raw=(ROOT/'extracted/overlays/overlay_0078.bin').read_bytes()
    states={}
    for dst,part,source in re.findall(r'(data_ov078_[0-9a-f]+)\.(lo|hi) = (data_ov078_[0-9a-f]+);',sinit):
        target,adjust=struct.unpack_from('<II',raw,int(source[-8:],16)-0x02123740)
        assert adjust==0 and 0x02123740<=target<0x02126660
        states.setdefault(dst,{})[part]=target
    handles=dict(re.findall(r'(?:func_02017acc|_ZN13SharedFilePtr9ConstructEj)\(&(data_ov078_[0-9a-f]+), (0x[0-9a-f]+)\)',sinit))
    assert len(states)==18 and all(set(v)=={'lo','hi'} for v in states.values())
    assert len(handles)==13
    return states,handles

def transform(text):
    if 'int daBmb_c::InitResources()' in text:
        # The retail constructor builds four contiguous carry-offset vectors.
        # Generated individual BSS labels neither run it nor preserve stride.
        for i,address in enumerate((0x0214ea18,0x0214ea1c,0x0214ea20)):
            name=f'data_ov102_{address:08x}'
            text=text.replace(f'  extern int {name}[];','')
            text=text.replace(name,f'(pc_bmb_carry+{i})')
        return ("#include <stddef.h>\nstatic const int pc_bmb_carry[12] = {"
                "0x1c000,-0x14000,0x28000,0x1c000,-0x14000,0x3c000,"
                "0x2b000,-0xa000,-0x14000,0x30000,-0x1e000,0x3c000};\n"+
                text+'\nstatic_assert(sizeof(daBmb_c)==0x400 && offsetof(daBmb_c,mFuse)==0x3ea,"native Bob-omb layout");\n')
    if 'int daBombking_c::KingBobOmb_SetState(void *pv)' not in text:
        return text
    if 'native King Bob-omb callbacks' in text:return text
    states,handles=inventory()
    text=text.replace('typedef daBombking_c::StateFunction PMF;', '''// native King Bob-omb callbacks: retail records have two eight-byte entries.
struct PcBossCallback { int (*call)(daBombking_c *); int adjustment; };
struct PcBossState { PcBossCallback enter, tick; };
static_assert(sizeof(PcBossCallback)==8 && sizeof(PcBossState)==16,"boss state ABI");''')
    old='''    mState = (PMF *)pv;
    PMF *q = mState;
    if (*q == 0) return 1;
    return (this->**q)();'''
    new='''    mState = static_cast<daBombking_c::StateFunction *>(pv);
    const auto *state = static_cast<const PcBossState *>(pv);
    return state->enter.call ? state->enter.call(this) : 1;'''
    assert text.count(old)==1;text=text.replace(old,new)
    old='''        PMF *p = mState + 1;
        (this->**p)();'''
    assert text.count(old)==1
    text=text.replace(old,'''        reinterpret_cast<const PcBossState *>(mState)->tick.call(this);''')
    # This recovered call left its implicit receiver in ARM r0. Supply the
    # held player's actual address for the native member ABI.
    text=text.replace('_ZN6Player9GetHealthEv() != 0','mHeldActor->GetHealth() != 0')
    suffix=['\nstatic_assert(sizeof(daBombking_c)==0x50c && offsetof(daBombking_c,mHealth)==0x500,"native boss layout");']
    for target in sorted({v for pair in states.values() for v in pair.values()}):
        call=f'func_ov078_{target:08x}(reinterpret_cast<char *>(self))' if target==0x021240a0 else f'self->func_ov078_{target:08x}()'
        suffix.append(f'static int pc_boss_{target:08x}(daBombking_c *self) {{ return {call}; }}')
    suffix.append('extern "C" {')
    for name in handles:suffix.append(f'BombkingFile {name} = {{}};')
    suffix += ['void pc_bombking_init_resources() {', 'static bool ready=false; if(ready)return;ready=true;']
    for name,file_id in handles.items():suffix.append(f'reinterpret_cast<SharedFilePtr *>(&{name})->Construct({file_id});')
    for name,pair in states.items():
        suffix.append(f'*reinterpret_cast<PcBossState *>(&{name}) = {{{{pc_boss_{pair["lo"]:08x},0}},{{pc_boss_{pair["hi"]:08x},0}}}};')
    suffix += ['}', '}']
    return '#include <stddef.h>\n'+text+'\n'+'\n'.join(suffix)+'\n'
