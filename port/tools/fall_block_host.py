"""Native startup and read-only probes for the original TH_DOWN_B platform."""
import re
import struct


def transform(root, text, leaf=False):
    if leaf:
        config = (root / 'config/arm9/overlays/ov063/delinks.txt').read_text()
        base = int(re.search(r'\.text\s+start:0x([\da-f]+)', config)[1], 16)
        raw = (root / 'extracted/overlays/overlay_0063.bin').read_bytes()
        assert struct.unpack_from('<3I', raw, 0x0211eb10-base) == (0x0211ef60, 0x0211ef58, 0x0211ec98)
        assert struct.unpack_from('<IHHIiiii', raw, 0x0211eb34-base) == (
            0x0211d47c, 66, 209, 2, 0, 0x80000, 0x1000000, 0)
        for name in ('ThFallBlockModelFilePtr', 'ThFallBlockCollisionFilePtr'):
            pattern = rf'{name}\(unsigned int (\w+)\) \{{ pc_sharedfile_handle_construct\(this, \1\); \}}'
            text, count = re.subn(pattern, lambda m: f'{name}(unsigned int {m[1]}) {{ words[0]={m[1]};words[1]=0; }}', text)
            assert count == 1, name
            old = f'~{name}() {{ pc_sharedfile_handle_destroy(this); }}'
            assert text.count(old) == 1
            text = text.replace(old, f'~{name}() {{}}', 1)
        anchor = 's32 daObjTh_Fall_Block_c::InitResources()\n{'
        assert text.count(anchor) == 1
        text = text.replace(anchor, anchor + '\n    pc_th_fall_init_files();', 1)
        # These are the same original eight-byte handles; defer their runtime
        # registration until the native asset system has started.
        anchor = '// @symbol daObjTh_Fall_Block_c_classInit'
        assert text.count(anchor) == 1
        text = text.replace(anchor, 'static void pc_th_fall_init_files();\n\n' + anchor, 1)
        text += '''
// The original descriptor contains three DS addresses. Bind all three lanes
// to canonical native storage rather than dereferencing the copied DS words.
extern "C" unsigned char data_ov063_0211ec98[];
extern "C" ResourceDescriptor data_ov063_0211eb10={
    &data_ov063_0211ef60,&data_ov063_0211ef58,
    reinterpret_cast<CLPS_Block *>(data_ov063_0211ec98)
};
static void pc_th_fall_init_files() {
    static bool ready=false;
    if(ready)return;
    data_ov063_0211ef60.Construct(1721);
    data_ov063_0211ef58.Construct(1722);
    ready=true;
}
static_assert(sizeof(ThFallBlockModelFilePtr)==8 && sizeof(ThFallBlockCollisionFilePtr)==8,"native falling-block file handles");
static_assert(sizeof(daObjTh_Fall_Block_c)==0x34c,"native TH_DOWN_B allocation");
'''
        return text

    helper = r'''
static void pc_fall_block_observe(const char *event,daObjFallBlock_c *block) {
    const char *path=std::getenv("SM64DS_PC_FALL_BLOCK_TRACE");if(!path||!*path)return;
    static FILE *file=nullptr;
    if(!file){file=std::fopen(path,"w");if(file)std::fputs("event,tick,uid,actor,x,y,z,rest_x,rest_y,rest_z,kill_y,state,timer,requested,delay,ready,grouped,previous,next,collider,vertical_speed,angle_x,angle_z,model,alive\n",file);}
    if(file){std::fprintf(file,"%s,%llu,%u,%u,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%d,%d,%u,%u\n",event,
        static_cast<unsigned long long>(ntr::rt_frame()),unsigned(block->uniqueID),unsigned(block->actorID),
        block->mPosX,block->mPosY,block->mPosZ,block->mRestPos.x,block->mRestPos.y,block->mRestPos.z,block->mKillY,
        unsigned(block->mState),unsigned(block->mStateTimer),unsigned(block->mShakeRequested),unsigned(block->mRespawnDelay),
        unsigned(block->mReady),unsigned(block->unk_341),unsigned(reinterpret_cast<std::uintptr_t>(block->mPrevInGroup)),
        unsigned(reinterpret_cast<std::uintptr_t>(block->mNextInGroup)),int(block->mMeshCollider.IsEnabled()),
        block->mVertSpeed,int(block->mAngleX),int(block->mAngleZ),
        unsigned(reinterpret_cast<std::uintptr_t>(block->mModel.data.modelFile)),unsigned(block->aliveState));std::fflush(file);}
}
'''
    anchor = '// @symbol _ZN16daObjFallBlock_cD0Ev'
    assert text.count(anchor) == 1
    text = text.replace(anchor, helper + '\n' + anchor, 1)
    for signature, event in (
        ('void daObjFallBlock_c::Kill()', 'kill'),
        ('int func_ov098_0213a2cc(daObjFallBlock_c *t, ResourceDescriptor *files)', 'cleanup'),
        ('int daObjFallBlock_c_RequestShake(daObjFallBlock_c *block, void *unused)', 'stand')):
        anchor = signature + '\n{'
        assert text.count(anchor) == 1, signature
        instance = 'this' if event == 'kill' else 't' if event == 'cleanup' else 'block'
        text = text.replace(anchor, anchor + f'\n    pc_fall_block_observe("{event}",{instance});', 1)
    anchor = 's32 daObjFallBlock_c::Behavior()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '\n    if(ntr::rt_frame()%5==0)pc_fall_block_observe("behavior",this);', 1)
    anchor = 's32 daObjFallBlock_c::Render()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '\n    pc_render_probe::Scope probe(this,0,&mModel.mat4x3);', 1)
    anchor = '    return 1;\n}\n}\n\n// @symbol daObjFallBlock_c_RequestShake'
    assert text.count(anchor) == 1
    text = text.replace(anchor, '    pc_fall_block_observe("ready",self);\n' + anchor, 1)
    text += '''
static_assert(sizeof(daObjFallBlock_c)==0x34c,"native falling-block layout");
static_assert(offsetof(daObjFallBlock_c,mRestPos)==0x320,"native falling-block rest position");
static_assert(offsetof(daObjFallBlock_c,mState)==0x33c,"native falling-block state");
static_assert(offsetof(daObjFallBlock_c,mPrevInGroup)==0x344,"native falling-block group links");
'''
    return ('#include <cstddef>\n#include <cstdint>\n#include <cstdio>\n#include <cstdlib>\n'
            '#include "ntr/rt.h"\n#include "hal/actor_render_probe.h"\n' + text)
