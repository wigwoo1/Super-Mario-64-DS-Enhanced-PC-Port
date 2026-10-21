"""Native castle-bird profile, typed state dispatch and recovered ARM hatch."""
import re,struct

def profile(root):
    config=(root/'config/arm9/overlays/ov009/delinks.txt').read_text()
    base=int(re.search(r'\.text\s+start:0x([0-9a-f]+)',config)[1],16)
    raw=(root/'extracted/overlays/overlay_0009.bin').read_bytes()
    row=struct.unpack_from('<IHHIiiii',raw,0x02113934-base)
    assert row[0]==0x02111a30,'Bird profile factory changed'
    return row[1:]

def transform(root,text,name):
    if name=='d_a_s_bird_head.cpp':return text
    if name!='d_a_s_bird.cpp':return text
    dispatch='    (this->*data_ov009_02113c48.slots[mState])();'
    assert text.count(dispatch)==1,'Bird state dispatch changed'
    text=text.replace(dispatch,'''    switch(mState) {
    case 0:func_ov009_021116ec();break;
    case 1:func_ov009_021115d8();break;
    case 2:func_ov009_0211145c(reinterpret_cast<char *>(this));break;
    case 3:func_ov009_02111234();break;
    default:return 0;
    }
    pc_bird_trace("state",this);''',1)
    text=text.replace('    mModelAnim.Render(0);','    mModelAnim.Render(0);\n    pc_bird_trace("render",this);',1)
    text=text.replace('        mState = zero;','        mState = zero;\n        pc_bird_trace("init",this);',1)
    text=text.replace('    data_ov009_02113c28.Release();','    pc_bird_trace("cleanup",this);\n    data_ov009_02113c28.Release();',1)
    prefix='''#include "daSBird_c.h"
#include "ntr/rt.h"
#include <cstdio>
#include <cstdlib>
#include <cstddef>
static void pc_bird_trace(const char *event,const daSBird_c *b) {
    if(!std::getenv("SM64DS_PC_BIRD_TRACE"))return;
    if(event[0]!='i' && event[0]!='c' && ntr::rt_frame()%61)return;
    std::fprintf(stderr,"[native bird] %s tick=%llu unique=%u leader=%u owner=%u state=%d pos=%d,%d,%d\\n",event,
        static_cast<unsigned long long>(ntr::rt_frame()),b->uniqueID,unsigned(b->mIsLeader),b->mOwnerID,b->mState,b->mPosX,b->mPosY,b->mPosZ);
}
static_assert(sizeof(daSBird_c)==0x184 && offsetof(daSBird_c,mState)==0x17c,"native bird layout");
static_assert(offsetof(daSBird_c,mModelAnim)==0xd4 && offsetof(daSBird_c,mPrevAngleX)==0x92,"native bird field offsets");
'''
    suffix='''
// Literal C equivalent of the recovered unenrolled state-2 ARM hatch.
extern "C" void func_ov009_0211145c(char *raw) {
    auto *b=reinterpret_cast<daSBird_c *>(raw);
    if(b->mIsLeader) {
        auto *player=reinterpret_cast<dActor_c *>(b->ClosestPlayer());
        if(!player)return;
        Vector3 diff;Vec3_Sub(&diff,&b->mPosX,&player->mPosX);
        if(Vec3_HorzLen(&diff)>0x7d0000)return;
        const int count=int(b->param1)&15;
        for(int i=0;i<count-1;++i) {
            auto *child=dActor_c::Spawn(343,0x10,*reinterpret_cast<Vector3 *>(&b->mPosX),nullptr,b->mAreaId,-1);
            if(child)static_cast<daSBird_c *>(child)->func_ov009_02111224(b->uniqueID);
        }
        b->mTargetPos.x=-0x14000;b->mTargetPos.z=static_cast<int>(0xff06a000u);
        func_0201267c(0x6a,&b->mCamSpacePosX);
    }
    b->mPrevAngleX=static_cast<short>(5000-RandomIntInternal(&data_0209e650)%4000);
    b->mPrevAngleY=static_cast<short>(RandomIntInternal(&data_0209e650)>>16);
    b->mFlySpeed=0x28000;b->mState=3;b->mFlags&=~0x10000u;
}
struct PcBirdProfile {void *factory;unsigned short behavior,render;unsigned flags;int offset,radius,distance,farDistance;};
extern "C" PcBirdProfile g_profile_SBIRD={(void *)&daSBird_c_classInit,
'''+','.join(str(v) for v in profile(root))+'};\n'
    return prefix+text+suffix
