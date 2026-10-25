#pragma once
#include <cmath>
#include <cstddef>

// Input-only QA path follower. No game state, collision or reward mutations.
namespace replay_path {
struct Point { double x, z, radius; };
struct Aim { std::size_t waypoint; double x, z; bool complete; };
inline Aim follow(const Point *p, std::size_t count, std::size_t at,
                  double x, double z, double lookahead, double corridor) {
    if(at>=count)return {at,x,z,true};
    while(at<count) {
        const double distance=std::hypot(x-p[at].x,z-p[at].z);
        bool passed=false;
        if(at && at+1<count) {
            const double dx=p[at].x-p[at-1].x,dz=p[at].z-p[at-1].z;
            const double length=std::hypot(dx,dz);
            if(length>0) {
                const double along=((x-p[at-1].x)*dx+(z-p[at-1].z)*dz)/length;
                const double cross=((x-p[at-1].x)*dz-(z-p[at-1].z)*dx)/length;
                passed=along>=length && std::abs(cross)<=corridor;
            }
        }
        if(distance>p[at].radius && !passed)break;
        ++at;
    }
    if(at>=count)return {at,x,z,true};
    double ax=p[at].x,az=p[at].z;
    if(at) {
        const double dx=ax-p[at-1].x,dz=az-p[at-1].z;
        const double squared=dx*dx+dz*dz;
        double t=squared>0?((x-p[at-1].x)*dx+(z-p[at-1].z)*dz)/squared:1;
        if(t<0)t=0;if(t>1)t=1;
        ax=p[at-1].x+t*dx;az=p[at-1].z+t*dz;
    }
    // Walk forward along the polyline, including around the next corner.
    for(std::size_t i=at;i<count;++i) {
        const double dx=p[i].x-ax,dz=p[i].z-az,length=std::hypot(dx,dz);
        if(length>lookahead && length>0)
            return {at,ax+dx*lookahead/length,az+dz*lookahead/length,false};
        lookahead-=length;ax=p[i].x;az=p[i].z;
    }
    return {at,ax,az,false};
}
}
