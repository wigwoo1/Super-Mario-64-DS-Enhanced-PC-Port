#pragma once

#include "dActor_c.h"
#include "ntr/gx.h"
#include "ntr/rt.h"
#include "ntr/video.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace pc_render_probe {
// Read-only, opt-in probe around one actor's synchronous model submission.
class Scope {
    const dActor_c *actor;
    int modelTranslation[3]{};
    unsigned dormant;
    size_t start=0;
    FILE *file=nullptr;
public:
    Scope(const dActor_c *a,unsigned hidden,const void *matrix)
        :actor(a),dormant(hidden) {
        const char *path=std::getenv("SM64DS_PC_ACTOR_RENDER_PROBE");
        if(!path || !*path)return;
        if(const char *range=std::getenv("SM64DS_PC_ACTOR_RENDER_PROBE_RANGE")) {
            unsigned long long first=0,last=0;
            if(std::sscanf(range,"%llu:%llu",&first,&last)!=2 ||
               ntr::rt_frame()<first || ntr::rt_frame()>last)return;
        }
        // Both retail matrix spellings put translation at byte 0x24. Avoid
        // selecting a Matrix4x3 definition before the actor includes common.h.
        if(matrix)std::memcpy(modelTranslation,static_cast<const unsigned char *>(matrix)+0x24,12);
        static FILE *trace=nullptr;
        if(!trace) {
            trace=std::fopen(path,"w");
            if(trace)std::fputs("tick,actor,uid,flags,clip_result,camera_depth,clip_limit,dormant,x,y,z,model_x,model_y,model_z,triangles,clipped_triangles,left,top,right,bottom\n",trace);
        }
        file=trace;
        if(file)ntr::gx_polygons(start);
    }
    ~Scope() {
        if(!file)return;
        size_t end=0;
        const auto *triangles=ntr::gx_polygons(end);
        unsigned clipped=0;
        float left=256,top=192,right=0,bottom=0;
        const auto settings=ntr::video_get_settings();
        const float aspect=(256.f*settings.height)/(192.f*settings.width);
        for(size_t i=start;i<end;++i) {
            auto triangle=triangles[i];
            if(triangle.perspective)for(auto &v:triangle.v)v.clip_x*=aspect;
            ntr::GxVertex vertices[12];
            const auto count=ntr::gx_clip_polygon(triangle,vertices);
            if(count<3)continue;
            clipped+=count-2;
            for(unsigned j=0;j<count;++j) {
                left=std::min(left,vertices[j].x);top=std::min(top,vertices[j].y);
                right=std::max(right,vertices[j].x);bottom=std::max(bottom,vertices[j].y);
            }
        }
        std::fprintf(file,"%llu,%u,%u,%08x,%u,%d,%d,%u,%d,%d,%d,%d,%d,%d,%zu,%u,%.3f,%.3f,%.3f,%.3f\n",
            static_cast<unsigned long long>(ntr::rt_frame()),unsigned(actor->actorID),
            unsigned(actor->uniqueID),unsigned(actor->mFlags),unsigned(actor->mClipResult),
            -actor->mCamSpacePosZ,actor->mClipDistance,dormant,
            actor->mPosX,actor->mPosY,actor->mPosZ,modelTranslation[0],modelTranslation[1],modelTranslation[2],
            end>=start?end-start:0,clipped,left,top,right,bottom);
        std::fflush(file);
    }
};
}
