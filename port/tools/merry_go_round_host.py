"""Native resource identities for the recovered spinning-floor class.

MERRYGOROUND159 and TERESAPIT160 are registered. The other two factory variants keep
their original implementation but require independent integration/acceptance.
"""
import re
import struct


def transform(root, text):
    old='_ZN10dBgW_KcMbg9TransformERK9Matrix4x3s(c + 0x15c, *(Matrix4x3 *)(c + 0x324), *(short *)(c + 0x8e))'
    assert text.count(old)==1
    text=text.replace(old,'reinterpret_cast<dBgW_KcMbg *>(c+0x15c)->Transform(*reinterpret_cast<Matrix4x3 *>(c+0x324),*reinterpret_cast<short *>(c+0x8e))',1)
    cfg=(root/'config/arm9/overlays/ov063/delinks.txt').read_text()
    base=int(re.search(r'\.text\s+start:0x([\da-f]+)',cfg)[1],16)
    raw=(root/'extracted/overlays/overlay_0063.bin').read_bytes()
    for address,expected in ((0x211e27c,(0x211eeb0,0x211eea8,0x211eeb8,0x211ee98)),
                              (0x211e28c,(0x211eea0,0x211eed0,0x211eec0,0x211eec8)),
                              (0x211e9e8,(0x211ec58,0x211ebf8,0x211ec78,0x211ec18))):
        assert struct.unpack_from('<4I',raw,address-base)==expected
    assert struct.unpack_from('<IHHIiiii',raw,0x211ea48-base)==(
        0x211d2e0,159,56,0,0x64000,0x64000,0x1000000,0xa00000)
    assert struct.unpack_from('<IHHIiiii',raw,0x211ea64-base)==(
        0x211d2a0,160,57,0,0x64000,0x64000,0x1000000,0xa00000)
    for kind in ('TrsTrapModelFilePtr','TrsTrapCollisionFilePtr'):
        pattern=rf'{kind}\(unsigned int (\w+)\) \{{ pc_sharedfile_handle_construct\(this, \1\); \}}'
        text,count=re.subn(pattern,lambda m:f'{kind}(unsigned int {m[1]}) {{words[0]={m[1]};words[1]=0;}}',text)
        assert count==1,kind
        old=f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old)==1;text=text.replace(old,f'~{kind}() {{}}',1)
    helper=r'''
static void pc_merry_init_files();
static void pc_merry_observe(const char *event,daTrsTrap_c *self) {
    const char *path=std::getenv("SM64DS_PC_MERRY_TRACE");if(!path||!*path)return;
    static FILE *file=nullptr;
    if(!file){file=std::fopen(path,"w");if(file)std::fputs("event,tick,uid,actor,x,y,z,index,state,timer,angle,speed,standing,visible,collider,model,alive,tilt,tilt_speed,area\n",file);}
    if(file){std::fprintf(file,"%s,%llu,%u,%u,%d,%d,%d,%d,%u,%u,%d,%d,%u,%u,%d,%u,%u,%d,%d,%d\n",event,
        static_cast<unsigned long long>(ntr::rt_frame()),unsigned(self->uniqueID),unsigned(self->actorID),
        self->mPosX,self->mPosY,self->mPosZ,self->mIndex,unsigned(self->mState),unsigned(self->mStateTimer),
        int(self->mAngleY),int(self->mAngVelY),self->mStandingActor?unsigned(self->mStandingActor->uniqueID):0,
        unsigned(self->mVisible),int(self->mMovingMeshCollider.IsEnabled()),
        unsigned(reinterpret_cast<std::uintptr_t>(self->mModel.data.modelFile)),unsigned(self->aliveState),
        int(self->mAngleX),int(self->mAngVelX),int(self->mAreaId));std::fflush(file);}
}
'''
    anchor='// @symbol _ZN11daTrsTrap_cD1Ev';assert text.count(anchor)==1
    text=text.replace(anchor,helper+'\n'+anchor,1)
    for signature,event in (('int daTrsTrap_c::InitResources()','init'),
                             ('int daTrsTrap_c::CleanupResources()','cleanup'),
                             ('daTrsTrap_c::~daTrsTrap_c()','destroy'),
                             ('int daTrsTrap_c::Behavior()','behavior')):
        anchor=signature+'\n{';assert text.count(anchor)==1,signature
        condition='if(ntr::rt_frame()%5==0 || mStandingActor)' if event=='behavior' else ''
        addition='\n    pc_merry_init_files();' if event=='init' else ''
        text=text.replace(anchor,anchor+addition+f'\n    {condition}pc_merry_observe("{event}",this);',1)
    anchor='int daTrsTrap_c::Render()\n{';assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    pc_render_probe::Scope probe(this,!mVisible,&mModel.mat4x3);',1)
    anchor='    return 1;\n}\n\n/* Source order is construction order:';assert text.count(anchor)==1
    text=text.replace(anchor,'    pc_merry_observe("ready",this);\n'+anchor,1)
    text+='''
// Native addresses in the original four-row ordering; no DS pointers escape.
extern "C" unsigned char data_ov063_0211ec58[],data_ov063_0211ebf8[],data_ov063_0211ec78[],data_ov063_0211ec18[];
SharedFilePtr *data_ov063_0211e27c[]={&data_ov063_0211eeb0,&data_ov063_0211eea8,&data_ov063_0211eeb8,&data_ov063_0211ee98};
SharedFilePtr *data_ov063_0211e28c[]={&data_ov063_0211eea0,&data_ov063_0211eed0,&data_ov063_0211eec0,&data_ov063_0211eec8};
CLPS_Block *data_ov063_0211e9e8[]={reinterpret_cast<CLPS_Block *>(data_ov063_0211ec58),reinterpret_cast<CLPS_Block *>(data_ov063_0211ebf8),reinterpret_cast<CLPS_Block *>(data_ov063_0211ec78),reinterpret_cast<CLPS_Block *>(data_ov063_0211ec18)};
static void pc_merry_init_files() {
    static bool ready=false;if(ready)return;
    data_ov063_0211eeb8.Construct(0x6bb);data_ov063_0211eeb0.Construct(0x6bd);
    data_ov063_0211ee98.Construct(0x6c1);data_ov063_0211eea8.Construct(0x6c3);
    data_ov063_0211eec0.Construct(0x6bc);data_ov063_0211eea0.Construct(0x6be);
    data_ov063_0211eec8.Construct(0x6c2);data_ov063_0211eed0.Construct(0x6c4);ready=true;
}
static_assert(sizeof(TrsTrapModelFilePtr)==8 && sizeof(TrsTrapCollisionFilePtr)==8,"native furniture handles");
static_assert(sizeof(daTrsTrap_c)==0x354,"native spinning-floor allocation");
static_assert(offsetof(daTrsTrap_c,mMovingMeshCollider)==0x15c,"native spinning-floor collider");
static_assert(offsetof(daTrsTrap_c,mClsnMat)==0x324,"native spinning-floor matrix");
'''
    return ('#include <cstddef>\n#include <cstdint>\n#include <cstdio>\n#include <cstdlib>\n'
            '#include "ntr/rt.h"\n#include "hal/actor_render_probe.h"\n'+text)
