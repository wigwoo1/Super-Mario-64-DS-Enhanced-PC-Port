"""Native resource startup and read-only tracing for recovered Boo actors."""
import re
import struct


def transform(root, text):
    # King Boo's mirror cutscene lives with Luigi in overlay55. Overlay27 has
    # an unrelated member at the same DS address; the raw call has no receiver.
    assert text.count('_ZN17daObjSlIceBlock_c16CleanupResourcesEv') == 2
    text = text.replace('_ZN17daObjSlIceBlock_c16CleanupResourcesEv','func_ov055_02111264')
    assert text.count('data_ov008_02111b6c') == 2
    text = text.replace('data_ov008_02111b6c','data_ov055_02111b6c')
    text = bind_fireball_caller(text)
    cfg = (root/'config/arm9/overlays/ov063/delinks.txt').read_text()
    base = int(re.search(r'\.text\s+start:0x([\da-f]+)', cfg)[1], 16)
    raw = (root/'extracted/overlays/overlay_0063.bin').read_bytes()
    profiles = (
        (0x211e770, (0x211c590,209,49,6,0x64000,0x64000,0x1000000,0xa00000)),
        (0x211e78c, (0x211c520,210,50,6,0xc8000,0xc8000,0x1000000,0xa00000)),
        (0x211e7a8, (0x211c4d0,212,52,2,0x64000,0x64000,0x1000000,0xa00000)),
        (0x211e7c4, (0x211c4a0,211,51,0,0,0x1000,0,0)),
    )
    for address, expected in profiles:
        assert struct.unpack_from('<IHHIiiii', raw, address-base) == expected
    for kind in ('TrsModelFileResHandle','TrsAnimationResFileHandle'):
        pattern = rf'{kind}\(unsigned int (\w+)\) \{{ pc_sharedfile_handle_construct\(this, \1\); \}}'
        text, count = re.subn(pattern, lambda m:
            f'{kind}(unsigned int {m[1]}) {{fileId={m[1]};file=nullptr;}}', text)
        assert count == 1, kind
        old = f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old) == 1
        text = text.replace(old, f'~{kind}() {{}}', 1)
    handles = (('edc4',0x427),('edec',0x6c9),('edf4',0x2cc),
               ('eddc',0x428),('ede4',0x2cf),('edd4',0x2cd),('edcc',0x2ce))
    initializer = '\nstatic void pc_boo_files() {\n    static bool ready=false;\n    if(ready)return;\n'
    for suffix, resource in handles:
        assert f'data_ov063_0211{suffix}(0x{resource:x})' in text
        initializer += f'    data_ov063_0211{suffix}.Construct(0x{resource:x});\n'
    initializer += '    ready=true;\n}\n'
    for cls in ('daTrs_c','daTBasket_c'):
        anchor = f'int {cls}::InitResources()\n{{'
        assert text.count(anchor) == 1
        text = text.replace(anchor, anchor+'\n    pc_boo_files();', 1)
    trace = r'''
static void pc_boo_observe(const char *event,daTrs_c *self) {
    const char *path=std::getenv("SM64DS_PC_BOO_TRACE");if(!path||!*path)return;
    static FILE *file=nullptr;
    if(!file){file=std::fopen(path,"w");if(file)std::fputs("event,tick,uid,actor,param,x,y,z,state,variant,timer,hp,children,gate,boss,visible,opacity,collision,area,player,model\n",file);}
    if(file){std::fprintf(file,"%s,%llu,%u,%u,%u,%d,%d,%d,%u,%u,%d,%d,%d,%d,%u,%u,%u,%u,%d,%u,%u\n",event,
        static_cast<unsigned long long>(ntr::rt_frame()),unsigned(self->uniqueID),unsigned(self->actorID),unsigned(self->param1),
        self->mPosX,self->mPosY,self->mPosZ,unsigned(self->unk_5cc),unsigned(self->unk_5cf),int(self->mStateTimer),
        int(self->mDataIdx),int(self->mChildDeaths),int(self->mTimer180),unsigned(self->mBigBooID),unsigned(self->mFlags_5d4.b3),
        unsigned(self->mOpacity),unsigned(self->mdCcAcPos_c.flags),int(self->mAreaIdx),
        self->mClosestPlayer?unsigned(self->mClosestPlayer->uniqueID):0,
        unsigned(reinterpret_cast<std::uintptr_t>(self->mModelAnim.data.modelFile)));std::fflush(file);}
}
'''
    anchor = '// @symbol _ZN7daTrs_cD1Ev'
    assert text.count(anchor) == 1
    text = text.replace(anchor, initializer+trace+'\n'+anchor, 1)
    for method, event in (('InitResources','init'),('Behavior','behavior'),('CleanupResources','cleanup')):
        anchor = f'int daTrs_c::{method}()\n{{'
        assert text.count(anchor) == 1, anchor
        condition = 'if(ntr::rt_frame()%5==0)' if method=='Behavior' else ''
        text = text.replace(anchor, anchor+f'\n    {condition}pc_boo_observe("{event}",this);', 1)
    anchor = '    mParticle0 = mParticle1;\n    return 1;'
    assert text.count(anchor) == 1
    text = text.replace(anchor,'    mParticle0 = mParticle1;\n    pc_boo_observe("ready",this);\n    return 1;',1)
    anchor = 'int daTrs_c::Render()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor,anchor+'\n    pc_render_probe::Scope probe(this,!mFlags_5d4.b3,&mModelAnim.mat4x3);',1)
    text += '\nstatic_assert(sizeof(daTrs_c)==0x5e0,"native Boo layout");\n'
    text += 'static_assert(offsetof(daTrs_c,unk_5cf)==0x5cf,"native Boo variant offset");\n'
    return '#include <cstddef>\n#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n#include "hal/actor_render_probe.h"\n'+text


