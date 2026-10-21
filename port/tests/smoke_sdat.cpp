#include "ntr/sdat.h"
#include "ntr/audio_ramp.h"
#include "../third_party/sseqplayer/Player.h"
#include <cstdio>
#include <stdexcept>
#include <vector>

#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"sound check failed at line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void u16(std::vector<uint8_t> &v,unsigned value){v.push_back(uint8_t(value));v.push_back(uint8_t(value>>8));}
static void u32(std::vector<uint8_t> &v,unsigned value){u16(v,value);u16(v,value>>16);}
static SWAV wave(unsigned format,unsigned loop_offset,unsigned words,const std::vector<uint8_t> &samples){
    std::vector<uint8_t> bytes{uint8_t(format),uint8_t(loop_offset!=0)};
    u16(bytes,8000);u16(bytes,2000);u16(bytes,loop_offset);u32(bytes,words);
    bytes.insert(bytes.end(),samples.begin(),samples.end());
    PseudoFile file;file.data=&bytes;SWAV result;result.Read(file);return result;
}
static void put32(std::vector<uint8_t> &b,size_t p,unsigned n){for(unsigned i=0;i<4;++i)b[p+i]=uint8_t(n>>(8*i));}
static void tag(std::vector<uint8_t> &b,size_t p,const char *s){for(unsigned i=0;i<4;++i)b[p+i]=uint8_t(s[i]);}
static void header(std::vector<uint8_t> &b,const char *type){tag(b,0,type);put32(b,4,0x0100feff);put32(b,8,unsigned(b.size()));b[12]=16;b[14]=1;}
// Entirely synthetic SDAT: silent resting sequences share an empty bank.
// INFO defines player0 limit1, player1 limit2, player2 disabled.
static std::vector<uint8_t> player_fixture(){
    std::vector<uint8_t> info(40);tag(info,0,"INFO");
    auto table=[&](unsigned index,unsigned count){
        const auto at=info.size();put32(info,8+index*4,unsigned(at));
        u32(info,count);info.resize(info.size()+count*4);return at;
    };
    auto seq=table(0,6);
    const unsigned priorities[]={20,80,20,10,20,20};
    const unsigned players[]={0,0,0,0,1,2};
    for(unsigned i=0;i<6;++i){
        put32(info,seq+4+i*4,unsigned(info.size()));u32(info,1);u16(info,0);
        info.push_back(127);info.push_back(96);info.push_back(uint8_t(priorities[i]));info.push_back(uint8_t(players[i]));u16(info,0);
    }
    auto banks=table(2,1);put32(info,banks+4,unsigned(info.size()));u32(info,0);
    for(unsigned i=0;i<4;++i)u16(info,0xffff);
    auto policy=table(4,3);
    for(unsigned i=0;i<3;++i){put32(info,policy+4+i*4,unsigned(info.size()));u16(info,i==0?1:i==1?2:0);u16(info,0);u32(info,0);}
    put32(info,4,unsigned(info.size()));
    std::vector<uint8_t> bank(60);header(bank,"SBNK");tag(bank,16,"DATA");put32(bank,20,44);
    std::vector<uint8_t> sequence(32);header(sequence,"SSEQ");tag(sequence,16,"DATA");put32(sequence,20,16);put32(sequence,24,28);
    sequence[28]=0x80;sequence[29]=0x87;sequence[30]=0x68;sequence[31]=0xff;
    std::vector<uint8_t> fat(44);tag(fat,0,"FAT ");put32(fat,4,44);put32(fat,8,2);
    const unsigned fat_at=64+unsigned(info.size()),bank_at=fat_at+44;
    put32(fat,12,bank_at);put32(fat,16,unsigned(bank.size()));put32(fat,28,bank_at+unsigned(bank.size()));put32(fat,32,unsigned(sequence.size()));
    std::vector<uint8_t> result(64);header(result,"SDAT");put32(result,0x18,64);put32(result,0x20,fat_at);
    for(const auto *block:{&info,&fat,&bank,&sequence})result.insert(result.end(),block->begin(),block->end());
    put32(result,8,unsigned(result.size()));return result;
}
int main(){
    ntr::AudioVolumeRamp ramp;
    ramp.set(0,4);CHECK(ramp.value()==127 && !ramp.done());
    ramp.tick();CHECK(ramp.value()==95);
    ramp.tick();CHECK(ramp.value()==63);
    ramp.set(127,2);ramp.tick();CHECK(ramp.value()==95);
    ramp.tick();CHECK(ramp.value()==127 && ramp.done());
    ramp.set(-1,0);CHECK(ramp.value()==0 && ramp.done());
    ramp.set(999,1);ramp.tick();CHECK(ramp.value()==127 && ramp.done());
    ramp.set(0,2147483647);ramp.tick();CHECK(ramp.value()==127 && !ramp.done());
    auto pcm16=wave(1,0,2,{0,128,255,255,0,0,255,127});
    CHECK(pcm16.data==std::vector<int16_t>({-32768,-1,0,32767}));
    auto pcm8=wave(0,0,1,{128,255,0,127});
    CHECK(pcm8.data==std::vector<int16_t>({-32768,-256,0,32512}));
    auto adpcm=wave(2,0,2,{0x18,0xfc,0,0,0x11,0x11,0x99,0x99});
    CHECK(adpcm.data==std::vector<int16_t>({-999,-998,-997,-996,-997,-998,-999,-1000}));
    CHECK(adpcm.loopOffset==0 && adpcm.nonLoopLength==adpcm.data.size());
    auto loop=wave(2,1,1,{0x18,0xfc,0,0,0x11,0x11,0x99,0x99});
    CHECK(loop.loopOffset==0 && loop.nonLoopLength==loop.data.size());
    bool rejected=false;
    try{wave(2,0,2,{0,0,89,0,0,0,0,0});}catch(const std::runtime_error&){rejected=true;}
    CHECK(rejected);rejected=false;
    try{wave(1,0,2,{0,0});}catch(const std::runtime_error&){rejected=true;}
    CHECK(rejected);
    rejected=false;
    try{wave(1,0,0x40000000u,{});}catch(const std::runtime_error&){rejected=true;}
    CHECK(rejected);rejected=false;
    try{wave(2,0,1,{0,0,0,0});}catch(const std::runtime_error&){rejected=true;}
    CHECK(rejected);
    ntr::SdatEngine engine;
    uint8_t bad[64]={};CHECK(!engine.load(bad,sizeof bad));CHECK(!engine.error().empty());
    CHECK(!engine.play_sequence(0) && !engine.play_effect(0,0));
    int16_t silence[128];for(auto &v:silence)v=123;
    engine.render(silence,64);for(auto v:silence)CHECK(v==0);
    // A sequence that jumps forever without yielding must fail, not hang audio.
    SSEQ seq;seq.data={0x94,0,0,0};PcSdatPlayer player;player.sampleRate=48000;
    CHECK(player.Setup(&seq));std::vector<uint8_t> out(4096);rejected=false;
    try{player.GenerateSamples(out,0,1024);}catch(const std::runtime_error&){rejected=true;}
    CHECK(rejected);
    seq.data={0x80,0x80};PcSdatPlayer truncated;truncated.sampleRate=48000;
    CHECK(truncated.Setup(&seq));rejected=false;
    try{truncated.GenerateSamples(out,0,1024);}catch(const std::runtime_error&){rejected=true;}
    CHECK(rejected);
    // A zero gate with note-wait chains samples by their actual lifetimes.
    // Keep the first synthetic sample alive across multiple sequencer ticks.
    SWAR samples;auto held=wave(0,0,2000,std::vector<uint8_t>(8000,64));
    samples.swavs.emplace(0,held);SBNK bank;bank.waveArc[0]=&samples;
    SBNKInstrument instrument;instrument.record=1;
    SBNKInstrumentRange range(0,127,1);range.swav=0;range.swar=0;
    range.noteNumber=60;range.attackRate=127;range.decayRate=127;
    range.sustainLevel=127;range.releaseRate=127;range.pan=64;
    instrument.ranges.push_back(range);bank.instruments.push_back(instrument);
    SSEQ chain;chain.bank=&bank;chain.data={60,127,0,0xff};
    PcSdatPlayer chaining;chaining.sampleRate=48000;CHECK(chaining.Setup(&chain));
    chaining.GenerateSamples(out,0,1024);
    CHECK(chaining.tracks[0].waitForVoices && !chaining.tracks[0].state[TS_END]);
    for(unsigned block=0;block<96;++block)chaining.GenerateSamples(out,0,1024);
    CHECK(!chaining.tracks[0].waitForVoices && chaining.tracks[0].state[TS_END]);
    chain.data={0xc7,0,60,127,0,0xff};
    PcSdatPlayer overlap;overlap.sampleRate=48000;CHECK(overlap.Setup(&chain));
    overlap.GenerateSamples(out,0,1024);
    CHECK(!overlap.tracks[0].waitForVoices && overlap.tracks[0].state[TS_END]);
    bool sample_tail=false;
    for(const auto &channel:overlap.channels)sample_tail|=channel.state>CS_NONE;
    CHECK(sample_tail); // End must preserve an ordinary sample's natural tail.
    // A tied looping voice has an indefinite gate, so the track's end must
    // release it. Otherwise an ended track keeps a sustained tone forever.
    samples.swavs[0].loop=1;
    chain.data={0xc8,1,60,127,12,0xff};
    PcSdatPlayer tied;tied.sampleRate=48000;CHECK(tied.Setup(&chain));
    for(unsigned block=0;block<96;++block)tied.GenerateSamples(out,0,1024);
    CHECK(tied.tracks[0].state[TS_END]);
    for(const auto &channel:tied.channels)CHECK(channel.state==CS_NONE);
    // Ending the tied track must not release another track's finite note.
    chain.data={0xfe,3,0,0x93,1,14,0,0,0xc8,1,60,127,12,0xff,60,127,120,0xff};
    PcSdatPlayer independent;independent.sampleRate=48000;CHECK(independent.Setup(&chain));
    for(unsigned block=0;block<24;++block)independent.GenerateSamples(out,0,1024);
    CHECK(independent.nTracks==2 && independent.tracks[0].state[TS_END]);
    bool other_track_active=false;
    for(const auto &channel:independent.channels) {
        if(channel.trackId==0)CHECK(channel.state==CS_NONE);
        if(channel.trackId==1 && channel.state>CS_NONE)other_track_active=true;
    }
    CHECK(other_track_active);
    auto archive=player_fixture();ntr::SdatEngine admission;
    CHECK(admission.load(archive.data(),archive.size()));
    CHECK(admission.play_sequence(0,100)==100);
    CHECK(admission.play_sequence(4,200)==200);
    CHECK(admission.play_sequence(1,101)==101);
    CHECK(!admission.active(100) && admission.active(101) && admission.active(200));
    CHECK(!admission.play_sequence(3,102) && admission.active(101)); // Lower priority rejected.
    CHECK(admission.play_sequence(1,103)==103 && !admission.active(101)); // Equal priority replaces oldest.
    CHECK(admission.play_sequence(4,201)==201 && admission.active(200));
    CHECK(admission.play_sequence(4,202)==202 && !admission.active(200));
    CHECK(admission.active(201) && admission.active(202));
    CHECK(admission.play_sequence(4,201)==201 && admission.active(202)); // Explicit handle restart.
    CHECK(!admission.play_sequence(5,203) && admission.errors()==0); // Disabled player.
    std::vector<int16_t> stereo(2048);admission.render(stereo.data(),1024);
    CHECK(admission.active(103) && admission.active(201) && admission.active(202));
    // Corrupt the PLAYER table pointer: parser must reject it before playback.
    put32(archive,64+8+4*4,unsigned(archive.size()-2));
    ntr::SdatEngine malformed;CHECK(!malformed.load(archive.data(),archive.size()));
    std::puts("PASS: PCM8/16 and signed ADPCM vectors, loop lengths, invalid/truncated waves, archive fallback and bounded track execution");
}
