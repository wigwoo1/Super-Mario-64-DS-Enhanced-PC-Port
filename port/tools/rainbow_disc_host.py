"""Native canonical resources for Rainbow Ride's rotating discs."""
import re
import struct


def transform(root, text, name):
    if name == 'daObjKaitendai_c.cpp':
        helper = r'''
static void pc_disc_observe(daObjKaitendai_c *a){
 const char *prefix=std::getenv("SM64DS_PC_UDLIFT_TRACE");if(!prefix||ntr::rt_frame()%5)return;
 static FILE *file=nullptr;
 if(!file){char path[2048];std::snprintf(path,sizeof(path),"%s.discs.csv",prefix);file=std::fopen(path,"w");if(file)std::fputs("tick,uid,actor,x,y,z,angle,step,collider,area,alive\n",file);}
 if(file){std::fprintf(file,"%llu,%u,%u,%d,%d,%d,%d,%d,%d,%d,%u\n",static_cast<unsigned long long>(ntr::rt_frame()),unsigned(a->uniqueID),unsigned(a->actorID),a->mPosX,a->mPosY,a->mPosZ,int(a->mAngleY),int(a->mPrevAngleZ),int(a->mMeshCollider.IsEnabled()),int(a->mAreaId),unsigned(a->aliveState));std::fflush(file);}
}
'''
        anchor='// @symbol _ZN16daObjKaitendai_cD0Ev'
        assert text.count(anchor)==1
        text=text.replace(anchor,helper+anchor)
        anchor='s32 daObjKaitendai_c::Behavior()\n{'
        assert text.count(anchor)==1
        text=text.replace(anchor,anchor+'\n    pc_disc_observe(this);')
        anchor='s32 daObjKaitendai_c::Render()\n{'
        assert text.count(anchor)==1
        text=text.replace(anchor,anchor+'\n    pc_render_probe::Scope probe(this,false,&mModel.mat4x3);')
        return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n#include "hal/actor_render_probe.h"\n'+text
    raw=(root/'extracted/overlays/overlay_0036.bin').read_bytes()
    base=0x021111a0
    assert struct.unpack_from('<IHHIiiii',raw,0x02113b50-base)==(0x02111544,129,259,2,409600,4096000,29491200,6553600)
    assert struct.unpack_from('<3I',raw,0x02113b2c-base)==(0x02114050,0x02114048,0x02112bc8)
    for kind in ('RcKaitendaiModelFile','RcKaitendaiCollisionFilePtr'):
        pattern=rf'{kind}\((?:unsigned int|u32) (\w+)\) \{{ pc_sharedfile_handle_construct\(this, \1\); \}}'
        text,n=re.subn(pattern,lambda m:f'{kind}(u32 {m[1]}) {{words[0]={m[1]};words[1]=0;}}',text)
        assert n==1,kind
        old=f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old)==1
        text=text.replace(old,f'~{kind}() {{}}')
    anchor='// @symbol daObjRc_Kaitendai_c_classInit'
    assert text.count(anchor)==1
    text=text.replace(anchor,'extern "C" RcKaitendaiModelFile data_ov036_02114050;\nextern "C" RcKaitendaiCollisionFilePtr data_ov036_02114048;\n'+anchor)
    anchor='s32 daObjRc_Kaitendai_c::InitResources()\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    static bool ready=false;if(!ready){data_ov036_02114050.Construct(0x6a7);data_ov036_02114048.Construct(0x6a8);ready=true;}')
    text+='''
extern "C" unsigned char data_ov036_02112bc8[];
extern "C" ResourceDescriptor data_ov036_02113b2c={&data_ov036_02114050,&data_ov036_02114048,reinterpret_cast<CLPS_Block *>(data_ov036_02112bc8)};
static_assert(sizeof(daObjRc_Kaitendai_c)==0x320,"native rotating-disc allocation");
'''
    return text
