"""Native startup/layout adapters for Battlefield's original moving scenery.

Only profiles 29/39/40 are enrolled here. The shared cartridge classes also
have other course variants; enrolling those requires reviewing their resources.
"""
import re


def lazy_handles(text, kinds):
    for kind in kinds:
        pattern = rf'{kind}\(unsigned int (\w+)\) \{{ pc_sharedfile_handle_construct\(this, \1\); \}}'
        text, count = re.subn(pattern, lambda m:
            f'{kind}(unsigned int {m[1]}) {{ words[0]={m[1]};words[1]=0; }}', text)
        assert count == 1, kind + ' constructor changed'
        old = f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old) == 1, kind + ' destructor changed'
        text = text.replace(old, f'~{kind}() {{}}', 1)
    return text


def observe(text, cls, fields, values):
    # Read-only diagnostics, after original behavior has run. Do not force the
    # range check or collider on: distant disabled collision is intentional.
    start = text.index(f'int {cls}::Behavior()')
    end = text.find('// @symbol', start)
    if end < 0:
        end = len(text)
    chunk = text[start:end]
    last = chunk.rfind('    return 1;')
    assert last >= 0
    trace = f'''    if(const char *path=std::getenv("SM64DS_PC_BATTLEFIELD_PLATFORM_TRACE")) {{
        static FILE *file=nullptr;
        if(!file){{file=std::fopen(path,"a");if(file)std::fputs("# {cls}:tick,uid,actor,x,y,z,angle_x,angle_y,collision,{fields}\\n",file);}}
        if(file && ntr::rt_frame()%5==0){{
            std::fprintf(file,"%llu,%u,%u,%d,%d,%d,%d,%d,%u,{','.join(['%d']*len(values))}\\n",
                static_cast<unsigned long long>(ntr::rt_frame()),unsigned(uniqueID),unsigned(actorID),
                mPosX,mPosY,mPosZ,int(mAngleX),int(mAngleY),unsigned(mMeshCollider.IsEnabled()),
                {','.join(values)});std::fflush(file);
        }}
    }}
'''
    text = text[:start] + chunk[:last] + trace + chunk[last:] + text[end:]
    render = f'int {cls}::Render()\n{{'
    assert text.count(render) == 1
    text = text.replace(render, render+'\n    pc_render_probe::Scope probe(this,0,&mModel.mat4x3);',1)
    return ('#include <cstdio>\n#include <cstdlib>\n#include <cassert>\n'
            '#include "ntr/rt.h"\n#include "hal/actor_render_probe.h"\n'+text)


