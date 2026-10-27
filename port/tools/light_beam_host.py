"""Native deferred model startup and opt-in observations for castle light37."""


def transform(text):
    old='C1HikariModelFilePtr(unsigned int id) { pc_sharedfile_handle_construct(this, id); }'
    assert text.count(old)==1
    text=text.replace(old,'C1HikariModelFilePtr(unsigned int id) { unk0=id;file=nullptr; }',1)
    old='~C1HikariModelFilePtr() { pc_sharedfile_handle_destroy(this); }'
    assert text.count(old)==1
    text=text.replace(old,'~C1HikariModelFilePtr() {}',1)
    helper=r'''
static void pc_hikari_observe(const char *event,daObjC1Hikari_c *self) {
    const char *path=std::getenv("SM64DS_PC_LIGHT_BEAM_TRACE");
    if(!path || !*path)return;
    static FILE *file=nullptr;
    if(!file) {file=std::fopen(path,"w");if(file)std::fputs("event,tick,uid,stars,activated,x,y,z,talk_player,sound_phase,sound_timer,hit,other,cylinder_x,cylinder_y,cylinder_z,model_file\n",file);}
    if(file) {
        std::fprintf(file,"%s,%llu,%u,%u,%u,%d,%d,%d,%u,%u,%u,%u,%u,%d,%d,%d,%u\n",event,
            static_cast<unsigned long long>(ntr::rt_frame()),unsigned(self->uniqueID),unsigned(NumStars()),
            unsigned((data_0209caa0[1]&0x80000000u)!=0),self->mPosX,self->mPosY,self->mPosZ,
            self->mTalkPlayer?unsigned(self->mTalkPlayer->uniqueID):0,unsigned(self->mSoundTimers[0]),
            unsigned(self->mSoundTimers[1]),unsigned(self->mCylinder.hitFlags),unsigned(self->mCylinder.otherOwner),
            self->mCylinder.pos.x,self->mCylinder.pos.y,self->mCylinder.pos.z,
            unsigned(reinterpret_cast<std::uintptr_t>(data_ov010_02112d50.file)));
        std::fflush(file);
    }
}
'''
    anchor='// @symbol daObjC1Hikari_c_classInit'
    assert text.count(anchor)==1
    text=text.replace(anchor,helper+'\n'+anchor,1)
    anchor='int daObjC1Hikari_c::InitResources()\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'''
    static bool ready=false;
    if(!ready) {data_ov010_02112d50.Construct(1453);ready=true;}
    pc_hikari_observe("init",this);
''',1)
    for condition,event in (('NumStars() < 0xe','stars-reject'),('data_0209caa0[1] & 0x80000000','activated-reject')):
        old=f'    if ({condition})\n        return 0;'
        assert text.count(old)==1
        text=text.replace(old,f'    if ({condition}) {{ pc_hikari_observe("{event}",this);return 0; }}',1)
    old='    UpdateModelTransform();\n    return 1;'
    assert text.count(old)==1
    text=text.replace(old,'    UpdateModelTransform();\n    pc_hikari_observe("ready",this);\n    return 1;',1)
    for method,event in (('Behavior','behavior'),('CleanupResources','cleanup')):
        anchor=f'int daObjC1Hikari_c::{method}()\n{{'
        assert text.count(anchor)==1
        observation='if(ntr::rt_frame()%5==0 || mCylinder.hitFlags)' if method=='Behavior' else ''
        text=text.replace(anchor,anchor+f'\n    {observation}pc_hikari_observe("{event}",this);',1)
    anchor='int daObjC1Hikari_c::Render()\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    pc_render_probe::Scope probe(this,0,&mModel.mat4x3);',1)
    text+='''
static_assert(sizeof(C1HikariModelFilePtr)==8,"native light-beam file handle");
static_assert(sizeof(daObjC1Hikari_c)==0x16c,"native castle light layout");
static_assert(offsetof(daObjC1Hikari_c,mModel)==0xd4,"native light model");
static_assert(offsetof(daObjC1Hikari_c,mCylinder)==0x124,"native light cylinder");
static_assert(offsetof(daObjC1Hikari_c,mTalkPlayer)==0x164,"native light talk pointer");
'''
    return '#include <cstddef>\n#include <cstdint>\n#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n#include "hal/actor_render_probe.h"\n'+text
