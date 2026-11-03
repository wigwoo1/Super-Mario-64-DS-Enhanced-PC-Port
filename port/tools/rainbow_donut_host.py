"""Host resource initialization and read-only observation of original donut blocks."""
import re


def transform(text):
    # MSVC retains the base's tail padding; the embedded collision member and
    # following fields move. Preserve the original callback's intent using the
    # native field rather than writing into the home-position storage.
    old = '*(unsigned char *)((char *)self + 0x4e8) = 1;'
    assert text.count(old) == 1
    text = text.replace(old, 'static_cast<daObjRc_Tikuwa_c *>(self)->mHadClsn = 1;')
    for kind in ('TikuwaModelFilePtr', 'TikuwaCollisionFilePtr'):
        pattern = rf'{kind}\((?:unsigned int|u32) (\w+)\) \{{ pc_sharedfile_handle_construct\(this, \1\); \}}'
        text, count = re.subn(pattern, lambda m: f'{kind}(u32 {m[1]}) {{words[0]={m[1]};words[1]=0;}}', text)
        assert count == 1, kind
        old = f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old) == 1
        text = text.replace(old, f'~{kind}() {{}}')
    anchor = 's32 daObjRc_Tikuwa_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '\n    static bool ready=false;if(!ready){data_ov036_0211408c.Construct(0x6af);data_ov036_02114084.Construct(0x6b0);ready=true;}')
    anchor = 's32 daObjRc_Tikuwa_c::Behavior()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + r'''
    if(std::getenv("SM64DS_PC_DONUT_TRACE") && ntr::rt_frame()%5==0){
        static FILE *f=std::fopen(std::getenv("SM64DS_PC_DONUT_TRACE"),"w");
        if(f){std::fprintf(f,"%llu,%u,%d,%u,%u,%d,%d,%d,%d\n",static_cast<unsigned long long>(ntr::rt_frame()),unsigned(uniqueID),int(mState),unsigned(mHadClsn),unsigned(mClsnTimer),mPosX,mPosY,mPosZ,int(mMeshCollider.IsEnabled()));std::fflush(f);}
    }
''')
    anchor = 's32 daObjRc_Tikuwa_c::Render()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '\n    pc_render_probe::Scope probe(this,false,&mModel.mat4x3);')
    text += '\nstatic_assert(offsetof(daObjRc_Tikuwa_c,mHadClsn)==0x4ec,"native donut contact field");\n'
    return '#include <cstddef>\n#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n#include "hal/actor_render_probe.h"\n' + text
