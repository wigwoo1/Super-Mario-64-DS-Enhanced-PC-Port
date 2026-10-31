"""Native resource and dispatch adaptation for the recovered iron-ball actor."""
import re
import struct


def transform(root,text):
    raw=(root/'extracted/overlays/overlay_0100.bin').read_bytes()
    base=0x02140d80
    assert struct.unpack_from('<IHHIiiii',raw,0x02147f58-base)==(0x0214316c,220,69,2,409600,8192000,25165824,25165824)
    for address,fn in zip((0x02147f38,0x02147f18,0x02147f20,0x02147f30,0x02147f28),
                          (0x02142b90,0x02142918,0x021424c0,0x0214272c,0x021424c0)):
        assert struct.unpack_from('<2I',raw,address-base)==(fn,0)
    text=text.replace('extern int data_02092138;','extern \"C\" int data_02092138;')
    old='extern char data_ov100_02148668;'
    assert text.count(old)==1
    text=text.replace(old,'extern "C" unsigned data_ov100_02148668[2]={0,0};')
    # The original table contains ARM ABI member descriptors. Native calls
    # retain each original variant's handler without casting its calling ABI.
    text,n=re.subn(r'int daIbl_c::Behavior\(\)\n\{.*?\n\}',r'''int daIbl_c::Behavior()
{
    pc_ironball_observe("behavior",this);
    if(mdCcAc_c.hitFlags && mdCcAc_c.otherOwner)pc_ironball_observe("contact",this);
    switch(mVariant){
    case 0:func_ov100_02142b90();break;
    case 1:func_ov100_02142918();break;
    case 2:case 4:func_ov100_021424c0();break;
    case 3:func_ov100_0214272c();break;
    }
    return 1;
}''',text,flags=re.S)
    assert n==1
    text,n=re.subn(r'int daIbl_c::Render\(\)\n\{.*?\n\}',r'''int daIbl_c::Render()
{
    pc_render_probe::Scope probe(this,mVariant==0,&mModel.mat4x3);
    if(mVariant)mModel.Render(reinterpret_cast<Vector3 *>(&mDrawScaleX));
    return 1;
}''',text,flags=re.S)
    assert n==1
    helper=r'''
static void pc_ironball_observe(const char *event,daIbl_c *a){
 const char *path=std::getenv("SM64DS_PC_IRONBALL_TRACE");if(!path||!*path)return;
 if(event[0]=='b' && ntr::rt_frame()%5)return;
 static FILE *file=nullptr;
 if(!file){file=std::fopen(path,"w");if(file)std::fputs("event,tick,level,uid,variant,param,x,y,z,speed,vertical,node,nodes,dispenser,children,timer,hit_wall,death,area,alive,hit_flags,other_owner,other_actor_id\n",file);}
 if(file){auto *other=a->mdCcAc_c.otherOwner?dActor_c::FindWithID(a->mdCcAc_c.otherOwner):nullptr;std::fprintf(file,"%s,%llu,%d,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%u,%u,%d,%u,%d,%d,%u,%u,%u,%u\n",event,static_cast<unsigned long long>(ntr::rt_frame()),int(data_0209f2f8),unsigned(a->uniqueID),unsigned(a->mVariant),unsigned(a->param1),a->mPosX,a->mPosY,a->mPosZ,a->mHorzSpeed,a->mVertSpeed,a->mPathNodeIndex,a->mNumPathNodes,a->mDispenser?unsigned(a->mDispenser->uniqueID):0u,unsigned(a->mLiveBalls),int(a->mStateTimer),unsigned(a->mHitWall),int(a->mDeathState),int(a->mAreaId),unsigned(a->aliveState),unsigned(a->mdCcAc_c.hitFlags),unsigned(a->mdCcAc_c.otherOwner),other?unsigned(other->actorID):0u);std::fflush(file);}
}
'''
    anchor='// @symbol _ZN7daIbl_cD1Ev'
    assert text.count(anchor)==1
    text=text.replace(anchor,helper+anchor)
    anchor='int daIbl_c::InitResources()\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    static bool ready=false;if(!ready){reinterpret_cast<SharedFilePtr *>(data_ov100_02148668)->Construct(0x9c00);ready=true;}')
    anchor='    mLiveBalls = 0;\n    return 1;'
    assert text.count(anchor)==1
    text=text.replace(anchor,'    mLiveBalls = 0;\n    pc_ironball_observe("ready",this);\n    return 1;')
    anchor='int daIbl_c::CleanupResources()\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    pc_ironball_observe("cleanup",this);')
    text+='\nstatic_assert(sizeof(daIbl_c)==0x3fc,"native iron-ball allocation");\nstatic_assert(offsetof(daIbl_c,mVariant)==0x3d0,"native iron-ball variant");\n'
    return '#include <cstddef>\n#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n#include "hal/actor_render_probe.h"\n'+text
