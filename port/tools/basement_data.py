from pathlib import Path
import struct
ROOT=Path(__file__).resolve().parents[2]
BASE=0x021111a0
START,HEADER,END=0x02111768,0x02111c24,0x02111c3c

def generate(root,out):
    image=(root/'extracted/overlays/overlay_0012.bin').read_bytes()
    blob=image[START-BASE:END-BASE]
    if len(blob)!=END-START:raise ValueError('Incomplete basement animation span')
    h=HEADER-START
    count=struct.unpack_from('<H',blob,h+16)[0]
    elements=struct.unpack_from('<I',blob,h+20)[0]
    if count!=1:raise ValueError('Basement animation channel count changed')
    offsets=[h+4,h+8,h+12,h+20]+[elements-START+i*28+4 for i in range(count)]
    for offset in offsets:
        if offset<0 or offset+4>len(blob):raise ValueError('BTA field outside local blob')
        pointer=struct.unpack_from('<I',blob,offset)[0]
        if not START<=pointer<END:raise ValueError('BTA pointer outside reviewed local span')
    lines=['// Generated from the local ROM. Never publish this data file.','#include <cstdint>','#include <cstring>','static_assert(sizeof(void *)==4, "Native DS data requires x86 pointers");','extern "C" {']
    def emit(name,data):
        lines.append('__declspec(align(4)) unsigned char '+name+'[] = {'+','.join(str(x) for x in data)+'};')
    emit('pc_basement_bta_bytes',blob)
    for address in [0x02111c90,0x02111cd0]:
        clps=image[address-BASE:address-BASE+32]
        if len(clps)!=32 or clps[:4]!=b'CLPS':raise ValueError('Incomplete or invalid basement collision material')
        emit(f'data_ov012_{address:08x}',clps)
    lines.append('void *pc_basement_animation() { static bool ready=false; if(!ready) {')
    for offset in offsets:
        relative=struct.unpack_from('<I',blob,offset)[0]-START
        lines.append(f'{{void *p=pc_basement_bta_bytes+{relative};std::memcpy(pc_basement_bta_bytes+{offset},&p,4);}}')
    lines.append(f'ready=true;}} return pc_basement_bta_bytes+{h}; }}\n}}')
    out.parent.mkdir(parents=True,exist_ok=True);out.write_text('\n'.join(lines)+'\n')
    return offsets
if __name__=='__main__':
    import sys
    print(generate(Path(sys.argv[1]),Path(sys.argv[2])))
