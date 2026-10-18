#include "ntr/texture_assets.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <utility>

namespace ntr {
namespace {
const TextureModelRecord records[]={
#define PC_TEXTURE_MODEL(path,role,basis) {path,TextureRole::role,basis},
#include "texture_model_roles.inc"
#undef PC_TEXTURE_MODEL
};
struct Identity {uint64_t fingerprint;size_t size;const char *path;};
const Identity identities[]={
#define PC_TEXTURE_MODEL_IDENTITY(hash,size,path) {hash,size,path},
#include "texture_model_identities.inc"
#undef PC_TEXTURE_MODEL_IDENTITY
};
std::map<const void *,TextureModelOrigin> origins;
TextureModelOrigin current;
TextureRole consumer_role=TextureRole::Unknown;
const TextureSharedModelRecord shared_models[]={
#define PC_TEXTURE_SHARED_MODEL(path,handle,symbol) {path,handle,symbol},
#include "texture_shared_models.inc"
#undef PC_TEXTURE_SHARED_MODEL
};
std::string canonical(const char *path) {
    std::string text=path?path:"";
    for(auto &c:text) {if(c=='\\')c='/';if(c>='A'&&c<='Z')c+='a'-'A';}
    return text;
}
const TextureModelRecord *find_path(const char *path) {
    const auto wanted=canonical(path);
    for(const auto &record:records)if(canonical(record.path)==wanted)return &record;
    return nullptr;
}
}
const TextureModelRecord *texture_model_records(size_t &count) {count=sizeof(records)/sizeof(records[0]);return records;}
TextureRole texture_model_path_role(const char *path) {
    const auto *record=find_path(path);return record?record->role:TextureRole::Unknown;
}
uint64_t texture_model_fingerprint(const void *bytes,size_t size) {
    uint64_t hash=14695981039346656037ull;
    const auto *p=static_cast<const unsigned char *>(bytes);
    for(size_t i=0;i<size;++i) {hash^=p[i];hash*=1099511628211ull;}
    return hash;
}
void texture_model_register(const void *model,size_t size,const char *path) {
    if(!model)return;
    origins.erase(model);
    TextureModelOrigin origin;
    if(const auto *record=find_path(path)) {
        origin.role=record->role;origin.path=record->path;
    } else if(size>=0x38) {
        const auto fingerprint=texture_model_fingerprint(model,size);
        bool found=false,ambiguous=false;
        for(const auto &identity:identities) {
            if(identity.size!=size || identity.fingerprint!=fingerprint)continue;
            const auto role=texture_model_path_role(identity.path);
            if(found && origin.role!=role)ambiguous=true;
            if(!found) {origin.role=role;origin.path=identity.path;found=true;}
        }
        if(ambiguous) {origin.role=TextureRole::Unknown;origin.path="ambiguous-shared-model";}
    }
    if(!origin.path.empty())origins.emplace(model,origin);
    const char *trace=std::getenv("SM64DS_PC_TEXTURE_OWNERSHIP_TRACE");
    if(trace && trace[0] && trace[0]!='0' && !origin.path.empty()) {
        std::fprintf(stderr,"[texture ownership] register role=%u size=%zu path=%s\n",
            static_cast<unsigned>(origin.role),size,origin.path.c_str());
    }
}
void texture_model_forget(const void *model) {origins.erase(model);}
void texture_model_forget_range(const void *begin,const void *end) {
    const auto first=reinterpret_cast<uintptr_t>(begin),last=reinterpret_cast<uintptr_t>(end);
    for(auto it=origins.begin();it!=origins.end();) {
        const auto pointer=reinterpret_cast<uintptr_t>(it->first);
        if(pointer>=first && pointer<last)it=origins.erase(it);else ++it;
    }
}
void texture_model_clear() {origins.clear();}
TextureModelOrigin texture_model_origin(const void *model) {
    const auto found=origins.find(model);return found==origins.end()?TextureModelOrigin{}:found->second;
}
const TextureModelOrigin &texture_model_current() {return current;}
const TextureSharedModelRecord *texture_shared_model_records(size_t &count) {
    count=sizeof(shared_models)/sizeof(shared_models[0]);return shared_models;
}
TextureRole texture_model_draw_role(TextureRole base,const char *path,TextureRole consumer) {
    if(base==TextureRole::Character || consumer==TextureRole::Character)return TextureRole::Character;
    if(base==TextureRole::Unknown && consumer==TextureRole::Object) {
        const auto wanted=canonical(path);
        for(const auto &entry:shared_models)if(canonical(entry.path)==wanted)return TextureRole::Object;
    }
    return base;
}
TextureRole texture_consumer_role() {return consumer_role;}
TextureConsumerScope::TextureConsumerScope(TextureRole role):previous(consumer_role) {
    consumer_role=role==TextureRole::Character || role==TextureRole::Object?role:TextureRole::Unknown;
}
TextureConsumerScope::~TextureConsumerScope() {consumer_role=previous;}
TextureModelScope::TextureModelScope(const void *model):previous(std::move(current)) {
    current=texture_model_origin(model);
    current.role=texture_model_draw_role(current.role,current.path.c_str(),consumer_role);
}
TextureModelScope::~TextureModelScope() {current=std::move(previous);}
}
