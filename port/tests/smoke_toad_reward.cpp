// Unit coverage of the VERBATIM original Toad talk/reward worker. Graphics,
// sound readiness, player dialogue state, save queries and actor spawning are
// explicit test seams. This is not natural map-entry/collection acceptance.
#include <cstddef>
#include <cstdio>
#include <cstdlib>
using u8=unsigned char;using s8=signed char;using u16=unsigned short;
using s16=short;using u32=unsigned;using s32=int;
struct Vector3 {int x,y,z;};
static int stars,collected,talk_state,sound_ready,spawn_count,end_count,message_count;
static unsigned spawned_kind,spawned_param;
static Vector3 spawned_position;
static int spawned_area;
struct Player {
    char pad0[0x5c];Vector3 position;char pad1[0x6d9-0x68];u8 mCharacter;
    int GetTalkState(){return talk_state;}
    int ShowMessage(void *,u32,const Vector3 *,u8,u8){++message_count;return 1;}
    template<class T>int ShowMessage(T &,u32,const Vector3 *,u8,u8){++message_count;return 1;}
};
static_assert(offsetof(Player,position)==0x5c && offsetof(Player,mCharacter)==0x6d9,"original player worker offsets");
struct fBase_c {void MarkForDestruction(){std::abort();}};
struct dActor_c {
    static fBase_c *FindWithID(u32){return nullptr;}
    static fBase_c *Spawn(u32 kind,u32 param,const Vector3 &pos,const void *,s8 area,s16){
        ++spawn_count;spawned_kind=kind;spawned_param=param;spawned_position=pos;spawned_area=area;return nullptr;
    }
};
struct daKinopio_c {
    char pad0[0x5c];int mPosX,mPosY,mPosZ;char pad1[0x8e-0x68];s16 mAngleY;
    char pad2[0xcc-0x90];s8 mAreaId;char pad3[0x1f4-0xcd];u32 mCapUniqueID;
    Player *mTalkPlayer;s32 mState;s16 head[4];u16 mMessageID;
    u8 mTalkFinished,mVariant,mStarID,mOpacity,mTargetOpacity,mStarReqIndex;
    u16 GetMessageID(){return mMessageID;}
    void SetState(int state){mState=state;}
    void SpawnSoundObj(int){std::abort();}
    void St_Talk_Main();
};
static_assert(sizeof(void *)==4 && sizeof(daKinopio_c)==0x210,"original worker x86 fixture size");
static_assert(offsetof(daKinopio_c,mPosX)==0x5c && offsetof(daKinopio_c,mAngleY)==0x8e,"original actor worker offsets");
static_assert(offsetof(daKinopio_c,mAreaId)==0xcc && offsetof(daKinopio_c,mTalkPlayer)==0x1f8,"original actor worker offsets");
static_assert(offsetof(daKinopio_c,mMessageID)==0x208 && offsetof(daKinopio_c,mStarReqIndex)==0x20f,"original reward offsets");
static u8 data_ov085_0212f27c[]={20,40,60};
static s8 data_0209f2f8=4;
static int data_0209caa0[2]={};
static s16 Vec3_HorzAngle(const Vector3 *,const Vector3 *){return 0;}
static bool ApproachLinear(short &,short,short){return true;}
static int func_02013a44(){return 0;}
static int IsStarCollectedInCurLevel(int){return collected;}
static u8 NumStars(){return static_cast<u8>(stars);}
static int _ZN5Sound7PlaySubEjjj5Fix12IiEb(u32,u32,u32,int,int){return sound_ready;}
struct Message {static void EndTalk(){++end_count;}};
#define M(p) (p)
#include "toad_reward_original.inc"
#undef M
static unsigned checks=0;
#define CHECK(c) do{++checks;if(!(c)){std::fprintf(stderr,"FAIL line=%d: %s\n",__LINE__,#c);return 1;}}while(0)
static daKinopio_c fresh(Player &player,unsigned index){
    daKinopio_c toad={};toad.mTalkPlayer=&player;toad.mPosX=1700*4096;
    toad.mPosY=-1350*4096;toad.mPosZ=-1900*4096;toad.mAreaId=4;
    toad.mState=1;toad.mStarID=static_cast<u8>(index+2);toad.mStarReqIndex=static_cast<u8>(index);
    toad.mMessageID=static_cast<u16>(0xaf5+index*2);return toad;
}
static void reset(int count,bool owned=false){
    stars=count;collected=owned;talk_state=2;sound_ready=1;
    spawn_count=end_count=message_count=0;spawned_kind=spawned_param=0;
}
int main(){
    Player player={};player.mCharacter=3;
    for(unsigned index=0;index<3;++index){
        const int requirement=data_ov085_0212f27c[index];
        for(int delta=-1;delta<=1;++delta){
            reset(requirement+delta);auto toad=fresh(player,index);const auto message=toad.mMessageID;
            toad.St_Talk_Main();
            CHECK(spawn_count==(delta>=0?1:0));CHECK(end_count==1);CHECK(toad.mTalkFinished==1);
            CHECK(toad.mMessageID==message+(delta>=0?1:0));CHECK(toad.mState==0);
            if(delta>=0){CHECK(spawned_kind==0xb2);CHECK(spawned_param==(index+2|0x20));
                CHECK(spawned_position.x==toad.mPosX && spawned_position.y==toad.mPosY+200*4096 && spawned_position.z==toad.mPosZ);
                CHECK(spawned_area==toad.mAreaId);}
            const int first=spawn_count;toad.St_Talk_Main();CHECK(spawn_count==first);
        }
        reset(requirement,true);auto owned=fresh(player,index);const auto old_message=owned.mMessageID;
        owned.St_Talk_Main();CHECK(spawn_count==0);CHECK(owned.mMessageID==old_message);
        auto reload=fresh(player,index);reload.St_Talk_Main();CHECK(spawn_count==0);
        reset(requirement);auto pending=fresh(player,index);sound_ready=0;
        pending.St_Talk_Main();CHECK(spawn_count==1);CHECK(end_count==0);CHECK(pending.mState==1);
        pending.St_Talk_Main();CHECK(spawn_count==1);CHECK(end_count==0);
        sound_ready=1;pending.St_Talk_Main();CHECK(spawn_count==1);CHECK(end_count==1);
        reset(requirement);auto waiting=fresh(player,index);talk_state=1;
        waiting.St_Talk_Main();CHECK(spawn_count==0 && waiting.mTalkFinished==0);
        talk_state=0;waiting.St_Talk_Main();CHECK(spawn_count==0 && message_count==1);
        talk_state=2;waiting.St_Talk_Main();CHECK(spawn_count==1);
    }
    reset(150);auto ordinary=fresh(player,0);ordinary.mStarID=0xff;
    ordinary.St_Talk_Main();CHECK(spawn_count==0);
    std::printf("PASS %u checks: original Toad 20/40/60 thresholds, spawn parameters/position, dialogue waits, sound waits and duplicate suppression. Unit seams; no natural collection claim.\n",checks);
    return 0;
}
