#pragma once
#include <cstdint>
#include "ntr/audio_layout.h"
namespace ntr {
bool audio_open();
void audio_close();
bool audio_ready();
void audio_set_layout(AudioLayout layout);
AudioOutput audio_output_status();
void audio_set_remastered_music(bool enabled);
uint32_t audio_sequence(uint32_t key,unsigned id);
uint32_t audio_effect(unsigned archive,unsigned id,int bank=-1,uint32_t key=0);
uint32_t audio_effect_at(unsigned archive,unsigned id,int bank,AudioPosition position);
uint32_t audio_effect_timed(unsigned archive,unsigned id,int bank,unsigned max_game_ticks);
void audio_stop_effect(unsigned archive,unsigned id,int fade_game_ticks=0);
uint32_t audio_long_effect(uint32_t key,unsigned archive,unsigned id,int bank=-1,
                           const AudioPosition *position=nullptr);
void audio_tick();
bool audio_active(uint32_t key);
void audio_stop(uint32_t key,int fade_frames=0);
void audio_volume(uint32_t key,int volume,int fade_frames=0);
void audio_pause_music(bool pause);
void audio_set_pause_menu(bool paused,bool rec_room_music);
void audio_pause_player(unsigned player,bool pause);
void audio_stop_player(unsigned player,int fade_frames=0);
void audio_stop_all();
}
