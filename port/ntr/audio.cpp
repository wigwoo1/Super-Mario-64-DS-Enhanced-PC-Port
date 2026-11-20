#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#include <audioclient.h>
#include <ks.h>
#include <ksmedia.h>
#include <mmddk.h>
#include <mmdeviceapi.h>
#include <wrl/client.h>
#include "ntr/audio.h"
#include "ntr/audio_output.h"
#include "ntr/music_tracks.h"
#include "ntr/fs.h"
#include "ntr/sdat.h"
#include "ntr/pause_music.h"
#include "ntr/remastered_music.h"
#include "ntr/rt.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace ntr { namespace {
constexpr unsigned hz=48000,frames=512,blocks=4;
using Microsoft::WRL::ComPtr;
using Clock=std::chrono::steady_clock;
constexpr auto device_poll_interval=std::chrono::seconds(1);
struct WaveTarget {
    UINT id=WAVE_MAPPER;
    AudioLayout layout=AudioLayout::Stereo;
    uint32_t mask=audio_channel_mask(AudioLayout::Stereo);
    std::wstring endpoint;
    bool operator==(const WaveTarget &other) const {
        return id==other.id && layout==other.layout && mask==other.mask && endpoint==other.endpoint;
    }
};
struct ComApartment {
    HRESULT result=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    ~ComApartment(){if(SUCCEEDED(result))CoUninitialize();}
};
std::atomic<AudioLayout> requested_layout{AudioLayout::Stereo};
std::atomic<AudioOutput> active_output{AudioOutput::Silent};
std::atomic<bool> requested_remaster{false};

WaveTarget resolve_output(AudioLayout requested) {
    if(requested==AudioLayout::Stereo)return {};
    const ComApartment apartment;
    if(FAILED(apartment.result) && apartment.result!=RPC_E_CHANGED_MODE)return {};

    ComPtr<IMMDeviceEnumerator> enumerator;
    ComPtr<IMMDevice> endpoint;
    ComPtr<IAudioClient> client;
    if(FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_INPROC_SERVER,
                              IID_PPV_ARGS(enumerator.GetAddressOf()))) ||
       FAILED(enumerator->GetDefaultAudioEndpoint(eRender,eConsole,endpoint.GetAddressOf())) ||
       FAILED(endpoint->Activate(__uuidof(IAudioClient),CLSCTX_INPROC_SERVER,nullptr,
                                reinterpret_cast<void **>(client.GetAddressOf()))))return {};
    WAVEFORMATEX *format=nullptr;
    if(FAILED(client->GetMixFormat(&format)) || !format)return {};
    const auto mask=format->wFormatTag==WAVE_FORMAT_EXTENSIBLE &&
                    format->cbSize>=sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)
        ? audio_surround_endpoint_mask(requested,format->nChannels,
                                      reinterpret_cast<WAVEFORMATEXTENSIBLE *>(format)->dwChannelMask)
        : 0u;
    CoTaskMemFree(format);
    if(!mask)return {};

    LPWSTR raw_id=nullptr;
    if(FAILED(endpoint->GetId(&raw_id)) || !raw_id)return {};
    const std::wstring endpoint_id(raw_id);
    CoTaskMemFree(raw_id);
    const UINT count=waveOutGetNumDevs();
    for(UINT id=0;id<count;++id) {
        const auto device=reinterpret_cast<HWAVEOUT>(static_cast<UINT_PTR>(id));
        ULONG bytes=0;
        if(waveOutMessage(device,DRV_QUERYFUNCTIONINSTANCEIDSIZE,reinterpret_cast<DWORD_PTR>(&bytes),0)!=MMSYSERR_NOERROR ||
           bytes<sizeof(wchar_t) || bytes%sizeof(wchar_t) || bytes>65536)continue;
        std::vector<wchar_t> wave_id(bytes/sizeof(wchar_t)+1);
        if(waveOutMessage(device,DRV_QUERYFUNCTIONINSTANCEID,reinterpret_cast<DWORD_PTR>(wave_id.data()),bytes)==MMSYSERR_NOERROR &&
           endpoint_id==wave_id.data())return {id,requested,mask,endpoint_id};
    }
    return {};
}