def transform(text, name):
    if name == 'daObjSeesaw_c.cpp':
        text = lazy_handles(text, ('SeesawModelFilePtr','SeesawCollisionFilePtr'))
        # These three labels are parallel columns of one 12-byte-stride guest
        # table. Battlefield uses row 1, whose native handles live in this TU.
        # Rebase that reviewed row rather than dereferencing cartridge pointers.
        declaration = '''extern SeesawModelFilePtr data_ov095_02137780;
extern SeesawCollisionFilePtr data_ov095_021377a0;
extern "C" unsigned char data_ov014_02114598[];
'''
        anchor = 'extern "C" {\nextern int data_ov095_02136f58[];'
        assert text.count(anchor) == 1
        text = text.replace(anchor,declaration+'\n'+anchor,1)
        for symbol,handle in [('021374a0','02137780'),('021374a4','021377a0')]:
            pattern = rf'\*\(SharedFilePtr \*\*\)\(data_ov095_{symbol} \+ (?:mVariant|idx|i) \* kFileStride\)'
            text, count = re.subn(pattern,'&data_ov095_'+handle,text)
            assert count == 2, symbol
        old = '*(void **)(data_ov095_021374a8 + i * kFileStride)'
        assert text.count(old) == 1
        text = text.replace(old,'data_ov014_02114598',1)
        anchor = '    idx = mVariant;'
        assert text.count(anchor) == 1
        text = text.replace(anchor,'''    assert(actorID==ACTOR_BOMB_SEESAW && mVariant==1);
    static bool ready=false;
    if(!ready){data_ov095_02137780.Construct(1445);data_ov095_021377a0.Construct(1446);ready=true;}
'''+anchor,1)
        return observe(text,'daObjSeesaw_c','variant,speed,pounded',
                       ['int(mVariant)','int(mAngleXSpeed)','int(mPoundedThisFrame)'])

    if name == 'daObjRotateUpdownLift_c.cpp':
        text = lazy_handles(text,('RotateUpdownLiftModelFile','RotateUpdownLiftCollisionFilePtr'))
        for original,local,kind in [('02134c30','pc_lift_models','UpdownFileSlot'),
                                    ('02134c34','pc_lift_collisions','UpdownFileSlot'),
                                    ('02134c38','pc_lift_clps','UpdownClpsSlot')]:
            old = f'extern {kind} data_ov091_{original}[];'
            assert text.count(old)==1
            text=text.replace(old,f'static {kind} {local}[3];',1)
            text=text.replace('data_ov091_'+original+'[',local+'[')
        # Node5.y is an interior label in the SAME ten-node path allocation.
        text=text.replace('extern UpdownTopY data_ov091_02134d1c[3];','')
        old='data_ov091_02134d1c[mVariant].y'
        assert text.count(old)==1
        text=text.replace(old,'data_ov091_02134cdc[mVariant].node[5].y',1)
        # Derived fields move when MSVC retains native base-class tail padding.
        replacements={
            '(Matrix4x3 *)(c + 0x348)':'reinterpret_cast<Matrix4x3 *>(pad_348)',
            '*(int *)(c + 0x36c)':'reinterpret_cast<Matrix4x3 *>(pad_348)->t.x',
            '*(int *)(c + 0x370)':'reinterpret_cast<Matrix4x3 *>(pad_348)->t.y',
            '*(int *)(c + 0x374)':'reinterpret_cast<Matrix4x3 *>(pad_348)->t.z',
            'c, c + 0x320, c + 0x348,':'this, &mShadowModel, pad_348,',
            '*(s16 *)((char *)this + 0x3a2)':'*reinterpret_cast<s16 *>(&mPitchBase)',
            '*(s16 *)((char *)stepAt + 0xa4)':'mPitchStep',
        }
        for old,new in replacements.items():
            assert old in text,old
            text=text.replace(old,new)
        anchor='int daObjRotateUpdownLift_c::InitResources()\n{'
        assert text.count(anchor)==1
        text=text.replace(anchor,'extern "C" void pc_lift_init_resources();\n'+anchor+'\n    pc_lift_init_resources();',1)
        text+='''
extern "C" unsigned char data_ov002_0210d834[];
extern "C" void pc_lift_init_resources() {
    static bool ready=false;if(ready)return;
    data_ov091_02135448.Construct(0x493);
    data_ov091_02135450.Construct(0x494);
    // Only UPDOWN_LIFT29 is registered; its two ordinary course paths share files.
    for(int i=0;i<2;++i){
        pc_lift_models[i].file=&data_ov091_02135448;
        pc_lift_collisions[i].file=&data_ov091_02135450;
        pc_lift_clps[i].clps=reinterpret_cast<CLPS_Block *>(data_ov002_0210d834);
    }
    ready=true;
}
'''
        return observe(text,'daObjRotateUpdownLift_c','variant,node,dead',
                       ['int(mVariant)','int(mWaypointIndex)','int(mIsDead)'])

    if name == 'd_a_obj_b_swdoor.cpp':
        text=lazy_handles(text,('BSwdoorModelFilePtr','BSwdoorClsnFilePtr'))
        old='extern ResourceDescriptor data_ov014_021145c4;'
        assert text.count(old)==1
        text=text.replace(old,'ResourceDescriptor data_ov014_021145c4;',1)
        anchor='    s32 r = func_ov002_020bad10(this, &data_ov014_021145c4);'
        assert text.count(anchor)==1
        text=text.replace(anchor,'    pc_battlefield_gate_init_resources();\n'+anchor,1)
        text='extern "C" void pc_battlefield_gate_init_resources();\n'+text
        text+='''
extern "C" unsigned char data_ov014_02114578[];
extern "C" void pc_battlefield_gate_init_resources() {
    static bool ready=false;if(ready)return;
    data_ov014_02114948.Construct(1443);data_ov014_02114940.Construct(1444);
    data_ov014_021145c4={&data_ov014_02114948,&data_ov014_02114940,
        reinterpret_cast<CLPS_Block *>(data_ov014_02114578)};
    ready=true;
}
'''
        return text

    assert name == 'daObjSwdoor_c.cpp',name
    # Four bytes straddle ARM base padding and the concrete leaf. Address the
    # two padding bytes relative to the native base's last actual member;
    # timer/event use the concrete leaf's actual native members.
    text=text.replace('#include "daObjSwdoor_c.h"','#include "daObjSwdoor_c.h"\n#include "daObjBSwdoor_c.h"',1)
    for old,new in {
        '*(u8*)(c + 0x31f)':'reinterpret_cast<u8 *>(&self->unk_31d)[2]',
        '*(u8*)(c+0x321)':'static_cast<daObjBSwdoor_c *>(self)->mEventBit',
        '*(u8*)(c+0x320)':'static_cast<daObjBSwdoor_c *>(self)->mTimer',
        '(u8*)(c+0x320)':'&static_cast<daObjBSwdoor_c *>(self)->mTimer',
        '(*LM(c+0x31f))':'(reinterpret_cast<u8 *>(&self->unk_31d)[2])',
        '*(u8*)(c+0x31e)':'reinterpret_cast<u8 *>(&self->unk_31d)[1]',
        '*(unsigned char *)((char *)self + 0x31e)':'reinterpret_cast<u8 *>(&self->unk_31d)[1]',
        '*(unsigned char *)((char *)self + 0x321)':'static_cast<daObjBSwdoor_c *>(self)->mEventBit',
        '*(unsigned char *)((char *)self + 0x31f)':'reinterpret_cast<u8 *>(&self->unk_31d)[2]',
        '*(unsigned char *)((char *)self + 0x320)':'static_cast<daObjBSwdoor_c *>(self)->mTimer',
    }.items():
        assert old in text,old
        text=text.replace(old,new)
    render='s32 daObjSwdoor_c::Render()\n{'
    assert text.count(render)==1
    text=text.replace(render,render+'\n    pc_render_probe::Scope probe(this,0,&mModel.mat4x3);',1)
    anchor='    self->UpdateModelPosAndRotY();\n    return 1;'
    assert text.count(anchor)==1
    trace='''    if(const char *path=std::getenv("SM64DS_PC_BATTLEFIELD_GATE_TRACE")) {
        static FILE *file=nullptr;
        if(!file){file=std::fopen(path,"w");if(file)std::fputs("tick,uid,actor,state,timer,event,event_on,direction,angle_y,collision\\n",file);}
        if(file && ntr::rt_frame()%5==0){
            auto *gate=static_cast<daObjBSwdoor_c *>(self);
            std::fprintf(file,"%llu,%u,%u,%u,%u,%u,%u,%u,%d,%u\\n",
                static_cast<unsigned long long>(ntr::rt_frame()),unsigned(self->uniqueID),unsigned(self->actorID),
                unsigned(reinterpret_cast<u8 *>(&self->unk_31d)[2]),unsigned(gate->mTimer),unsigned(gate->mEventBit),
                _ZN5Event6GetBitEj(gate->mEventBit),unsigned(reinterpret_cast<u8 *>(&self->unk_31d)[1]),
                int(self->mAngleY),unsigned(self->mMeshCollider.IsEnabled()));std::fflush(file);
        }
    }
'''
    text=text.replace(anchor,trace+anchor,1)
    text+='''
static_assert(offsetof(dBgActor_c,unk_31d)+3<=sizeof(dBgActor_c),
              "switch-gate direction/state must stay in native base padding");
static_assert(offsetof(daObjBSwdoor_c,mTimer)>=sizeof(dBgActor_c),
              "switch-gate timer must not overlap native base padding");
'''
    return ('#include <cstdio>\n#include <cstdlib>\n#include <cstddef>\n'
            '#include "ntr/rt.h"\n#include "hal/actor_render_probe.h"\n'+text)
