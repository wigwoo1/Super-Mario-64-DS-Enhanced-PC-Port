"""Native callbacks and deferred handles for the original haunted casket."""
import re

def transform(root,text):
    for kind in ('CasketModelFilePtr','CasketCollisionFilePtr'):
        pattern=rf'{kind}\((?:unsigned int|u32) (\w+)\) \{{ pc_sharedfile_handle_construct\(this, \1\); \}}'
        text,n=re.subn(pattern,lambda m:f'{kind}(u32 {m[1]}) {{words[0]={m[1]};words[1]=0;}}',text);assert n==1
        old=f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}';assert text.count(old)==1;text=text.replace(old,f'~{kind}() {{}}')
    text,n=re.subn(r'CasketState data_ov071_02122ecc\[\] = \{.*?\n\};','CasketState data_ov071_02122ecc[] = {\n{&daObjCasket_c::St_Wait_Init,&daObjCasket_c::St_Wait_Main,"WAIT"},\n{&daObjCasket_c::St_StandUp_Init,&daObjCasket_c::St_StandUp_Main,"STANDUP"}\n};',text,flags=re.S);assert n==1
    anchor='int daObjCasket_c::InitResources()\n{';assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    static bool files=false;if(!files){data_ov071_021230d0.Construct(0x5b4);data_ov071_021230d8.Construct(0x5b5);files=true;}')
    anchor='int daObjCasket_c::Render()\n{';assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    pc_render_probe::Scope probe(this,false,&mModel.mat4x3);')
    helper=r'''
static void pc_casket_observe(daObjCasket_c *a) {
 const char *prefix=std::getenv("SM64DS_PC_BOO_TRACE");if(!prefix||ntr::rt_frame()%5)return;
 static FILE *file=nullptr;
 if(!file){char path[2048];std::snprintf(path,sizeof(path),"%s.caskets.csv",prefix);file=std::fopen(path,"w");if(file)std::fputs("tick,uid,param,state,timer,angle_x,angle_y,angle_z,x,y,z,collider,area,alive\n",file);}
 if(file){std::fprintf(file,"%llu,%u,%u,%d,%u,%d,%d,%d,%d,%d,%d,%d,%d,%u\n",static_cast<unsigned long long>(ntr::rt_frame()),unsigned(a->uniqueID),unsigned(a->param1),a->mState,unsigned(a->mStateTimer),int(a->mAngleX),int(a->mAngleY),int(a->mAngleZ),a->mPosX,a->mPosY,a->mPosZ,int(a->mMeshCollider.IsEnabled()),int(a->mAreaId),unsigned(a->aliveState));std::fflush(file);}
}
'''
    anchor='// @symbol daObjCasket_c_classInit';assert text.count(anchor)==1
    text=text.replace(anchor,helper+anchor)
    anchor='int daObjCasket_c::Behavior()\n{';assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    pc_casket_observe(this);')
    text+='\nstatic_assert(sizeof(daObjCasket_c)==0x32c,"native casket allocation");\nstatic_assert(offsetof(daObjCasket_c,mState)==0x320,"native casket state");\nstatic_assert(sizeof(CasketState)==12,"native casket state row");\nstatic_assert(sizeof(CasketModelFilePtr)==8 && sizeof(CasketCollisionFilePtr)==8,"native casket handles");\n'
    return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n#include <cstddef>\n#include "hal/actor_render_probe.h"\n'+text
