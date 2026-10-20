#pragma once
namespace pc_stage_options {
// Scope a controller action as a real native touch for one PS_Update call.
// The destructor restores both the touch lanes and the original key record.
class Frame {
public:
    Frame();
    ~Frame();
    Frame(const Frame&)=delete;
    Frame& operator=(const Frame&)=delete;
private:
    unsigned slot=16;
    unsigned short keys=0;
    unsigned char saved[4]{};
    bool injected=false;
};
}
