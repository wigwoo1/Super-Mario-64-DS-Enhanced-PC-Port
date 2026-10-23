"""Defer the original lava bridge's resource handles until native FS startup."""
import re


def instrument(text):
    marker = '    return 1;\n}\n\n// @symbol _ZN16daObjFl_London_c6RenderEv'
    assert text.count(marker) == 1
    trace = '''    if(const char *path=std::getenv("SM64DS_PC_LAVA_BRIDGE_TRACE")) {
        static FILE *file=nullptr;
        if(!file) {file=std::fopen(path,"w");if(file)std::fputs("tick,uid,x,y,z,tilt,cooldown,direction,collision,tx,ty,tz\\n",file);}
        if(file && ntr::rt_frame()%5==0) {
            std::fprintf(file,"%llu,%u,%d,%d,%d,%d,%u,%u,%u,%d,%d,%d\\n",
                static_cast<unsigned long long>(ntr::rt_frame()),unsigned(uniqueID),mPosX,mPosY,mPosZ,
                int(mAngleZ),unsigned(mCooldown),unsigned(mFlag),unsigned(mMeshCollider.IsEnabled()),
                mClsnMat.m[9],mClsnMat.m[10],mClsnMat.m[11]);std::fflush(file);
        }
    }
'''
    text=text.replace(marker,trace+marker)
    text=text.replace('s32 daObjFl_London_c::Render()\n{',
        's32 daObjFl_London_c::Render()\n{\n    pc_render_probe::Scope bridgeProbe(this,0,&mModel.mat4x3);')
    return '#include <cstdio>\n#include <cstdlib>\n#include "hal/actor_render_probe.h"\n'+text


def transform(text):
    for kind in ('LondonModelFile', 'LondonCollisionFilePtr'):
        pattern = rf'{kind}\(unsigned int (\w+)\) \{{ pc_sharedfile_handle_construct\(this, \1\); \}}'
        text, count = re.subn(pattern, lambda m: f'{kind}(unsigned int {m[1]}) {{ words[0]={m[1]};words[1]=0; }}', text)
        assert count == 1, kind + ' constructor changed'
        text = text.replace(f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}', f'~{kind}() {{}}')
    return instrument(text) + '''
static_assert(sizeof(daObjFl_London_c)==0x324,"native lava bridge tail padding");
extern "C" void pc_lava_bridge_init_resources() {
    static bool ready=false;if(ready)return;ready=true;
    data_ov022_02114580.Construct(0x5fd);
    data_ov022_02114578.Construct(0x5fe);
}
'''
