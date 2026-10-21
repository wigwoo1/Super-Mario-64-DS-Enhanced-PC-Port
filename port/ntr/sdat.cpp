#include "ntr/sdat.h"
#include "ntr/audio_ramp.h"
#include "../third_party/sseqplayer/Player.h"
#include <algorithm>
#include <array>
#include <map>
#include <stdexcept>
#include <tuple>
#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdlib>

namespace ntr {
namespace {
struct Bytes {
    std::vector<uint8_t> data;
    void span(size_t p,size_t n) const {
        if(p>data.size() || n>data.size()-p)throw std::runtime_error("SDAT section outside local asset");
    }
    unsigned u16(size_t p) const {span(p,2);return data[p]|(unsigned(data[p+1])<<8);}
    unsigned u32(size_t p) const {span(p,4);return u16(p)|(u16(p+2)<<16);}
    void magic(size_t p,const char *text) const {span(p,4);if(std::memcmp(data.data()+p,text,4))throw std::runtime_error("SDAT resource signature mismatch");}
    Bytes slice(size_t p,size_t n) const {span(p,n);return {{data.begin()+p,data.begin()+p+n}};}
};
int bounded_volume(int n) {return std::max(0,std::min(127,n));}
struct Entry {unsigned file=0,bank=0,volume=127,priority=64,channel_priority=64,player=0;bool present=false;};
struct Effect {std::vector<uint8_t> data;std::vector<Entry> entries;std::vector<unsigned> offsets;};
struct Voice {
    uint32_t slot=0;
    AudioVolumeRamp volume;
    bool stopping=false;
    bool paused=false;
    unsigned player_id=0;
    unsigned priority=0;
    std::shared_ptr<SSEQ> sequence;
    std::unique_ptr<PcSdatPlayer> player;
};
}

struct SdatEngine::Impl {
    unsigned rate;
    Bytes archive;
    std::vector<Bytes> files;
    std::vector<Entry> sequences;
    std::vector<Effect> effects;
    std::vector<unsigned> player_limits;
    std::vector<std::unique_ptr<SWAR>> waves;
    std::vector<std::shared_ptr<SBNK>> banks;
    std::map<std::tuple<int,unsigned,unsigned>,std::shared_ptr<SSEQ>> cache;
    std::vector<Voice> voices;
    std::vector<uint8_t> scratch;
    std::vector<int32_t> mix;
    std::string last_error;
    uint64_t error_count=0;
    uint32_t next_slot=256;
    explicit Impl(unsigned hz):rate(hz) {if(hz<8000 || hz>192000)throw std::invalid_argument("SDAT output sample rate");}
    std::vector<unsigned> table(unsigned info,unsigned index) const {
        const unsigned relative=archive.u32(info+8+index*4);
        if(!relative)return {};
        const size_t start=size_t(info)+relative;
        const unsigned count=archive.u32(start);
        if(count>65536)throw std::runtime_error("SDAT INFO table count");
        archive.span(start+4,size_t(count)*4);
        std::vector<unsigned> result(count);
        for(unsigned i=0;i<count;++i) {
            const unsigned offset=archive.u32(start+4+i*4);
            result[i]=offset?info+offset:0;
        }
        return result;
    }
    Entry entry(unsigned offset) const {
        Entry e;if(!offset)return e;
        archive.span(offset,10);
        e.present=true;e.file=archive.u32(offset);e.bank=archive.u16(offset+4);
        e.volume=archive.data[offset+6];e.channel_priority=archive.data[offset+7];e.priority=archive.data[offset+8];e.player=archive.data[offset+9];
        return e;
    }
    Bytes &file(unsigned id) {
        if(id>=files.size())throw std::runtime_error("SDAT file ID outside FAT");
        return files[id];
    }
    template<class T> std::unique_ptr<T> resource(unsigned id,const char *signature) {
        auto &bytes=file(id);bytes.magic(0,signature);
        PseudoFile stream;stream.data=&bytes.data;
        auto result=std::make_unique<T>();result->Read(stream);return result;
    }
    void load(const uint8_t *bytes,size_t size) {
        if(!bytes || size<64 || size>64*1024*1024)throw std::runtime_error("SDAT archive size");
        archive.data.assign(bytes,bytes+size);
        archive.magic(0,"SDAT");
        if(archive.u16(4)!=0xfeff || archive.u16(6)!=0x100 || archive.u32(8)!=size)throw std::runtime_error("SDAT header version or length");
        const unsigned info=archive.u32(0x18),fat=archive.u32(0x20);
        archive.magic(info,"INFO");archive.magic(fat,"FAT ");
        const unsigned count=archive.u32(fat+8);
        if(count>65536)throw std::runtime_error("SDAT FAT count");
        archive.span(fat+12,size_t(count)*16);
        for(unsigned i=0;i<count;++i)files.push_back(archive.slice(archive.u32(fat+12+i*16),archive.u32(fat+16+i*16)));
        const auto players=table(info,4);
        player_limits.assign(players.size(),32); // Legacy fallback for absent records.
        for(unsigned i=0;i<players.size();++i)if(players[i]) {
            archive.span(players[i],8);
            player_limits[i]=archive.u16(players[i]);
        }
        const auto wave_table=table(info,3);
        waves.resize(wave_table.size());
        for(unsigned i=0;i<wave_table.size();++i)if(wave_table[i])waves[i]=resource<SWAR>(archive.u32(wave_table[i]),"SWAR");
        const auto bank_table=table(info,2);
        banks.resize(bank_table.size());
        // The retail resource cache keys banks by FAT file ID. Multiple INFO
        // records can add complementary SWAR bindings to the same bank file.
        std::map<unsigned,std::shared_ptr<SBNK>> bank_files;
        for(unsigned i=0;i<bank_table.size();++i)if(bank_table[i]) {
            const unsigned bank_file=archive.u32(bank_table[i]);
            auto &canonical=bank_files[bank_file];
            if(!canonical){canonical=resource<SBNK>(bank_file,"SBNK");canonical->filename=std::to_string(i);}
            banks[i]=canonical;
            for(unsigned slot=0;slot<4;++slot) {
                const unsigned wave=archive.u16(bank_table[i]+4+slot*2);
                if(wave!=0xffff) {
                    if(wave>=waves.size() || !waves[wave])throw std::runtime_error("SDAT bank references absent wave archive");
                    if(banks[i]->waveArc[slot] && banks[i]->waveArc[slot]!=waves[wave].get())throw std::runtime_error("SDAT shared bank requires runtime wave rebinding");
                    banks[i]->waveArc[slot]=waves[wave].get();
                }
            }
        }
        for(unsigned offset:table(info,0))sequences.push_back(entry(offset));
        for(unsigned offset:table(info,1)) {
            effects.emplace_back();if(!offset)continue;
            auto &bytes=file(archive.u32(offset));bytes.magic(0,"SSAR");
            const unsigned start=bytes.u32(0x18),entries=bytes.u32(0x1c);
            if(entries>65536)throw std::runtime_error("SSAR entry count");
            bytes.span(0x20,size_t(entries)*12);bytes.span(start,0);
            auto &effect=effects.back();effect.data.assign(bytes.data.begin()+start,bytes.data.end());
            for(unsigned i=0;i<entries;++i) {
                const size_t p=0x20+i*12;const unsigned address=bytes.u32(p);
                Entry e;e.present=address!=0xffffffff;e.bank=bytes.u16(p+4);
                e.volume=bytes.data[p+6];e.channel_priority=bytes.data[p+7];e.priority=bytes.data[p+8];e.player=bytes.data[p+9];
                if(e.present && address>=effect.data.size())throw std::runtime_error("SSAR entry outside event data");
                effect.entries.push_back(e);effect.offsets.push_back(address);
            }
        }
    }
    uint32_t play(int group,unsigned id,int bank_override,uint32_t slot,int volume) {
        try {
            Entry e;unsigned offset=0;
            if(group<0) {
                if(id>=sequences.size() || !sequences[id].present)return 0;
                e=sequences[id];
            } else {
                if(unsigned(group)>=effects.size() || id>=effects[group].entries.size() || !effects[group].entries[id].present)return 0;
                e=effects[group].entries[id];offset=effects[group].offsets[id];
            }
            if(bank_override>=0)e.bank=unsigned(bank_override);
            if(e.bank>=banks.size() || !banks[e.bank])throw std::runtime_error("SDAT sequence requires a valid runtime instrument bank");
            const auto key=std::make_tuple(group,id,e.bank);
            auto found=cache.find(key);
            if(found==cache.end()) {
                std::shared_ptr<SSEQ> seq;
                if(group<0)seq=resource<SSEQ>(e.file,"SSEQ");
                else {seq=std::make_shared<SSEQ>();seq->data=effects[group].data;}
                seq->bank=banks[e.bank].get();seq->info.vol=uint8_t(e.volume);
                found=cache.emplace(key,std::move(seq)).first;
            }
            if(!slot)slot=next_slot++;
            const unsigned limit=e.player<player_limits.size()?player_limits[e.player]:32;
            if(!limit)return 0;
            unsigned occupied=0;const Voice *victim=nullptr;
            for(const auto &old:voices)if(old.slot!=slot && old.player_id==e.player) {
                ++occupied;
                // The vector is in start order: equal priorities replace oldest.
                if(!victim || old.priority<victim->priority)victim=&old;
            }
            uint32_t replace=0;
            if(occupied>=limit) {
                if(!victim || victim->priority>e.priority)return 0;
                replace=victim->slot;
            }
            const auto removed=std::count_if(voices.begin(),voices.end(),[&](const Voice &old){return old.slot==slot || old.slot==replace;});
            // Shared hardware-channel arbitration remains a separate check.
            if(voices.size()-removed>=32)throw std::runtime_error("SDAT concurrent player limit");
            Voice v;v.slot=slot;v.volume.set(bounded_volume(volume),0);v.sequence=found->second;v.player_id=e.player;v.priority=e.priority;
            v.player=std::make_unique<PcSdatPlayer>();v.player->sampleRate=rate;
            v.player->prio=uint8_t(e.channel_priority);v.player->interpolation=INTERPOLATION_LINEAR;
            v.player->sseqVol=Cnv_Sust(uint8_t(e.volume));
            if(!v.player->Setup(v.sequence.get()))throw std::runtime_error("SDAT track allocation");
            auto &track=v.player->tracks[v.player->trackIds[0]];
            track.pos=v.sequence->data.data()+offset;
            // Validate/setup the new player before changing existing playback.
            if(replace && std::getenv("SM64DS_PC_AUDIO_TRACE"))
                std::fprintf(stderr,"[native audio arbitration] player=%u limit=%u replaced=%u by=%u priority=%u\n",e.player,limit,replace,slot,e.priority);
            voices.erase(std::remove_if(voices.begin(),voices.end(),[&](const Voice &old){return old.slot==slot || old.slot==replace;}),voices.end());
            voices.push_back(std::move(v));return slot;
        } catch(const std::exception &e) {last_error=e.what();++error_count;return 0;}
    }
    bool finished(const Voice &v) const {
        for(unsigned i=0;i<v.player->nTracks;++i)if(!v.player->tracks[v.player->trackIds[i]].state[TS_END])return false;
        for(const auto &channel:v.player->channels)if(channel.state>CS_NONE)return false;
        return true;
    }
};

SdatEngine::SdatEngine(unsigned rate):impl_(std::make_unique<Impl>(rate)){}
SdatEngine::~SdatEngine()=default;
bool SdatEngine::load(const uint8_t *bytes,size_t size) {
    auto next=std::make_unique<Impl>(impl_->rate);
    try {next->load(bytes,size);impl_=std::move(next);return true;}
    catch(const std::exception &e) {impl_=std::make_unique<Impl>(impl_->rate);impl_->last_error=e.what();++impl_->error_count;return false;}
}
uint32_t SdatEngine::play_sequence(unsigned id,uint32_t slot,int volume){return impl_->play(-1,id,-1,slot,volume);}
uint32_t SdatEngine::play_effect(unsigned archive,unsigned id,int bank,uint32_t slot,int volume){return impl_->play(int(archive),id,bank,slot,volume);}
void SdatEngine::stop(uint32_t slot,int frames){
    auto &voices=impl_->voices;
    if(frames<=0)voices.erase(std::remove_if(voices.begin(),voices.end(),[slot](const Voice &v){return v.slot==slot;}),voices.end());
    else for(auto &v:voices)if(v.slot==slot){v.stopping=true;v.volume.set(0,frames);}
}
void SdatEngine::stop_all(){impl_->voices.clear();}
void SdatEngine::pause(uint32_t slot,bool paused){for(auto &v:impl_->voices)if(v.slot==slot)v.paused=paused;}
void SdatEngine::pause_player(unsigned player,bool paused){for(auto &v:impl_->voices)if(v.player_id==player)v.paused=paused;}
void SdatEngine::stop_player(unsigned player,int frames){
    std::vector<uint32_t> slots;
    for(const auto &v:impl_->voices)if(v.player_id==player)slots.push_back(v.slot);
    for(auto slot:slots)stop(slot,frames);
}
void SdatEngine::volume(uint32_t slot,int value,int frames){for(auto &v:impl_->voices)if(v.slot==slot && !v.stopping)v.volume.set(value,frames);}
void SdatEngine::tick(){
    auto &voices=impl_->voices;
    for(auto &v:voices)v.volume.tick();
    voices.erase(std::remove_if(voices.begin(),voices.end(),[](const Voice &v){return v.stopping && v.volume.done();}),voices.end());
}
void SdatEngine::variable(uint32_t slot,unsigned index,int value){if(index<32)for(auto &v:impl_->voices)if(v.slot==slot)v.player->variables[index]=int16_t(value);}
bool SdatEngine::active(uint32_t slot)const{for(const auto &v:impl_->voices)if(v.slot==slot)return true;return false;}
void SdatEngine::render(int16_t *stereo,size_t frames) {
    if(!stereo || frames>192000)throw std::invalid_argument("SDAT render span");
    auto &m=*impl_;m.mix.assign(frames*2,0);m.scratch.resize(frames*4);
    for(auto it=m.voices.begin();it!=m.voices.end();) {
        bool remove=false;
        if(!it->paused) {
            try {
                it->player->GenerateSamples(m.scratch,0,unsigned(frames));
                for(size_t i=0;i<frames*2;++i) {
                    const int sample=int16_t(unsigned(m.scratch[i*2])|(unsigned(m.scratch[i*2+1])<<8));
                    m.mix[i]+=sample*it->volume.value()/127;
                }
                remove=m.finished(*it);
            } catch(const std::exception &e) {m.last_error=e.what();++m.error_count;remove=true;}
        }
        if(remove)it=m.voices.erase(it);else ++it;
    }
    for(size_t i=0;i<frames*2;++i)stereo[i]=int16_t(std::max(-32768,std::min(32767,m.mix[i])));
}
unsigned SdatEngine::sequence_slots()const{return unsigned(impl_->sequences.size());}
unsigned SdatEngine::effect_archives()const{return unsigned(impl_->effects.size());}
unsigned SdatEngine::effect_slots(unsigned group)const{return group<impl_->effects.size()?unsigned(impl_->effects[group].entries.size()):0;}
bool SdatEngine::has_sequence(unsigned id)const{return id<impl_->sequences.size() && impl_->sequences[id].present;}
bool SdatEngine::has_effect(unsigned group,unsigned id)const{return group<impl_->effects.size() && id<impl_->effects[group].entries.size() && impl_->effects[group].entries[id].present;}
uint64_t SdatEngine::errors()const{return impl_->error_count;}
const std::string &SdatEngine::error()const{return impl_->last_error;}
}
