#pragma once
#include "dActor_c.h"
#include "ntr/rt.h"
#include <cstdio>
#include <cstdlib>

// Optional observation of the original callbacks. Never invokes an attack,
// changes an actor, or substitutes a character eligibility rule.
namespace pc_block_probe {
inline void event(const char *name, const dActor_c *block,
                  const dActor_c *other = nullptr) {
    const char *path = std::getenv("SM64DS_PC_BLOCK_TRACE");
    if (!path || !*path || !block) return;
    static FILE *trace = nullptr;
    if (!trace) {
        trace = std::fopen(path, "w");
        if (trace) std::fputs("event,tick,actor,uid,alive,other_actor,other_uid,other_param,x,y,z,other_x,other_y,other_z\n", trace);
    }
    if (!trace) return;
    std::fprintf(trace, "%s,%llu,%u,%u,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d\n",
        name, static_cast<unsigned long long>(ntr::rt_frame()),
        unsigned(block->actorID), unsigned(block->uniqueID), unsigned(block->aliveState),
        other ? unsigned(other->actorID) : 0, other ? unsigned(other->uniqueID) : 0,
        other ? unsigned(other->param1) : 0,
        block->mPosX, block->mPosY, block->mPosZ,
        other ? other->mPosX : 0, other ? other->mPosY : 0, other ? other->mPosZ : 0);
    std::fflush(trace);
}
}