struct State {
    struct EffectInfo { unsigned archive,id; uint64_t stop_tick; };
    SdatEngine engine;
    PauseMusic pause_music{engine};
    std::mutex mutex;
    std::atomic<bool> running{false},ready{false};
    std::thread worker;
    std::thread music_worker;
    std::condition_variable music_work;
    std::set<unsigned> queued_music;
    std::map<unsigned,std::shared_ptr<const MusicClip>> music_clips;
    std::map<uint32_t,unsigned> mapped_music;
    int decoding_music=-1;
    bool remastered=false;
    HWAVEOUT device=nullptr;
    std::array<WAVEHDR,blocks> headers{};
    std::array<std::array<int16_t,frames*audio_max_channels>,blocks> buffers{};
    WaveTarget target;
    AudioLayout device_layout=AudioLayout::Stereo;
    AudioLayout last_request=AudioLayout::Stereo;
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
    AudioLayout test_layout=AudioLayout::Stereo;
    std::array<int16_t,frames*audio_max_channels> test_mix{};
    std::array<int16_t,frames*2> stereo_capture{};
    std::array<int,audio_max_channels> channel_peak{};
    uint64_t positioned_starts=0,positioned_updates=0;
};
State *state=nullptr;
void word(FILE *file,uint32_t value,unsigned count){for(unsigned i=0;i<count;++i)std::fputc(uint8_t(value>>(i*8)),file);}
void wave_header(FILE *file,uint32_t size){
    std::fwrite("RIFF",1,4,file);word(file,size+36,4);std::fwrite("WAVEfmt ",1,8,file);
    word(file,16,4);word(file,1,2);word(file,2,2);word(file,hz,4);word(file,hz*4,4);
    word(file,4,2);word(file,16,2);std::fwrite("data",1,4,file);word(file,size,4);
}
void close_device(State &s) {
    if(s.device) {
        waveOutReset(s.device);
        for(unsigned i=0;i<s.prepared;++i)waveOutUnprepareHeader(s.device,&s.headers[i],sizeof(WAVEHDR));
        waveOutClose(s.device);
        s.device=nullptr;
    }
    s.prepared=0;
    s.headers={};
}
void cleanup(State &s){
    close_device(s);
    if(s.capture){std::rewind(s.capture);wave_header(s.capture,uint32_t(s.capture_bytes));std::fclose(s.capture);s.capture=nullptr;}
}

MMRESULT open_device(State &s,const WaveTarget &target) {
    const unsigned channels=audio_channel_count(target.layout);
    WAVEFORMATEXTENSIBLE format{};
    auto &base=format.Format;
    base.wFormatTag=channels==2?WAVE_FORMAT_PCM:WAVE_FORMAT_EXTENSIBLE;
    base.nChannels=static_cast<WORD>(channels);
    base.nSamplesPerSec=hz;
    base.wBitsPerSample=16;
    base.nBlockAlign=static_cast<WORD>(channels*sizeof(int16_t));
    base.nAvgBytesPerSec=hz*base.nBlockAlign;
    if(channels>2) {
        base.cbSize=sizeof(format)-sizeof(base);
        format.Samples.wValidBitsPerSample=16;
        format.dwChannelMask=target.mask;
        format.SubFormat=KSDATAFORMAT_SUBTYPE_PCM;
    }
    auto result=waveOutOpen(&s.device,target.id,&base,0,0,channels==2?CALLBACK_NULL:WAVE_FORMAT_DIRECT);
    if(result!=MMSYSERR_NOERROR){s.device=nullptr;return result;}
    result=waveOutPause(s.device);
    for(unsigned i=0;i<blocks && result==MMSYSERR_NOERROR;++i) {
        s.buffers[i].fill(0);
        auto &header=s.headers[i];
        header.lpData=reinterpret_cast<char *>(s.buffers[i].data());
        header.dwBufferLength=frames*base.nBlockAlign;
        result=waveOutPrepareHeader(s.device,&header,sizeof(WAVEHDR));
        if(result==MMSYSERR_NOERROR) {
            ++s.prepared;
            result=waveOutWrite(s.device,&header,sizeof(WAVEHDR));
        }
    }
    if(result==MMSYSERR_NOERROR)result=waveOutRestart(s.device);
    if(result!=MMSYSERR_NOERROR)close_device(s);
    else s.device_layout=target.layout;
    return result;
}

