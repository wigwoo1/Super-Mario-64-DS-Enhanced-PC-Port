#pragma once
#include <limits>
namespace ntr {
struct MenuTarget { int id,left,top,right,bottom; };
inline int menu_focus_index(const MenuTarget *items,int count,int id) {
    for(int i=0;i<count;++i)if(items[i].id==id)return i;
    return -1;
}
// An original widget can disappear briefly during its press animation. Keep
// its identity until it returns; an absent target cannot receive confirmation.
inline int menu_focus_initialize(const MenuTarget *items,int count,int current) {
    return current>=0?current:(count?items[0].id:-1);
}
inline int menu_focus_move(const MenuTarget *items,int count,int current,int dx,int dy) {
    if(!count)return -1;
    int from=menu_focus_index(items,count,current);
    if(from<0)return items[0].id;
    const int fx=items[from].left+items[from].right,fy=items[from].top+items[from].bottom;
    long long best=std::numeric_limits<long long>::max();int selected=current;
    for(int i=0;i<count;++i)if(i!=from) {
        const int x=items[i].left+items[i].right-fx,y=items[i].top+items[i].bottom-fy;
        const int forward=x*dx+y*dy,side=x*dy-y*dx;
        if(forward<=0)continue;
        const long long score=1ll*forward*forward+4ll*side*side;
        if(score<best){best=score;selected=items[i].id;}
    }
    return selected;
}
}
