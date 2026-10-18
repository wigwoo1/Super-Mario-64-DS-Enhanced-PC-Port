// Native disk replacement for cartridge backup I/O. Retail SaveData callers
// retain their 0=good, 1=error, 2=missing read convention and inverted save result.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "SaveData.h"
#include <cstdio>
#include <cstring>
#include <cwchar>

namespace {
constexpr unsigned header_size=24, max_payload=0x2e4, path_capacity=1024;
constexpr unsigned char signature[8]={'S','M','6','4','P','C','0','1'};
unsigned get32(const unsigned char *p) {
    return unsigned(p[0]) | unsigned(p[1])<<8 | unsigned(p[2])<<16 | unsigned(p[3])<<24;
}
void put32(unsigned char *p,unsigned v) {for(unsigned i=0;i<4;++i)p[i]=static_cast<unsigned char>(v>>(i*8));}
unsigned checksum(const unsigned char *p,unsigned n) {
    unsigned crc=~0u;
    for(unsigned i=0;i<n;++i) {
        const unsigned char b=i>=16 && i<20 ? 0 : p[i];
        crc^=b;
        for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u & (0u-(crc&1)));
    }
    return ~crc;
}
bool valid_payload(const char *p,unsigned n,unsigned slot) {
    if(!p || slot>3 || n!=(slot==3 ? max_payload : 0x44u))return false;
    const auto *b=reinterpret_cast<const unsigned char *>(p);
    return get32(b)==(slot==3 ? 0x30303035u : 0x30303038u) && (slot==3 || b[0x41]<4);
}
struct Paths {
    wchar_t file[path_capacity], backup[path_capacity], temporary[path_capacity], lock[path_capacity];
    HANDLE held=INVALID_HANDLE_VALUE;
    ~Paths(){if(held!=INVALID_HANDLE_VALUE)CloseHandle(held);}
    bool open(unsigned slot) {
        wchar_t directory[path_capacity];
        const DWORD configured=GetEnvironmentVariableW(L"SM64DS_PC_SAVE_DIRECTORY",directory,path_capacity);
        if(configured>=path_capacity)return false;
        if(!configured) {
            const DWORD length=GetModuleFileNameW(nullptr,directory,path_capacity);
            if(!length || length>=path_capacity)return false;
            auto *slash=std::wcsrchr(directory,L'\\');if(!slash)return false;
            *slash=0;
            if(wcscat_s(directory,L"\\saves"))return false;
        }
        if(!CreateDirectoryW(directory,nullptr) && GetLastError()!=ERROR_ALREADY_EXISTS)return false;
        const DWORD attributes=GetFileAttributesW(directory);
        if(attributes==INVALID_FILE_ATTRIBUTES || !(attributes&FILE_ATTRIBUTE_DIRECTORY))return false;
        const wchar_t *name=slot==3 ? L"minigames" : slot==0 ? L"slot1" : slot==1 ? L"slot2" : L"slot3";
        if(swprintf_s(file,L"%s\\%s.sav",directory,name)<0 ||
           swprintf_s(backup,L"%s.bak",file)<0 || swprintf_s(temporary,L"%s.tmp",file)<0 ||
           swprintf_s(lock,L"%s.lock",file)<0)return false;
        // Persistent lock filename; deleting it after close would introduce a
        // second-lock race. A crashed process automatically releases its handle.
        held=CreateFileW(lock,GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
        return held!=INVALID_HANDLE_VALUE;
    }
};
int read_file(const wchar_t *path,char *out,unsigned size,unsigned slot) {
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE) {
        const DWORD error=GetLastError();
        return error==ERROR_FILE_NOT_FOUND || error==ERROR_PATH_NOT_FOUND ? 2 : 1;
    }
    unsigned char bytes[header_size+max_payload];DWORD count=0;LARGE_INTEGER length{};
    const bool read=GetFileSizeEx(file,&length) && length.QuadPart==header_size+size &&
        ReadFile(file,bytes,header_size+size,&count,nullptr) && count==header_size+size;
    CloseHandle(file);
    if(!read || std::memcmp(bytes,signature,8) || get32(bytes+8)!=slot || get32(bytes+12)!=size ||
       get32(bytes+20)!=0 || get32(bytes+16)!=checksum(bytes,header_size+size) ||
       !valid_payload(reinterpret_cast<const char *>(bytes+header_size),size,slot))return 1;
    std::memcpy(out,bytes+header_size,size);return 0;
}
}

