#include "ntr/minimap_hud.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace ntr {
namespace {
constexpr uint32_t kText = 0xfffff2beu;
constexpr uint32_t kTextOutline = 0xd0000000u;
MinimapHudArt art;
constexpr std::array<std::array<unsigned char,7>,10> kDigits7{{
    {{14,17,19,21,25,17,14}},{{4,12,4,4,4,4,14}},{{14,17,1,2,4,8,31}},
    {{30,1,1,14,1,1,30}},{{2,6,10,18,31,2,2}},{{31,16,16,30,1,1,30}},
    {{14,16,16,30,17,17,14}},{{31,1,2,4,8,8,8}},{{14,17,17,14,17,17,14}},
    {{14,17,17,15,1,1,14}}
}};

void pixel(uint32_t *pixels,unsigned width,unsigned height,int x,int y,uint32_t color) {
    if(x>=0 && y>=0 && unsigned(x)<width && unsigned(y)<height)
        pixels[size_t(y)*width+unsigned(x)]=color;
}

void rect(uint32_t *pixels,unsigned width,unsigned height,int x,int y,int w,int h,uint32_t color) {
    for(int py=0;py<h;++py)for(int px=0;px<w;++px)pixel(pixels,width,height,x+px,y+py,color);
}

void number(uint32_t *pixels,unsigned width,unsigned height,int x,int y,unsigned value,
            int cell_width,int cell_height) {
    char text[12]{};std::snprintf(text,sizeof(text),"%u",std::min(value,999u));
    const int bold_x=std::max(1,cell_width/5);
    // Keep a visible outer rim beyond the rightward bold pass. At 720p this
    // is a few output pixels; it grows proportionally for 4K HUD rendering.
    const int outline_x=bold_x+2,outline_y=std::max(2,cell_height/3);
    int cursor=x;
    // Draw the thin outline for the complete number first so neighboring
    // glyphs cannot paint over one another's edges.
    for(const char *ch=text;*ch;++ch) {
        if(*ch>='0'&&*ch<='9') {
            const auto &rows=kDigits7[*ch-'0'];
            for(int py=0;py<7;++py)for(int px=0;px<5;++px)if(rows[py]&(1u<<(4-px)))
                rect(pixels,width,height,cursor+px*cell_width-outline_x,
                     y+py*cell_height-outline_y,cell_width+2*outline_x,
                     cell_height+2*outline_y,kTextOutline);
            cursor+=6*cell_width+bold_x;
        }
    }
    cursor=x;
    for(const char *ch=text;*ch;++ch) {
        if(*ch>='0'&&*ch<='9') {
            const auto &rows=kDigits7[*ch-'0'];
            for(int py=0;py<7;++py)for(int px=0;px<5;++px)if(rows[py]&(1u<<(4-px))) {
                const int cell_x=cursor+px*cell_width;
                const int cell_y=y+py*cell_height;
                rect(pixels,width,height,cell_x,cell_y,cell_width,cell_height,kText);
                rect(pixels,width,height,cell_x+bold_x,cell_y,cell_width,cell_height,kText);
            }
            cursor+=6*cell_width+bold_x;
        }
    }
}

int count_width(unsigned value,int cell_width) {
    char text[12]{};std::snprintf(text,sizeof(text),"%u",std::min(value,999u));
    const int digits=std::max(1,int(std::strlen(text)));
    const int bold_x=std::max(1,cell_width/5);
    return std::max(1,(digits*6-1)*cell_width+digits*bold_x);
}

void over_pixel(uint32_t *pixels,unsigned width,unsigned height,int x,int y,uint32_t color) {
    if(x<0||y<0||unsigned(x)>=width||unsigned(y)>=height)return;
    uint32_t &dst=pixels[size_t(y)*width+unsigned(x)];
    const unsigned sa=color>>24,da=dst>>24,inv=255-sa;
    const unsigned alpha=sa+(da*inv+127)/255;
    if(!alpha){dst=0;return;}
    auto channel=[&](unsigned shift){
        const unsigned sc=(color>>shift)&255,dc=(dst>>shift)&255;
        const unsigned premult=sc*sa+(dc*da*inv+127)/255;
        return (premult+alpha/2)/alpha;
    };
    dst=(alpha<<24)|(channel(16)<<16)|(channel(8)<<8)|channel(0);
}

void icon(uint32_t *pixels,unsigned width,unsigned height,int x,int y,
          int target_width,int target_height,const HudSprite &sprite,bool smooth) {
    if(!sprite.width || !sprite.height || sprite.pixels.size()!=size_t(sprite.width)*sprite.height)return;
    const int w=std::max(1,std::min(target_width,int(width)-x));
    const int h=std::max(1,std::min(target_height,int(height)-y));
    for(int py=0;py<h;++py)for(int px=0;px<w;++px) {
        uint32_t color=0;
        if(!smooth) {
            color=sprite.pixels[size_t(py)*sprite.height/h*sprite.width+px*sprite.width/w];
        } else {
            const double fx=std::clamp((px+0.5)*sprite.width/w-0.5,0.0,double(sprite.width-1));
            const double fy=std::clamp((py+0.5)*sprite.height/h-0.5,0.0,double(sprite.height-1));
            const unsigned x0=unsigned(fx),y0=unsigned(fy),x1=std::min(x0+1,sprite.width-1),y1=std::min(y0+1,sprite.height-1);
            const double tx=fx-x0,ty=fy-y0;
            const uint32_t samples[4]={sprite.pixels[size_t(y0)*sprite.width+x0],sprite.pixels[size_t(y0)*sprite.width+x1],
                                       sprite.pixels[size_t(y1)*sprite.width+x0],sprite.pixels[size_t(y1)*sprite.width+x1]};
            const double weights[4]={(1-tx)*(1-ty),tx*(1-ty),(1-tx)*ty,tx*ty};
            double a=0,r=0,g=0,b=0;
            for(unsigned i=0;i<4;++i){const double alpha=samples[i]>>24;a+=weights[i]*alpha;r+=weights[i]*alpha*((samples[i]>>16)&255);g+=weights[i]*alpha*((samples[i]>>8)&255);b+=weights[i]*alpha*(samples[i]&255);}
            if(a>0)color=(unsigned(std::lround(a))<<24)|(unsigned(std::lround(r/a))<<16)|(unsigned(std::lround(g/a))<<8)|unsigned(std::lround(b/a));
        }
        if(color>>24)over_pixel(pixels,width,height,x+px,y+py,color);
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
                       unsigned stars,unsigned coins,unsigned lives,unsigned health,unsigned character,
                       MinimapHudBounds visible_bounds) {
    if(!pixels||!width||!height)return;
    std::fill(pixels,pixels+size_t(width)*height,0u);
    if(!visible)return;

    const double scale=std::max(0.01,settings.minimap_scale/260.0);
    const double raster_x=double(width)/256.0,raster_y=double(height)/192.0;
    const int icon_width=std::max(1,int(std::lround(12.0*scale*raster_x)));
    const int icon_height=std::max(1,int(std::lround(12.0*scale*raster_y)));
    const int digit_width=std::max(1,int(std::lround(scale*raster_x)));
    const int digit_height=std::max(1,int(std::lround(scale*raster_y)));
    const int pad_x=std::max(1,int(std::lround(scale*raster_x)));
    const int pad_y=std::max(1,int(std::lround(scale*raster_y)));
    const int glyph_height=7*digit_height;
    const int row_height=std::max(icon_height,glyph_height)+2*pad_y;
    const double map_left=double(width)*settings.minimap_x/1000.0;
    const double map_top=double(height)*settings.minimap_y/1000.0;
    const double map_w=double(width)*settings.minimap_scale/1000.0;
    const double map_h=double(height)*settings.minimap_scale/1000.0;
    visible_bounds.left=std::min(visible_bounds.left,999u);
    visible_bounds.top=std::min(visible_bounds.top,999u);
    visible_bounds.right=std::clamp(visible_bounds.right,visible_bounds.left+1,1000u);
    visible_bounds.bottom=std::clamp(visible_bounds.bottom,visible_bounds.top+1,1000u);
    const double visible_left=map_left+map_w*visible_bounds.left/1000.0;
    const double visible_top=map_top+map_h*visible_bounds.top/1000.0;
    const double visible_right=map_left+map_w*visible_bounds.right/1000.0;
    const double visible_bottom=map_top+map_h*visible_bounds.bottom/1000.0;
    const int gap=std::max(1,int(std::lround(scale*raster_x)));
    const unsigned counts[4]={stars,coins,std::min(health,8u),lives};
    const HudSprite *icons[4]={&art.star,&art.coin,&art.health[std::min(health,8u)],&art.lives[character<4?character:3]};
    const auto card_width=[&](unsigned value) {
        return 2*pad_x+icon_width+pad_x+count_width(value,digit_width);
    };
    const auto draw_card=[&](int x,int y,unsigned index) {
        const int iy=y+(row_height-icon_height)/2;
        icon(pixels,width,height,x+pad_x,iy,icon_width,icon_height,*icons[index],width>256||height>192);
        number(pixels,width,height,x+2*pad_x+icon_width,
               y+(row_height-glyph_height)/2,counts[index],digit_width,digit_height);
    };

    // Keep the requested Star/coin pair beside the map and the health/lives
    // pair beneath it. This mirrors the mockup while keeping all four cards
    // anchored to the user-positioned/resized minimap.
    const int side_width=std::max(card_width(counts[0]),card_width(counts[1]));
    const int map_x=int(std::lround(visible_left)),map_y=int(std::lround(visible_top));
    const int map_right=int(std::lround(visible_right));
    const int map_bottom=int(std::lround(visible_bottom));
    const int side_left=map_x-gap-side_width;
    const int side_right=map_right+gap;
    int side_x=side_left>=0?side_left:
        (side_right+side_width<=int(width)?side_right:std::clamp(side_left,0,std::max(0,int(width)-side_width)));
    const int side_y=std::clamp(map_y,0,std::max(0,int(height)-2*(row_height+1)));
    draw_card(side_x,side_y,0);
    draw_card(side_x,side_y+row_height+1,1);

    const int health_width=card_width(counts[2]),life_width=card_width(counts[3]);
    const int bottom_width=health_width+gap+life_width;
    int bottom_x=int(std::lround((visible_left+(visible_right-visible_left)/2.0)-bottom_width/2.0));
    bottom_x=std::clamp(bottom_x,0,std::max(0,int(width)-bottom_width));
    const int below_y=map_bottom+gap;
    const int above_y=map_y-gap-row_height-1;
    int bottom_y=below_y+row_height+1<=int(height)?below_y:
        (above_y>=0?above_y:std::clamp(below_y,0,std::max(0,int(height)-row_height-1)));
    draw_card(bottom_x,bottom_y,2);
    draw_card(bottom_x+health_width+gap,bottom_y,3);
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
