"""Native Toad resources, typed two-state callbacks and original profile."""
import re
import struct

def inventory(root):
    config=(root/'config/arm9/overlays/ov085/delinks.txt').read_text()
    base=int(re.search(r'\.text\s+start:0x([\da-f]+)',config)[1],16)
    image=(root/'extracted/overlays/overlay_0085.bin').read_bytes()
    row=struct.unpack_from('<IHHIiiii',image,0x0212fe6c-base)
    assert row[0]==0x02129cd0,'Toad factory changed'
    for address,target in ((0x0212fe40,0x021294f0),(0x0212fe48,0x02129470),
                           (0x0212fe30,0x0212943c),(0x0212fe38,0x021291ac)):
        assert struct.unpack_from('<II',image,address-base)==(target,0),'Toad callback changed'
    init=(root/'src/unnamed/ov085/__sinit_ov085_0212f2a8.c').read_text()
    for name,resource in (('data_ov085_02130480',0x35c),('data_ov085_02130488',0x35d),('data_ov085_02130490',0x35e)):
        assert re.search(r'\('+name+r', 0x'+format(resource,'x')+r'\)',init),'Toad resource changed'
    return row[1:]

def transform(root,text):
    profile=inventory(root)
    state='extern daKinopio_c::State data_ov085_0212fe88[];'
    assert text.count(state)==1,'Toad state declaration changed'
    text=text.replace(state,'extern "C" daKinopio_c::State data_ov085_0212fe88[];')
    talk='    if (mTalkPlayer->StartTalk(*this, 0) == 0)\n        return;'
    assert text.count(talk)==1,'Toad talk gate changed'
    text=text.replace(talk,'''    const int pc_started=mTalkPlayer->StartTalk(*this,0);
    if(std::getenv("SM64DS_PC_TOAD_TRACE"))std::fprintf(stderr,
        "[native Toad start-talk] tick=%llu id=%u result=%d speed=%d hit=%08x other=%u\\n",
        static_cast<unsigned long long>(ntr::rt_frame()),uniqueID,pc_started,mTalkPlayer->mHorzSpeed,mCollider.hitFlags,mCollider.otherOwner);
    if(!pc_started)return;''',1)
    for name in ('data_ov085_02130480','data_ov085_02130488','data_ov085_02130490'):
        old='extern SharedFilePtr '+name+';'
        assert text.count(old)==1,'Toad file declaration changed'
        text=text.replace(old,'extern PcToadFile '+name+';')
    for signature,event in (('int daKinopio_c::InitResources()','init'),
                            ('int daKinopio_c::CleanupResources()','cleanup'),
                            ('int daKinopio_c::Behavior()','behavior'),
                            ('int daKinopio_c::Render()','render')):
        anchor=signature+'\n{'
        assert text.count(anchor)==1,'Toad lifecycle changed: '+signature
        extra='    pc_toad_init_resources();\n' if event=='init' else ''
        if event=='behavior':extra='    pc_toad_capture_contact(this);\n'
        text=text.replace(anchor,anchor+'\n'+extra+'    pc_toad_trace("'+event+'",this);',1)
    prefix='''#include "common.h"
#include "SharedFilePtr.h"
#include "daKinopio_c.h"
#include "ntr/rt.h"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
struct PcToadFile : SharedFilePtr { unsigned words[2]; };
static_assert(sizeof(PcToadFile)==8,"Toad shared-file storage");
static_assert(sizeof(daKinopio_c)==0x210 && offsetof(daKinopio_c,mState)==0x1fc,"Toad actor layout");
static_assert(offsetof(daKinopio_c,mModelAnim)==0x108 && offsetof(daKinopio_c,mShadowModel)==0x16c,"Toad render layout");
extern "C" void pc_toad_init_resources();
struct PcToadContact {unsigned id=0,other=0,flags=0;uint64_t tick=0;};
static PcToadContact pc_toad_contacts[16];
static void pc_toad_capture_contact(const daKinopio_c *t) {
    if(!std::getenv("SM64DS_PC_TEST_TOAD_TALK"))return;
    auto &v=pc_toad_contacts[t->uniqueID%16];
    v.id=t->uniqueID;v.other=t->mCollider.otherOwner;v.flags=t->mCollider.hitFlags;v.tick=ntr::rt_frame();
}
extern "C" bool pc_toad_contact_recent(unsigned id,unsigned player) {
    const auto &v=pc_toad_contacts[id%16];
    return v.id==id && v.other==player && (v.flags&0x08000000u) && ntr::rt_frame()-v.tick<=8;
}
static void pc_toad_trace(const char *event,const daKinopio_c *t) {
    if(!std::getenv("SM64DS_PC_TOAD_TRACE"))return;
    if(event[0]!='i' && event[0]!='c' && ntr::rt_frame()%61)return;
    std::fprintf(stderr,"[native Toad] %s tick=%llu id=%u state=%d message=%u variant=%u star=%u opacity=%u pos=%d,%d,%d head=%d,%d target=%d,%d hit=%08x other=%u radius=%d height=%d\\n",
        event,static_cast<unsigned long long>(ntr::rt_frame()),t->uniqueID,t->mState,t->mMessageID,
        unsigned(t->mVariant),unsigned(t->mStarID),unsigned(t->mOpacity),t->mPosX,t->mPosY,t->mPosZ,
        t->mHeadYaw,t->mHeadPitch,t->mHeadYawTarget,t->mHeadPitchTarget,t->mCollider.hitFlags,t->mCollider.otherOwner,
        int(t->mCollider.radius),int(t->mCollider.height));
}
'''
    suffix='''
extern "C" {
PcToadFile data_ov085_02130480={},data_ov085_02130488={},data_ov085_02130490={};
daKinopio_c::State data_ov085_0212fe88[2]={
    {&daKinopio_c::St_Idle_Init,&daKinopio_c::St_Idle_Main,0},
    {&daKinopio_c::St_Talk_Init,&daKinopio_c::St_Talk_Main,0}
};
void pc_toad_init_resources() {
    static bool ready=false;if(ready)return;ready=true;
    data_ov085_02130480.Construct(0x35c);
    data_ov085_02130488.Construct(0x35d);
    data_ov085_02130490.Construct(0x35e);
}
struct PcToadProfile {void *factory;unsigned short behavior,render;unsigned flags;int offset,radius,distance,farDistance;};
PcToadProfile g_profile_KINOPIO={(void *)&daKinopio_c_classInit,
'''+','.join(str(v) for v in profile)+'''};
}
'''
    return prefix+text+suffix
