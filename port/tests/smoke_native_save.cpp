#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "SaveData.h"
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <cstdlib>
#include <initializer_list>
extern "C" {
unsigned char data_0209caa0[0x400]={},data_0209f2d8[1]={};
void CpuCopy8(const void *src,void *dst,unsigned size){std::memcpy(dst,src,size);}
void func_0205a588(void *dst,int value,int bytes){std::memset(dst,value,bytes);}
}
extern "C" int func_02013c84(unsigned char,FileSaveData *,int,FileSaveData *);
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL line %u: %s (Windows %lu)\n",__LINE__,#x,GetLastError());return 1;}}while(0)
static bool corrupt(const wchar_t *path) {
    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    SetFilePointer(file,30,nullptr,FILE_BEGIN);const char byte=static_cast<char>(0xa5);DWORD count=0;
    const bool good=WriteFile(file,&byte,1,&count,nullptr)&&count==1;CloseHandle(file);return good;
}
static bool equal(const char *a,const char *b,unsigned size){return !std::memcmp(a,b,size);}
int main(int argc,char **argv) {
    alignas(4) char first[0x44],second[0x44],read[0x2e4],mg[0x2e4];
    SaveData::SetDefaultValues(reinterpret_cast<FileSaveData *>(first));
    first[0x20]=0x39;std::memcpy(second,first,sizeof(first));second[0x20]=0x67;second[0x41]=1;
    if(argc==2 && !std::strcmp(argv[1],"--reopen")) {
        CHECK(SaveData::ReadDataFromCart(read,0x44,0)==0 && equal(read,second,0x44));
        std::puts("PASS child: saved data survives process restart");return 0;
    }
    wchar_t temp[1024],directory[1024];CHECK(GetTempPathW(1024,temp)>0);
    CHECK(swprintf_s(directory,L"%ssm64ds-save-\x03a9-%lu-%llu",temp,GetCurrentProcessId(),GetTickCount64())>0);
    CHECK(CreateDirectoryW(directory,nullptr));CHECK(SetEnvironmentVariableW(L"SM64DS_PC_SAVE_DIRECTORY",directory));
    std::fwprintf(stderr,L"Test fixture: %ls\n",directory);
    std::memset(read,0x7e,sizeof(read));CHECK(SaveData::ReadDataFromCart(read,0x44,0)==2 && read[0]==0x7e);
    CHECK(SaveData::ReadDataFromCart(nullptr,0x44,0)==1);
    CHECK(SaveData::ReadDataFromCart(read,0x44,4)==1 && SaveData::ReadDataFromCart(read,1,0)==1);
    CHECK(SaveData::SaveDataToCart(first,1,0)==1 && SaveData::SaveDataToCart(first,0x44,4)==1);
    CHECK(SaveData::SaveDataToCart(first,0x44,0)==0);
    CHECK(SaveData::ReadDataFromCart(read,0x44,0)==0 && equal(read,first,0x44));
    CHECK(SaveData::SaveDataToCart(second,0x44,0)==0);
    CHECK(SaveData::ReadDataFromCart(read,0x44,0)==0 && equal(read,second,0x44));
    // Launch a fresh process, rather than proving persistence from any cache.
    wchar_t executable[1024],command[2100];CHECK(GetModuleFileNameW(nullptr,executable,1024));
    CHECK(swprintf_s(command,L"\"%s\" --reopen",executable)>0);
    STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION process{};
    CHECK(CreateProcessW(nullptr,command,nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&process));
    CHECK(WaitForSingleObject(process.hProcess,20000)==WAIT_OBJECT_0);
    DWORD result=1;CHECK(GetExitCodeProcess(process.hProcess,&result));CloseHandle(process.hThread);CloseHandle(process.hProcess);CHECK(!result);
    for(unsigned slot=1;slot<3;++slot)CHECK(SaveData::SaveDataToCart(first,0x44,slot)==0);
    SaveData::SetDefaultValuesMg(reinterpret_cast<MinigameSaveData *>(mg));mg[0x100]=0x56;
    CHECK(SaveData::SaveDataToCart(mg,sizeof(mg),3)==0);
    CHECK(SaveData::ReadDataFromCart(read,sizeof(mg),3)==0 && equal(read,mg,sizeof(mg)));
    // Exact retail wrappers must keep their 1=success convention.
    std::memcpy(data_0209caa0,second,sizeof(second));std::memcpy(data_0209caa0+0x44,mg,sizeof(mg));
    data_0209caa0[0x328]=2;CHECK(SaveData::SaveCurrentFile()==1);
    CHECK(SaveData::ReadFileData(2,reinterpret_cast<FileSaveData *>(read))==1 && read[0x20]==0x67);
    wchar_t primary[1100],backup[1100],lock[1100],temporary[1100];
    swprintf_s(primary,L"%s\\slot1.sav",directory);swprintf_s(backup,L"%s.bak",primary);
    swprintf_s(lock,L"%s.lock",primary);swprintf_s(temporary,L"%s.tmp",primary);
    CHECK(corrupt(primary));CHECK(SaveData::ReadDataFromCart(read,0x44,0)==0 && equal(read,first,0x44));
    CHECK(SaveData::SaveDataToCart(second,0x44,0)==0); // repair must retain good backup
    CHECK(corrupt(primary));CHECK(SaveData::ReadDataFromCart(read,0x44,0)==0 && equal(read,first,0x44));
    CHECK(corrupt(backup));std::memset(read,0x7e,sizeof(read));
    CHECK(SaveData::ReadDataFromCart(read,0x44,0)==1 && read[0]==0x7e);
    CHECK(SaveData::SaveDataToCart(second,0x44,0)==1); // corrupt originals not replaced
    CHECK(DeleteFileW(primary)&&DeleteFileW(backup));
    CHECK(SaveData::SaveDataToCart(first,0x44,0)==0);
    CHECK(CreateDirectoryW(temporary,nullptr)); // blocked temporary write leaves target unchanged
    CHECK(SaveData::SaveDataToCart(second,0x44,0)==1);
    CHECK(SaveData::ReadDataFromCart(read,0x44,0)==0 && equal(read,first,0x44));
    CHECK(RemoveDirectoryW(temporary));
    HANDLE held=CreateFileW(lock,GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    CHECK(held!=INVALID_HANDLE_VALUE);CHECK(SaveData::ReadDataFromCart(read,0x44,0)==1);
    CHECK(SaveData::SaveDataToCart(second,0x44,0)==1);CloseHandle(held);
    CHECK(SaveData::ReadDataFromCart(read,0x44,0)==0 && equal(read,first,0x44));
    // Missing primary recovers backup; failed replacement does not invent data.
    CHECK(SaveData::SaveDataToCart(second,0x44,0)==0);CHECK(DeleteFileW(primary));
    CHECK(SaveData::ReadDataFromCart(read,0x44,0)==0 && equal(read,first,0x44));
    CHECK(SaveData::SaveDataToCart(second,0x44,0)==0);
    CHECK(SetEnvironmentVariableW(L"SM64DS_PC_SAVE_DIRECTORY",primary));
    CHECK(SaveData::SaveDataToCart(second,0x44,0)==1);
    CHECK(SetEnvironmentVariableW(L"SM64DS_PC_SAVE_DIRECTORY",directory));
    // The actual SDK Copy8 convention is source,destination. Exercise the
    // retail helper used by original-menu copy/select commands, not a remake.
    char unchanged[0x44];std::memcpy(unchanged,first,sizeof(first));
    CHECK(func_02013c84(0,reinterpret_cast<FileSaveData *>(first),1,reinterpret_cast<FileSaveData *>(second))==1);
    CHECK(equal(first,unchanged,sizeof(first)) && second[0x20]==0x39 && second[0x41]==3 && (second[4]&1));
    CHECK(SaveData::ReadDataFromCart(read,0x44,1)==0 && equal(read,second,0x44));
    CHECK(func_02013c84(2,reinterpret_cast<FileSaveData *>(second),-1,reinterpret_cast<FileSaveData *>(data_0209caa0))==1);
    CHECK(data_0209caa0[0x328]==2 && data_0209caa0[0x20]==0x39 && data_0209caa0[0x41]==3);
    CHECK(SaveData::EraseSaveFile(1,second)==1);
    CHECK(SaveData::ReadDataFromCart(read,0x44,1)==0 && read[0x20]==0 && read[0x41]==3 && !(read[4]&1));
    // Only this test's flat fixture directory is cleaned, never user saves.
    const wchar_t *names[]={L"slot1",L"slot2",L"slot3",L"minigames"};
    for(const auto *name:names)for(const auto *suffix:{L".sav",L".sav.bak",L".sav.tmp",L".sav.lock"}) {
        wchar_t path[1100];swprintf_s(path,L"%s\\%s%s",directory,name,suffix);DeleteFileW(path);
    }
    CHECK(RemoveDirectoryW(directory));
    std::puts("PASS: independent slots/minigames, real-process reload, CRC/backup recovery, corrupt-file preservation, blocked write, lock contention, Unicode path, retail SaveCurrentFile");
}
