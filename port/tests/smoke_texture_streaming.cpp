#include "ntr/gpu.h"
#include "ntr/gx.h"
#include "ntr/mmio.h"
#include "ntr/texture_assets.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>
namespace {
int failures;
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
constexpr uint64_t first_key=0x5a00000000000000ull;
uint32_t original[64];
std::string filename(uint64_t key) {
    char path[96];std::snprintf(path,sizeof(path),"textures/4k/%016llx.png",key);return path;
}
uint32_t color(unsigned i) {return 0xff000000u|((32+i*5)<<16)|((210-i*4)<<8)|(40+i*3);}
void quad(int left,int right,ntr::TextureReplacementRef r) {
    using namespace ntr;
    gx_bind_texture(original,8,8,8,8,texture_pixels_info(original,8,8),r);
    gx_write_port(0x040004a4,0x001f10c0);gx_write_port(0x04000480,0x7fff);gx_write_port(0x04000500,1);
    const short xy[4][2]={{short(left),-3277},{short(right),-3277},{short(right),3277},{short(left),3277}};
    const unsigned uv[4][2]={{0,128},{128,128},{128,0},{0,0}};
    for(unsigned i=0;i<4;++i) {
        gx_write_port(0x04000488,uv[i][0]|(uv[i][1]<<16));
        gx_write_port(0x0400048c,uint16_t(xy[i][0])|(uint32_t(uint16_t(xy[i][1]))<<16));gx_write_port(0x0400048c,0);
    }
    gx_write_port(0x04000504,0);
}
void scene(unsigned count,ntr::TextureRole role=ntr::TextureRole::Level,uint64_t key=first_key) {
    ntr::gx_reset();for(unsigned i=0;i<count;++i)
        quad(-4096+8192*i/count,-4096+8192*(i+1)/count,{key+i,role,true});
}
bool close_rgb(uint32_t a,uint32_t b,int tolerance=1) {
    for(unsigned shift=0;shift<24;shift+=8)if(std::abs(int((a>>shift)&255)-int((b>>shift)&255))>tolerance)return false;
    return true;
}
}
int main() {
    using namespace ntr;namespace fs=std::filesystem;
    const auto old=fs::current_path();
    const auto temp=old/("texture-streaming-smoke-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(temp/"textures/4k");fs::current_path(temp);
    CHECK(io_init());if(failures)return 1;
    CHECK(gpu_open(nullptr,true));if(failures)return 1;
    gpu_set_texture_budget_bytes(128ull*1024*1024);
    texture_set_backend(TextureBackend::NativeGpu);texture_set_quality(TextureQuality::Upscaled);
    std::fill(std::begin(original),std::end(original),0xffffffff);
    {
        // Reuse one staging vector to author 32 diagnostic PNGs (2 GiB decoded).
        std::vector<uint32_t> pixels(4096u*4096u);
        for(unsigned i=0;i<32;++i) {
            std::fill(pixels.begin(),pixels.end(),color(i));
            if(!i)for(unsigned y=0;y<4096;++y)
                std::fill(pixels.begin()+y*4096+2048,pixels.begin()+(y+1)*4096,(color(i)&0xffffff)|0x80000000);
            CHECK(texture_write_png(filename(first_key+i).c_str(),pixels,4096,4096));
        }
    }
    Framebuffer base;for(auto &row:base.px)for(auto &p:row)p=0xff102030;
    VideoSettings settings;settings.width=256;settings.height=192;settings.minimap=false;settings.upscale_2d=true;
    scene(32);CHECK(gpu_render(base,nullptr,settings,true));const auto stress=gpu_frame_stats();
    CHECK(stress.cache_bytes<=stress.cache_budget && stress.cache_budget==128ull*1024*1024);
    CHECK(stress.replacement_uploads>=33 && stress.evictions>=31);
    CHECK(stress.staging_peak_bytes==64ull*1024*1024);
    std::vector<uint32_t> rendered;unsigned width=0,height=0;CHECK(gpu_readback(rendered,width,height));
    CHECK(width==256 && height==192);
    if(rendered.size()!=256u*192u) {gpu_close();fs::current_path(old);return 1;}
    for(unsigned i=0;i<32;++i)CHECK(close_rgb(rendered[96*256+i*8+2],color(i)));
    uint32_t blended=0xff000000;
    for(unsigned shift=0;shift<24;shift+=8) {
        const unsigned c=(color(0)>>shift)&255,b=(0xff102030>>shift)&255;
        blended|=((c*17+b*15+16)/32)<<shift;
    }
    CHECK(close_rgb(rendered[96*256+6],blended));
    std::printf("STRESS: 32 x4096 assets, uploads=%llu evictions=%llu cache=%llu budget=%llu staging_peak=%llu\n",
        stress.replacement_uploads,stress.evictions,stress.cache_bytes,stress.cache_budget,stress.staging_peak_bytes);
    // Missing paths are attempted once per settings epoch, even across frames.
    const uint64_t absent=first_key+100;
    scene(1,TextureRole::Object,absent);CHECK(gpu_render(base,nullptr,settings,true));CHECK(gpu_frame_stats().replacement_attempts==1);
    CHECK(gpu_render(base,nullptr,settings,true));CHECK(gpu_frame_stats().replacement_attempts==0);
    CHECK(texture_write_png(filename(absent).c_str(),std::vector<uint32_t>(64,0xffab5678),8,8));
    CHECK(gpu_render(base,nullptr,settings,true));CHECK(gpu_frame_stats().replacement_attempts==0);
    texture_set_quality(TextureQuality::Original);texture_set_quality(TextureQuality::Upscaled);
    CHECK(gpu_render(base,nullptr,settings,true));CHECK(gpu_frame_stats().replacement_uploads==1);
    CHECK(gpu_readback(rendered,width,height));CHECK(close_rgb(rendered[96*256+128],0xffab5678));
    // A new epoch replaces already cached assets too.
    CHECK(texture_write_png(filename(absent).c_str(),std::vector<uint32_t>(64,0xff5678ab),8,8));
    CHECK(gpu_render(base,nullptr,settings,true));CHECK(gpu_frame_stats().replacement_uploads==0);
    texture_set_quality(TextureQuality::Original);texture_set_quality(TextureQuality::Upscaled);
    CHECK(gpu_render(base,nullptr,settings,true));CHECK(gpu_readback(rendered,width,height));
    CHECK(close_rgb(rendered[96*256+128],0xff5678ab));
    // Role is checked again at the GPU boundary, including malicious eligible refs.
    for(auto role:{TextureRole::Character,TextureRole::Unknown}) {
        scene(1,role,absent);CHECK(gpu_render(base,nullptr,settings,true));CHECK(gpu_frame_stats().replacement_attempts==0);
        CHECK(gpu_readback(rendered,width,height));CHECK(close_rgb(rendered[96*256+128],0xffffffff));
    }
    // Invalid data and decoded size limits fail safely, before a large allocation.
    const uint64_t invalid=first_key+101;std::ofstream(filename(invalid))<<"not a PNG";
    scene(1,TextureRole::Object,invalid);CHECK(gpu_render(base,nullptr,settings,true));CHECK(gpu_readback(rendered,width,height));
    CHECK(close_rgb(rendered[96*256+128],0xffffffff));
    std::vector<uint32_t> decoded;int tw=0,th=0;
    CHECK(!texture_load_replacement_pixels(first_key,decoded,tw,th,1024));CHECK(decoded.empty());
    // Native register decode retains 8x8 originals and passes replacement identity.
    gx_reset();auto *vram=reinterpret_cast<unsigned short *>(0x06800000u);
    for(unsigned i=0;i<64;++i)vram[i]=0x801f;
    unsigned char model[64]={};texture_model_register(model,64,"data/normal_obj/tree/bomb_tree.bmd");
    {
        TextureModelScope scope(model);gx_teximage_param(7u<<26);gx_write_port(0x04000500,0);
        for(int i=0;i<3;++i){gx_write_port(0x0400048c,uint16_t(i*512));gx_write_port(0x0400048c,0);}
        gx_write_port(0x04000504,0);size_t n=0;const auto *t=gx_polygons(n);CHECK(n==1);
        CHECK(t[0].tw==8 && t[0].th==8 && t[0].replacement.eligible && t[0].replacement.role==TextureRole::Object);
        CHECK(gx_texture_cache_bytes()==256);
    }
    // CPU tooling remains eager; software runtime explicitly retains originals.
    texture_model_register(model,64,"ARCHIVE/arc0.narc#5");
    for(auto consumer:{TextureRole::Object,TextureRole::Character,TextureRole::Unknown}) {
        gx_reset();TextureConsumerScope owner(consumer);TextureModelScope source(model);
        gx_teximage_param(7u<<26);gx_write_port(0x04000500,0);
        for(int i=0;i<3;++i){gx_write_port(0x0400048c,uint16_t(i*512));gx_write_port(0x0400048c,0);}
        gx_write_port(0x04000504,0);size_t n=0;const auto *tri=gx_polygons(n);CHECK(n==1);
        CHECK(tri[0].tw==8 && tri[0].th==8);
        CHECK(tri[0].replacement.eligible==(consumer==TextureRole::Object));
        CHECK(tri[0].replacement.role==consumer);
    }
    TextureDesc d;d.width=d.height=8;d.format=7;d.data=reinterpret_cast<unsigned char *>(vram);d.data_len=128;d.role=TextureRole::Object;
    CHECK(texture_write_png(texture_replacement_filename(d).c_str(),std::vector<uint32_t>(256,0xff123456),16,16));
    CHECK(texture_decode(d,decoded,&tw,&th));CHECK(tw==16);
    d.defer_replacement=true;CHECK(texture_decode(d,decoded,&tw,&th));CHECK(tw==8);
    texture_model_clear();gx_reset();gpu_close();texture_set_backend(TextureBackend::CpuDecode);texture_set_quality(TextureQuality::Original);
    fs::current_path(old);fs::remove_all(temp);
    if(failures)return 1;
    std::puts("PASS: bounded 4K GPU LRU, alpha across eviction, missing-file memo/reload, exclusions, original CPU retention and size guard");
}
