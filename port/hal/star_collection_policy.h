#pragma once

namespace pc_star {
// Mission Stars in the fifteen story courses can use the retail non-exiting
// celebration/save path. Secret courses, keys, VS, silver stars and scripted
// marker-only rewards keep their original transitions.
inline bool can_stay(bool enabled, unsigned actor, unsigned mode, int course,
                     unsigned star, unsigned kind, unsigned home_state) {
    return enabled && actor == 178 && mode == 0 && course >= 0 && course < 15 &&
           star >= 1 && star <= 7 && kind != 3 && kind != 5 && kind < 8 &&
           home_state != 9;
}
}
