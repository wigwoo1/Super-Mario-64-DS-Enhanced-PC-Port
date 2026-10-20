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
    std::puts("PASS: PCM8/16 and signed ADPCM vectors, loop lengths, invalid/truncated waves, archive fallback and bounded track execution");
}