int SaveData::ReadDataFromCart(char *out,u32 size,u32 slot) {
    if(!out || slot>3 || size!=(slot==3 ? max_payload : 0x44u))return 1;
    Paths paths;if(!paths.open(slot)){std::fprintf(stderr,"[native save] slot=%u storage unavailable or busy\n",slot);return 1;}
    char payload[max_payload];
    const int primary=read_file(paths.file,payload,size,slot);
    const int backup=primary ? read_file(paths.backup,payload,size,slot) : 2;
    if(!primary || !backup) {
        std::memcpy(out,payload,size);
        std::fprintf(stderr,"[native save] slot=%u loaded=%s bytes=%u\n",slot,primary?"backup":"primary",size);
        return 0;
    }
    const int result=primary==2 && backup==2 ? 2 : 1;
    std::fprintf(stderr,"[native save] slot=%u read=%s; files preserved\n",slot,result==2?"missing":"failed");
    return result;
}

int SaveData::SaveDataToCart(char *data,u32 size,u32 slot) {
    if(!valid_payload(data,size,slot))return 1;
    Paths paths;if(!paths.open(slot)){std::fprintf(stderr,"[native save] slot=%u storage unavailable or busy\n",slot);return 1;}
    char previous[max_payload];
    const int primary=read_file(paths.file,previous,size,slot);
    const int backup=read_file(paths.backup,previous,size,slot);
    // Never replace an unrecoverable file with fresh defaults. A good backup
    // permits repair, and remains untouched when the primary itself is invalid.
    if(primary && backup && !(primary==2 && backup==2))return 1;
    unsigned char bytes[header_size+max_payload]={};
    std::memcpy(bytes,signature,8);put32(bytes+8,slot);put32(bytes+12,size);
    std::memcpy(bytes+header_size,data,size);put32(bytes+16,checksum(bytes,header_size+size));
    HANDLE file=CreateFileW(paths.temporary,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return 1;
    DWORD count=0;
    const char *phase="write/flush";
    bool good=WriteFile(file,bytes,header_size+size,&count,nullptr) && count==header_size+size && FlushFileBuffers(file);
    DWORD error=good?0:GetLastError();
    CloseHandle(file);
    if(good){phase="verify";good=!read_file(paths.temporary,previous,size,slot) && !std::memcmp(previous,data,size);error=good?0:GetLastError();}
    if(good) {
        phase="replace";
        if(primary==2)good=MoveFileExW(paths.temporary,paths.file,MOVEFILE_WRITE_THROUGH)!=0;
        else good=ReplaceFileW(paths.file,paths.temporary,primary==0?paths.backup:nullptr,
                              REPLACEFILE_IGNORE_MERGE_ERRORS,nullptr,nullptr)!=0;
        error=good?0:GetLastError();
    }
    if(!good)DeleteFileW(paths.temporary);
    std::fprintf(stderr,"[native save] slot=%u write=%s bytes=%u\n",slot,good?"committed":"failed",size);
    if(!good)std::fwprintf(stderr,L"[native save] failure phase=%hs Windows=%lu path=%ls\n",phase,error,paths.file);
    return good ? 0 : 1;
}

void pc_report_save_error() {
    std::fprintf(stderr,"[native save] Cannot load saved progress; course launch cancelled. Existing files were preserved.\n");
    wchar_t automation[8]={};
    if(GetEnvironmentVariableW(L"SM64DS_PC_AUTOMATION_ONLY",automation,8) && automation[0]==L'1')return;
    MessageBoxW(nullptr,L"Saved progress could not be loaded. The course was not opened, and existing save files were preserved.\n\nClose any other copy of the game and try again. Save files are in the saves folder beside the game.",
                L"Super Mario 64 DS - Save could not be loaded",MB_OK|MB_ICONERROR);
}

extern "C" void pc_report_save_write_error() {
    std::fprintf(stderr,"[native save] Save failed; gameplay continues without reporting success or quitting.\n");
    wchar_t automation[8]={};
    if(GetEnvironmentVariableW(L"SM64DS_PC_AUTOMATION_ONLY",automation,8) && automation[0]==L'1')return;
    MessageBoxW(nullptr,L"Progress could not be saved. The game will stay open so you can try again.\n\nCheck that the saves folder is writable and that another copy of the game is not using it.",
                L"Super Mario 64 DS - Save failed",MB_OK|MB_ICONERROR);
}
