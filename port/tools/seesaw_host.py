"""Adapt the original Lethal Lava Land seesaw for native startup and tracing."""
import re


def transform(text):
    for kind in ('SeesawModelFile', 'SeesawCollisionFilePtr'):
        pattern = rf'{kind}\(unsigned int (\w+)\) \{{ pc_sharedfile_handle_construct\(this, \1\); \}}'
        text, count = re.subn(
            pattern,
            lambda match: f'{kind}(unsigned int {match[1]}) {{ words[0]={match[1]};words[1]=0; }}',
            text)
        assert count == 1, kind + ' constructor changed'
        old = f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old) == 1, kind + ' destructor changed'
        text = text.replace(old, f'~{kind}() {{}}', 1)

    behavior = 's32 daObjFl_Seesaw_c::Behavior()\n{'
    render = 's32 daObjFl_Seesaw_c::Render()\n{'
    assert text.count(behavior) == 1 and text.count(render) == 1
    trace = '''
    if(const char *path=std::getenv("SM64DS_PC_SEESAW_TRACE")) {
        static FILE *file=nullptr;
        if(!file) {file=std::fopen(path,"w");if(file)std::fputs("tick,uid,x,y,z,tilt,cooldown,step,collision,slot\\n",file);}
        if(file && ntr::rt_frame()%10==0) {
            std::fprintf(file,"%llu,%u,%d,%d,%d,%d,%u,%d,%u,%u\\n",
                static_cast<unsigned long long>(ntr::rt_frame()),unsigned(uniqueID),mPosX,mPosY,mPosZ,
                int(mAngleX),unsigned(mSwingCooldown),int(mSwingStep),unsigned(mMeshCollider.IsEnabled()),
                unsigned(mMeshCollider.slotIdx));
            std::fflush(file);
        }
    }
'''
    # Observe the completed original range check. A distant disabled collider
    # is intentional; do not force it on before IsClsnInRange runs.
    begin = text.index(behavior)
    end = text.index('/* Slot 0. */', begin)
    body = text[begin:end]
    assert body.count('    return 1;') == 1
    text = text[:begin] + body.replace('    return 1;', trace + '    return 1;', 1) + text[end:]
    text = text.replace(render, render + '\n    pc_render_probe::Scope seesawProbe(this,0,&mModel.mat4x3);', 1)
    text = '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n#include "hal/actor_render_probe.h"\n' + text
    text += '''
// Host base-class layout can include different tail padding than ARM9; the
// native factory allocates sizeof(daObjFl_Seesaw_c) through the host ABI.
extern "C" void pc_fl_seesaw_init_resources() {
    static bool ready=false;if(ready)return;ready=true;
    data_ov022_021145a8.Construct(0x610);
    data_ov022_021145a0.Construct(0x611);
}
'''
    return text
