#include "ntr/camera_controls.h"
#include <cmath>
#include <cstdio>
static double turn(unsigned rate,ntr::VideoSettings settings) {
    ntr::FreeCameraResponse response;double total=0;
    for(unsigned i=0;i<rate;++i)total+=response.update(32767,0,settings,1.0/rate).yaw;
    return total;
}
int main() {
    ntr::VideoSettings settings;
    settings.camera_acceleration=0;
    if(std::abs(turn(30,settings)-1.56)>1e-10)return 1;
    settings.camera_sensitivity=200;
    if(std::abs(turn(60,settings)-3.12)>1e-10)return 2;
    settings.camera_invert_x=true;
    if(std::abs(turn(240,settings)+3.12)>1e-10)return 3;
    settings=ntr::VideoSettings{};
    const auto normal=turn(30,settings);
    if(std::abs(normal-turn(60,settings))>1e-10 || std::abs(normal-turn(240,settings))>1e-10)return 4;
    ntr::FreeCameraResponse response;
    const auto first=response.update(32767,32767,settings,1.0/60);
    const auto second=response.update(32767,32767,settings,1.0/60);
    if(first.yaw<=0 || second.yaw<=first.yaw || second.yaw>=1.56/60)return 5;
    if(response.update(0,0,settings,1.0/60).yaw!=0)return 6;
    settings.camera_invert_y=true;settings.camera_acceleration=0;
    const auto inverted=response.update(32767,32767,settings,1.0/60);
    if(inverted.yaw<=0 || inverted.pitch>=0)return 7;
    response.reset();
    const auto dead=response.update(6500,-6500,settings,1.0/60);
    if(dead.yaw!=0 || dead.pitch!=0)return 8;
    const auto partial=response.update(16000,0,settings,1.0/60);
    if(partial.yaw<=0 || partial.yaw>=inverted.yaw/2)return 9;
    if(response.update(32767,32767,settings,0).yaw!=0)return 10;
    settings.camera_acceleration=2;
    if(!(turn(60,settings)<normal))return 11;
    settings.camera_acceleration=0;settings.right_stick_deadzone=30;response.reset();
    if(response.update(9000,0,settings,1.0/60).yaw!=0)return 12;
    response.reset();
    if(response.update(16000,0,settings,1.0/60).yaw<=0)return 13;
    std::puts("PASS free-camera sensitivity, independent inversion, gentle ramp, deadzone, immediate stop, and 30/60/240 update-rate invariance");
}
