// Prepare visible distant actor shadows before the Stage drains its shadow
// list. This updates visual resources only; behavior/collision stay dormant.
#include "daKrb_c.h"
#include "daBmb_c.h"
#include "daCoin_c.h"
#include "daWanwan_c.h"
#include "dClipper.h"
#include "hal/render_distance_policy.h"
#include "ntr/rt.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>

extern "C" {
extern dClipper data_0209f43c;
extern dExtShadowModel_c *data_0209cef4;
extern u8 data_0209ceec;
u8 IsAreaShowing(int);
void func_ov102_0214b444(void *);
void func_ov014_02112788(char *);
void Matrix4x3_FromRotationY(void *,int);
// Local extern: original exported scalar ABI across the Fix12 by-value wall.
int _ZN8dClipper13Func_020150E8ER7Vector35Fix12IiEPh(char *,Vector3 *,int,u8 *);
void _ZN8dActor_c19DropShadowRadHeightER17dExtShadowModel_cR9Matrix4x35Fix12IiES5_j(
    dActor_c *,dExtShadowModel_c *,Matrix4x3 *,int,int,u8);
}

extern "C" void pc_prepare_extended_shadows() {
    const auto mode=ntr::video_get_settings().render_distance;
    if(mode==ntr::RenderDistance::Minimum || data_0209ceec)return;
    std::vector<dExtShadowModel_c *> queued;
    for(auto *p=data_0209cef4;p && queued.size()<4096;p=p->next)queued.push_back(p);
    unsigned visited=0;
    for(auto *actor=dActor_c::Next(nullptr);actor && visited++<4096;actor=dActor_c::Next(actor)) {
        const auto kind=actor->actorID;
        const bool chompFirstView=kind==219 && (actor->mFlags&0x18) &&
            (actor->mAreaId<0 || IsAreaShowing(actor->mAreaId)) && actor->mCamSpacePosZ!=0x7fffffff;
        if(!((kind>=200 && kind<=202) || kind==206 || kind==219 || (kind>=288 && kind<=290)) || actor->aliveState!=1 ||
           actor->shouldBeKilled || !(actor->mFlags&0x10) || (actor->mFlags&0x40000) ||
           ((actor->mFlags&0x20) && !chompFirstView))continue;
        dExtShadowModel_c *shadow=nullptr;
        if(kind>=200 && kind<=202) {
            auto *goomba=static_cast<daKrb_c *>(actor);
            if(goomba->mIsDormant)continue;
            shadow=&goomba->mShadowModel;
        } else if(kind==206) {
            auto *bomb=static_cast<daBmb_c *>(actor);
            if(!bomb->mShouldRender || bomb->mState==5 || bomb->mDeathState)continue;
            shadow=&bomb->mShadowModel;
        } else if(kind==219) {
            auto *chomp=static_cast<daWanwan_c *>(actor);
            shadow=&chomp->mShadowModel;
            // Its visual worker queues the head and all seven links together.
            bool partial=false;
            for(auto &link:chomp->mLinkShadows)
                if(std::find(queued.begin(),queued.end(),&link)!=queued.end())partial=true;
            if(partial)continue;
        } else {
            auto *coin=static_cast<daCoin_c *>(actor);
            if(!(coin->mCoinFlags&1) || (kind==289 && coin->mInBrickBlock))continue;
            shadow=&coin->mShadowModel;
        }
        if(std::find(queued.begin(),queued.end(),shadow)!=queued.end())continue;
        Vector3 camera{actor->mCamSpacePosX,actor->mCamSpacePosY,actor->mCamSpacePosZ};
        u8 hint=actor->mClipResult&3;
        const auto savedFar=data_0209f43c.mFarZ;
        data_0209f43c.mFarZ=pc_render_distance::kMaximumDepth;
        const int depth=_ZN8dClipper13Func_020150E8ER7Vector35Fix12IiEPh(
            reinterpret_cast<char *>(&data_0209f43c),&camera,actor->mClipRadius,&hint);
        data_0209f43c.mFarZ=savedFar;
        const auto limit=pc_render_distance::limit(mode,actor->mFarDistance,false);
        if(!pc_render_distance::visible(depth,limit))continue;
        // Do not call the full placement helper: its cap update also changes
        // cap release flags. Only the persistent model matrix and shadow queue
        // are needed here, before RenderAll freezes the queue for this frame.
        const auto flags=actor->mFlags;
        actor->mFlags&=kind==219?~0x38u:~0x10u;
        if(kind>=200 && kind<=202) {
            auto *goomba=static_cast<daKrb_c *>(actor);
            auto &matrix=goomba->mModelAnim.mat4x3;
            Matrix4x3_FromRotationY(&matrix,actor->mAngleY);
            matrix.t.x=actor->mPosX>>3;matrix.t.y=actor->mPosY>>3;matrix.t.z=actor->mPosZ>>3;
            _ZN8dActor_c19DropShadowRadHeightER17dExtShadowModel_cR9Matrix4x35Fix12IiES5_j(
                actor,shadow,&matrix,actor->mScaleX*0x50,
                goomba->mWithMeshClsn.IsOnGround()?0x1e000:0x96000,0xf);
        } else if(kind==206) {
            // Original shadow-only helper: handles ground height and carrier,
            // without the full placement worker's particles/carry-state writes.
            func_ov102_0214b444(actor);
        } else if(kind==219) {
            // Original placement/shadow worker; never advances the dormant
            // state machine, chain simulation, collisions or release timer.
            func_ov014_02112788(reinterpret_cast<char *>(actor));
        } else {
            // Original coin visual worker respects floor/no-shadow gates.
            // Its clip-radius update belongs to Behavior, so restore that field.
            const auto radius=actor->mClipRadius;
            static_cast<daCoin_c *>(actor)->func_ov002_020b14d8();
            actor->mClipRadius=radius;
        }
        actor->mFlags=flags;
        if(kind==219) {
            bool found=false;unsigned count=0;
            for(auto *p=data_0209cef4;p && count++<4096;p=p->next)if(p==shadow)found=true;
            if(!found)continue;
        } else if(data_0209cef4!=shadow)continue;
        queued.push_back(shadow);
        if(const char *path=std::getenv("SM64DS_PC_DISTANT_SHADOW_TRACE")) {
            static FILE *file=nullptr;
            if(!file) {file=std::fopen(path,"w");if(file)std::fputs("tick,actor,uid,depth,limit,flags\n",file);}
            if(file) {std::fprintf(file,"%llu,%u,%u,%d,%lld,%08x\n",
                static_cast<unsigned long long>(ntr::rt_frame()),unsigned(actor->actorID),
                unsigned(actor->uniqueID),depth,static_cast<long long>(limit),unsigned(actor->mFlags));std::fflush(file);}
        }
    }
}
