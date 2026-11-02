"""Native resource initialization for the recovered Owl actor."""
import struct


def transform(root, text):
    raw = (root / 'extracted/overlays/overlay_0094.bin').read_bytes()
    assert struct.unpack_from('<IHHIiiii', raw, 0x02136a34-0x02135700) == (
        0x02136798, 234, 89, 0, 409600, 819200, 16777216, 16777216)
    # Native member pointers are compiler-owned; these state objects are
    # rebuilt by C++ below rather than copied from ARM descriptors.
    old = 'typedef char OwlState_size_must_be_0x10[sizeof(OwlState) == 0x10 ? 1 : -1];'
    assert text.count(old) == 1
    text = text.replace(old, 'static_assert(sizeof(OwlState)==sizeof(daOwl_c::State),"native Owl state layout");')
    old = 'if (*(int *)((char *)o + 8) != 0)'
    assert text.count(old) == 1
    text = text.replace(old, 'if (o->mMain != nullptr)')
    anchor = '    mCurrentState = state;'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '''
    if(std::getenv("SM64DS_PC_OWL_TRACE"))std::fprintf(stderr,"[native owl] state tick=%llu uid=%u state=%d talk=%u pos=%d,%d,%d\\n",
        static_cast<unsigned long long>(ntr::rt_frame()),uniqueID,
        state==OWL_STATE_DORMANT?0:state==OWL_STATE_HOVER?1:state==OWL_STATE_TALK?2:state==OWL_STATE_CARRY?3:4,
        unsigned(mTalkState),mPosX,mPosY,mPosZ);
''')
    anchor = '    mModelAnim.Render(0);'
    assert text.count(anchor) == 1
    text = text.replace(anchor, '    pc_render_probe::Scope probe(this,false,&mModelAnim.mat4x3);\n'+anchor)
    for kind in ('OwlModelFilePtr', 'daOwlAnimationFilePtr'):
        old = f'{kind}(unsigned int fileID) {{ pc_sharedfile_handle_construct(this, fileID); }}'
        assert text.count(old) == 1
        text = text.replace(old, f'{kind}(unsigned int fileID) {{ words[0]=fileID;words[1]=0; }}')
        old = f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old) == 1
        text = text.replace(old, f'~{kind}() {{}}')
    anchor = 'int daOwl_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '''
    static bool ready=false;
    if(!ready){
        data_ov094_02136ae0.Construct(0x3cb);
        data_ov094_02136af8.Construct(0x3cc);
        data_ov094_02136ae8.Construct(0x3cd);
        data_ov094_02136af0.Construct(0x3ce);
        ready=true;
    }
    std::fprintf(stderr,"[native owl] init tick=%llu uid=%u pos=%d,%d,%d\\n",
        static_cast<unsigned long long>(ntr::rt_frame()),uniqueID,mPosX,mPosY,mPosZ);
''')
    text += '\nstatic_assert(sizeof(daOwl_c)==0x3f8,"native Owl allocation");\n'
    anchor = 'int daOwl_c::CleanupResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '\n    if(std::getenv("SM64DS_PC_OWL_TRACE"))std::fprintf(stderr,"[native owl] cleanup tick=%llu uid=%u\\n",static_cast<unsigned long long>(ntr::rt_frame()),uniqueID);')
    return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n#include "hal/actor_render_probe.h"\n' + text
