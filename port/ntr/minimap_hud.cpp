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
MinimapHudArt art;

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

void icon(uint32_t *pixels,unsigned width,unsigned height,int x,int y,int size,const HudSprite &sprite) {
    if(!sprite.width || !sprite.height || sprite.pixels.size()!=size_t(sprite.width)*sprite.height)return;
    const double factor=double(size)/std::max(sprite.width,sprite.height);
    const int w=std::max(1,int(std::lround(sprite.width*factor))),h=std::max(1,int(std::lround(sprite.height*factor)));
    for(int py=0;py<h;++py)for(int px=0;px<w;++px) {
        const uint32_t color=sprite.pixels[size_t(py*sprite.height/h)*sprite.width+px*sprite.width/w];
        if(color>>24)pixel(pixels,width,height,x+(size-w)/2+px,y+(size-h)/2+py,color);
    }
}
}

void minimap_hud_set_art(MinimapHudArt value) {art=std::move(value);}

bool minimap_hud_decode(HudSprite &out,const uint8_t *attributes,size_t bytes,
                       const uint8_t *tiles,size_t tile_bytes,const uint8_t *palette,size_t palette_bytes) {
    if(!attributes||!tiles||!palette||bytes<8)return false;
    struct Part {int x,y;unsigned w,h,tile,pal;bool flip_x,flip_y;};
    std::vector<Part> parts;int left=512,top=256,right=-512,bottom=-256;
    const unsigned dimensions[3][4][2]={{{8,8},{16,16},{32,32},{64,64}},{{16,8},{32,8},{32,16},{64,32}},{{8,16},{8,32},{16,32},{32,64}}};
    auto word=[](const uint8_t *p){return unsigned(p[0])|(unsigned(p[1])<<8);};
    bool terminated=false;
    for(size_t offset=0;offset+8<=bytes && parts.size()<128;offset+=8) {
        const auto *p=attributes+offset;
        const unsigned a=word(p),b=word(p+2),c=word(p+4),shape=a>>14,size=b>>14;
        if(shape==3 || (a&0x2300))return false; // these templates are regular 4bpp sprites
        int x=b&511,y=a&255;if(x>=256)x-=512;if(y>=128)y-=256;
        const unsigned w=dimensions[shape][size][0],h=dimensions[shape][size][1];
        parts.push_back({x,y,w,h,c&1023,c>>12,bool(b&0x1000),bool(b&0x2000)});
        left=std::min(left,x);top=std::min(top,y);right=std::max(right,x+int(w));bottom=std::max(bottom,y+int(h));
        if(word(p+6)==0xffff){terminated=true;break;}
    }
    if(!terminated||right<=left||bottom<=top||right-left>128||bottom-top>128)return false;
    HudSprite result;result.width=right-left;result.height=bottom-top;
    result.pixels.resize(size_t(result.width)*result.height);
    for(const auto &part:parts)for(unsigned y=0;y<part.h;++y)for(unsigned x=0;x<part.w;++x) {
        const unsigned sx=part.flip_x?part.w-1-x:x,sy=part.flip_y?part.h-1-y:y;
        const size_t address=(part.tile+(sy/8)*32+sx/8)*32+(sy%8)*4+(sx%8)/2;
        if(address>=tile_bytes)return false;
        const unsigned index=(tiles[address]>>((sx&1)*4))&15;
        if(!index)continue;
        const size_t pal=(part.pal*16+index)*2;
        if(pal+1>=palette_bytes)return false;
        const unsigned color=word(palette+pal);
        auto expand=[](unsigned c){return (c<<3)|(c>>2);};
        auto &dst=result.pixels[size_t(part.y-top+y)*result.width+part.x-left+x];
        // The first OAM entry has priority over later entries.
        if(!dst)dst=0xff000000u|(expand(color&31)<<16)|(expand((color>>5)&31)<<8)|expand((color>>10)&31);
    }
    out=std::move(result);return true;
}

void minimap_hud_build(uint32_t *pixels,unsigned width,unsigned height,
                       const VideoSettings &settings,bool visible,
                       unsigned stars,unsigned coins,unsigned lives,unsigned health,unsigned character) {
    if(!pixels||!width||!height)return;
    std::fill(pixels,pixels+size_t(width)*height,0u);
    if(!visible)return;

    const double scale=std::max(0.01,settings.minimap_scale/260.0);
    const int icon_size=std::max(1,int(std::lround(12.0*scale)));
    const int digit_scale=std::max(1,int(std::lround(scale)));
    const int pad=std::max(1,int(std::lround(scale)));
    const int glyph_height=5*digit_scale,glyph_width=11*digit_scale;
    const int row_height=std::max(icon_size,glyph_height)+2*pad;
    const int digits_width=std::max(4*digit_scale,glyph_width);
    const int panel_width=2*pad+icon_size+pad+digits_width;
    const int panel_height=4*row_height+2*pad;
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

    const unsigned counts[4]={stars,coins,std::min(health,8u),lives};
    const HudSprite *icons[4]={&art.star,&art.coin,&art.health[std::min(health,8u)],&art.lives[character<4?character:3]};
    for(unsigned row=0;row<4;++row) {
        const int y=panel_y+pad+int(row)*row_height;
        rect(pixels,width,height,panel_x+1,y-1,panel_width-2,row_height,kPanel);
        outline_rect(pixels,width,height,panel_x,y-1,panel_width,row_height,kPanelEdge);
        const int ix=panel_x+pad,iy=y+(row_height-icon_size)/2;
        icon(pixels,width,height,ix,iy,icon_size,*icons[row]);
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
