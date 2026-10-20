#pragma once
#include <cstdint>
namespace ntr {
bool audio_open();
void audio_close();
bool audio_ready();
uint32_t audio_sequence(uint32_t key,unsigned id);
uint32_t audio_effect(unsigned archive,unsigned id,int bank=-1,uint32_t key=0);
uint32_t audio_long_effect(uint32_t key,unsigned archive,unsigned id,int bank=-1);
void audio_tick();
bool audio_active(uint32_t key);
void audio_stop(uint32_t key,int fade_frames=0);
void audio_volume(uint32_t key,int volume,int fade_frames=0);
void audio_pause_music(bool pause);
void audio_pause_player(unsigned player,bool pause);
void audio_stop_player(unsigned player,int fade_frames=0);
void audio_stop_all();
}