def bind_fireball_caller(text):
    old = '''            if (((unsigned int)*(u16*)&this->mFlags_5d4 << 23) >> 31)
                *(u8*)(fb + 0x36e) = 1;
            else
                *(u8*)(fb + 0x36e) = 0;
            *(int*)(fb + 0x364) = 0x3e8000;
            func_02012694(0x122, &this->mCamSpacePosX);'''
    assert text.count(old) == 1
    text = text.replace(old, '''            if (fb) {
                auto *ball = reinterpret_cast<daFPknBall_c *>(fb);
                ball->mMirrored = (static_cast<unsigned>(*(u16*)&mFlags_5d4) >> 8) & 1;
                ball->mMaxDistance = 0x3e8000;
                func_02012694(0x122, &this->mCamSpacePosX);
            }''')
    return text.replace('#include "common.h"', '#include "common.h"\n#include "daFPknBall_c.h"', 1)


def transform_fireball(text, name):
    if name == 'dActor_c.cpp':
        signature = 'extern "C" void _ZN8dActor_c13SpawnFireballERK7Vector3PK10Vector3_165Fix12IiES7_j('
        body = '        *(Fix12i *)((char *)a + 0x35c) = unk35c;\n    }\n}'
        assert text.count(signature) == 1 and text.count(body) == 1
        return (text.replace('#include "common.h"', '#include "common.h"\n#include "daFPknBall_c.h"', 1)
                    .replace(signature, signature.replace('void ', 'dActor_c *'))
                    .replace(body, '        static_cast<daFPknBall_c *>(a)->mTargetSpeed = unk35c;\n    }\n    return a;\n}'))

    anchor = 'int daFPknBall_c::Behavior() {'
    assert text.count(anchor) == 1
    default_range = '    mMaxDistance = 0x5dc000;'
    assert text.count(default_range) == 1
    text = text.replace(default_range, '    if (!mMaxDistance) mMaxDistance = 0x5dc000;')
    text = text.replace(anchor, anchor + r'''
    if (std::getenv("SM64DS_PC_BOO_TRACE") && param1 == 4) {
        const auto tick = ntr::rt_frame();
        if (mStateTimer == 0 || tick % 10 == 0)
            std::fprintf(stderr,"[native King Boo fireball] tick=%llu uid=%u side=%u range=%d travel=%d pos=%d,%d,%d\n",
                         static_cast<unsigned long long>(tick),uniqueID,unsigned(mMirrored),mMaxDistance,
                         mDistanceFlown,mPosX,mPosY,mPosZ);
    }
''')
    return ('#include <cstddef>\n#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n' + text +
            '\nstatic_assert(sizeof(daFPknBall_c)==0x378,"native fireball layout");\n')