void configure_device(State &s,WaveTarget target,AudioLayout requested) {
    close_device(s);
    s.target=std::move(target);
    s.last_request=requested;
    auto result=open_device(s,s.target);
    const auto surround_result=result;
    if(result!=MMSYSERR_NOERROR && s.target.layout!=AudioLayout::Stereo)
        result=open_device(s,WaveTarget{});
    s.ready=result==MMSYSERR_NOERROR;
    const auto mode=s.ready?static_cast<AudioOutput>(static_cast<unsigned>(s.device_layout)+1):AudioOutput::Silent;
    active_output=mode;
    std::fprintf(stderr,"[native audio output] requested=%s active=%s code=%u surround_code=%u\n",
                 audio_layout_name(requested),audio_output_name(mode),unsigned(result),unsigned(surround_result));
    if(s.ready)std::fprintf(stderr,"[native audio] ready rate=48000 channels=%u bits=16 buffer_frames=2048 music_slots=%u effect_archives=%u\n",
        audio_channel_count(s.device_layout),s.engine.sequence_slots(),s.engine.effect_archives());
}

void capture_block(State &s,const int16_t *mixed,unsigned channels) {
    if(!s.capture || s.capture_bytes+frames*4>128*1024*1024)return;
    if(channels>2) {
        // Keep existing diagnostic WAVs readable as stereo across device
        // changes. The device itself receives the unaltered speaker mix.
        for(unsigned frame=0;frame<frames;++frame) {
            const auto *sample=mixed+frame*channels;
            for(unsigned side=0;side<2;++side) {
                const int surrounds=sample[4+side]+(channels==8?sample[6+side]:0);
                const int value=sample[side]+sample[2]*181/256+surrounds/2;
                s.stereo_capture[frame*2+side]=static_cast<int16_t>(std::clamp(value,-32768,32767));
            }
        }
        mixed=s.stereo_capture.data();
    }
    std::fwrite(mixed,4,frames,s.capture);
    s.capture_bytes+=frames*4;
}

void apply_replacement(State &s,uint32_t slot,unsigned id) {
    if(!s.remastered) {
        s.engine.music_replacement(slot,{});
        return;
    }
    const auto cached=s.music_clips.find(id);
    if(cached!=s.music_clips.end()) {
        if(cached->second && s.engine.music_replacement(slot,cached->second))
            std::fprintf(stderr,"[remastered music] applied id=%u slot=%u tick=%llu\n",id,slot,s.game_ticks);
        return;
    }
    if(s.decoding_music!=static_cast<int>(id) && s.queued_music.insert(id).second)
        s.music_work.notify_one();
}

void update_music_mode(State &s) {
    const bool enabled=requested_remaster.load();
    if(enabled==s.remastered)return;
    s.remastered=enabled;
    if(enabled) {
        // Recheck files that were missing when the previous mode was enabled.
        for(auto it=s.music_clips.begin();it!=s.music_clips.end();) {
            if(!it->second)it=s.music_clips.erase(it);
            else ++it;
        }
    } else s.queued_music.clear();
    for(const auto &[slot,id]:s.mapped_music)apply_replacement(s,slot,id);
    std::fprintf(stderr,"[remastered music] mode=%s tick=%llu\n",enabled?"remastered":"original",s.game_ticks);
}

void release_unused_music(State &s,unsigned id) {
    const auto found=s.music_clips.find(id);
    if(found!=s.music_clips.end() && found->second &&
       std::none_of(s.mapped_music.begin(),s.mapped_music.end(),[id](const auto &entry){return entry.second==id;}))
        s.music_clips.erase(found);
}

void forget_music(State &s,uint32_t slot) {
    const auto found=s.mapped_music.find(slot);
    if(found==s.mapped_music.end())return;
    const auto id=found->second;
    s.mapped_music.erase(found);
    release_unused_music(s,id);
}

