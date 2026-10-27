"""Native sound-handle initialization and read-only SET_SE observations."""


def transform(text):
    old='namespace Sound {\nu32 PlayLong(u32 handle, u32 bank, u32 soundId, const Vector3 &pos, s16 pitch);\n}'
    assert text.count(old)==1
    text=text.replace(old,'#include "Sound.h"',1)
    # Sound's native fast path bypasses its retail distance eligibility test.
    # Retain that original test for these positional emitters before refreshing
    # a native long-sound handle; unrefreshed voices expire normally.
    old='    mSoundHandle = Sound::PlayLong(mSoundHandle, 3,'
    assert text.count(old)==1
    text=text.replace(old,'''    if(!func_02048a1c(reinterpret_cast<int *>(&mCamSpacePosX),3,data_ov002_0210b498[param1])) {
        if(ntr::rt_frame()%31==0)pc_ambient_observe("range-reject",this,mSoundHandle);
        return 1;
    }
    mSoundHandle = Sound::PlayLong(mSoundHandle, 3,''',1)
    helper=r'''
extern "C" int func_02048a1c(int *,int,int);
static void pc_ambient_observe(const char *event,daSetSE_c *self,unsigned handle=0) {
    const char *path=std::getenv("SM64DS_PC_AMBIENT_TRACE");
    if(!path || !*path)return;
    static FILE *file=nullptr;
    if(!file) {file=std::fopen(path,"w");if(file)std::fputs("event,tick,uid,level,param,sound,disabled,handle,active,x,y,z,cam_x,cam_y,cam_z,underwater,music\n",file);}
    if(file) {
        const auto effect=self->param1&15u;
        const int sound=effect<11?data_ov002_0210b498[effect]:-1;
        std::fprintf(file,"%s,%llu,%u,%d,%u,%d,%d,%u,%u,%d,%d,%d,%d,%d,%d,%u,%d\n",event,
            static_cast<unsigned long long>(ntr::rt_frame()),unsigned(self->uniqueID),int(data_0209f2f8),
            unsigned(effect),sound,data_ov002_02110aec,handle,unsigned(ntr::audio_active(handle)),
            self->mPosX,self->mPosY,self->mPosZ,self->mCamSpacePosX,self->mCamSpacePosY,self->mCamSpacePosZ,
            unsigned(data_0209f318 && data_0209f318->IsUnderwater()),data_0209b4ac);
        std::fflush(file);
    }
}
'''
    anchor='// @symbol daSetSE_c_classInit'
    assert text.count(anchor)==1
    text=text.replace(anchor,helper+'\n'+anchor,1)
    anchor='int daSetSE_c::InitResources()\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    mSoundHandle=0;\n    pc_ambient_observe("init",this);',1)
    old='    data_ov002_02110aec = disabled;\n    return 1;'
    assert text.count(old)==1
    text=text.replace(old,'    data_ov002_02110aec = disabled;\n    pc_ambient_observe("ready",this);\n    return 1;',1)
    old='    if (data_ov002_02110aec != 0)\n        return 1;'
    assert text.count(old)==1
    text=text.replace(old,'    if (data_ov002_02110aec != 0) {\n        if(ntr::rt_frame()%31==0)pc_ambient_observe("disabled",this);\n        return 1;\n    }',1)
    old='''            cam->IsUnderwater())
            return 1;'''
    assert text.count(old)==1
    text=text.replace(old,'''            cam->IsUnderwater()) {
            if(ntr::rt_frame()%31==0)pc_ambient_observe("underwater-suppressed",this,mSoundHandle);
            return 1;
        }''',1)
    old='''        (u32)data_ov002_0210b498[param1], *(Vector3 *)&mCamSpacePosX, 0);
    return 1;'''
    assert text.count(old)==1
    text=text.replace(old,'''        (u32)data_ov002_0210b498[param1], *(Vector3 *)&mCamSpacePosX, 0);
    if(ntr::rt_frame()%5==0)pc_ambient_observe("sound",this,mSoundHandle);
    return 1;''',1)
    for method in ('CleanupResources','OnPendingDestroy'):
        anchor=('int' if method=='CleanupResources' else 'void')+' daSetSE_c::'+method+'()\n{'
        assert text.count(anchor)==1
        text=text.replace(anchor,anchor+'\n    pc_ambient_observe("'+method+'",this,mSoundHandle);',1)
    text+='''
static_assert(sizeof(daSetSE_c)==0xd8,"native ambient emitter layout");
static_assert(offsetof(daSetSE_c,mSoundHandle)==0xd4,"native ambient sound handle");
'''
    return '#include <cstddef>\n#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n#include "ntr/audio.h"\n'+text
