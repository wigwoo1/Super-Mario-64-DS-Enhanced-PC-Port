#include "ntr/sdl_gamepad.h"
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <SDL3/SDL.h>

namespace {
HMODULE library=nullptr;
SDL_Gamepad *gamepad=nullptr;
bool attempted=false,initialized=false;
ULONGLONG next_discovery=0;
#define SDL_FUNCTION(name) decltype(&name) api_##name=nullptr
SDL_FUNCTION(SDL_InitSubSystem);SDL_FUNCTION(SDL_QuitSubSystem);
SDL_FUNCTION(SDL_SetHint);SDL_FUNCTION(SDL_SetGamepadEventsEnabled);
SDL_FUNCTION(SDL_SetJoystickEventsEnabled);SDL_FUNCTION(SDL_UpdateGamepads);
SDL_FUNCTION(SDL_GetGamepads);SDL_FUNCTION(SDL_free);SDL_FUNCTION(SDL_OpenGamepad);
SDL_FUNCTION(SDL_CloseGamepad);SDL_FUNCTION(SDL_GamepadConnected);
SDL_FUNCTION(SDL_GetGamepadButton);SDL_FUNCTION(SDL_GetGamepadAxis);
SDL_FUNCTION(SDL_GetGamepadButtonLabel);SDL_FUNCTION(SDL_GetGamepadName);
SDL_FUNCTION(SDL_GetError);
#undef SDL_FUNCTION

bool initialize() {
    if(attempted)return initialized;
    attempted=true;
    wchar_t path[32768];const DWORD length=GetModuleFileNameW(nullptr,path,32768);
    if(!length || length>=32768)return false;
    wchar_t *slash=wcsrchr(path,L'\\');if(!slash)return false;
    if(wcscpy_s(slash+1,32768-size_t(slash+1-path),L"SDL3.dll"))return false;
    library=LoadLibraryExW(path,nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!library)return false; // Existing ROM-free kits can still use XInput.
#define SDL_LOAD(name) api_##name=reinterpret_cast<decltype(api_##name)>(GetProcAddress(library,#name));if(!api_##name){FreeLibrary(library);library=nullptr;return false;}
    SDL_LOAD(SDL_InitSubSystem);SDL_LOAD(SDL_QuitSubSystem);SDL_LOAD(SDL_SetHint);
    SDL_LOAD(SDL_SetGamepadEventsEnabled);SDL_LOAD(SDL_SetJoystickEventsEnabled);
    SDL_LOAD(SDL_UpdateGamepads);SDL_LOAD(SDL_GetGamepads);SDL_LOAD(SDL_free);
    SDL_LOAD(SDL_OpenGamepad);SDL_LOAD(SDL_CloseGamepad);SDL_LOAD(SDL_GamepadConnected);
    SDL_LOAD(SDL_GetGamepadButton);SDL_LOAD(SDL_GetGamepadAxis);
    SDL_LOAD(SDL_GetGamepadButtonLabel);SDL_LOAD(SDL_GetGamepadName);SDL_LOAD(SDL_GetError);
#undef SDL_LOAD
    api_SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_SWITCH,"1");
    api_SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
    initialized=api_SDL_InitSubSystem(SDL_INIT_GAMEPAD);
    if(!initialized)std::fprintf(stderr,"[controller] SDL initialization failed: %s\n",api_SDL_GetError());
    else {
        api_SDL_SetGamepadEventsEnabled(false);api_SDL_SetJoystickEventsEnabled(false);
        std::atexit(ntr::sdl_gamepad_shutdown);
        std::fprintf(stderr,"[controller] SDL HID gamepad support ready (Nintendo Bluetooth/USB, Xbox and mapped controllers)\n");
    }
    return initialized;
}
uint16_t face_mask(SDL_GamepadButton button,uint16_t position) {
    switch(api_SDL_GetGamepadButtonLabel(gamepad,button)) {
    case SDL_GAMEPAD_BUTTON_LABEL_A:return 0x1000;
    case SDL_GAMEPAD_BUTTON_LABEL_B:return 0x2000;
    case SDL_GAMEPAD_BUTTON_LABEL_X:return 0x4000;
    case SDL_GAMEPAD_BUTTON_LABEL_Y:return 0x8000;
    default:return position;
    }
}
int16_t invert_axis(int16_t value) {return value==-32768?32767:static_cast<int16_t>(-value);}
}
#endif

