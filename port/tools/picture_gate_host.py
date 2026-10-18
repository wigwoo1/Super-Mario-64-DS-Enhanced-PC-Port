"""Native construction and shared resource ownership for original painting actors."""
from pathlib import Path
import re
import struct
ROOT=Path(__file__).resolve().parents[2]

def transform(text):
    if 'extern "C" daPicGate_c *daPicGate_c_classInit()' in text:
        handles=re.findall(r'PicGateFilePtr (data_ov080_[0-9a-f]+)\((0x[0-9a-f]+)\);',text)
        assert len(handles)==19
        raw=(ROOT/'extracted/overlays/overlay_0080.bin').read_bytes()
        order=struct.unpack_from('<19I',raw,0x0212775c-0x02123740)
        assert [int(n[-8:],16) for n,_ in handles]==list(order)
        # Empty base + two words is the original eight-byte shared handle.
        text=text.replace('    PicGateFilePtr(u32 fileID);\n    ~PicGateFilePtr();','')
        text=re.sub(r'PicGateFilePtr (data_ov080_[0-9a-f]+)\((0x[0-9a-f]+)\);',r'PicGateFilePtr \1 = {};',text)
        old='''    int *p = (int *)_ZN7fBase_cnwEj(sizeof(daPicGate_c));
    if (p) {
        _ZN8dActor_cC2Ev(p);
        p[0] = (int)_ZTV11daPicGate_c;
    }
    return (daPicGate_c *)p;'''
        assert text.count(old)==1
        text=text.replace(old,'''    pc_picgate_prepare();
    auto *p = new daPicGate_c;
    if(p) {
        ++pc_picgate_live;
        const char *trace=std::getenv("SM64DS_PC_PICGATE_TRACE");
        if(trace && trace[0]=='1') std::fprintf(stderr,"[native painting] actor created live=%u\\n",pc_picgate_live);
    }
    return p;''')
        text='#include <cstddef>\n#include <cstdio>\n#include <cstdlib>\n#include <cstring>\nextern "C" { void pc_picgate_prepare(); extern unsigned pc_picgate_live; }\n'+text
        suffix=['static_assert(sizeof(daPicGate_c)==0x1bc && offsetof(daPicGate_c,mCells)==0x1a0,"painting actor layout");',
                'static_assert(sizeof(PicGateFilePtr)==8,"painting shared handle layout");',
                'extern "C" {', 'unsigned pc_picgate_live=0;',
                'alignas(4) unsigned char pc_picgate_textures[19*24]={};',
                'SharedFilePtr *pc_picgate_handles[19]={'+','.join('&'+n for n,_ in handles)+'};',
                'void pc_picgate_prepare() {',
                'if(!pc_picgate_live) { std::memset(pc_picgate_textures,0,sizeof(pc_picgate_textures)); const char *trace=std::getenv("SM64DS_PC_PICGATE_TRACE"); if(trace && trace[0]==49) std::fprintf(stderr,"[native painting] texture cache reset\\n"); }',
                'static bool ready=false; if(ready)return;ready=true;']
        suffix += [f'{n}.Construct({fid});' for n,fid in handles]
        suffix += ['}', '}']
        text+='\n'+'\n'.join(suffix)+'\n'
    if 'u8 *daPicGate_c::func_ov080_02125630(s32 i)' in text:
        text='#include <cstdio>\n#include <cstdlib>\n'+text
        text=text.replace('extern SharedFilePtr *data_ov080_0212775c[];',
            'extern "C" SharedFilePtr *pc_picgate_handles[19];\n#define data_ov080_0212775c pc_picgate_handles')
        text=text.replace('extern unsigned char data_ov080_02128688[];\nextern unsigned char data_ov080_0212869c[];',
            'extern "C" unsigned char pc_picgate_textures[19*24];\n#define data_ov080_02128688 pc_picgate_textures\n#define data_ov080_0212869c (pc_picgate_textures+20)')
        anchor='daPicGate_c::~daPicGate_c()\n{\n}'
        assert text.count(anchor)==1
        text=text.replace(anchor,'extern "C" unsigned pc_picgate_live;\n'+anchor.replace('{\n}','{\n    if(pc_picgate_live) --pc_picgate_live;\n    const char *trace=std::getenv("SM64DS_PC_PICGATE_TRACE"); if(trace && trace[0]==49) std::fprintf(stderr,"[native painting] actor destroyed live=%u\\n",pc_picgate_live);\n}'))
    return text
