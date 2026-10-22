#include "ntr/sdl_gamepad.h"
#include "ntr/controller_bindings.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#define CHECK(test) do {if(!(test)){std::fprintf(stderr,"FAIL line %d: %s (%s)\n",__LINE__,#test,SDL_GetError());return 1;}}while(0)
int main(int argc,char **argv) {
    if(argc>1 && !std::strcmp(argv[1],"--watch")) {
        for(unsigned i=0;i<1800;++i) {
            ntr::InputState state{};const bool live=ntr::sdl_gamepad_poll(state);
            if(i%30==0) {std::printf("pad=%d buttons=%04x left=%d,%d right=%d,%d\n",live,state.buttons,state.left_x,state.left_y,state.right_x,state.right_y);std::fflush(stdout);}
            SDL_Delay(16);
        }
        return 0;
    }
    // Test only the virtual device. Connected user controllers must not win
    // first-device discovery or make the disconnect assertion nondeterministic.
    // --watch above deliberately retains ordinary physical-device discovery.
    const char *physical_backends[]={SDL_HINT_JOYSTICK_HIDAPI,SDL_HINT_JOYSTICK_RAWINPUT,
        SDL_HINT_JOYSTICK_WGI,SDL_HINT_XINPUT_ENABLED,SDL_HINT_JOYSTICK_DIRECTINPUT,SDL_HINT_JOYSTICK_GAMEINPUT};
    for(const char *hint:physical_backends) {
        SDL_SetHintWithPriority(hint,"0",SDL_HINT_OVERRIDE);
        CHECK(SDL_GetHint(hint) && !std::strcmp(SDL_GetHint(hint),"0"));
    }
    CHECK(SDL_InitSubSystem(SDL_INIT_GAMEPAD));
    int isolated_count=0;auto *isolated_ids=SDL_GetGamepads(&isolated_count);
    CHECK(isolated_count==0);SDL_free(isolated_ids);
    SDL_VirtualJoystickDesc desc{};SDL_INIT_INTERFACE(&desc);
    desc.type=SDL_JOYSTICK_TYPE_GAMEPAD;desc.name="Nintendo Switch Pro Controller QA";
    desc.vendor_id=0x057e;desc.product_id=0x2009;
    desc.naxes=SDL_GAMEPAD_AXIS_COUNT;desc.nbuttons=SDL_GAMEPAD_BUTTON_COUNT;
    desc.button_mask=(1u<<SDL_GAMEPAD_BUTTON_COUNT)-1;desc.axis_mask=(1u<<SDL_GAMEPAD_AXIS_COUNT)-1;
    const auto id=SDL_AttachVirtualJoystick(&desc);CHECK(id);
    isolated_ids=SDL_GetGamepads(&isolated_count);
    CHECK(isolated_count==1 && isolated_ids && isolated_ids[0]==id);SDL_free(isolated_ids);
    auto *joy=SDL_OpenJoystick(id);CHECK(joy);
    auto *pad=SDL_OpenGamepad(id);CHECK(pad);
    std::printf("virtual type=%d south label=%d east label=%d\n",SDL_GetGamepadType(pad),SDL_GetGamepadButtonLabel(pad,SDL_GAMEPAD_BUTTON_SOUTH),SDL_GetGamepadButtonLabel(pad,SDL_GAMEPAD_BUTTON_EAST));
    CHECK(SDL_GetGamepadType(pad)==SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO);
    CHECK(SDL_GetGamepadButtonLabel(pad,SDL_GAMEPAD_BUTTON_EAST)==SDL_GAMEPAD_BUTTON_LABEL_A);
    ntr::InputState state{};CHECK(ntr::sdl_gamepad_poll(state));CHECK(!state.buttons);
    // Physical Nintendo A (east) confirms menus, jumps in gameplay; B attacks.
    CHECK(SDL_SetJoystickVirtualButton(joy,SDL_GAMEPAD_BUTTON_EAST,true));
    CHECK(SDL_SetJoystickVirtualAxis(joy,SDL_GAMEPAD_AXIS_LEFTX,16384));
    CHECK(SDL_SetJoystickVirtualAxis(joy,SDL_GAMEPAD_AXIS_LEFTY,-32768));
    CHECK(SDL_SetJoystickVirtualAxis(joy,SDL_GAMEPAD_AXIS_RIGHTY,12345));
    CHECK(SDL_SetJoystickVirtualAxis(joy,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,32767));
    CHECK(ntr::sdl_gamepad_poll(state));CHECK(state.buttons==0x1000);
    CHECK(state.left_x==16384 && state.left_y==32767 && state.right_y==-12345 && state.right_trigger==255);
    CHECK(ntr::controller_face_buttons(state.buttons,true)==1 && ntr::controller_face_buttons(state.buttons,false)==2);
    CHECK(SDL_SetJoystickVirtualButton(joy,SDL_GAMEPAD_BUTTON_EAST,false));
    CHECK(SDL_SetJoystickVirtualButton(joy,SDL_GAMEPAD_BUTTON_SOUTH,true));
    CHECK(SDL_SetJoystickVirtualButton(joy,SDL_GAMEPAD_BUTTON_WEST,true));
    CHECK(SDL_SetJoystickVirtualButton(joy,SDL_GAMEPAD_BUTTON_RIGHT_STICK,true));
    CHECK(SDL_SetJoystickVirtualButton(joy,SDL_GAMEPAD_BUTTON_DPAD_UP,true));
    CHECK(SDL_SetJoystickVirtualButton(joy,SDL_GAMEPAD_BUTTON_START,true));
    CHECK(ntr::sdl_gamepad_poll(state));CHECK(state.buttons==(0x2000|0x8000|0x80|0x1|0x10));
    CHECK(ntr::controller_face_buttons(state.buttons,false)==(1|0x400));
    SDL_CloseGamepad(pad);SDL_CloseJoystick(joy);CHECK(SDL_DetachVirtualJoystick(id));
    state={};CHECK(!ntr::sdl_gamepad_poll(state));CHECK(!state.buttons && !state.left_x);
    ntr::sdl_gamepad_shutdown();SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    std::puts("PASS: real SDL virtual Nintendo device, A/B label mapping, menu/gameplay buttons, analog axes, triggers, camera click, D-pad and disconnect");
    return 0;
}