void load_music(State &s) {
    std::unique_lock<std::mutex> lock(s.mutex);
    while(s.running) {
        s.music_work.wait(lock,[&]{return !s.running || !s.queued_music.empty();});
        if(!s.running)break;
        const unsigned id=*s.queued_music.begin();
        s.queued_music.erase(s.queued_music.begin());
        s.decoding_music=static_cast<int>(id);
        lock.unlock();
        auto loaded=load_remastered_track(id);
        lock.lock();
        if(!s.running)break;
        s.decoding_music=-1;
        s.music_clips[id]=std::move(loaded.clip);
        const auto &clip=s.music_clips[id];
        std::fprintf(stderr,"[remastered music] id=%u status=%s frames=%zu loop=%zu:%zu\n",
            id,loaded.status,clip?clip->stereo.size()/2:0,clip?clip->loop_begin:0,clip?clip->loop_end:0);
        if(s.remastered && requested_remaster)
            for(const auto &[slot,sequence]:s.mapped_music)if(id==sequence)apply_replacement(s,slot,id);
        release_unused_music(s,id);
    }
}

void worker(State &s){
    auto next_probe=Clock::now()+device_poll_interval;
    auto last_buffer=Clock::now();
    while(s.running){
        const auto now=Clock::now();
        const auto requested=requested_layout.load();
        const bool changed=requested!=s.last_request;
        if(changed || (now>=next_probe && (requested!=AudioLayout::Stereo || !s.device))) {
            auto target=resolve_output(requested);
            if(!(s.target==target) || !s.device) {
                configure_device(s,std::move(target),requested);
                last_buffer=Clock::now();
            } else if(changed) {
                s.last_request=requested;
                std::fprintf(stderr,"[native audio output] requested=%s active=%s reused=1\n",
                    audio_layout_name(requested),audio_output_name(active_output.load()));
            }
            next_probe=Clock::now()+device_poll_interval;
        }
        if(!s.device){Sleep(20);continue;}
        const auto device_channels=audio_channel_count(s.device_layout);
        const auto mix_layout=s.test_layout==AudioLayout::Stereo?s.device_layout:s.test_layout;
        const bool diagnostic=mix_layout!=s.device_layout;
        for(unsigned i=0;i<blocks && s.running;++i)if(s.headers[i].dwFlags&WHDR_DONE){
            const unsigned channels=audio_channel_count(mix_layout);
            auto *mixed=diagnostic?s.test_mix.data():s.buffers[i].data();
            {
                std::lock_guard<std::mutex> lock(s.mutex);
                update_music_mode(s);
                const auto errors=s.engine.errors();
                s.engine.render(mixed,frames,mix_layout);
                if(s.engine.errors()!=errors)std::fprintf(stderr,"[native audio] render-error: %s\n",s.engine.error().c_str());
                for(auto it=s.music.begin();it!=s.music.end();){
                    if(!s.engine.active(it->first)){
                        forget_music(s,it->first);
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
            for(unsigned sample=0;sample<frames*channels;++sample) {
                const auto value=mixed[sample];
                const int peak=std::abs(int(value));
                s.nonzero+=value!=0;s.peak=std::max(s.peak,peak);
                if(channels>2)s.channel_peak[sample%channels]=std::max(s.channel_peak[sample%channels],peak);
            }
            s.rendered+=frames;
            capture_block(s,mixed,channels);
            s.submitted_nonzero+=audio_prepare_submission(s.buffers[i].data(),frames*device_channels,s.output_muted);
            if(waveOutWrite(s.device,&s.headers[i],sizeof(WAVEHDR))!=MMSYSERR_NOERROR) {
                std::fprintf(stderr,"[native audio] device-write failed; retrying default output\n");
                close_device(s);
                break;
            }
            last_buffer=Clock::now();
        }
        if(s.device && Clock::now()-last_buffer>std::chrono::seconds(2)) {
            std::fprintf(stderr,"[native audio] device stopped returning buffers; retrying default output\n");
            close_device(s);
        }
        if(!s.device) {
            s.ready=false;
            active_output=AudioOutput::Silent;
            next_probe=Clock::now()+device_poll_interval;
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
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    const char *test_layout=std::getenv("SM64DS_PC_TEST_AUDIO_LAYOUT");
    if(s->output_muted && automation && std::strcmp(automation,"1")==0 && test_layout) {
        if(std::strcmp(test_layout,"5.1")==0)s->test_layout=AudioLayout::Surround51;
        else if(std::strcmp(test_layout,"7.1")==0)s->test_layout=AudioLayout::Surround71;
    }
    const int id=fs_id_by_path("data/sound_data.sdat");
    const int bytes=fs_size(id);
    if(bytes<=0 || bytes>64*1024*1024){std::fprintf(stderr,"[native audio] local SDAT unavailable; silent fallback\n");return false;}
    std::vector<uint8_t> archive(bytes);
    if(fs_read(id,archive.data(),bytes)!=bytes || !s->engine.load(archive.data(),archive.size())){
        std::fprintf(stderr,"[native audio] archive load failed: %s; silent fallback\n",s->engine.error().c_str());return false;
    }
    const char *no_device=std::getenv("SM64DS_PC_AUDIO_NO_DEVICE");
    if(no_device && no_device[0]=='1') {
        std::fprintf(stderr,"[native audio] device-open failed code=%u; silent fallback\n",unsigned(MMSYSERR_NODRIVER));
        return false;
    }
    const auto requested=requested_layout.load();
    configure_device(*s,resolve_output(requested),requested);
    if(s->test_layout==AudioLayout::Stereo)
        if(const char *path=std::getenv("SM64DS_PC_AUDIO_CAPTURE"))if(*path){s->capture=std::fopen(path,"wb");if(s->capture)wave_header(s->capture,0);}
    s->running=true;
    state=s.release();state->worker=std::thread(worker,std::ref(*state));
    state->music_worker=std::thread(load_music,std::ref(*state));
    std::fprintf(stderr,"[native audio] output_muted=%u (capture precedes device gate)\n",unsigned(state->output_muted));
    return state->ready;
}
void audio_close(){
    if(!state)return;
    state->ready=false;
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->running=false;
    }
    state->music_work.notify_one();
    if(state->worker.joinable())state->worker.join();
    if(state->music_worker.joinable())state->music_worker.join();
    MMTIME time{};time.wType=TIME_SAMPLES;
    const auto result=state->device?waveOutGetPosition(state->device,&time,sizeof time):MMSYSERR_NODRIVER;
    std::fprintf(stderr,"[native audio] shutdown rendered=%llu nonzero=%llu peak=%d starts=%llu errors=%llu device_position_type=%u device_position=%u query=%u completed=%llu game_ticks=%llu\n",
        state->rendered,state->nonzero,state->peak,state->starts,state->engine.errors(),time.wType,time.u.sample,result,state->completed,state->game_ticks);
    std::fprintf(stderr,"[native audio] submission output_muted=%u nonzero_samples=%llu\n",unsigned(state->output_muted),state->submitted_nonzero);
    if(state->test_layout!=AudioLayout::Stereo) {
        const unsigned channels=audio_channel_count(state->test_layout);
        std::fprintf(stderr,"[native audio spatial QA] channels=%u mask=%x positioned_starts=%llu positioned_updates=%llu peaks=",
            channels,unsigned(audio_channel_mask(state->test_layout)),state->positioned_starts,state->positioned_updates);
        for(unsigned i=0;i<channels;++i)std::fprintf(stderr,"%s%d",i?",":"",state->channel_peak[i]);
        std::fputc('\n',stderr);
    }
    cleanup(*state);delete state;state=nullptr;
    active_output=AudioOutput::Silent;
}
bool audio_ready(){return state && state->ready;}
void audio_set_layout(AudioLayout layout){requested_layout=layout;}
AudioOutput audio_output_status(){return active_output.load();}
void audio_set_remastered_music(bool enabled){requested_remaster=enabled;}
uint32_t audio_sequence(uint32_t key,unsigned id){
    if(!audio_ready())return 0;std::lock_guard<std::mutex> lock(state->mutex);
    const auto voice=state->engine.play_sequence(id,key);
    if(voice) {
        state->music[voice]=true;
        state->pause_music.music_started(voice);
        const auto old=state->mapped_music.find(voice);
        if(old!=state->mapped_music.end() && old->second!=id)forget_music(*state,voice);
        if(named_music_track(id)) {
            state->mapped_music[voice]=id;
            apply_replacement(*state,voice,id);
        }
        ++state->starts;
    }
    if(state->starts<3000)std::fprintf(stderr,"[native audio] sequence id=%u name=%s key=%u result=%u error=%s tick=%llu\n",id,ntr::music_track_name(id)?ntr::music_track_name(id):"other",key,voice,voice?"":state->engine.error().c_str(),state->game_ticks);
    return voice;
}
static uint32_t start_effect(unsigned group,unsigned id,int bank,unsigned max_game_ticks,uint32_t key,
                             const AudioPosition *position=nullptr){
    if(!audio_ready())return 0;std::lock_guard<std::mutex> lock(state->mutex);
    const auto voice=state->engine.play_effect(group,id,bank,key);
    if(voice){forget_music(*state,voice);state->music[voice]=false;state->effects[voice]={group,id,max_game_ticks?state->game_ticks+max_game_ticks:0};++state->starts;}
    if(voice && position){state->engine.effect_position(voice,*position);++state->positioned_starts;}
    if(voice && group==3 && id==17 && std::getenv("SM64DS_PC_AUDIO_TRACE"))
        std::fprintf(stderr,"[native audio event] coin-pickup start key=%u runtime_frame=%llu audio_tick=%llu\n",
            voice,static_cast<unsigned long long>(rt_frame()),static_cast<unsigned long long>(state->game_ticks));
    if(state->starts<3000)std::fprintf(stderr,"[native audio] effect archive=%u id=%u bank=%d result=%u error=%s tick=%llu\n",group,id,bank,voice,voice?"":state->engine.error().c_str(),state->game_ticks);
    return voice;
}
uint32_t audio_effect(unsigned group,unsigned id,int bank,uint32_t key){
    return start_effect(group,id,bank,0,key);
}
uint32_t audio_effect_at(unsigned group,unsigned id,int bank,AudioPosition position){
    return start_effect(group,id,bank,0,0,&position);
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
uint32_t audio_long_effect(uint32_t key,unsigned group,unsigned id,int bank,const AudioPosition *position){
    if(!audio_ready())return 0;std::lock_guard<std::mutex> lock(state->mutex);
    if(!state->engine.active(key)){
        key=state->engine.play_effect(group,id,bank);if(key)++state->starts;
        if(std::getenv("SM64DS_PC_AUDIO_LONG_TRACE"))std::fprintf(stderr,
            "[native long audio] event=start tick=%llu archive=%u id=%u bank=%d key=%u error=%s\n",
            static_cast<unsigned long long>(rt_frame()),group,id,bank,key,key?"":state->engine.error().c_str());
    }
    if(key){state->long_refs[key]=state->game_ticks;state->music[key]=false;}
    if(key && position){state->engine.effect_position(key,*position);++state->positioned_updates;}
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
void audio_stop(uint32_t key,int frames){if(audio_ready()){std::lock_guard<std::mutex> lock(state->mutex);state->engine.stop(key,frames);if(frames<=0){forget_music(*state,key);state->music.erase(key);}if(frames>0)std::fprintf(stderr,"[native audio] stop-fade key=%u frames=%d tick=%llu\n",key,frames,state->game_ticks);else if(std::getenv("SM64DS_PC_AUDIO_TRACE"))std::fprintf(stderr,"[native audio] stop key=%u tick=%llu\n",key,state->game_ticks);}}
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
void audio_stop_all(){if(audio_ready()){std::lock_guard<std::mutex> lock(state->mutex);state->pause_music.reset();state->engine.stop_all();state->music.clear();state->effects.clear();state->long_refs.clear();state->mapped_music.clear();state->queued_music.clear();for(auto it=state->music_clips.begin();it!=state->music_clips.end();)if(it->second)it=state->music_clips.erase(it);else ++it;if(std::getenv("SM64DS_PC_AUDIO_TRACE"))std::fprintf(stderr,"[native audio] stop-all tick=%llu\n",state->game_ticks);}}
}
