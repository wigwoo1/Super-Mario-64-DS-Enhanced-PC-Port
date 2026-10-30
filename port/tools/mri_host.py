"""Native state callbacks and deferred resources for the recovered Mr. I."""
def transform(root, text, beam=False):
    if beam:
        # This scalar proxy aliases Vector3's empty destructor in the ROM.
        assert text.count('~EyBmVec3();')==1
        text=text.replace('~EyBmVec3();','~EyBmVec3() {}')
        for method in ('Behavior','SpawnDestroyEffect'):
            anchor=('int' if method=='Behavior' else 'void')+' daEyBm_c::'+method+'()\n{'
            assert text.count(anchor)==1
            text=text.replace(anchor,anchor+'\n    if(std::getenv("SM64DS_PC_BOO_TRACE"))std::fprintf(stderr,"[native eye beam] event='+method+' tick=%llu uid=%u life=%u pos=%d,%d,%d\\n",static_cast<unsigned long long>(ntr::rt_frame()),unsigned(uniqueID),unsigned(mLifeTimer),mPosX,mPosY,mPosZ);')
        anchor='    Player *player = (Player *)found;'
        assert text.count(anchor)==1
        # Log only the real hurt call, after native collision/vanish checks.
        anchor='    DaEyBmVector3Words position;'
        assert text.count(anchor)==1
        text=text.replace(anchor,'    if(std::getenv("SM64DS_PC_BOO_TRACE"))std::fprintf(stderr,"[native eye beam] event=player-hit tick=%llu uid=%u player=%u\\n",static_cast<unsigned long long>(ntr::rt_frame()),unsigned(uniqueID),unsigned(player->uniqueID));\n'+anchor)
        return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n'+text+'\nstatic_assert(sizeof(daEyBm_c)==0x334,"native eye projectile allocation");\n'
    old='extern daEykn_c::State data_ov071_02123088[];'
    assert text.count(old)==1
    text=text.replace(old,'daEykn_c::State data_ov071_02123088[] = {\n'+',\n'.join('{&daEykn_c::St_'+s+'_Init,&daEykn_c::St_'+s+'_Main}' for s in ('Wait','Attack','Die'))+'\n};')
    for suffix in ('38','40','48'):
        old='extern LoadedFile data_ov071_021230'+suffix+';'
        assert text.count(old)==1
        text=text.replace(old,'LoadedFile data_ov071_021230'+suffix+' = {};')
    old='extern SharedFilePtr data_ov071_02123050;'
    assert text.count(old)==1
    text=text.replace(old,'LoadedFile pc_mri_model_storage = {};\nSharedFilePtr &data_ov071_02123050 = *reinterpret_cast<SharedFilePtr *>(&pc_mri_model_storage);')
    old='extern SharedFilePtr *data_ov071_021226a4[2];'
    assert text.count(old)==1
    text=text.replace(old,'SharedFilePtr *data_ov071_021226a4[2] = {reinterpret_cast<SharedFilePtr *>(&data_ov071_02123038),reinterpret_cast<SharedFilePtr *>(&data_ov071_02123040)};')
    old='extern SharedFilePtr *data_ov071_021226a0;'
    assert text.count(old)==1
    text=text.replace(old,'SharedFilePtr *data_ov071_021226a0 = reinterpret_cast<SharedFilePtr *>(&data_ov071_02123048);')
    anchor='s32 daEykn_c::InitResources()\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'''
    static bool ready=false;
    if(!ready){
        data_ov071_02123050.Construct(0x2f7);
        data_ov071_021226a4[0]->Construct(0x2f8);
        data_ov071_021226a4[1]->Construct(0x2fb);
        data_ov071_021226a0->Construct(0x2f9);
        ready=true;
    }
''')
    for symbol in ('02123088','02123038','02123040','02123048','02123050','021226a4','021226a0'):
        text=text.replace('data_ov071_'+symbol,'pc_mri_'+symbol)
    text+='\nstatic_assert(sizeof(daEykn_c)==0x218,"native eye allocation");\nstatic_assert(sizeof(daEykn_c::State)==8,"native eye state row");\nstatic_assert(sizeof(LoadedFile)==8,"native eye resource storage");\n'
    anchor='int daEykn_c::Behavior()\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'''
    if(const char *prefix=std::getenv("SM64DS_PC_BOO_TRACE")){
        static FILE *file=nullptr;
        if(!file){char path[2048];std::snprintf(path,sizeof(path),"%s.mri.csv",prefix);file=std::fopen(path,"w");if(file)std::fputs("tick,uid,actor,state,circle,shot,x,y,z,yaw,area,alive\\n",file);}
        if(file && ntr::rt_frame()%5==0){std::fprintf(file,"%llu,%u,%u,%d,%d,%u,%d,%d,%d,%d,%d,%u\\n",static_cast<unsigned long long>(ntr::rt_frame()),unsigned(uniqueID),unsigned(actorID),mStateID,mCircleAngle,unsigned(mShotTimer),mPosX,mPosY,mPosZ,int(mAngleY),int(mAreaId),unsigned(aliveState));std::fflush(file);}
    }
''')
    anchor='int daEykn_c::Render()\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    pc_render_probe::Scope probe(this,false,&mModelAnim.mat4x3);')
    return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n#include "hal/actor_render_probe.h"\n'+text
