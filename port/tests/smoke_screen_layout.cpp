#include "ntr/video.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
namespace {
int failures;
#define CHECK(x) do { if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);++failures;} }while(0)
}
int main() {
    using namespace ntr;
    const auto resized=screen_fit(1280,720,{0,0,1000,1000});
    CHECK(resized.x==0 && resized.y==219 && resized.width==1000 && resized.height==562);
    Framebuffer top{},bottom{};
    for(unsigned y=0;y<SCREEN_H;++y) for(unsigned x=0;x<SCREEN_W;++x) {
        top.px[y][x]=0xff112233;bottom.px[y][x]=0xff000000u|(y<<8)|x;
    }
    for(auto dims:{ScreenRect{0,0,1280,720},ScreenRect{0,0,3840,2160},ScreenRect{0,0,3440,1474},ScreenRect{0,0,777,999}})
        for(unsigned mode=0;mode<3;++mode) {
            const auto r=screen_layout(static_cast<MinigameLayout>(mode),dims.width,dims.height);
            for(const auto &p:{r.top,r.bottom}) {
                CHECK(p.width>0 && p.height>0 && p.x>=0 && p.y>=0 && p.x+p.width<=dims.width && p.y+p.height<=dims.height);
                CHECK(std::abs(p.width*3-p.height*4)<=3);
            }
            CHECK(mode==0?r.top.y+r.top.height<=r.bottom.y:
                mode==1?r.top.x+r.top.width<=r.bottom.x:r.bottom.x+r.bottom.width<=r.top.x);
            uint16_t x=0,y=0;
            CHECK(!screen_touch(r.bottom,r.top.x+r.top.width/2,r.top.y+r.top.height/2,x,y));
            CHECK(!screen_touch(r.bottom,r.bottom.x-1,r.bottom.y,x,y));
            CHECK(!screen_touch(r.bottom,r.bottom.x+r.bottom.width,r.bottom.y,x,y));
            CHECK(!screen_touch(r.bottom,r.bottom.x,r.bottom.y+r.bottom.height,x,y));
            CHECK(screen_touch(r.bottom,r.bottom.x,r.bottom.y,x,y) && x==0 && y==0);
            CHECK(screen_touch(r.bottom,r.bottom.x+r.bottom.width/2,r.bottom.y+r.bottom.height/2,x,y) && std::abs(int(x)-128)<=1 && std::abs(int(y)-96)<=1);
            std::vector<uint32_t> pixels;
            screen_compose(top,bottom,r,dims.width,dims.height,pixels);
            CHECK(pixels.size()==size_t(dims.width)*dims.height);
            CHECK(pixels[size_t(r.top.y+r.top.height/2)*dims.width+r.top.x+r.top.width/2]==0xff112233);
            CHECK(pixels[size_t(r.bottom.y+r.bottom.height/2)*dims.width+r.bottom.x+r.bottom.width/2]==(0xff000000u|(y<<8)|x));
        }
    video_set_minigame_active(true);
    CHECK(video_dual_screen_active() && !video_minimap_visible());
    video_set_pause_screen(true);CHECK(!video_dual_screen_active());
    video_set_pause_screen(false);CHECK(video_dual_screen_active());
    video_set_minigame_active(false);CHECK(!video_dual_screen_active());
    for(unsigned mode=0;mode<3;++mode) {
        VideoSettings s;s.minigame_layout=static_cast<MinigameLayout>(mode);video_set_settings(s);
        CHECK(video_save_settings("screen-layout-smoke.ini"));
        video_set_settings(VideoSettings{});CHECK(video_load_settings("screen-layout-smoke.ini"));
        CHECK(video_get_settings().minigame_layout==static_cast<MinigameLayout>(mode));
    }
    for(const char *invalid:{"minigame_layout=3\n","minigame_layout=256\n","minigame_layout=-1\n"}) {
        FILE *f=std::fopen("screen-layout-smoke.ini","wb");CHECK(f);if(!f)return 1;
        std::fputs(invalid,f);std::fclose(f);CHECK(video_load_settings("screen-layout-smoke.ini"));
        CHECK(video_get_settings().minigame_layout==MinigameLayout::TopBottom);
    }
    std::remove("screen-layout-smoke.ini");
    std::puts("Screen layout: geometry, touch bounds, composition, scene state, settings persistence checked");
    return failures?1:0;
}