namespace ntr {
bool sdl_gamepad_poll(InputState &state) {
#if defined(_WIN32)
    if(!initialize())return false;
    api_SDL_UpdateGamepads();
    if(gamepad && !api_SDL_GamepadConnected(gamepad)) {
        api_SDL_CloseGamepad(gamepad);gamepad=nullptr;next_discovery=0;
        std::fprintf(stderr,"[controller] gamepad disconnected\n");
    }
    if(!gamepad && GetTickCount64()>=next_discovery) {
        next_discovery=GetTickCount64()+500;
        int count=0;auto *ids=api_SDL_GetGamepads(&count);
        for(int i=0;ids && i<count && !gamepad;++i)gamepad=api_SDL_OpenGamepad(ids[i]);
        api_SDL_free(ids);
        if(gamepad)std::fprintf(stderr,"[controller] connected: %s\n",api_SDL_GetGamepadName(gamepad));
    }
    if(!gamepad)return false;
    uint16_t buttons=0;
    const SDL_GamepadButton faces[]={SDL_GAMEPAD_BUTTON_SOUTH,SDL_GAMEPAD_BUTTON_EAST,SDL_GAMEPAD_BUTTON_WEST,SDL_GAMEPAD_BUTTON_NORTH};
    for(unsigned i=0;i<4;++i)if(api_SDL_GetGamepadButton(gamepad,faces[i]))buttons|=face_mask(faces[i],uint16_t(0x1000u<<i));
    const struct {SDL_GamepadButton button;uint16_t mask;} rest[]={
        {SDL_GAMEPAD_BUTTON_DPAD_UP,1},{SDL_GAMEPAD_BUTTON_DPAD_DOWN,2},
        {SDL_GAMEPAD_BUTTON_DPAD_LEFT,4},{SDL_GAMEPAD_BUTTON_DPAD_RIGHT,8},
        {SDL_GAMEPAD_BUTTON_START,0x10},{SDL_GAMEPAD_BUTTON_BACK,0x20},
        {SDL_GAMEPAD_BUTTON_LEFT_STICK,0x40},{SDL_GAMEPAD_BUTTON_RIGHT_STICK,0x80},
        {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,0x100},{SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,0x200}};
    for(const auto &binding:rest)if(api_SDL_GetGamepadButton(gamepad,binding.button))buttons|=binding.mask;
    state.buttons=buttons;
    state.left_x=api_SDL_GetGamepadAxis(gamepad,SDL_GAMEPAD_AXIS_LEFTX);
    state.left_y=invert_axis(api_SDL_GetGamepadAxis(gamepad,SDL_GAMEPAD_AXIS_LEFTY));
    state.right_x=api_SDL_GetGamepadAxis(gamepad,SDL_GAMEPAD_AXIS_RIGHTX);
    state.right_y=invert_axis(api_SDL_GetGamepadAxis(gamepad,SDL_GAMEPAD_AXIS_RIGHTY));
    state.left_trigger=uint8_t(std::max(0,int(api_SDL_GetGamepadAxis(gamepad,SDL_GAMEPAD_AXIS_LEFT_TRIGGER)))*255/32767);
    state.right_trigger=uint8_t(std::max(0,int(api_SDL_GetGamepadAxis(gamepad,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)))*255/32767);
    return true;
#else
    (void)state;return false;
#endif
}
void sdl_gamepad_shutdown() {
#if defined(_WIN32)
    if(gamepad){api_SDL_CloseGamepad(gamepad);gamepad=nullptr;}
    if(initialized){api_SDL_QuitSubSystem(SDL_INIT_GAMEPAD);initialized=false;}
    if(library){FreeLibrary(library);library=nullptr;}
#endif
}
}
