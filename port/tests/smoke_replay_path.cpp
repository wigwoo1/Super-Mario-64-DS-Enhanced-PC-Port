#include "../hal/replay_path.h"
#include <cstdio>
int main() {
    using namespace replay_path;
    const Point p[]={{0,0,10},{1000,0,10},{1000,1000,10}};
    auto a=follow(p,3,1,1100,100,300,200);
    if(a.waypoint!=2 || a.x!=1000 || a.z!=400)return 1;
    a=follow(p,3,1,1100,300,300,200);
    if(a.waypoint!=1)return 2; // Too far off course: do not silently skip.
    a=follow(p,3,1,900,0,300,200);
    if(a.waypoint!=1 || a.x!=1000 || a.z!=200)return 3;
    a=follow(p,3,2,1000,1100,300,200);
    if(a.complete || a.waypoint!=2)return 4; // Final destination must be reached.
    a=follow(p,3,2,1000,995,300,200);
    if(!a.complete || a.waypoint!=3)return 5;
    const Point repeated[]={{0,0,1},{0,0,1},{100,0,1}};
    a=follow(repeated,3,1,0,0,30,20);
    if(a.waypoint!=2 || a.x!=30 || !std::isfinite(a.z))return 6;
    std::puts("PASS: missed marker, corridor, corner lookahead, final arrival, duplicate point");
}
