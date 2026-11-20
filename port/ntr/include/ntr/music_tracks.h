#pragma once

namespace ntr {
// Sequence IDs from the locally supplied SM64DS sound archive. The game
// selects these through its original scene and power-up music calls.
enum class MusicTrack : unsigned {
    Wanted = 13,             // NCS_BGM_CASINOM3
    Coincentration = 14,     // NCS_BGM_MINICOIN
    WingCap = 51,            // NCS_BGM_MUTEKI (Powerful Mario)
    Castle = 57,             // NCS_BGM_SHIRO
    DireDireDocks = 59       // NCS_BGM_WATER
};

struct NamedMusicTrack {
    const char *name;
    MusicTrack id;
    const wchar_t *file_stem;
};

inline constexpr NamedMusicTrack requested_music_tracks[] = {
    {"Wanted", MusicTrack::Wanted, L"wanted"},
    {"Coincentration", MusicTrack::Coincentration, L"coincentration"},
    {"Wing Cap", MusicTrack::WingCap, L"wing-cap"},
    {"Castle", MusicTrack::Castle, L"castle"},
    {"Dire Dire Docks", MusicTrack::DireDireDocks, L"dire-dire-docks"}
};

inline const NamedMusicTrack *named_music_track(unsigned id) {
    for (const auto &track:requested_music_tracks)
        if (static_cast<unsigned>(track.id)==id) return &track;
    return nullptr;
}

inline const char *music_track_name(unsigned id) {
    const auto *track=named_music_track(id);
    return track?track->name:nullptr;
}
}
