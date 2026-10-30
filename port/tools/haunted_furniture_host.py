"""Host construction and deferred resource handles for original haunted furniture."""
import re

def transform(root, text, piano):
    kinds = ('PianoModelFilePtr','PianoCollisionFilePtr','PianoAnimationFilePtr') if piano else ('ChairModelFilePtr',)
    for kind in kinds:
        pattern=rf'{kind}\((?:unsigned int|u32) (\w+)\) \{{ pc_sharedfile_handle_construct\(this, \1\); \}}'
        text,n=re.subn(pattern,lambda m:f'{kind}(u32 {m[1]}) {{ words[0]={m[1]};words[1]=0; }}',text)
        assert n==1,(kind,n)
        old=f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old)==1,kind
        text=text.replace(old,f'~{kind}() {{}}')
    cls='daPiano_c' if piano else 'daChair_c'
    handles=[('data_ov063_0211ef80',0x40a),('data_ov063_0211ef88',0x40b),('data_ov063_0211ef90',0x40c)] if piano else [('data_ov020_02114af0',0x2d0)]
    helper='static void pc_furniture_files() { static bool ready=false;if(ready)return;\n'
    helper+='\n'.join(f'    {name}.Construct({asset});' for name,asset in handles)
    helper+='\n    ready=true;\n}\n'
    state='mStateIdx' if piano else 'mState'
    helper+='''static void pc_furniture_observe(%s *a) {
    const char *prefix=std::getenv("SM64DS_PC_BOO_TRACE");if(!prefix||ntr::rt_frame()%%5)return;
    static FILE *file=nullptr;if(!file){char path[2048];std::snprintf(path,sizeof(path),"%%s.%s.csv",prefix);file=std::fopen(path,"w");if(file)std::fputs("tick,uid,actor,state,x,y,z,area,alive\\n",file);}
    if(file){std::fprintf(file,"%%llu,%%u,%%u,%%d,%%d,%%d,%%d,%%d,%%u\\n",static_cast<unsigned long long>(ntr::rt_frame()),unsigned(a->uniqueID),unsigned(a->actorID),int(a->%s),a->mPosX,a->mPosY,a->mPosZ,int(a->mAreaId),unsigned(a->aliveState));std::fflush(file);}
}
''' % (cls,'piano' if piano else 'chair',state)
    anchor=f'// @symbol _ZN{len(cls)}{cls}13InitResourcesEv'
    assert text.count(anchor)==1,anchor
    text=text.replace(anchor,helper+anchor)
    anchor=f'{"int" if piano else "s32"} {cls}::InitResources()\n{{'
    assert text.count(anchor)==1,anchor
    text=text.replace(anchor,anchor+'\n    pc_furniture_files();')
    for method,addition in (
            ('Behavior','pc_furniture_observe(this);'),
            ('Render','pc_render_probe::Scope probe(this,false,&'+('mModelAnim' if piano else 'mModel')+'.mat4x3);')):
        anchor=f'{"int" if piano else "s32"} {cls}::{method}()\n{{'
        assert text.count(anchor)==1,anchor
        text=text.replace(anchor,anchor+'\n    '+addition)
    if piano:
        pattern=r'extern "C" daPiano_c \*daPiano_c_classInit\(\)\n\{.*?\n\}'
        text,n=re.subn(pattern,'extern "C" daPiano_c *daPiano_c_classInit()\n{\n    return new daPiano_c();\n}',text,flags=re.S)
        assert n==1,n
    else:
        text=text.replace('ST->counter','mStateTimer').replace('ST->timer','mActionTimer')
        anchor='void daChair_c::Break()\n{'
        assert text.count(anchor)==1
        text=text.replace(anchor,anchor+'\n    if(std::getenv("SM64DS_PC_BOO_TRACE")){std::fprintf(stderr,"[native chair break] tick=%llu uid=%u pos=%d,%d,%d\\n",static_cast<unsigned long long>(ntr::rt_frame()),unsigned(uniqueID),mPosX,mPosY,mPosZ);std::fflush(stderr);}')
    for kind in kinds:
        text+=f'\nstatic_assert(sizeof({kind})==8,"native furniture resource handle");\n'
    return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n#include "hal/actor_render_probe.h"\n'+text
