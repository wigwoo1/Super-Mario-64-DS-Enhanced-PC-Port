// Local-asset acceptance tool. Outputs stay private; no ROM data is compiled in.
#include "ntr/sdat.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstring>
#include <stdexcept>
static void word(FILE *f,uint32_t n,unsigned bytes){for(unsigned i=0;i<bytes;++i)std::fputc(uint8_t(n>>(i*8)),f);}
static void wav(FILE *f,unsigned frames){
    std::fwrite("RIFF",1,4,f);word(f,36+frames*4,4);std::fwrite("WAVEfmt ",1,8,f);
    word(f,16,4);word(f,1,2);word(f,2,2);word(f,48000,4);word(f,192000,4);
    word(f,4,2);word(f,16,2);std::fwrite("data",1,4,f);word(f,frames*4,4);
}
int main(int argc,char **argv){
    if(argc<3){std::puts("probe_sdat <local.sdat> --audit OR --players OR --controls OR --effect <archive> <id> <seconds> <output.wav> OR <output.wav> <music-ID> [seconds]");return 2;}
    std::ifstream file(argv[1],std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)),{});
    ntr::SdatEngine engine;if(!engine.load(bytes.data(),bytes.size())){std::fprintf(stderr,"load failed: %s\n",engine.error().c_str());return 1;}
    std::vector<int16_t> buffer(2048);
    if(!std::strcmp(argv[2],"--players")){
        auto check=[](bool ok,const char *message){if(!ok)throw std::runtime_error(message);};
        try {
            const auto music=engine.play_sequence(0);
            const auto motor=engine.play_effect(3,332,32);
            check(music && motor,"original music/motor start");engine.render(buffer.data(),1024);
            const auto stop=engine.play_effect(3,333,32);
            check(stop && !engine.active(motor) && engine.active(music),"player15 replacement or music isolation");
            engine.render(buffer.data(),1024);check(!engine.active(stop),"empty motor-stop cue completion");
            engine.stop_all();const auto shot=engine.play_effect(3,335,32);
            const auto flight=engine.play_effect(0,185);
            check(shot && flight,"original launch/flight start");
            unsigned shot_end=0,flight_end=0;
            for(unsigned block=1;block<=240;++block){
                engine.render(buffer.data(),1024);
                if(!shot_end && !engine.active(shot))shot_end=block*1024;
                if(!flight_end && !engine.active(flight))flight_end=block*1024;
            }
            check(shot_end && flight_end && !engine.errors(),"launch/flight natural completion");
            std::printf("PASS: original player15 motor replacement, empty stop cue and independent music; cannon shot %.3fs/flight %.3fs complete naturally, errors=%llu\n",double(shot_end)/48000,double(flight_end)/48000,engine.errors());return 0;
        } catch(const std::exception &e){std::fprintf(stderr,"player check failed: %s\n",e.what());return 1;}
    }
    if(!std::strcmp(argv[2],"--effect")){
        if(argc<7)return 2;
        const unsigned group=unsigned(std::strtoul(argv[3],nullptr,0));
        const unsigned id=unsigned(std::strtoul(argv[4],nullptr,0));
        const unsigned seconds=unsigned(std::strtoul(argv[5],nullptr,0));
        if(!seconds || seconds>30 || !engine.has_effect(group,id))return 2;
        const int bank=group==3?32:-1;
        const auto voice=engine.play_effect(group,id,bank);
        if(!voice){std::fprintf(stderr,"effect %u:%u failed: %s\n",group,id,engine.error().c_str());return 1;}
        const unsigned frames=48000*seconds;
        FILE *output=std::fopen(argv[6],"wb");if(!output)return 1;
        wav(output,frames);
        uint64_t nonzero=0;int peak=0;unsigned completed_frame=0;
        for(unsigned offset=0;offset<frames;offset+=1024){
            const auto count=std::min(1024u,frames-offset);
            engine.render(buffer.data(),count);
            for(unsigned i=0;i<count*2;++i){nonzero+=buffer[i]!=0;peak=std::max(peak,std::abs(int(buffer[i])));}
            std::fwrite(buffer.data(),4,count,output);
            if(!completed_frame && !engine.active(voice))completed_frame=offset+count;
        }
        std::fclose(output);
        std::printf("effect=%u:%u bank=%d completed_after_seconds=",group,id,bank);
        if(completed_frame)std::printf("%.3f",double(completed_frame)/48000.0);else std::printf(">%u",seconds);
        std::printf(" nonzero=%llu peak=%d active_after=%u errors=%llu\n",nonzero,peak,unsigned(engine.active(voice)),engine.errors());
        return engine.errors()?1:0;
    }
    if(!std::strcmp(argv[2],"--controls")){
        auto check=[](bool ok,const char *message){if(!ok)throw std::runtime_error(message);};
        auto silent=[&](){return std::all_of(buffer.begin(),buffer.end(),[](int16_t x){return x==0;});};
        try{
            const uint32_t slot=12345;
            check(engine.play_sequence(0,slot)==slot,"explicit music handle");
            for(unsigned i=0;i<48;++i)engine.render(buffer.data(),1024);
            check(engine.active(slot) && !silent(),"music did not become audible");
            engine.pause(slot,true);
            for(unsigned i=0;i<48;++i){engine.render(buffer.data(),1024);check(silent(),"paused music emitted samples");}
            check(engine.active(slot),"pause discarded the sequence");
            engine.pause(slot,false);engine.volume(slot,0);
            for(unsigned i=0;i<48;++i){engine.render(buffer.data(),1024);check(silent(),"zero volume emitted samples");}
            engine.volume(slot,127);
            bool resumed=false;for(unsigned i=0;i<48;++i){engine.render(buffer.data(),1024);resumed|=!silent();}
            check(resumed,"unmute/resume remained silent");
            check(engine.play_sequence(0,slot)==slot,"repeat handle replacement");
            engine.stop(slot);engine.render(buffer.data(),1024);
            check(!engine.active(slot) && silent(),"stopped handle still active/audible");
            const auto effect=engine.play_effect(2,0);
            check(effect!=0,"one-shot effect did not start");
            for(unsigned i=0;i<480 && engine.active(effect);++i)engine.render(buffer.data(),1024);
            check(!engine.active(effect),"one-shot effect never completed");
            engine.play_sequence(0);engine.stop_all();engine.render(buffer.data(),1024);
            check(silent() && !engine.errors(),"stop-all/errors");
            check(engine.play_sequence(0,slot)==slot,"fade sequence start");
            engine.stop(slot,4);
            for(unsigned i=0;i<3;++i){engine.tick();check(engine.active(slot),"timed stop completed too early");}
            engine.volume(slot,127); // a normal update cannot cancel an in-flight stop
            engine.tick();check(!engine.active(slot),"timed stop did not finish on fourth update");
            engine.play_sequence(0,slot);engine.pause(slot,true);engine.stop(slot,2);
            engine.tick();check(engine.active(slot),"paused fade completed too early");
            engine.tick();check(!engine.active(slot),"paused fade never completed");
            engine.play_sequence(0,slot);engine.stop_player(255,1);
            engine.tick();check(engine.active(slot),"unrelated player stopped this sequence");
            for(unsigned player=0;player<32;++player)engine.stop_player(player,3);
            engine.tick();engine.tick();check(engine.active(slot),"player stop ignored fade length");
            engine.tick();check(!engine.active(slot),"player fade did not complete");
            engine.play_sequence(0,slot);engine.stop(slot,10);
            engine.play_sequence(0,slot);for(unsigned i=0;i<10;++i)engine.tick();
            check(engine.active(slot),"stale fade stopped a replacement sequence");
            engine.stop_all();engine.render(buffer.data(),1024);
            check(silent() && !engine.errors(),"fade completion errors");
            std::puts("PASS: native music pause/resume, volume/mute, handle replacement, stop, one-shot completion, timed stop/player fades and paused fade completion");return 0;
        }catch(const std::exception &e){std::fprintf(stderr,"controls: %s\n",e.what());return 1;}
    }
    if(!std::strcmp(argv[2],"--audit")){
        unsigned music=0,effects=0;
        const unsigned music_blocks=argc>3?unsigned(std::max(1,std::atoi(argv[3]))*48000+1023)/1024:48;
        const unsigned effect_blocks=argc>4?unsigned(std::max(1,std::atoi(argv[4]))*48000+1023)/1024:6;
        for(unsigned id=0;id<engine.sequence_slots();++id)if(engine.has_sequence(id)){
            engine.stop_all();if(!engine.play_sequence(id)){std::fprintf(stderr,"music %u: %s\n",id,engine.error().c_str());return 1;}
            for(unsigned block=0;block<music_blocks;++block)engine.render(buffer.data(),1024);
            if(engine.errors()){std::fprintf(stderr,"music %u: %s\n",id,engine.error().c_str());return 1;}++music;
        }
        for(unsigned group=0;group<engine.effect_archives();++group)for(unsigned id=0;id<engine.effect_slots(group);++id)if(engine.has_effect(group,id)){
            engine.stop_all();if(!engine.play_effect(group,id,group==3?32:-1)){std::fprintf(stderr,"effect %u:%u: %s\n",group,id,engine.error().c_str());return 1;}
            for(unsigned block=0;block<effect_blocks;++block)engine.render(buffer.data(),1024);
            if(engine.errors()){std::fprintf(stderr,"effect %u:%u: %s\n",group,id,engine.error().c_str());return 1;}++effects;
        }
        std::printf("PASS: %u music sequences (%u blocks each), %u effect starts (%u blocks each); all banks/waves loaded; no decoder/execution errors. Archive3 uses explicit QA bank32.\n",music,music_blocks,effects,effect_blocks);return 0;
    }
    if(argc<4)return 2;
    const unsigned frames=48000*unsigned(argc>4?std::atoi(argv[4]):10);
    auto voice=engine.play_sequence(unsigned(std::atoi(argv[3])));
    if(!voice){std::fprintf(stderr,"sequence failed: %s\n",engine.error().c_str());return 1;}
    FILE *output=std::fopen(argv[2],"wb");if(!output)return 1;wav(output,frames);
    uint64_t nonzero=0;int peak=0;
    for(unsigned offset=0;offset<frames;offset+=1024){
        const auto count=std::min(1024u,frames-offset);engine.render(buffer.data(),count);
        for(unsigned i=0;i<count*2;++i){nonzero+=buffer[i]!=0;peak=std::max(peak,std::abs(int(buffer[i])));}
        std::fwrite(buffer.data(),4,count,output);
    }
    std::fclose(output);
    std::printf("rendered %u frames, nonzero=%llu peak=%d active=%u errors=%llu %s\n",frames,nonzero,peak,unsigned(engine.active(voice)),engine.errors(),engine.error().c_str());
    return engine.errors()||!nonzero?1:0;
}
