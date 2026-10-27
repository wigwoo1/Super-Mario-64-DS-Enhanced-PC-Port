"""Native startup and read-only probes for the original question switch."""
import re


def transform(text):
    for kind in ('HatenaModelFilePtr', 'HatenaAnimationFilePtr', 'HatenaCollisionFilePtr'):
        pattern = rf'{kind}\(unsigned int (\w+)\) \{{ pc_sharedfile_handle_construct\(this, \1\); \}}'
        text, count = re.subn(pattern, lambda m:
            f'{kind}(unsigned int {m[1]}) {{ words[0]={m[1]};words[1]=0; }}', text)
        assert count == 1, kind+' constructor changed'
        old = f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old) == 1
        text = text.replace(old, f'~{kind}() {{}}', 1)
    for name in ('data_ov002_0210d8b4', 'data_ov002_0210d774'):
        old = f'extern CLPS_Block {name};'
        assert text.count(old) == 1
        text = text.replace(old, f'extern "C" CLPS_Block {name};', 1)
    old = 'extern u32 data_0209caa0[];'
    assert text.count(old) == 1
    text = text.replace(old, 'extern "C" u32 data_0209caa0[];', 1)
    anchor = 's32 daObjHatenaSwitch_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor+'''
    static bool ready=false;
    if(!ready) {
        data_ov002_0210dd60.Construct(1133);
        data_ov002_0210dd68.Construct(1132);
        data_ov002_0210dd58.Construct(1135);
        data_ov002_0210dd50.Construct(1134);
        ready=true;
    }
''', 1)
    anchor = '    mPressedThisFrame = 0;\n    return 1;'
    assert text.count(anchor) == 1
    trace = r'''
    if(const char *path=std::getenv("SM64DS_PC_HATENA_SWITCH_TRACE")) {
        static FILE *file=nullptr;
        if(!file) {file=std::fopen(path,"w");if(file)std::fputs("tick,uid,x,y,z,press_timer,contact,talking,player,saved,pressed_mesh,collision,animation,finished\n",file);}
        if(file && ntr::rt_frame()%5==0) {
            std::fprintf(file,"%llu,%u,%d,%d,%d,%u,%u,%u,%u,%u,%u,%u,%d,%u\n",
                static_cast<unsigned long long>(ntr::rt_frame()),unsigned(uniqueID),mPosX,mPosY,mPosZ,
                unsigned(mPressTimer),unsigned(mPressedThisFrame),unsigned(mTalking),
                mTalkingPlayer?unsigned(mTalkingPlayer->uniqueID):0,
                unsigned((data_0209caa0[1]&0x80000000u)!=0),
                unsigned(mActiveMeshCollider==&mMovingMeshCollider),unsigned(mActiveMeshCollider->IsEnabled()),
                mModelAnim.currFrame,unsigned(mModelAnim.Finished()));
            std::fflush(file);
        }
    }
'''
    text = text.replace(anchor, trace+anchor, 1)
    anchor = 's32 daObjHatenaSwitch_c::Render()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor+'\n    pc_render_probe::Scope probe(this,0,&mModelAnim.mat4x3);', 1)
    text += '''
static_assert(sizeof(HatenaModelFilePtr)==8,"native question switch file handle");
static_assert(offsetof(daObjHatenaSwitch_c,mActiveMeshCollider)==sizeof(dBgActor_c),"native question switch base boundary");
'''
    return ('#include <cstddef>\n#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n'
            '#include "hal/actor_render_probe.h"\n'+text)
