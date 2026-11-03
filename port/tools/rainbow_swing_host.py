"""Canonical host resources and optional observation of the original pendulum."""
import re


def transform(text):
    for kind in ('RcBurankoModelFilePtr', 'RcBurankoClsnFilePtr'):
        pattern = rf'{kind}\((?:unsigned int|u32) (\w+)\) \{{ pc_sharedfile_handle_construct\(this, \1\); \}}'
        text, count = re.subn(pattern, lambda m: f'{kind}(u32 {m[1]}) {{words[0]={m[1]};words[1]=0;}}', text)
        assert count == 1, kind
        old = f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old) == 1
        text = text.replace(old, f'~{kind}() {{}}')
    anchor = 'int daObjRcBuranko_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '\n    static bool ready=false;if(!ready){data_ov036_02114028.Construct(1685);data_ov036_02114020.Construct(1686);ready=true;}')
    anchor = 'int daObjRcBuranko_c::Behavior()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + r'''
    if(std::getenv("SM64DS_PC_DONUT_TRACE") && ntr::rt_frame()%5==0){
        static FILE *f=nullptr;
        if(!f){char path[2048];std::snprintf(path,sizeof(path),"%s.swings.csv",std::getenv("SM64DS_PC_DONUT_TRACE"));f=std::fopen(path,"w");}
        if(f){std::fprintf(f,"%llu,%u,%d,%d,%d,%d,%d,%d\n",static_cast<unsigned long long>(ntr::rt_frame()),unsigned(uniqueID),mPosX,mPosY,mPosZ,int(mAngleZ),int(mSwingVelZ),int(mMeshCollider.IsEnabled()));std::fflush(f);}
    }
''')
    anchor = 'int daObjRcBuranko_c::Render()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '\n    pc_render_probe::Scope probe(this,false,&mModel.mat4x3);')
    return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n#include "hal/actor_render_probe.h"\n' + text
