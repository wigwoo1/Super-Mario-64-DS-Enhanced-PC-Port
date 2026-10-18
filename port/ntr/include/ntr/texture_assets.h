#pragma once
#include "ntr/texture.h"
#include <cstddef>
#include <string>

namespace ntr {
struct TextureModelRecord {const char *path;TextureRole role;const char *basis;};
const TextureModelRecord *texture_model_records(size_t &count);
TextureRole texture_model_path_role(const char *path);
uint64_t texture_model_fingerprint(const void *bytes,size_t size);
struct TextureModelOrigin {TextureRole role=TextureRole::Unknown;std::string path;};
// Register before relocation/shrinking. Archive copies are recognized by full
// model identity, while ambiguous identities retain original textures.
void texture_model_register(const void *model,size_t size,const char *path);
void texture_model_forget(const void *model);
void texture_model_forget_range(const void *begin,const void *end);
void texture_model_clear();
TextureModelOrigin texture_model_origin(const void *model);
const TextureModelOrigin &texture_model_current();
struct TextureSharedModelRecord {const char *path;uint16_t handle;const char *symbol;};
const TextureSharedModelRecord *texture_shared_model_records(size_t &count);
TextureRole texture_model_draw_role(TextureRole base,const char *path,TextureRole consumer);
TextureRole texture_consumer_role();
// Actor draw scope must surround ModelComponents::Render. It never changes
// registered file ownership; model scope derives an effective per-draw role.
class TextureConsumerScope {
    TextureRole previous;
public:
    explicit TextureConsumerScope(TextureRole role);
    ~TextureConsumerScope();
    TextureConsumerScope(const TextureConsumerScope &)=delete;
    TextureConsumerScope &operator=(const TextureConsumerScope &)=delete;
};
class TextureModelScope {
    TextureModelOrigin previous;
public:
    explicit TextureModelScope(const void *model);
    ~TextureModelScope();
    TextureModelScope(const TextureModelScope &)=delete;
    TextureModelScope &operator=(const TextureModelScope &)=delete;
};
}
