#include "ntr/minimap_hud.h"
#include "ntr/fs.h"
#include <array>
#include <vector>
#include <cstring>
#include <cstdio>

extern "C" {
int GetOwnerLanguage();
unsigned pc_fs_load_compressed_file_at(unsigned,void *);
unsigned pc_fs_load_file_at(unsigned,void *);
extern unsigned char *_ZN3OAM10POWER_STARE,*_ZN3OAM4COINE;
extern unsigned char *_ZN3OAM10LIFE_ICONSE;
extern unsigned data_ov002_0210c230[],data_ov002_0210c254[],data_ov002_0210c278[];
extern unsigned char data_ov002_0210c690[],data_ov002_0210d4f0[],data_ov002_0210d5d0[];
extern int data_ov002_0210c29c[];
extern unsigned short data_ov002_0210c310[];
}

namespace {
struct Image {
    ntr::OverlayInfo info{};std::vector<uint8_t> data;
    bool load(unsigned id) {
        if(ntr::fs_overlay_info(id,&info)!=0)return false;
        data.resize(size_t(info.ram_size)+info.bss_size);
        return ntr::fs_overlay_load(id,data.data(),int(data.size()))==int(data.size());
    }
    const uint8_t *at(unsigned address) const {
        if(address<info.ram_address || address-info.ram_address>=data.size())return nullptr;
        return data.data()+address-info.ram_address;
    }
    size_t available(unsigned address) const {return at(address)?data.size()-(address-info.ram_address):0;}
};
bool append(std::vector<uint8_t> &list,const uint8_t *p,size_t available) {
    if(!p)return false;
    for(size_t offset=0;offset+8<=available && offset<128*8;offset+=8) {
        list.insert(list.end(),p+offset,p+offset+8);
        if(p[offset+6]==255 && p[offset+7]==255)return true;
    }
    return false;
}
}

// Decode private copies of the exact resources loaded by dMeter_c. Never call
// its initializer here: that writes the live main/sub OBJ VRAM and palettes.
bool pc_minimap_hud_art_prepare() {
    static int attempted_language=-1;static bool ready=false;
    const int language=GetOwnerLanguage();
    if(attempted_language==language)return ready;
    attempted_language=language;ready=false;
    auto failed=[&](int line){std::fprintf(stderr,"[minimap HUD art] decode unavailable at line %d, language=%d\n",line,language);return false;};
    auto live_hash=[](){
        uint64_t hash=14695981039346656037ull;
        const uintptr_t addresses[]={0x06400000,0x06600000,0x05000000,0x07000000};
        const size_t sizes[]={0x10000,0x10000,0x800,0x800};
        for(unsigned i=0;i<4;++i)for(size_t j=0;j<sizes[i];++j)hash=(hash^reinterpret_cast<const uint8_t *>(addresses[i])[j])*1099511628211ull;
        return hash;
    };
    const auto before=live_hash();
    Image common,stage;
    if(!common.load(1)||!stage.load(2))return failed(__LINE__);
    static std::array<uint8_t,0x20000> tiles,base_palette,extra_palette;
    const unsigned graphics=language==5?0x270:language==4?0x26e:language==3?0x26c:language==2?0x26a:0x269;
    const unsigned archive=language==5?0xb000:language==4?0xac00:language==3?0xa800:language==2?0xa400:0xa000;
    tiles.fill(0);base_palette.fill(0);extra_palette.fill(0);
    const unsigned base_size=pc_fs_load_compressed_file_at(graphics,tiles.data());
    const unsigned extra_size=pc_fs_load_compressed_file_at(archive+14,tiles.data()+0x2000);
    if(!base_size || !extra_size || pc_fs_load_file_at(archive+10,base_palette.data())<0x120 ||
       pc_fs_load_file_at(0x980f,extra_palette.data())<0xe0)return failed(__LINE__);
    std::array<uint8_t,512> palette;
    std::memcpy(palette.data(),base_palette.data(),0x120);
    std::memcpy(palette.data()+0x120,extra_palette.data(),0xe0);
    ntr::MinimapHudArt art;
    auto decode=[&](ntr::HudSprite &sprite,const uint8_t *p,size_t bytes,const uint8_t *colors) {
        return ntr::minimap_hud_decode(sprite,p,bytes,tiles.data(),tiles.size(),colors,palette.size());
    };
    // overlay1_image exposes these named data symbols as DS-address pointer
    // variables. Resolve their values against the immutable overlay copy.
    if(!decode(art.star,common.at(unsigned(reinterpret_cast<uintptr_t>(_ZN3OAM10POWER_STARE))),8,palette.data()) ||
       !decode(art.coin,common.at(unsigned(reinterpret_cast<uintptr_t>(_ZN3OAM4COINE))),8,palette.data()))return failed(__LINE__);
    const auto *life_table=common.at(unsigned(reinterpret_cast<uintptr_t>(_ZN3OAM10LIFE_ICONSE)));
    if(!life_table)return failed(__LINE__);
    for(unsigned character=0;character<4;++character) {
        unsigned address;std::memcpy(&address,life_table+character*4,4);
        if(!decode(art.lives[character],common.at(address),common.available(address),palette.data()))return failed(__LINE__);
    }
    const auto *health_table=language==5?data_ov002_0210c254:
        language==4 || language==2?data_ov002_0210c278:data_ov002_0210c230;
    const auto *frame=language==5?data_ov002_0210d5d0:
        language==4 || language==2?data_ov002_0210d4f0:data_ov002_0210c690;
    for(unsigned health=0;health<=8;++health) {
        std::vector<uint8_t> combined;
        const unsigned address=health_table[health];
        if(!append(combined,stage.at(address),stage.available(address)))return failed(__LINE__);
        combined[combined.size()-2]=combined[combined.size()-1]=0;
        if(!append(combined,frame,128*8))return failed(__LINE__);
        auto colors=palette;
        const int offset=data_ov002_0210c29c[health];
        if(offset<0 || offset>48)return failed(__LINE__);
        std::memcpy(colors.data()+0x60,data_ov002_0210c310+offset,32);
        if(!decode(art.health[health],combined.data(),combined.size(),colors.data()))return failed(__LINE__);
    }
    // dCamera_c::Render uses this four-part original OAM template at128,96.
    // Its HUD tiles/palette are decoded privately, like the counter artwork.
    if(!decode(art.cannon_sight,stage.at(0x0210c3b0),32,palette.data()) ||
       art.cannon_sight.width!=32 || art.cannon_sight.height!=32)return failed(__LINE__);
    for(auto &pixel:art.cannon_sight.pixels)if(pixel>>24)
        pixel=(pixel&0xffffffu)|0xbf000000u; // original source coefficient12/16
    if(live_hash()!=before)return failed(__LINE__);
    ntr::minimap_hud_set_art(std::move(art));ready=true;
    std::fprintf(stderr,"[minimap HUD art] original Star/coin/4 life/9 health/32x32 cannon sight sprites decoded privately, live OBJ/palettes/OAM unchanged, language=%d\n",language);
    return true;
}
