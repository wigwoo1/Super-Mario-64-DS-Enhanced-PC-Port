"""Native resources and state callbacks for the recovered vertical lift."""
import re
import struct


def transform(root, text):
    old='void ApproachLinear(int &value, int target, int step);'
    assert text.count(old)==1
    text=text.replace(old,'int ApproachLinear(int &value, int target, int step);')
    cfg=(root/'config/arm9/overlays/ov095/delinks.txt').read_text()
    base=int(re.search(r'\.text\s+start:0x([\da-f]+)',cfg)[1],16)
    raw=(root/'extracted/overlays/overlay_0095.bin').read_bytes()
    for address,expected in (
            (0x02136f68,(0x02137898,0x021378a8,0x021378c0)),
            (0x02136f74,(0x021378b0,0x021378b8,0x021378a0)),
            (0x021375a4,(0x0211ec38,0x02113aa0,0x02112aa8))):
        assert struct.unpack_from('<3I',raw,address-base)==expected
    for address,target in ((0x02137594,0x02136368),(0x0213757c,0x02136298),
                           (0x02137584,0x02136178),(0x0213758c,0x02136090),
                           (0x0213759c,0x02136104)):
        assert struct.unpack_from('<2I',raw,address-base)==(target,0)
    for kind in ('UdliftModelFile','UdliftCollisionFilePtr'):
        old=f'{kind}(unsigned int fileID) {{ pc_sharedfile_handle_construct(this, fileID); }}'
        assert text.count(old)==1,kind
        text=text.replace(old,f'{kind}(unsigned int fileID) {{words[0]=fileID;words[1]=0;}}')
        old=f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old)==1
        text=text.replace(old,f'~{kind}() {{}}')
    states=('StateWait','StateMoveUp','StateMoveDown','StateStop','StateStopAtBottom')
    pattern=r'State data_ov095_02137910\[6\] = \{.*?\n\};'
    replacement='State data_ov095_02137910[6] = {\n'+''.join(f'    &daUdlift_c::{name},\n' for name in states)+'    nullptr\n};'
    text,count=re.subn(pattern,replacement,text,flags=re.S)
    assert count==1
    helper=r'''
static void pc_udlift_init_files() {
    static bool ready=false;if(ready)return;
    data_ov095_02137898.Construct(0x6bf);data_ov095_021378a8.Construct(0x5db);data_ov095_021378c0.Construct(0x6ab);
    data_ov095_021378b0.Construct(0x6c0);data_ov095_021378b8.Construct(0x5dc);data_ov095_021378a0.Construct(0x6ac);
    ready=true;
}
static void pc_udlift_observe(const char *event,daUdlift_c *a) {
    const char *path=std::getenv("SM64DS_PC_UDLIFT_TRACE");if(!path||!*path)return;
    static FILE *file=nullptr;
    if(!file){file=std::fopen(path,"w");if(file)std::fputs("event,tick,uid,actor,variant,state,x,y,z,top,bottom,middle,speed,rider,ridden,armed,at_bottom,collider,model,area,alive\n",file);}
    if(file){std::fprintf(file,"%s,%llu,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u,%d,%u,%d,%u\n",event,
        static_cast<unsigned long long>(ntr::rt_frame()),unsigned(a->uniqueID),unsigned(a->actorID),a->mVariant,a->mState,
        a->mPosX,a->mPosY,a->mPosZ,a->mTopY,a->mBottomY,a->mMiddleY,a->mVertSpeed,
        a->mRider?unsigned(a->mRider->uniqueID):0u,unsigned(a->mIsRidden),unsigned(a->mIsArmed),unsigned(a->mIsAtBottom),
        int(a->mMeshCollider.IsEnabled()),unsigned(reinterpret_cast<std::uintptr_t>(a->mModel.data.modelFile)),int(a->mAreaId),unsigned(a->aliveState));std::fflush(file);}
}
'''
    anchor='// @symbol daUdlift_c_classInit_UDLIFT_TERESA'
    assert text.count(anchor)==1
    text=text.replace(anchor,helper+anchor)
    anchor='int daUdlift_c::InitResources()\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    pc_udlift_init_files();')
    anchor='    mSoundHandle = 0;\n    return 1;'
    assert text.count(anchor)==1
    text=text.replace(anchor,'    mSoundHandle = 0;\n    pc_udlift_observe("ready",this);\n    return 1;')
    anchor='int daUdlift_c::Behavior()\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    if(ntr::rt_frame()%5==0 || mIsRidden)pc_udlift_observe("behavior",this);')
    anchor='int daUdlift_c::CleanupResources()\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    pc_udlift_observe("cleanup",this);')
    anchor='int daUdlift_c::Render()\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    pc_render_probe::Scope probe(this,false,&mModel.mat4x3);')
    text+='''
extern "C" unsigned char data_ov063_0211ec38[],data_ov021_02113aa0[],data_ov036_02112aa8[];
SharedFilePtr *data_ov095_02136f68[]={&data_ov095_02137898,&data_ov095_021378a8,&data_ov095_021378c0};
SharedFilePtr *data_ov095_02136f74[]={&data_ov095_021378b0,&data_ov095_021378b8,&data_ov095_021378a0};
CLPS_Block *data_ov095_021375a4[]={reinterpret_cast<CLPS_Block *>(data_ov063_0211ec38),reinterpret_cast<CLPS_Block *>(data_ov021_02113aa0),reinterpret_cast<CLPS_Block *>(data_ov036_02112aa8)};
static_assert(sizeof(UdliftModelFile)==8 && sizeof(UdliftCollisionFilePtr)==8,"native lift resource handles");
static_assert(sizeof(State)==4,"native lift state stride");
static_assert(sizeof(daUdlift_c)==0x34c,"native lift allocation");
static_assert(offsetof(daUdlift_c,mRider)==0x320,"native lift derived fields");
'''
    return '#include <cstddef>\n#include <cstdint>\n#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n#include "hal/actor_render_probe.h"\n'+text
