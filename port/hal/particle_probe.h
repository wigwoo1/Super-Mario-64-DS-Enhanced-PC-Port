#pragma once

#include "ntr/gx.h"
#include "ntr/rt.h"
#include "ntr/video.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace pc_particle_probe {
// Opt-in observation of the original x86/ARM32-compatible particle records.
// Never edits an emitter, particle, callback, resource or graphics command.
inline FILE *trace=nullptr;
inline bool enabled(){const char *p=std::getenv("SM64DS_PC_PARTICLE_TRACE");return p && *p;}
template<class T> inline T read(const void *p,size_t offset){
    T value{};if(p)std::memcpy(&value,static_cast<const unsigned char *>(p)+offset,sizeof(value));return value;
}
inline int definition(const void *manager,const void *system){
    if(!manager || !system)return -1;
    const auto first=read<std::uintptr_t>(manager,0x1c),entry=read<std::uintptr_t>(system,0x18);
    if(!first || entry<first || (entry-first)%32 || (entry-first)/32>=read<unsigned short>(manager,0x24))return -1;
    return int((entry-first)/32);
}
inline bool selected(int id){
    const char *s=std::getenv("SM64DS_PC_PARTICLE_TRACE_ID");
    if(!s || !*s)return id==10;
    // Some original effects are a pair of emitters (e.g. the sleep bubble).
    // A comma-separated observation filter records both in the same run.
    while(*s){
        char *end=nullptr;
        const auto value=std::strtoul(s,&end,0);
        if(end==s)return false;
        if(value==static_cast<unsigned>(id))return true;
        if(*end!=',')return false;
        s=end+1;
    }
    return false;
}
inline void row(const char *phase,const void *manager,const void *system,int id,
                size_t triangles=0,unsigned clipped=0){
    if(!enabled())return;
    if(!trace){trace=std::fopen(std::getenv("SM64DS_PC_PARTICLE_TRACE"),"w");
        if(trace)std::fputs("phase,tick,id,manager,system,active,free,particles,children,age,flags,kind,triangles,clipped_triangles\n",trace);}
    if(!trace)return;
    const auto *entry=read<const void *>(system,0x18);
    const auto *data=read<const void *>(entry,0);
    std::fprintf(trace,"%s,%llu,%d,%p,%p,%d,%d,%d,%d,%u,%08x,%u,%zu,%u\n",phase,
        static_cast<unsigned long long>(ntr::rt_frame()),id,manager,system,
        read<int>(manager,8),read<int>(manager,0x10),read<int>(system,0xc),read<int>(system,0x14),
        unsigned(read<unsigned short>(system,0x38)),read<unsigned>(system,0x1c),
        (read<unsigned>(data,0)>>4)&3,triangles,clipped);
    std::fflush(trace);
}
inline void created(const void *manager,int id,const void *result){
    if(!enabled() || !selected(id))return;
    const void *system=result;
    // NewSimple can intentionally return null for self-destroying definitions;
    // the original manager has nevertheless inserted the system in its list.
    if(!system){const void *head=read<const void *>(manager,4);if(definition(manager,head)==id)system=head;}
    row("create",manager,system,id);
}
inline void update(const void *manager){
    if(!enabled())return;
    if(ntr::rt_frame()%60==0)row("manager",manager,nullptr,-1);
    auto *system=read<const void *>(manager,4);
    for(unsigned i=0;system && i<128;++i){
        int id=definition(manager,system);
        if(selected(id))row("update",manager,system,id);
        system=read<const void *>(system,0);
    }
}
class Scope {
    const void *manager=nullptr,*system=nullptr;
    int id=-1;size_t first=0;
public:
    explicit Scope(const void *m){
        if(!enabled())return;
        manager=m;system=read<const void *>(m,0x34);id=definition(m,system);
        if(selected(id))ntr::gx_polygons(first);else manager=nullptr;
    }
    ~Scope(){
        if(!manager)return;
        size_t last=0;const auto *polys=ntr::gx_polygons(last);unsigned clipped=0;
        const auto s=ntr::video_get_settings();const float aspect=256.f*s.height/(192.f*s.width);
        for(size_t i=first;i<last;++i){auto p=polys[i];
            if(p.perspective)for(auto &v:p.v)v.clip_x*=aspect;
            ntr::GxVertex vertices[12];auto count=ntr::gx_clip_polygon(p,vertices);
            if(count>=3)clipped+=count-2;
        }
        row("render",manager,system,id,last>=first?last-first:0,clipped);
    }
};
}
