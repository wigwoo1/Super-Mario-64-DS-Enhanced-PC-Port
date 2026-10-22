#pragma once
#include "ntr/video.h"
#include <algorithm>
#include <cmath>

namespace ntr {
struct CameraOrbitStep { double yaw=0, pitch=0; };
class FreeCameraResponse {
    double x_velocity=0, y_velocity=0;
    static double axis(int raw,double &velocity,unsigned acceleration,double seconds,unsigned deadzone_percent) {
        const double deadzone=std::clamp<unsigned>(deadzone_percent,0,50)*32767.0/100.0;
        const double magnitude=std::clamp((std::abs(raw)-deadzone)/(32767.0-deadzone),0.0,1.0);
        // A small response curve gives fine control near the center. Maximum
        // travel retains the full configured speed, including with smoothing.
        const double curved=0.85*magnitude+0.15*magnitude*magnitude*magnitude;
        const double target=raw<0?-curved:curved;
        if(target==0) {velocity=0;return 0;} // No drift/coasting after release.
        if(target*velocity<0)velocity=0;
        if(!acceleration) {velocity=target;return target*seconds;}
        const double tau=acceleration==2?0.16:0.08;
        const double decay=std::exp(-seconds/tau);
        // Integrate the ramp analytically so its turn angle is independent
        // of camera update/presentation rate.
        const double integral=target*seconds+(velocity-target)*tau*(1-decay);
        velocity=target+(velocity-target)*decay;
        return integral;
    }
public:
    void reset() {x_velocity=y_velocity=0;}
    CameraOrbitStep update(int x,int y,const VideoSettings &settings,double seconds) {
        seconds=std::clamp(seconds,0.0,0.1);
        const double sensitivity=std::clamp<unsigned>(settings.camera_sensitivity,25,300)/100.0;
        return {axis(x,x_velocity,settings.camera_acceleration,seconds,settings.right_stick_deadzone)*1.56*sensitivity*(settings.camera_invert_x?-1:1),
                axis(y,y_velocity,settings.camera_acceleration,seconds,settings.right_stick_deadzone)*1.05*sensitivity*(settings.camera_invert_y?-1:1)};
    }
};
}
