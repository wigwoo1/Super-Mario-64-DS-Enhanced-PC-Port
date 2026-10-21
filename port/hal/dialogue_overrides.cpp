#include "hal/dialogue_overrides.h"
#include <algorithm>
#include <cstring>

namespace pc_dialogue {
uint32_t crc32(const uint8_t *data,size_t size,uint32_t previous){
    uint32_t crc=~previous;
    for(size_t i=0;i<size;++i){crc^=data[i];for(int bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&uint32_t(-int(crc&1)));}
    return ~crc;
}
static uint32_t word(const uint8_t *p){return p[0]|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
bool Overrides::load(const uint8_t *file,size_t size,unsigned language){
    entries.clear();
    if(!file || size<16 || size>4*1024*1024 || std::memcmp(file,"SM64TXT1",8) || word(file+8)!=language)return false;
    const unsigned count=word(file+12);
    if(count>4096)return false;
    std::vector<Entry> pending;pending.reserve(count);size_t cursor=16;
    for(unsigned i=0;i<count;++i){
        if(size-cursor<12)return false;
        Entry e{word(file+cursor),word(file+cursor+4),{}};
        const size_t length=word(file+cursor+8);cursor+=12;
        if(e.id>65535 || !length || length>8192 || length>size-cursor)return false;
        e.text.assign(file+cursor,file+cursor+length);cursor+=length;pending.push_back(std::move(e));
    }
    if(cursor!=size)return false;
    std::sort(pending.begin(),pending.end(),[](const Entry&a,const Entry&b){return a.id<b.id;});
    for(size_t i=1;i<pending.size();++i)if(pending[i-1].id==pending[i].id)return false;
    entries=std::move(pending);return true;
}
static bool structure(const uint8_t *p,size_t size,const uint8_t *widths,unsigned limit,
                      std::vector<uint8_t> &controls){
    if(!size || size>8192)return false;
    unsigned width=0,glyphs=0;
    for(size_t i=0;i<size;){
        const unsigned c=p[i++];
        if(c==0xff){controls.push_back(0xff);return i==size && (!limit || width<=limit);}
        if(c==0xfe){
            if(i>=size)return false;
            const unsigned n=p[i];
            if(n<3 || n-1>size-i)return false;
            controls.insert(controls.end(),p+i-1,p+i-1+n);i+=n-1;continue;
        }
        if(c>=0xd0)controls.push_back(static_cast<uint8_t>(c));
        if(c==0xfd){if(limit && width>limit)return false;width=0;glyphs=0;}
        else {if(++glyphs>255)return false;width+=widths[c];}
    }
    return false;
}
const uint8_t *Overrides::resolve(unsigned id,const uint8_t *original,size_t size,
                                  const uint8_t *info,const uint8_t *widths)const{
    if(!original || !info || !widths)return original;
    auto it=std::lower_bound(entries.begin(),entries.end(),id,[](const Entry&e,unsigned value){return e.id<value;});
    if(it==entries.end() || it->id!=id || crc32(original,size,crc32(info,4))!=it->original_crc)return original;
    const unsigned width=info[0]|unsigned(info[1])<<8;
    if(!width)return original;
    std::vector<uint8_t> before,after;
    if(!structure(original,size,widths,0,before) ||
       !structure(it->text.data(),it->text.size(),widths,(std::min)(width,255u),after) || before!=after)return original;
    return it->text.data();
}
}
