#include "hal/dsmt_scene.h"
#include "dScDSMT_c.h"
#include "Heap.h"
#include "Model.h"
#include "dExtFrameCtrl_c.h"
#include "TextureSequence.h"
#include <new>
#include <cstdlib>
#include <cstdio>
#include <cstddef>
#include <set>
#include <cstring>
#include "ntr/ppu.h"
#include "ntr/rt.h"
#include "ntr/gx.h"
#include "ntr/video.h"
#include "hal/pc_pause_settings_state.h"
extern "C" {extern void *data_0209d4a8;extern void *data_0209d524;}
static_assert(sizeof(dScDSMT_c)==0x64,"Native DSMT scene layout");
static_assert(sizeof(dScDSMT_c::graphCallback_c)==4,"Native DSMT graphics callback layout");
static_assert(offsetof(dScDSMT_c,unk_050)==0x50,"Native DSMT embedded callback offset");
static void *active_graph;
bool pc_dsmt_active(){return active_graph && data_0209d4a8==active_graph;}
extern "C" void *data_ov007_0210342c;
void pc_dsmt_phase(int phase){
    if(!std::getenv("SM64DS_PC_DSMT_DUMP"))return;
    static unsigned updates=0;if(phase==0)++updates;if(updates>4)return;
    auto *g=static_cast<unsigned char*>(data_ov007_0210342c);
    auto *controller=*reinterpret_cast<int**>(g+12);
    auto *entry=*reinterpret_cast<int**>(g+0x114);
    auto *panel=reinterpret_cast<int*>(entry[0]);auto *state=reinterpret_cast<int*>(panel[1]);
    std::fprintf(stderr,"[native DSMT phase] update=%u phase=%d controller=%08x,%08x,%08x,%08x panel=%08x,%08x,%08x,%08x visible=%d\n",updates,phase,
        controller[0],controller[1],controller[2],controller[3],state[0],state[1],state[2],state[3],panel[8]);
}
bool pc_dsmt_requested(){const char *v=std::getenv("SM64DS_PC_ORIGINAL_MENU");return v && v[0]=='1';}
int pc_title_text_mode_get(){return ntr::video_get_settings().brain_rot_text ? 1 : 0;}
void pc_title_text_mode_set(int enabled){
    auto settings=ntr::video_get_settings();
    if(settings.brain_rot_text==(enabled!=0))return;
    settings.brain_rot_text=enabled!=0;
    ntr::video_set_settings(settings);
}
extern "C" {
extern int data_ov007_02102dd8,data_ov007_02103464,data_ov007_02103468;
extern unsigned char data_ov007_02102df8[];
extern unsigned char *data_ov007_02103478[];
void func_ov007_020b8b80(int,const unsigned char *,int);
void func_ov007_020c0638(void *,int,int,unsigned short);
}
extern "C" void pc_settings_capture_title_font(const void *font,unsigned base_tile) {
    const bool valid=pc_pause::cache_options_font(static_cast<const unsigned char *>(font),base_tile,data_ov007_02102df8);
    std::fprintf(stderr,"[settings typography] original Options font captured=%u base_tile=%u\n",unsigned(valid),base_tile);
}
extern "C" void pc_title_text_label_draw(){
    auto *context=static_cast<unsigned char *>(data_ov007_0210342c);
    if(!context)return;
    auto *entry=*reinterpret_cast<unsigned char **>(context+0xcc);
    if(!entry)return;
    auto *layer=*reinterpret_cast<unsigned char **>(entry+4);
    if(!layer)return;
    auto *map=*reinterpret_cast<unsigned short **>(layer+0x14);
    const unsigned width=*reinterpret_cast<unsigned short *>(layer+0x1c);
    const unsigned height=*reinterpret_cast<unsigned short *>(layer+0x1e);
    if(!map || width!=32 || height<12)return;
    // BG3's original Backlight caption occupies x24..103, y80..95.
    // Remove only its map cells; draw the new heading through the original
    // variable-width menu font in BG1, alongside the original On/Off text.
    const unsigned short blank=map[0];
    for(unsigned y=10;y<12;++y)for(unsigned x=3;x<13;++x)map[y*width+x]=blank;
    func_ov007_020c0638(layer,0,0,0);
    static const unsigned char label[]={0x0b,0x1b,0x0a,0x12,0x17,0x4d,0x1b,0x18,0x1d,0xff};
    const int pal=data_ov007_02102dd8,x=data_ov007_02103464,y=data_ov007_02103468;
    data_ov007_02102dd8=0xf;data_ov007_02103464=64;data_ov007_02103468=10;
    func_ov007_020b8b80(0,label,0);
    // Palette 14/15 are the original menu's white/black text inks. Outline
    // this small caption so it stays readable over the moving character.
    auto *pixels=data_ov007_02103478[0];
    auto offset=[](unsigned px,unsigned py){return (py/8*32+px/8)*32+(py%8)*4+(px%8)/2;};
    auto get=[&](unsigned px,unsigned py){return (pixels[offset(px,py)]>>((px&1)*4))&15;};
    auto put=[&](unsigned px,unsigned py,unsigned ink){
        auto &v=pixels[offset(px,py)];const unsigned shift=(px&1)*4;
        v=static_cast<unsigned char>((v&~(15u<<shift))|(ink<<shift));
    };
    bool ink[18][82]={};
    for(unsigned py=79;py<97;++py)for(unsigned px=23;px<105;++px)ink[py-79][px-23]=get(px,py)==15;
    for(unsigned py=80;py<96;++py)for(unsigned px=24;px<104;++px)if(ink[py-79][px-23]) {
        for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)
            if(!ink[py+dy-79][px+dx-23])put(px+dx,py+dy,15);
        put(px,py,14);
    }
    data_ov007_02102dd8=pal;data_ov007_02103464=x;data_ov007_02103468=y;
}
void pc_dsmt_trace(const char *step){const char *v=std::getenv("SM64DS_PC_DSMT_TRACE");if(v && v[0]=='1'){std::fprintf(stderr,"[native DSMT] %s\n",step);std::fflush(stderr);}}
void *pc_create_dsmt_scene(){
    pc_dsmt_trace("native factory");
    auto *scene=new dScDSMT_c;
    if(scene){new(scene->unk_050)dScDSMT_c::graphCallback_c;active_graph=scene->unk_050;
        scene->fader.currInterp=0;scene->fader.speed=0;scene->fader.color=0;}
    return scene;
}
// The cartridge borrows the fixed arena 0211d9c0..02140d80 for this heap.
// Give that same-sized arena native storage rather than subtract DS labels.
extern "C" void *func_0201a458(){
    alignas(32) static unsigned char arena[0x02140d80u-0x0211d9c0u];
    if(!data_0209d524)data_0209d524=Heap::CreateRootHeap(arena,sizeof(arena));
    return data_0209d524;
}
extern "C" void _ZN4Heap7DestroyEv(Heap *self){self->Destroy();}
extern "C" void _ZN15dExtFrameCtrl_c17UpdateFileOffsetsER8BCA_File(BCA_File *file){dExtFrameCtrl_c::UpdateFileOffsets(*file);}
extern "C" void _ZN15TextureSequence17UpdateFileOffsetsER8BTP_File(BTP_File *file){TextureSequence::UpdateFileOffsets(*file);}
extern "C" void _ZN5Model13LoadTexAndPalER8BMD_File(BMD_File *file){Model::LoadTexAndPal(*file);}
namespace cstd {long long ldiv(int,int);}
extern "C" long long _ZN4cstd4ldivEii(int a,int b){return cstd::ldiv(a,b);}
// This recovered caller declares GX as a class with a static function. The
// existing namespace implementation has exactly the same cdecl parameters.
#pragma comment(linker,"/alternatename:?LoadOBJ@GX@@SAXPBXII@Z=?LoadOBJ@GX@@YAXPBXII@Z")
namespace GX {void LoadOBJ(const void *,unsigned,unsigned);void LoadBG0Char(const void *,unsigned,unsigned);}
extern "C" void _ZN2GX7LoadOBJEPKvjj(const void *p,unsigned offset,unsigned size){GX::LoadOBJ(p,offset,size);}
extern "C" void _ZN2GX11LoadBG0CharEPKvjj(const void *p,unsigned offset,unsigned size){GX::LoadBG0Char(p,offset,size);}
extern "C" void pc_dsmt_graph_prepare(){
    if(!pc_dsmt_active())ntr::video_set_menu_focus(-1,0,0,0);
    if(active_graph && data_0209d4a8==active_graph) {
        static_cast<dScDSMT_c::graphCallback_c *>(active_graph)->GraphCallback2();
        static bool captured=false;
        const char *prefix=std::getenv("SM64DS_PC_DSMT_DUMP");
        const char *layer_tick=std::getenv("SM64DS_PC_DSMT_LAYER_TICK");
        if(prefix && prefix[0] && !layer_tick && ntr::rt_frame()==2980) {
            static ntr::Framebuffer text_layer;
            ntr::ppu_scanout(ntr::ENGINE_A,text_layer,true,0x02);
            char path[2048];std::snprintf(path,sizeof(path),"%s-dialog-bg1.bmp",prefix);
            ntr::ppu_write_bmp(path,text_layer);
            unsigned opaque=0;for(auto &row:text_layer.px)for(auto pixel:row)if(pixel>>24)++opaque;
            std::fprintf(stderr,"[native DSMT dialog] DISPCNT=%08x BG0=%04x BG1=%04x BG1_pixels=%u\n",
                *reinterpret_cast<volatile unsigned*>(0x04000000),
                *reinterpret_cast<volatile unsigned short*>(0x04000008),
                *reinterpret_cast<volatile unsigned short*>(0x0400000a),opaque);
        }
        // Compare the same animation moments across software and GPU captures.
        // This observes original state without calling any update functions.
        if(prefix && prefix[0] && !layer_tick && (ntr::rt_frame()==300 || ntr::rt_frame()==600 || ntr::rt_frame()==1180)) {
            char path[2048];
            std::snprintf(path,sizeof(path),"%s-frame-%llu.csv",prefix,(unsigned long long)ntr::rt_frame());
            if(FILE *out=std::fopen(path,"w")) {
                size_t count=0;const auto *triangles=ntr::gx_display_polygons(count);
                if(ntr::rt_frame()==1180) {
                    auto *ctx=static_cast<unsigned char*>(data_ov007_0210342c);
                    for(int k=0x78;k<=0xa0;k+=4) {
                        auto *t=*reinterpret_cast<unsigned**>(ctx+k);
                        if(!t) continue;
                        std::fprintf(stderr,"[menu source] slot=%x flags=%x fmt=%u src=%x bytes=%u dest=%x palette=%x bytes=%u dest=%x\n",k,t[0],t[1],t[2],t[3],t[4],t[7],t[8],t[9]);
                        for(int part=0;part<2;++part) {
                            const int p=part?7:2;
                            if(!(t[0]&(1u<<part)) || !t[p] || t[p+1]>0x80000) continue;
                            char source_path[2048];
                            std::snprintf(source_path,sizeof(source_path),"%s-source-%x-%d.bin",prefix,k,part);
                            if(FILE *source=std::fopen(source_path,"wb")) {
                                std::fwrite(reinterpret_cast<void*>(t[p]),1,t[p+1],source);std::fclose(source);
                            }
                            std::fprintf(stderr,"[menu source match] slot=%x part=%d equal=%d\n",k,part,std::memcmp(reinterpret_cast<void*>(t[p]),reinterpret_cast<void*>((part?0x06880000:0x06800000)+t[p+2]),t[p+1])==0);
                        }
                    }
                    char vram_path[2048];
                    std::snprintf(vram_path,sizeof(vram_path),"%s-vram-1180.bin",prefix);
                    if(FILE *vram=std::fopen(vram_path,"wb")) {
                        std::fwrite(reinterpret_cast<void*>(0x06800000),1,0xa4000,vram);
                        std::fclose(vram);
                    }
                    std::set<const unsigned*> seen;
                    for(size_t i=0;i<count;++i) {
                        const auto &t=triangles[i];
                        if(!t.tex || !seen.insert(t.tex).second) continue;
                        char texpath[2048];
                        std::snprintf(texpath,sizeof(texpath),"%s-texture-%u.ppm",prefix,unsigned(i));
                        if(FILE *tex=std::fopen(texpath,"wb")) {
                            std::fprintf(tex,"P6\n%d %d\n255\n",t.tw,t.th);
                            for(int p=0;p<t.tw*t.th;++p) {
                                unsigned char rgb[]={static_cast<unsigned char>(t.tex[p]>>16),static_cast<unsigned char>(t.tex[p]>>8),static_cast<unsigned char>(t.tex[p])};
                                std::fwrite(rgb,1,3,tex);
                            }
                            std::fclose(tex);
                        }
                    }
                }
                std::fputs("triangle,vertex,x,y,z,w,clip_x,clip_y,clip_z,viewport_x,viewport_y,viewport_w,viewport_h,perspective\n",out);
                for(size_t i=0;i<count;++i) for(int j=0;j<3;++j) {
                    const auto &t=triangles[i];const auto &v=t.v[j];
                    std::fprintf(out,"%u,%d,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%d,%d,%d,%d,%d\n",
                        unsigned(i),j,v.x,v.y,v.z,v.w,v.clip_x,v.clip_y,v.clip_z,t.viewport_x,t.viewport_y,t.viewport_w,t.viewport_h,t.perspective);
                }
                std::fclose(out);
            }
            auto *g=static_cast<unsigned char*>(data_ov007_0210342c);
            auto *state=*reinterpret_cast<int**>(g+8);
            auto *wrapper=*reinterpret_cast<unsigned char**>(g+0x100);
            auto *model=*reinterpret_cast<unsigned char**>(wrapper+4);
            auto *ctrl=*reinterpret_cast<unsigned short**>(model+0x18);
            auto *components=*reinterpret_cast<unsigned char**>(model);
            auto *matrix=*reinterpret_cast<int**>(components+0xc);
            std::fprintf(stderr,"[native DSMT animation] tick=%llu state=%08x timer=%d callback=%d scale=%d,%d,%d frame=%u/%u root=",
                (unsigned long long)ntr::rt_frame(),state[0],state[3],*reinterpret_cast<int*>(wrapper+0x18)!=0,
                *reinterpret_cast<int*>(model+0x40),*reinterpret_cast<int*>(model+0x44),*reinterpret_cast<int*>(model+0x48),ctrl[8],ctrl[9]);
            for(int i=0;i<12;++i)std::fprintf(stderr,"%s%d",i?",":"",matrix[i]);
            std::fputc('\n',stderr);
        }
        const bool requested_layer=layer_tick && ntr::rt_frame()==std::strtoull(layer_tick,nullptr,10);
        if(prefix && prefix[0] && ((!layer_tick && ((!captured && ntr::rt_frame()>=300) || ntr::rt_frame()==1780)) || requested_layer)) {
            captured=true;
            char frame_prefix[2048];
            if(ntr::rt_frame()==1780) {
                std::snprintf(frame_prefix,sizeof(frame_prefix),"%s-file-select",prefix);
                prefix=frame_prefix;
            }
            if(requested_layer) {
                std::snprintf(frame_prefix,sizeof(frame_prefix),"%s-requested",prefix);
                prefix=frame_prefix;
            }
            char path[2048];
            static ntr::Framebuffer layer;
            for(int e=0;e<2;++e) {
                const auto engine=static_cast<ntr::Engine>(e);
                for(int bg=0;bg<4;++bg) {
                    ntr::ppu_scanout(engine,layer,false,1u<<bg);
                    std::snprintf(path,sizeof(path),"%s-%c-bg%d.bmp",prefix,'a'+e,bg);
                    ntr::ppu_write_bmp(path,layer);
                }
                ntr::ppu_scanout(engine,layer,false,0);
                ntr::ppu_scanout_obj(engine,layer);
                std::snprintf(path,sizeof(path),"%s-%c-obj.bmp",prefix,'a'+e);
                ntr::ppu_write_bmp(path,layer);
                const auto base=0x04000000u+e*0x1000u;
                std::fprintf(stderr,"[native DSMT layers] engine=%d display=%08x bg=%04x,%04x,%04x,%04x\n",e,
                    *reinterpret_cast<volatile unsigned*>(base),
                    *reinterpret_cast<volatile unsigned short*>(base+8),*reinterpret_cast<volatile unsigned short*>(base+10),
                    *reinterpret_cast<volatile unsigned short*>(base+12),*reinterpret_cast<volatile unsigned short*>(base+14));
            }
            std::snprintf(path,sizeof(path),"%s-registers.bin",prefix);
            if(FILE *out=std::fopen(path,"wb")){std::fwrite(reinterpret_cast<void*>(0x04000000u),1,0x1100,out);std::fclose(out);}
            std::snprintf(path,sizeof(path),"%s-oam.bin",prefix);
            if(FILE *out=std::fopen(path,"wb")){std::fwrite(reinterpret_cast<void*>(0x07000000u),1,0x800,out);std::fclose(out);}
            std::snprintf(path,sizeof(path),"%s-triangles.csv",prefix);
            if(FILE *out=std::fopen(path,"w")) {
                size_t count=0;const auto *triangles=ntr::gx_display_polygons(count);
                std::fprintf(out,"triangle,vertex,x,y,z,w,u,v,tw,th,attributes,perspective,color\n");
                for(size_t i=0;i<count;++i)for(int j=0;j<3;++j){const auto&t=triangles[i];const auto&v=t.v[j];
                    std::fprintf(out,"%u,%d,%g,%g,%g,%g,%g,%g,%d,%d,%08x,%d,%08x\n",unsigned(i),j,v.x,v.y,v.z,v.w,v.u,v.v,t.tw,t.th,t.attributes,t.perspective,v.color);}
                std::fclose(out);
            }
            auto *context=static_cast<unsigned char*>(data_ov007_0210342c);
            // Inspect the original BG upload lists without changing their state.
            // Layouts come from 020b72a0/020b7364/020c03b0/020c076c.
            if (auto *layers=*reinterpret_cast<unsigned char**>(context+0x74)) {
                for(int slot=8;slot<=12;slot+=4) {
                    auto *list=*reinterpret_cast<int**>(layers+slot);
                    if(!list)continue;
                    std::fprintf(stderr,"[native DSMT bg-list] slot=%d flags=%x mask=%x\n",slot,list[0],list[1]);
                    auto *node=reinterpret_cast<int*>(list[2]);
                    for(int index=0;node && index<64;++index) {
                        const auto *size=reinterpret_cast<unsigned short*>(node+7);
                        std::fprintf(stderr,"[native DSMT bg-node] slot=%d index=%d bg=%d priority=%d active=%d dirty=%d buffer=%08x offset=%x size=%ux%u\n",
                            slot,index,node[0],node[1],node[2],node[3],unsigned(node[4]),node[6],size[0],size[1]);
                        node=reinterpret_cast<int*>(node[12]);
                    }
                }
            }
            for(int k=4;k<=12;k+=4) {
                auto *state=*reinterpret_cast<unsigned char**>(context+k);
                std::fprintf(stderr,"[native DSMT state] offset=%d current=%d pending=%d work=%d previous=%d timer=%d\n",k,
                    *reinterpret_cast<short*>(state),*reinterpret_cast<short*>(state+2),*reinterpret_cast<int*>(state+4),
                    *reinterpret_cast<short*>(state+10),*reinterpret_cast<int*>(state+12));
            }
            for(int i=0;i<24;++i) {
                auto *entry=*reinterpret_cast<unsigned char**>(context+0x114+4*i);
                auto *parameters=*reinterpret_cast<int**>(entry);
                auto *quad=*reinterpret_cast<int**>(entry+4);
                auto *state=reinterpret_cast<short*>(parameters[1]);
                std::fprintf(stderr,"[native DSMT quad] i=%d visible=%d at=%d,%d quad=%08x position=%d,%d,%d size=%d,%d scale=%d,%d flags=%08x state=%d pending=%d timer=%d alpha=%d\n",i,
                    parameters[8],parameters[2],parameters[3],unsigned(quad),quad[5],quad[6],quad[7],quad[8],quad[9],quad[15],quad[16],quad[0],state[0],state[1],reinterpret_cast<int*>(state)[3],quad[20]);
            }
        }
    }
}
