#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace ntr {
constexpr unsigned music_sample_rate=48000;
struct MusicClip {
    std::vector<int16_t> stereo;
    size_t loop_begin=0,loop_end=0; // Frame offsets. The track starts at frame zero.
    bool valid() const {
        return stereo.size()%2==0 && loop_begin<loop_end && loop_end<=stereo.size()/2;
    }
    size_t frame_at(uint64_t elapsed) const {
        return elapsed<loop_end?static_cast<size_t>(elapsed):
            loop_begin+static_cast<size_t>((elapsed-loop_end)%(loop_end-loop_begin));
    }
};

struct RemasteredLoad {
    std::shared_ptr<const MusicClip> clip;
    const char *status="unmapped";
};
RemasteredLoad load_remastered_track(unsigned id);
}
