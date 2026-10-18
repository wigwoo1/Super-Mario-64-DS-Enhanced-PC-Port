#include "ntr/fs.h"
#include "ntr/lz77.h"
#include "ntr/texture.h"
#include "ntr/texture_assets.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
uint32_t word(const std::vector<uint8_t> &b,size_t p) {
    if(p>b.size() || b.size()-p<4)throw std::runtime_error("word outside model");
    return b[p]|(uint32_t(b[p+1])<<8)|(uint32_t(b[p+2])<<16)|(uint32_t(b[p+3])<<24);
}
void span(const std::vector<uint8_t> &b,size_t p,size_t count) {
    if(p>b.size() || count>b.size()-p)throw std::runtime_error("table outside model");
}
std::string name(const std::vector<uint8_t> &b,size_t p) {
    if(p==0)return "";
    span(b,p,1);std::string out;
    while(p<b.size() && b[p]) {auto c=b[p++];out+=c=='\t'||c=='\r'||c=='\n'?' ':char(c);}
    if(p==b.size())throw std::runtime_error("unterminated model name");return out;
}
std::string hex(uint64_t value) {char text[32];std::snprintf(text,sizeof(text),"%016llx",static_cast<unsigned long long>(value));return text;}
std::vector<uint8_t> member(const std::vector<uint8_t> &archive,unsigned index) {
    span(archive,0,16);if(std::memcmp(archive.data(),"NARC",4))throw std::runtime_error("invalid NARC");
    size_t pos=archive[12]|(unsigned(archive[13])<<8),start=0,end=0,image=0,image_size=0;bool found=false;
    while(pos+8<=archive.size()) {
        const auto size=word(archive,pos+4);if(size<8)throw std::runtime_error("invalid NARC block");span(archive,pos,size);
        if(!std::memcmp(archive.data()+pos,"BTAF",4)) {
            const auto count=word(archive,pos+8)&0xffff;
            if(index>=count)throw std::runtime_error("NARC member out of range");
            start=word(archive,pos+12+index*8);end=word(archive,pos+16+index*8);found=true;
        }
        if(!std::memcmp(archive.data()+pos,"GMIF",4)) {image=pos+8;image_size=size-8;}
        pos+=size;
    }
    if(!found||end<start||end>image_size)throw std::runtime_error("invalid NARC member range");
    return {archive.begin()+image+start,archive.begin()+image+end};
}
const char *role(ntr::TextureRole r) {return r==ntr::TextureRole::Level?"Level":r==ntr::TextureRole::Object?"Object":r==ntr::TextureRole::Character?"Character":"Unknown";}
}
int main(int argc,char **argv) {
    if(argc<3) {std::fprintf(stderr,"Usage: texture_inventory ROM OUTPUT_DIR [--export]\n");return 2;}
    if(!ntr::fs_mount_rom(argv[1]))return 3;
    bool export_images=false;std::filesystem::path fixture;
    for(int arg=3;arg<argc;++arg) {
        if(!std::strcmp(argv[arg],"--export"))export_images=true;
        else if(!std::strcmp(argv[arg],"--test-pack") && arg+1<argc)fixture=argv[++arg];
        else return 2;
    }
    if(!fixture.empty())std::filesystem::create_directories(fixture/"textures/4k");
    const std::filesystem::path output=argv[2];std::filesystem::create_directories(output/"source");
    std::ofstream models(output/"models.tsv"),textures(output/"textures.tsv"),bindings(output/"bindings.tsv");
    if(!models||!textures||!bindings)return 4;
    models<<"file_id\tpath\trole\tbasis\tbytes\tfingerprint\ttextures\tpalettes\tmaterials\tarchive_identity_role\n";
    textures<<"file_id\tmodel\ttexture_id\tname\twidth\theight\tformat\tflags\trepeat_x\trepeat_y\tflip_x\tflip_y\tmaterial_bindings\n";
    bindings<<"file_id\tmodel\trole\tmaterial_id\tmaterial\ttexture_id\ttexture\tpalette_id\tpalette\twidth\theight\tformat\tkey\tzero_alpha\tpartial_alpha\tmaterial_flags\n";
    size_t model_count=0,texture_count=0,binding_count=0,unused=0,errors=0;
    std::set<std::string> exported,fixtures;
    ntr::texture_set_quality(ntr::TextureQuality::Original);
    size_t record_count;const auto *records=ntr::texture_model_records(record_count);
    for(size_t record=0;record<record_count;++record) {
        const std::string path=records[record].path;const auto separator=path.find('#');
        const std::string container=path.substr(0,separator);
        const int id=ntr::fs_id_by_path(container.c_str());
        try {
            const int packed_size=ntr::fs_size(id);if(packed_size<=0)throw std::runtime_error("empty model");
            std::vector<uint8_t> packed(packed_size);if(ntr::fs_read(id,packed.data(),packed_size)!=packed_size)throw std::runtime_error("short read");
            if(separator!=std::string::npos)packed=member(packed,static_cast<unsigned>(std::stoul(path.substr(separator+1))));
            std::vector<uint8_t> b;const int unpacked=ntr::lz77_size(packed.data(),static_cast<int>(packed.size()));
            if(unpacked>0) {b.resize(unpacked);if(ntr::lz77_unpack(packed.data(),static_cast<int>(packed.size()),b.data(),unpacked)!=unpacked)throw std::runtime_error("decompression failed");}
            else b=std::move(packed);
            span(b,0,0x38);
            const auto nt=word(b,0x14),pt=word(b,0x18),np=word(b,0x1c),pp=word(b,0x20),nm=word(b,0x24),pm=word(b,0x28);
            if(nt>4096||np>4096||nm>4096)throw std::runtime_error("excessive model table");
            span(b,pt,size_t(nt)*20);span(b,pp,size_t(np)*16);span(b,pm,size_t(nm)*48);
            const auto model_role=ntr::texture_model_path_role(path.c_str());
            size_t manifest_count;const auto *manifest=ntr::texture_model_records(manifest_count);const char *basis="missing ownership record";
            for(size_t i=0;i<manifest_count;++i)if(path==manifest[i].path)basis=manifest[i].basis;
            ntr::texture_model_register(b.data(),b.size(),nullptr);
            const auto archive_role=ntr::texture_model_origin(b.data()).role;
            ntr::texture_model_forget(b.data());
            models<<id<<'\t'<<path<<'\t'<<role(model_role)<<'\t'<<basis<<'\t'<<b.size()<<'\t'<<hex(ntr::texture_model_fingerprint(b.data(),b.size()))<<'\t'<<nt<<'\t'<<np<<'\t'<<nm<<'\t'<<role(archive_role)<<'\n';
            std::vector<unsigned> references(nt);
            for(unsigned material=0;material<nm;++material) {
                const size_t m=pm+material*48u;const unsigned ti=word(b,m+4),pi=word(b,m+8);
                if(ti==0xffffffffu)continue;
                if(ti>=nt)throw std::runtime_error("material texture out of range");
                ++references[ti];const size_t t=pt+ti*20u;const auto flags=word(b,t+16),data=word(b,t+4),bytes=word(b,t+8);
                ntr::TextureDesc d;d.width=8<<((flags>>20)&7);d.height=8<<((flags>>23)&7);d.format=(flags>>26)&7;d.color0_transparent=(flags>>29)&1;d.role=model_role;
                span(b,data,bytes);d.data=b.data()+data;d.data_len=bytes;
                if(d.format==5) {span(b,size_t(data)+bytes,bytes/2);d.index=b.data()+data+bytes;d.index_len=bytes/2;}
                std::string palette_name;
                if(d.format!=7) {
                    if(pi>=np)throw std::runtime_error("material palette out of range");
                    const size_t p=pp+pi*16u;const auto pd=word(b,p+4),ps=word(b,p+8);span(b,pd,ps);
                    d.pal=b.data()+pd;d.pal_len=ps;palette_name=name(b,word(b,p));
                }
                std::vector<uint32_t> pixels;int w=0,h=0;
                if(!ntr::texture_decode(d,pixels,&w,&h))throw std::runtime_error("texture decode failed");
                const auto key=hex(ntr::texture_replacement_key(d));
                // Synthetic diagnostic colors, never production artwork.
                // Deliberately supply a character PNG to prove it is rejected.
                const auto texture_name=name(b,word(b,t));
                if(!fixture.empty() && ((path=="data/stage/main_castle/main_castle_all.bmd" && texture_name=="mc_grass") ||
                    (path=="data/player/b_mario_all.bmd" && texture_name=="mario_body")) && fixtures.insert(key).second) {
                    const uint32_t color=model_role==ntr::TextureRole::Character?0xff00ffff:0xffe040e0;
                    if(!ntr::texture_write_png((fixture/"textures/4k"/(key+".png")).string().c_str(),
                        std::vector<uint32_t>(4096u*4096u,color),4096,4096))throw std::runtime_error("test PNG export failed");
                }
                unsigned zero=0,partial=0;for(auto p:pixels) {zero+=(p>>24)==0;partial+=(p>>24)>0&&(p>>24)<255;}
                bindings<<id<<'\t'<<path<<'\t'<<role(model_role)<<'\t'<<material<<'\t'<<name(b,word(b,m))<<'\t'<<ti<<'\t'<<name(b,word(b,t))<<'\t'<<pi<<'\t'<<palette_name<<'\t'<<w<<'\t'<<h<<'\t'<<d.format<<'\t'<<key<<'\t'<<zero<<'\t'<<partial<<'\t'<<word(b,m+0x24)<<'\n';
                if(export_images && exported.insert(key).second && !ntr::texture_write_png((output/"source"/(key+".png")).string().c_str(),pixels,w,h))throw std::runtime_error("PNG export failed");
                ++binding_count;
            }
            for(unsigned texture=0;texture<nt;++texture) {
                const size_t t=pt+texture*20u;const auto f=word(b,t+16);
                textures<<id<<'\t'<<path<<'\t'<<texture<<'\t'<<name(b,word(b,t))<<'\t'<<(8<<((f>>20)&7))<<'\t'<<(8<<((f>>23)&7))<<'\t'<<((f>>26)&7)<<'\t'<<f<<'\t'<<((f>>16)&1)<<'\t'<<((f>>17)&1)<<'\t'<<((f>>18)&1)<<'\t'<<((f>>19)&1)<<'\t'<<references[texture]<<'\n';
                unused+=references[texture]==0;++texture_count;
            }
            ++model_count;
        } catch(const std::exception &e) {std::fprintf(stderr,"FAIL %s: %s\n",path.c_str(),e.what());++errors;}
    }
    ntr::fs_unmount();
    std::printf("INVENTORY: models=%zu textures=%zu material_bindings=%zu unreferenced_textures=%zu unique_reference_PNGs=%zu errors=%zu\n",model_count,texture_count,binding_count,unused,exported.size(),errors);
    if(!fixture.empty())std::printf("TEST_PACK: %zu synthetic fixtures; not AI artwork\n",fixtures.size());
    return errors?1:0;
}
