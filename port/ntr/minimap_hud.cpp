#include "ntr/minimap_hud.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

namespace ntr {
namespace {
constexpr uint32_t kPanel = 0xd9201408u;
constexpr uint32_t kPanelEdge = 0xff5d3b18u;
constexpr uint32_t kText = 0xfffff2beu;
constexpr uint32_t kStar = 0xffffd32au;
constexpr uint32_t kCoin = 0xffffb51cu;
constexpr uint32_t kLife = 0xff50c82fu;

void pixel(uint32_t *pixels,unsigned width,unsigned height,int x,int y,uint32_t color) {
    if(x>=0 && y>=0 && unsigned(x)<width && unsigned(y)<height)
        pixels[size_t(y)*width+unsigned(x)]=color;
}

void rect(uint32_t *pixels,unsigned width,unsigned height,int x,int y,int w,int h,uint32_t color) {
    for(int py=0;py<h;++py)for(int px=0;px<w;++px)pixel(pixels,width,height,x+px,y+py,color);
}

void outline_rect(uint32_t *pixels,unsigned width,unsigned height,int x,int y,int w,int h,uint32_t color) {
    rect(pixels,width,height,x,y,w,1,color);rect(pixels,width,height,x,y+h-1,w,1,color);
    rect(pixels,width,height,x,y,1,h,color);rect(pixels,width,height,x+w-1,y,1,h,color);
}

constexpr std::array<std::array<unsigned char,5>,10> kDigits{{
    {{7,5,5,5,7}},{{2,6,2,2,7}},{{7,1,7,4,7}},{{7,1,7,1,7}},{{5,5,7,1,1}},
    {{7,4,7,1,7}},{{7,4,7,5,7}},{{7,1,1,1,1}},{{7,5,7,5,7}},{{7,5,7,1,7}}
}};

void number(uint32_t *pixels,unsigned width,unsigned height,int x,int y,unsigned value,int scale) {
    char text[12]{};std::snprintf(text,sizeof(text),"%u",std::min(value,999u));
    int cursor=x;
    for(const char *ch=text;*ch;++ch) {
        if(*ch>='0'&&*ch<='9') {
            const auto &rows=kDigits[*ch-'0'];
            for(int py=0;py<5;++py)for(int px=0;px<3;++px)if(rows[py]&(1u<<(2-px)))
                rect(pixels,width,height,cursor+px*scale,y+py*scale,scale,scale,kText);
            cursor+=4*scale;
        }
    }
}

void icon(uint32_t *pixels,unsigned width,unsigned height,int x,int y,int size,unsigned type) {
    if(type==0) {
        // Five-point gold star with a dark pixel outline.
        static constexpr unsigned char star[7][7]={
            {0,0,0,1,0,0,0},{0,0,1,1,1,0,0},{1,1,1,1,1,1,1},
            {0,1,1,1,1,1,0},{0,1,1,0,1,1,0},{1,1,0,0,0,1,1},{1,0,0,0,0,0,1}};
        for(int py=0;py<size;++py)for(int px=0;px<size;++px) {
            const int sx=px*7/size,sy=py*7/size;
            if(star[sy][sx])pixel(pixels,width,height,x+px,y+py,kStar);
        }
        for(int py=0;py<size;++py)for(int px=0;px<size;++px)if(star[py*7/size][px*7/size]) {
            bool edge=false;
            for(int oy=-1;oy<=1&&!edge;++oy)for(int ox=-1;ox<=1;++ox) {
                const int nx=px+ox,ny=py+oy;
                if(nx<0||ny<0||nx>=size||ny>=size||!star[ny*7/size][nx*7/size]){edge=true;break;}
            }
            if(edge)pixel(pixels,width,height,x+px,y+py,kPanelEdge);
        }
    } else if(type==1) {
        const int center=(size-1)/2,rx=std::max(1,size/3),ry=std::max(1,size/2);
        for(int py=0;py<size;++py)for(int px=0;px<size;++px) {
            const int dx=px-center,dy=py-center;
            const int metric=dx*dx*ry*ry+dy*dy*rx*rx;
            if(metric<=rx*rx*ry*ry) {
                bool edge=false;
                for(int delta=-1;delta<=1;delta+=2) {
                    const int xx=dx+delta,yy=dy+delta;
                    if((xx*xx*ry*ry+dy*dy*rx*rx)>rx*rx*ry*ry ||
                       (dx*dx*ry*ry+yy*yy*rx*rx)>rx*rx*ry*ry)edge=true;
                }
                pixel(pixels,width,height,x+px,y+py,edge?kPanelEdge:((px==size/3)?0xffffe979u:kCoin));
            }
        }
    } else {
        // Green Yoshi life marker, scaled from a small readable pixel silhouette.
        static constexpr unsigned char yoshi[7][7]={
            {0,1,1,1,0,0,0},{1,1,1,1,1,0,0},{1,1,1,1,1,1,0},
            {1,0,1,1,1,1,1},{1,1,1,1,1,1,0},{0,1,1,1,1,0,0},{0,0,1,0,1,0,0}};
        for(int py=0;py<size;++py)for(int px=0;px<size;++px)if(yoshi[py*7/size][px*7/size])
            pixel(pixels,width,height,x+px,y+py,kLife);
        const int eye=std::max(1,size/7);
        rect(pixels,width,height,x+size*2/7,y+size/7,eye,eye,0xff182318u);
        rect(pixels,width,height,x+size*4/7,y+size/7,eye,eye,0xff182318u);
    }
}
}

void minimap_hud_build(uint32_t *pixels,unsigned width,unsigned height,
                       const VideoSettings &settings,bool visible,
                       unsigned stars,unsigned coins,unsigned lives) {
    if(!pixels||!width||!height)return;
    std::fill(pixels,pixels+size_t(width)*height,0u);
    if(!visible)return;

    const double scale=std::max(0.01,settings.minimap_scale/260.0);
    const int icon_size=std::max(1,int(std::lround(5.0*scale)));
    const int digit_scale=std::max(1,int(std::lround(scale)));
    const int pad=std::max(1,int(std::lround(scale)));
    const int glyph_height=5*digit_scale,glyph_width=11*digit_scale;
    const int row_height=std::max(icon_size,glyph_height)+2*pad;
    const int digits_width=std::max(4*digit_scale,glyph_width);
    const int panel_width=2*pad+icon_size+pad+digits_width;
    const int panel_height=3*row_height+2*pad;
    const double map_left=double(width)*settings.minimap_x/1000.0;
    const double map_top=double(height)*settings.minimap_y/1000.0;
    const double map_w=double(width)*settings.minimap_scale/1000.0;
    const double map_h=double(height)*settings.minimap_scale/1000.0;
    const int gap=std::max(2,int(std::lround(3.0*scale)));
    const int right=int(std::lround(map_left+map_w))+gap;
    const int left=int(std::floor(map_left))-gap-panel_width;
    int panel_x;
    if(right+panel_width<=int(width))panel_x=right;
    else if(left>=0)panel_x=left;
    else panel_x=(width-(panel_width+map_w)>=map_left)?right:left;
    panel_x=std::clamp(panel_x,0,std::max(0,int(width)-panel_width));
    int panel_y=int(std::lround(map_top+(map_h-panel_height)/2.0));
    panel_y=std::clamp(panel_y,0,std::max(0,int(height)-panel_height));

    const unsigned counts[3]={stars,coins,lives};
    for(unsigned row=0;row<3;++row) {
        const int y=panel_y+pad+int(row)*row_height;
        rect(pixels,width,height,panel_x+1,y-1,panel_width-2,row_height,kPanel);
        outline_rect(pixels,width,height,panel_x,y-1,panel_width,row_height,kPanelEdge);
        const int ix=panel_x+pad,iy=y+(row_height-icon_size)/2;
        icon(pixels,width,height,ix,iy,icon_size,row);
        number(pixels,width,height,ix+icon_size+pad,
               y+(row_height-glyph_height)/2,counts[row],digit_scale);
    }
}

void minimap_hud_blend(uint32_t *destination,const uint32_t *overlay,
                       unsigned width,unsigned height) {
    if(!destination||!overlay)return;
    for(size_t i=0;i<size_t(width)*height;++i) {
        const uint32_t src=overlay[i];const unsigned alpha=src>>24;
        if(!alpha)continue;
        if(alpha==255){destination[i]=src;continue;}
        const uint32_t dst=destination[i];const unsigned inv=255-alpha;
        const unsigned r=(((src>>16)&255)*alpha+((dst>>16)&255)*inv)/255;
        const unsigned g=(((src>>8)&255)*alpha+((dst>>8)&255)*inv)/255;
        const unsigned b=((src&255)*alpha+(dst&255)*inv)/255;
        destination[i]=(dst&0xff000000u)|(r<<16)|(g<<8)|b;
    }
}
}
