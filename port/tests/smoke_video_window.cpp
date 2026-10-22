#include "ntr/video.h"
#include "ntr/gpu.h"
#include "ntr/mmio.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int main() {
    char directory[MAX_PATH],path[MAX_PATH],title[128];
    if(!GetCurrentDirectoryA(MAX_PATH,directory))return 1;
    std::snprintf(path,sizeof(path),"%s\\sm64ds-window-%lu.ini",directory,GetCurrentProcessId());
    std::snprintf(title,sizeof(title),"SM64DS resolution-window smoke %lu",GetCurrentProcessId());
    if(GetFileAttributesA(path)!=INVALID_FILE_ATTRIBUTES)return 1;
    FILE *file=std::fopen(path,"wb");if(!file)return 2;
    std::fputs("width=256\nheight=192\nwindow_width=900\nwindow_height=600\n",file);std::fclose(file);
    _putenv_s("SM64DS_PC_SETTINGS_FILE",path);_putenv_s("SM64DS_PC_AUTOMATION_ONLY","1");
    _putenv_s("SM64DS_PC_NATIVE_AUDIO","0");
    if(!ntr::io_init() || !ntr::video_open(title))return 3;
    HWND window=FindWindowA("SM64DSNativeVideoWindow",title);
    DWORD owner=0;GetWindowThreadProcessId(window,&owner);
    if(!window || owner!=GetCurrentProcessId())return 4;
    RECT client{};GetClientRect(window,&client);
    if(client.right!=900 || client.bottom!=600)return 5;
    // Exercise this test process's window only. Resizing must not change raster size.
    SetWindowPos(window,nullptr,100,80,1024,768,SWP_NOZORDER|SWP_NOACTIVATE);
    const auto original=ntr::video_get_settings();
    if(original.width!=256 || original.height!=192)return 6;
    GetClientRect(window,&client);const RECT resized_client=client;
    auto check=[&](unsigned width,unsigned height,bool dual) {
        RECT before{},after{};GetWindowRect(window,&before);
        const bool maximized=IsZoomed(window)!=FALSE;
        auto s=ntr::video_get_settings();s.width=width;s.height=height;
        ntr::video_set_settings(s);ntr::video_set_minigame_active(dual);
        if(!ntr::video_present_frame())return false;
        GetWindowRect(window,&after);
        std::vector<uint32_t> pixels;unsigned w=0,h=0;
        if(!ntr::gpu_readback(pixels,w,h) || w!=width || h!=height)return false;
        if(!EqualRect(&before,&after) || (IsZoomed(window)!=FALSE)!=maximized)return false;
        std::printf("PASS raster=%ux%u dual=%u maximized=%u unchanged_window=%ld,%ld,%ld,%ld\n",w,h,dual,maximized,before.left,before.top,before.right,before.bottom);
        return true;
    };
    for(auto size:std::vector<std::pair<unsigned,unsigned>>{{256,192},{1280,720},{1920,1080},{3840,2160}})
        if(!check(size.first,size.second,false))return 7;
    for(unsigned layout=0;layout<3;++layout) {
        auto s=ntr::video_get_settings();s.minigame_layout=static_cast<ntr::MinigameLayout>(layout);ntr::video_set_settings(s);
        if(!check(256,192,true) || !check(3840,2160,true))return 8;
    }
    ntr::video_set_minigame_active(false);
    ShowWindow(window,SW_MAXIMIZE);
    if(!IsZoomed(window) || !check(256,192,false) || !check(3840,2160,false))return 9;
    if(!ntr::video_save_settings())return 10;
    ntr::video_close();
    if(!ntr::video_open(title))return 11;
    window=FindWindowA("SM64DSNativeVideoWindow",title);
    if(!IsZoomed(window) || ntr::video_get_settings().width!=3840)return 12;
    ShowWindow(window,SW_RESTORE);GetClientRect(window,&client);
    if(client.right!=resized_client.right || client.bottom!=resized_client.bottom)return 13;
    ntr::video_close();DeleteFileA(path);
    std::puts("PASS separate window preferences, resized/maximized bounds, Native DS/720p/1080p/4K GPU targets, all dual layouts and restart");
    return 0;
}
