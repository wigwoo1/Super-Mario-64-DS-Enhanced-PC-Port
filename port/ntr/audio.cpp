#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#include "ntr/audio.h"
#include "ntr/audio_output.h"
#include "ntr/music_tracks.h"
#include "ntr/fs.h"
#include "ntr/sdat.h"
#include "ntr/pause_music.h"
#include "ntr/rt.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

namespace ntr { namespace {
constexpr unsigned hz=48000,frames=512,blocks=4;
struct State {
    struct EffectInfo { unsigned archive,id; uint64_t stop_tick; };
    SdatEngine engine;
    PauseMusic pause_music{engine};
    std::mutex mutex;
    std::atomic<bool> running{false},ready{false};
    std::thread worker;
    HWAVEOUT device=nullptr;
    std::array<WAVEHDR,blocks> headers{};
    std::array<std::array<int16_t,frames*2>,blocks> buffers{};
    std::map<uint32_t,bool> music;
    std::map<uint32_t,EffectInfo> effects;
    std::map<uint32_t,uint64_t> long_refs;
    uint64_t game_ticks=0;
    FILE *capture=nullptr;
    uint64_t rendered=0,nonzero=0,capture_bytes=0,starts=0,completed=0;
    int peak=0;
    unsigned prepared=0;
    bool output_muted=false;
    uint64_t submitted_nonzero=0;
};
State *state=nullptr;
void word(FILE *file,uint32_t value,unsigned count){for(unsigned i=0;i<count;++i)std::fputc(uint8_t(value>>(i*8)),file);}
void wave_header(FILE *file,uint32_t size){
    std::fwrite("RIFF",1,4,file);word(file,size+36,4);std::fwrite("WAVEfmt ",1,8,file);
    word(file,16,4);word(file,1,2);word(file,2,2);word(file,hz,4);word(file,hz*4,4);
    word(file,4,2);word(file,16,2);std::fwrite("data",1,4,file);word(file,size,4);
}
void cleanup(State &s){
    if(s.device){waveOutReset(s.device);for(unsigned i=0;i<s.prepared;++i)waveOutUnprepareHeader(s.device,&s.headers[i],sizeof(WAVEHDR));waveOutClose(s.device);s.device=nullptr;}
    if(s.capture){std::rewind(s.capture);wave_header(s.capture,uint32_t(s.capture_bytes));std::fclose(s.capture);s.capture=nullptr;}
}
void worker(State &s){
    while(s.running){
        for(unsigned i=0;i<blocks && s.running;++i)if(s.headers[i].dwFlags&WHDR_DONE){
            {
                std::lock_guard<std::mutex> lock(s.mutex);
                const auto errors=s.engine.errors();
                s.engine.render(s.buffers[i].data(),frames);
                if(s.engine.errors()!=errors)std::fprintf(stderr,"[native audio] render-error: %s\n",s.engine.error().c_str());
                for(auto it=s.music.begin();it!=s.music.end();){
                    if(!s.engine.active(it->first)){
                        if(it->second)std::fprintf(stderr,"[native audio] sequence-completed key=%u rendered=%llu\n",it->first,s.rendered);
                        if(!it->second) {
                            auto effect=s.effects.find(it->first);
                            if(effect!=s.effects.end() && std::getenv("SM64DS_PC_AUDIO_TRACE"))
                                std::fprintf(stderr,"[native audio] effect-completed archive=%u id=%u key=%u tick=%llu\n",effect->second.archive,effect->second.id,it->first,s.game_ticks);
                            s.effects.erase(it->first);
                        }
                        ++s.completed;it=s.music.erase(it);
                    }else ++it;
                }
            }
            for(auto sample:s.buffers[i]){s.nonzero+=sample!=0;s.peak=std::max(s.peak,std::abs(int(sample)));}
            s.rendered+=frames;
            if(s.capture && s.capture_bytes+frames*4<=128*1024*1024){std::fwrite(s.buffers[i].data(),4,frames,s.capture);s.capture_bytes+=frames*4;}
            s.submitted_nonzero+=audio_prepare_submission(s.buffers[i].data(),s.buffers[i].size(),s.output_muted);
            if(waveOutWrite(s.device,&s.headers[i],sizeof(WAVEHDR))!=MMSYSERR_NOERROR){
                std::fprintf(stderr,"[native audio] device-write failed; silent completion fallback enabled\n");s.ready=false;s.running=false;
            }
        }
        Sleep(2);
    }
}
}
bool audio_open(){
    const char *enabled=std::getenv("SM64DS_PC_NATIVE_AUDIO");
    if(!enabled || std::strcmp(enabled,"1"))return false; // diagnostic until gameplay acceptance
    if(state)return state->ready;
    auto s=std::make_unique<State>();
    const char *muted=std::getenv("SM64DS_PC_AUDIO_MUTED");
    s->output_muted=muted && std::strcmp(muted,"1")==0;
    const int id=fs_id_by_path("data/sound_data.sdat");
    const int bytes=fs_size(id);
    if(bytes<=0 || bytes>64*1024*1024){std::fprintf(stderr,"[native audio] local SDAT unavailable; silent fallback\n");return false;}
    std::vector<uint8_t> archive(bytes);
    if(fs_read(id,archive.data(),bytes)!=bytes || !s->engine.load(archive.data(),archive.size())){
        std::fprintf(stderr,"[native audio] archive load failed: %s; silent fallback\n",s->engine.error().c_str());return false;
    }
    WAVEFORMATEX format{};format.wFormatTag=WAVE_FORMAT_PCM;format.nChannels=2;
    format.nSamplesPerSec=hz;format.wBitsPerSample=16;format.nBlockAlign=4;format.nAvgBytesPerSec=hz*4;
    const char *no_device=std::getenv("SM64DS_PC_AUDIO_NO_DEVICE");
    const auto result=no_device && no_device[0]=='1'?MMSYSERR_NODRIVER:waveOutOpen(&s->device,WAVE_MAPPER,&format,0,0,CALLBACK_NULL);
    if(result!=MMSYSERR_NOERROR){std::fprintf(stderr,"[native audio] device-open failed code=%u; silent fallback\n",unsigned(result));return false;}
    waveOutPause(s->device);
    for(unsigned i=0;i<blocks;++i){
        s->headers[i].lpData=reinterpret_cast<char *>(s->buffers[i].data());
        s->headers[i].dwBufferLength=frames*4;
        if(waveOutPrepareHeader(s->device,&s->headers[i],sizeof(WAVEHDR))!=MMSYSERR_NOERROR){cleanup(*s);return false;}
        ++s->prepared;
        if(waveOutWrite(s->device,&s->headers[i],sizeof(WAVEHDR))!=MMSYSERR_NOERROR){cleanup(*s);return false;}
    }
    if(const char *path=std::getenv("SM64DS_PC_AUDIO_CAPTURE"))if(*path){s->capture=std::fopen(path,"wb");if(s->capture)wave_header(s->capture,0);}
    s->running=true;s->ready=true;
    state=s.release();state->worker=std::thread(worker,std::ref(*state));
    waveOutRestart(state->device);
    std::fprintf(stderr,"[native audio] ready rate=48000 channels=2 bits=16 buffer_frames=2048 music_slots=%u effect_archives=%u\n",state->engine.sequence_slots(),state->engine.effect_archives());
    std::fprintf(stderr,"[native audio] output_muted=%u (capture precedes device gate)\n",unsigned(state->output_muted));
    return true;
}
void audio_close(){
    if(!state)return;
    state->ready=false;state->running=false;if(state->worker.joinable())state->worker.join();
    MMTIME time{};time.wType=TIME_SAMPLES;
    const auto result=waveOutGetPosition(state->device,&time,sizeof time);
    std::fprintf(stderr,"[native audio] shutdown rendered=%llu nonzero=%llu peak=%d starts=%llu errors=%llu device_position_type=%u device_position=%u query=%u completed=%llu game_ticks=%llu\n",
        state->rendered,state->nonzero,state->peak,state->starts,state->engine.errors(),time.wType,time.u.sample,result,state->completed,state->game_ticks);
    std::fprintf(stderr,"[native audio] submission output_muted=%u nonzero_samples=%llu\n",unsigned(state->output_muted),state->submitted_nonzero);
    cleanup(*state);delete state;state=nullptr;
}
bool audio_ready(){return state && state->ready;}
uint32_t audio_sequence(uint32_t key,unsigned id){
    if(!audio_ready())return 0;std::lock_guard<std::mutex> lock(state->mutex);
    const auto voice=state->engine.play_sequence(id,key);
    if(voice){state->music[voice]=true;state->pause_music.music_started(voice);++state->starts;}
    if(state->starts<3000)std::fprintf(stderr,"[native audio] sequence id=%u name=%s key=%u result=%u error=%s tick=%llu\n",id,ntr::music_track_name(id)?ntr::music_track_name(id):"other",key,voice,voice?"":state->engine.error().c_str(),state->game_ticks);
    return voice;
}
static uint32_t start_effect(unsigned group,unsigned id,int bank,unsigned max_game_ticks,uint32_t key){
    if(!audio_ready())return 0;std::lock_guard<std::mutex> lock(state->mutex);
    const auto voice=state->engine.play_effect(group,id,bank,key);
    if(voice){state->music[voice]=false;state->effects[voice]={group,id,max_game_ticks?state->game_ticks+max_game_ticks:0};++state->starts;}
    if(voice && group==3 && id==17 && std::getenv("SM64DS_PC_AUDIO_TRACE"))
        std::fprintf(stderr,"[native audio event] coin-pickup start key=%u runtime_frame=%llu audio_tick=%llu\n",
            voice,static_cast<unsigned long long>(rt_frame()),static_cast<unsigned long long>(state->game_ticks));
    if(state->starts<3000)std::fprintf(stderr,"[native audio] effect archive=%u id=%u bank=%d result=%u error=%s tick=%llu\n",group,id,bank,voice,voice?"":state->engine.error().c_str(),state->game_ticks);
    return voice;
}
uint32_t audio_effect(unsigned group,unsigned id,int bank,uint32_t key){
    return start_effect(group,id,bank,0,key);
}
uint32_t audio_effect_timed(unsigned group,unsigned id,int bank,unsigned max_game_ticks){
    return start_effect(group,id,bank,max_game_ticks,0);
}
void audio_stop_effect(unsigned group,unsigned id,int fade_game_ticks){
    if(!audio_ready())return;std::lock_guard<std::mutex> lock(state->mutex);
    unsigned stopped=0;
    for(auto it=state->effects.begin();it!=state->effects.end();){
        if(it->second.archive!=group || it->second.id!=id){++it;continue;}
        state->engine.stop(it->first,fade_game_ticks);
        if(fade_game_ticks<=0){state->music.erase(it->first);it=state->effects.erase(it);}
        else {it->second.stop_tick=0;++it;}
        ++stopped;
    }
    if(stopped)std::fprintf(stderr,"[native audio] stop-effect archive=%u id=%u voices=%u fade_ticks=%d tick=%llu\n",group,id,stopped,fade_game_ticks,state->game_ticks);
}
bool audio_active(uint32_t key){if(!audio_ready())return false;std::lock_guard<std::mutex> lock(state->mutex);return state->engine.active(key);}
uint32_t audio_long_effect(uint32_t key,unsigned group,unsigned id,int bank){
    if(!audio_ready())return 0;std::lock_guard<std::mutex> lock(state->mutex);
    if(!state->engine.active(key)){
        key=state->engine.play_effect(group,id,bank);if(key)++state->starts;
        if(std::getenv("SM64DS_PC_AUDIO_LONG_TRACE"))std::fprintf(stderr,
            "[native long audio] event=start tick=%llu archive=%u id=%u bank=%d key=%u error=%s\n",
            static_cast<unsigned long long>(rt_frame()),group,id,bank,key,key?"":state->engine.error().c_str());
    }
    if(key){state->long_refs[key]=state->game_ticks;state->music[key]=false;}
    return key;
}
void audio_tick(){
    if(!audio_ready())return;std::lock_guard<std::mutex> lock(state->mutex);++state->game_ticks;state->engine.tick();
    for(auto it=state->effects.begin();it!=state->effects.end();){
        if(it->second.stop_tick && state->game_ticks>=it->second.stop_tick){
            state->engine.stop(it->first,4);it->second.stop_tick=0;++it;continue;
        }
        if(!state->engine.active(it->first)){state->music.erase(it->first);it=state->effects.erase(it);}
        else ++it;
    }
    for(auto it=state->long_refs.begin();it!=state->long_refs.end();){
        if(!state->engine.active(it->first) || state->game_ticks-it->second>2){
            if(std::getenv("SM64DS_PC_AUDIO_LONG_TRACE"))std::fprintf(stderr,
                "[native long audio] event=expired tick=%llu key=%u idle_ticks=%llu\n",
                static_cast<unsigned long long>(rt_frame()),it->first,state->game_ticks-it->second);
            state->engine.stop(it->first);state->music.erase(it->first);it=state->long_refs.erase(it);
        }
        else ++it;
    }
}
void audio_stop(uint32_t key,int frames){if(audio_ready()){std::lock_guard<std::mutex> lock(state->mutex);state->engine.stop(key,frames);if(frames<=0)state->music.erase(key);if(frames>0)std::fprintf(stderr,"[native audio] stop-fade key=%u frames=%d tick=%llu\n",key,frames,state->game_ticks);else if(std::getenv("SM64DS_PC_AUDIO_TRACE"))std::fprintf(stderr,"[native audio] stop key=%u tick=%llu\n",key,state->game_ticks);}}
void audio_volume(uint32_t key,int volume,int frames){if(audio_ready()){std::lock_guard<std::mutex> lock(state->mutex);state->engine.volume(key,volume,frames);}}
void audio_pause_music(bool pause){if(audio_ready()){std::lock_guard<std::mutex> lock(state->mutex);for(auto pair:state->music)if(pair.second)state->engine.pause(pair.first,pause);}}
void audio_set_pause_menu(bool paused,bool rec_room_music){
    if(!audio_ready())return;
    std::lock_guard<std::mutex> lock(state->mutex);
    const bool requested=paused && rec_room_music;
    if(requested==state->pause_music.requested())return;
    std::vector<uint32_t> music;
    for(auto pair:state->music)if(pair.second)music.push_back(pair.first);
    const auto retained=state->pause_music.retained();
    state->pause_music.update(requested,music);
    std::fprintf(stderr,"[pause music] tick=%llu requested=%u active=%u theme=5 retained=%u restored=%u\n",
        static_cast<unsigned long long>(rt_frame()),unsigned(requested),unsigned(state->pause_music.active()),
        unsigned(state->pause_music.retained()),requested?0u:unsigned(retained));
}
void audio_pause_player(unsigned player,bool pause){if(audio_ready()){std::lock_guard<std::mutex> lock(state->mutex);state->engine.pause_player(player,pause);std::fprintf(stderr,"[native audio] player-pause player=%u paused=%u tick=%llu\n",player,unsigned(pause),state->game_ticks);}}
void audio_stop_player(unsigned player,int frames){if(audio_ready()){std::lock_guard<std::mutex> lock(state->mutex);state->engine.stop_player(player,frames);if(frames>0)std::fprintf(stderr,"[native audio] player-stop-fade player=%u frames=%d tick=%llu\n",player,frames,state->game_ticks);}}
void audio_stop_all(){if(audio_ready()){std::lock_guard<std::mutex> lock(state->mutex);state->pause_music.reset();state->engine.stop_all();state->music.clear();state->effects.clear();state->long_refs.clear();if(std::getenv("SM64DS_PC_AUDIO_TRACE"))std::fprintf(stderr,"[native audio] stop-all tick=%llu\n",state->game_ticks);}}
}
