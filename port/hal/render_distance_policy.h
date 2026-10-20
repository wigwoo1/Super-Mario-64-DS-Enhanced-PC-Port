#pragma once

#include <cstdint>
#include "ntr/video.h"

namespace pc_render_distance {
constexpr int32_t kOutsideFrustum = 0x7fffffff;
constexpr int32_t kMaximumDepth = 0x3fffffff;

inline int64_t limit(ntr::RenderDistance distance, int32_t native_limit,
                     bool native_double_range) {
    int64_t value=native_limit;
    if(native_double_range)value*=2;
    switch(distance) {
    case ntr::RenderDistance::Low: value*=2; break;
    case ntr::RenderDistance::Medium: value*=4; break;
    case ntr::RenderDistance::EntireMap: return kMaximumDepth;
    case ntr::RenderDistance::Minimum: break;
    }
    return value>kMaximumDepth?kMaximumDepth:value;
}

inline int32_t projection_far(ntr::RenderDistance distance, int32_t native_far) {
    if(distance==ntr::RenderDistance::Minimum || native_far<=0)return native_far;
    if(distance==ntr::RenderDistance::EntireMap)return kMaximumDepth;
    const int64_t multiplier=distance==ntr::RenderDistance::Low?2:4;
    const int64_t value=static_cast<int64_t>(native_far)*multiplier;
    return static_cast<int32_t>(value>kMaximumDepth?kMaximumDepth:value);
}

inline bool visible(int32_t depth, int64_t limit_value) {
    return depth!=kOutsideFrustum && depth>=0 && depth<=limit_value;
}
}
