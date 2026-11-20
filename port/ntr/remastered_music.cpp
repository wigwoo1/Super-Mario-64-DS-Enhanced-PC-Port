#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>
#include "ntr/remastered_music.h"
#include "ntr/music_tracks.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

namespace ntr { namespace {
using Microsoft::WRL::ComPtr;
constexpr size_t max_frames=music_sample_rate*60u*10u;
constexpr DWORD audio_stream=MF_SOURCE_READER_FIRST_AUDIO_STREAM;

// Resolve the decoder at runtime so Windows editions without Media
// Foundation can still launch and use the original music.
struct MediaDecoder {
    HRESULT apartment=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    HMODULE platform=nullptr,readwrite=nullptr;
    decltype(&MFShutdown) shutdown=nullptr;
    decltype(&MFCreateMediaType) media_type=nullptr;
    decltype(&MFCreateSourceReaderFromURL) source_reader=nullptr;
    bool start() {
        if(FAILED(apartment) && apartment!=RPC_E_CHANGED_MODE)return false;
        platform=LoadLibraryExW(L"mfplat.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        readwrite=LoadLibraryExW(L"mfreadwrite.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        if(!platform || !readwrite)return false;
        const auto startup=reinterpret_cast<decltype(&MFStartup)>(GetProcAddress(platform,"MFStartup"));
        const auto stop=reinterpret_cast<decltype(&MFShutdown)>(GetProcAddress(platform,"MFShutdown"));
        media_type=reinterpret_cast<decltype(media_type)>(GetProcAddress(platform,"MFCreateMediaType"));
        source_reader=reinterpret_cast<decltype(source_reader)>(GetProcAddress(readwrite,"MFCreateSourceReaderFromURL"));
        if(!startup || !stop || !media_type || !source_reader || FAILED(startup(MF_VERSION,MFSTARTUP_FULL)))return false;
        shutdown=stop;
        return true;
    }
    ~MediaDecoder() {
        if(shutdown)shutdown();
        if(readwrite)FreeLibrary(readwrite);
        if(platform)FreeLibrary(platform);
        if(SUCCEEDED(apartment))CoUninitialize();
    }
};

bool stereo_pcm(IMFSourceReader &reader) {
    ComPtr<IMFMediaType> type;
    UINT32 rate=0,channels=0,bits=0;
    GUID subtype{};
    return SUCCEEDED(reader.GetCurrentMediaType(audio_stream,type.GetAddressOf())) &&
        SUCCEEDED(type->GetGUID(MF_MT_SUBTYPE,&subtype)) && subtype==MFAudioFormat_PCM &&
        SUCCEEDED(type->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND,&rate)) && rate==music_sample_rate &&
        SUCCEEDED(type->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS,&channels)) && channels==2 &&
        SUCCEEDED(type->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,&bits)) && bits==16;
}

bool decode_clip(MediaDecoder &decoder,const std::filesystem::path &path,MusicClip &clip) {
    ComPtr<IMFSourceReader> reader;
    ComPtr<IMFMediaType> type;
    if(FAILED(decoder.source_reader(path.c_str(),nullptr,reader.GetAddressOf())) ||
       FAILED(reader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS,FALSE)) ||
       FAILED(reader->SetStreamSelection(audio_stream,TRUE)) ||
       FAILED(decoder.media_type(type.GetAddressOf())) ||
       FAILED(type->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Audio)) ||
       FAILED(type->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_PCM)) ||
       FAILED(type->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND,music_sample_rate)) ||
       FAILED(type->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS,2)) ||
       FAILED(type->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,16)) ||
       FAILED(type->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT,4)) ||
       FAILED(type->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND,music_sample_rate*4)) ||
       FAILED(reader->SetCurrentMediaType(audio_stream,nullptr,type.Get())) || !stereo_pcm(*reader.Get()))
        return false;

    unsigned empty_reads=0;
    for(;;) {
        DWORD flags=0;
        ComPtr<IMFSample> sample;
        if(FAILED(reader->ReadSample(audio_stream,0,nullptr,&flags,nullptr,sample.GetAddressOf())) ||
           (flags&MF_SOURCE_READERF_ERROR) ||
           ((flags&MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) && !stereo_pcm(*reader.Get())))return false;
        if(sample) {
            ComPtr<IMFMediaBuffer> buffer;
            DWORD length=0;
            if(FAILED(sample->ConvertToContiguousBuffer(buffer.GetAddressOf())) ||
               FAILED(buffer->GetCurrentLength(&length)) || length%4 ||
               length/4>max_frames-clip.stereo.size()/2)return false;
            BYTE *data=nullptr;
            DWORD locked_length=0;
            if(length) {
                const size_t previous=clip.stereo.size();
                clip.stereo.resize(previous+length/sizeof(int16_t));
                if(FAILED(buffer->Lock(&data,nullptr,&locked_length)))return false;
                const bool complete=data && locked_length==length;
                if(complete)std::memcpy(clip.stereo.data()+previous,data,length);
                buffer->Unlock();
                if(!complete)return false;
                empty_reads=0;
            } else ++empty_reads;
        } else ++empty_reads;
        if(flags&MF_SOURCE_READERF_ENDOFSTREAM)break;
        if(empty_reads>64)return false;
    }
    return !clip.stereo.empty();
}

bool load_loop(const std::filesystem::path &file,MusicClip &clip) {
    clip.loop_end=clip.stereo.size()/2;
    std::error_code error;
    if(!std::filesystem::exists(file,error))return !error;
    if(!std::filesystem::is_regular_file(file,error) || error ||
       std::filesystem::file_size(file,error)>256 || error)return false;
    std::ifstream input(file);
    uint64_t begin_ms=0,end_ms=0;
    std::string trailing;
    if(!(input>>begin_ms>>end_ms) || (input>>trailing) || !input.eof() ||
       begin_ms>=end_ms || end_ms>max_frames/(music_sample_rate/1000))return false;
    clip.loop_begin=static_cast<size_t>(begin_ms)*(music_sample_rate/1000);
    clip.loop_end=static_cast<size_t>(end_ms)*(music_sample_rate/1000);
    return clip.valid();
}
} // namespace

RemasteredLoad load_remastered_track(unsigned id) {
    const auto *track=named_music_track(id);
    if(!track)return {};
    try {
        const auto stem=std::filesystem::absolute(std::filesystem::path(L"music-remastered")/track->file_stem);
        auto file=stem;file+=L".mp3";
        std::error_code error;
        if(!std::filesystem::exists(file,error))return {nullptr,error?"file-error":"missing"};
        if(!std::filesystem::is_regular_file(file,error) || error)return {nullptr,"file-error"};
        MediaDecoder decoder;
        if(!decoder.start())return {nullptr,"decoder-unavailable"};
        auto clip=std::make_shared<MusicClip>();
        if(!decode_clip(decoder,file,*clip))return {nullptr,"decode-error"};
        auto loop=stem;loop+=L".loop";
        if(!load_loop(loop,*clip))return {nullptr,"invalid-loop"};
        return {std::move(clip),"decoded"};
    } catch(const std::exception &) {
        return {nullptr,"file-error"};
    }
}
}
