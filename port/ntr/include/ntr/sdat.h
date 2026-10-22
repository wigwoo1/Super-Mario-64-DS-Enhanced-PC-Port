#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace ntr {
// Native sequencer/mixer. Reads only caller-supplied local asset bytes; no DS
// driver, game objects, device callbacks or save state are involved here.
class SdatEngine {
public:
    explicit SdatEngine(unsigned sample_rate=48000);
    ~SdatEngine();
    SdatEngine(const SdatEngine &)=delete;
    SdatEngine &operator=(const SdatEngine &)=delete;
    bool load(const uint8_t *bytes,size_t size);
    uint32_t play_sequence(unsigned id,uint32_t slot=0,int volume=127);
    // Host menu music has an independent player group; retail music is retained.
    uint32_t play_sequence_isolated(unsigned id,uint32_t slot);
    uint32_t play_effect(unsigned archive,unsigned id,int bank_override=-1,uint32_t slot=0,int volume=127);
    void stop(uint32_t slot,int fade_frames=0);
    void stop_all();
    void pause(uint32_t slot,bool paused);
    void suspend(uint32_t slot,bool suspended); // independent of the retail pause flag
    void pause_player(unsigned player,bool paused);
    void stop_player(unsigned player,int fade_frames=0);
    void volume(uint32_t slot,int value,int fade_frames=0);
    void tick();
    void variable(uint32_t slot,unsigned variable,int value);
    bool active(uint32_t slot) const;
    void render(int16_t *stereo,size_t frames);
    unsigned sequence_slots() const;
    unsigned effect_archives() const;
    unsigned effect_slots(unsigned archive) const;
    bool has_sequence(unsigned id) const;
    bool has_effect(unsigned archive,unsigned id) const;
    uint64_t errors() const;
    const std::string &error() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
