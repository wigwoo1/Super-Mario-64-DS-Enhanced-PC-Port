#include "ntr/texture_assets.h"
#include "ntr/gx.h"
#include "ntr/mmio.h"
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <vector>
namespace {
int failures;
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
const ntr::GxTriangle &triangle() {
    ntr::gx_write_port(0x04000500,0);
    for(int i=0;i<3;++i) {ntr::gx_write_port(0x0400048c,uint16_t(i*512));ntr::gx_write_port(0x0400048c,0);}
    ntr::gx_write_port(0x04000504,0);
    size_t count;const auto *t=ntr::gx_polygons(count);return t[count-1];
}
}
int main() {
    using namespace ntr;namespace fs=std::filesystem;
    size_t count;texture_model_records(count);CHECK(count==505);
    CHECK(texture_model_path_role("DATA\\PLAYER\\B_MARIO_ALL.BMD")==TextureRole::Character);
    CHECK(texture_model_path_role("data/normal_obj/fish/fish.bmd")==TextureRole::Character);
    CHECK(texture_model_path_role("data/normal_obj/tree/bomb_tree.bmd")==TextureRole::Object);
    CHECK(texture_model_path_role("data/stage/main_castle/main_castle_all.bmd")==TextureRole::Level);
    CHECK(texture_model_path_role("data/DSMT/EUR/texBank_E.bmd")==TextureRole::Unknown);
    CHECK(texture_model_path_role("nonexistent.bmd")==TextureRole::Unknown);
    // All four body/capped/hatless head tables plus the wing attachment have
    // source/ROM evidence, including anonymous archive copies used in play.
    for(unsigned member:{126u,127u,128u,129u,130u,131u,184u,185u,186u,189u,194u,195u,196u})
        CHECK(texture_model_path_role(("ARCHIVE/arc0.narc#"+std::to_string(member)).c_str())==TextureRole::Character);
    CHECK(texture_model_path_role("ARCHIVE/ar1.narc#1")==TextureRole::Character);
    CHECK(texture_model_path_role("ARCHIVE/ar1.narc#2")==TextureRole::Character);
    CHECK(texture_model_path_role("ARCHIVE/ar1.narc#9")==TextureRole::Object);
    CHECK(texture_model_path_role("ARCHIVE/ar1.narc#13")==TextureRole::Object);
    CHECK(texture_model_path_role("ARCHIVE/ar1.narc#5")==TextureRole::Level);
    for(unsigned member:{5u,7u,16u,17u,20u})
        CHECK(texture_model_path_role(("ARCHIVE/arc0.narc#"+std::to_string(member)).c_str())==TextureRole::Unknown);

    unsigned char models[3][64]={};
    texture_model_register(models[0],64,"data/normal_obj/tree/bomb_tree.bmd");
    texture_model_register(models[1],64,"data/player/b_mario_all.bmd");
    texture_model_register(models[2],64,"data/stage/main_castle/main_castle_all.bmd");
    CHECK(texture_model_current().role==TextureRole::Unknown);
    {
        TextureModelScope object(models[0]);CHECK(texture_model_current().role==TextureRole::Object);
        {TextureModelScope player(models[1]);CHECK(texture_model_current().role==TextureRole::Character);}
        CHECK(texture_model_current().role==TextureRole::Object);
    }
    CHECK(texture_model_current().role==TextureRole::Unknown);
    texture_model_forget_range(models[0],models[1]);CHECK(texture_model_origin(models[0]).role==TextureRole::Unknown);
    CHECK(texture_model_origin(models[1]).role==TextureRole::Character);
    texture_model_register(models[0],64,"data/normal_obj/tree/bomb_tree.bmd");

    CHECK(io_init());gx_reset();
    auto *vram=reinterpret_cast<unsigned short *>(0x06800000u);
    for(unsigned i=0;i<64;++i)vram[i]=0x801f;
    TextureDesc d;d.width=d.height=8;d.format=7;d.data=reinterpret_cast<unsigned char *>(vram);d.data_len=128;
    const auto key=texture_replacement_key(d);CHECK(key!=0);
    const auto old=fs::current_path();const auto temp=old/("texture-ownership-smoke-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    CHECK(fs::create_directory(temp));fs::current_path(temp);fs::create_directories("textures/4k");
    const auto filename=texture_replacement_filename(d);
    CHECK(texture_write_png(filename.c_str(),std::vector<uint32_t>(256,0xffabcdef),16,16));
    texture_set_quality(TextureQuality::Upscaled);
    {
        TextureModelScope object(models[0]);gx_teximage_param(7u<<26);CHECK(triangle().tw==16);
        {TextureModelScope player(models[1]);gx_teximage_param(7u<<26);CHECK(triangle().tw==8 && triangle().tex[0]==0xffff0000);}
        gx_teximage_param(7u<<26);CHECK(triangle().tw==16 && triangle().tex[0]==0xffabcdef);
        {TextureModelScope level(models[2]);gx_teximage_param(7u<<26);CHECK(triangle().tw==16);}
    }
    gx_teximage_param(7u<<26);CHECK(triangle().tw==8); // No origin cannot inherit eligible cached art.
    size_t shared_count;const auto *shared=texture_shared_model_records(shared_count);CHECK(shared_count==9);
    for(size_t i=0;i<shared_count;++i) {
        CHECK(texture_model_path_role(shared[i].path)==TextureRole::Unknown);
        CHECK(texture_model_draw_role(TextureRole::Unknown,shared[i].path,TextureRole::Object)==TextureRole::Object);
        CHECK(texture_model_draw_role(TextureRole::Unknown,shared[i].path,TextureRole::Character)==TextureRole::Character);
        CHECK(texture_model_draw_role(TextureRole::Unknown,shared[i].path,TextureRole::Unknown)==TextureRole::Unknown);
    }
    CHECK(texture_model_draw_role(TextureRole::Character,shared[0].path,TextureRole::Object)==TextureRole::Character);
    CHECK(texture_model_draw_role(TextureRole::Unknown,"unknown-model",TextureRole::Object)==TextureRole::Unknown);
    CHECK(texture_model_draw_role(TextureRole::Unknown,"ARCHIVE/arc0.narc#192",TextureRole::Object)==TextureRole::Unknown);
    texture_model_register(models[0],64,"ARCHIVE/arc0.narc#5");
    CHECK(texture_consumer_role()==TextureRole::Unknown);
    {
        TextureConsumerScope coin(TextureRole::Object);
        {TextureModelScope model(models[0]);gx_teximage_param(7u<<26);CHECK(triangle().tw==16);}
        {
            TextureConsumerScope enemy(TextureRole::Character);
            {TextureModelScope model(models[0]);gx_teximage_param(7u<<26);CHECK(triangle().tw==8);}
            {TextureModelScope object(models[2]);CHECK(texture_model_current().role==TextureRole::Character);}
            // An independent nested object draw is explicit and then restores enemy scope.
            {TextureConsumerScope nested(TextureRole::Object);TextureModelScope model(models[0]);
                gx_teximage_param(7u<<26);CHECK(triangle().tw==16);}
            CHECK(texture_consumer_role()==TextureRole::Character);
        }
        CHECK(texture_consumer_role()==TextureRole::Object);
        {TextureModelScope model(models[1]);gx_teximage_param(7u<<26);CHECK(triangle().tw==8);}
    }
    CHECK(texture_consumer_role()==TextureRole::Unknown);
    {TextureModelScope model(models[0]);gx_teximage_param(7u<<26);CHECK(triangle().tw==8);}
    CHECK(texture_model_origin(models[0]).role==TextureRole::Unknown);

    // Referenced palette entries change keys; VRAM padding does not.
    unsigned char indices[64]={},pal[512]={};d.format=4;d.data=indices;d.data_len=64;d.pal=pal;d.pal_len=2;
    const auto finite=texture_replacement_key(d);d.pal_len=512;pal[511]=255;
    CHECK(texture_replacement_key(d)==finite);pal[0]=1;CHECK(texture_replacement_key(d)!=finite);
    unsigned char compressed[4]={},info[2]={0,0x40}; // format5 interpolation uses only two colors.
    d.width=d.height=4;d.format=5;d.data=compressed;d.data_len=4;d.index=info;d.index_len=2;d.pal_len=4;
    const auto compact=texture_replacement_key(d);d.pal_len=512;pal[4]=0xff;
    CHECK(texture_replacement_key(d)==compact);
    // New content at the same VRAM address cannot reuse old pixels. Reclaim
    // history after presentation without invalidating either geometry buffer.
    texture_set_quality(TextureQuality::Original);gx_reset();
    for(unsigned frame=0;frame<100;++frame) {
        vram[0]=static_cast<unsigned short>(0x8000|frame);
        gx_teximage_param(7u<<26);const auto pixel=triangle().tex[0];
        CHECK(pixel!=0xffabcdef);
        gx_write_port(0x04000540,0);CHECK(gx_texture_cache_entries()<=2);
    }
    texture_model_forget(models[1]);CHECK(texture_model_origin(models[1]).role==TextureRole::Unknown);
    texture_model_register(models[0],64,"unrecognized.bin");CHECK(texture_model_origin(models[0]).role==TextureRole::Unknown);
    texture_model_clear();gx_reset();texture_set_quality(TextureQuality::Original);
    fs::remove(filename);fs::remove("textures/4k/manifest.tsv");fs::remove("textures/4k");fs::remove("textures");fs::current_path(old);fs::remove(temp);
    if(failures)return 1;
    std::puts("PASS: explicit ownership, nested render contexts, lifetime/range removal, shared-key character exclusion, palette key stability, live VRAM content changes and bounded cache history");
}
