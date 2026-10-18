"""Preserve overlay 5's contiguous tables and relocate its OAM pointers."""
from pathlib import Path
import json
import re
import struct
import sys

BASE, START, END = 0x020bfec0, 0x020c2250, 0x020c3020

def owned_names(root):
    text=(root/'config/arm9/overlays/ov005/symbols.txt').read_text()
    return [(name,int(address,16)) for name,address in re.findall(
        r'^(data_ov005_\w+) kind:data\([^)]*\) addr:0x([0-9a-f]+)',text,re.M)]

def image(root):
    raw=(root/'extracted/overlays/overlay_0005.bin').read_bytes()
    if len(raw)!=END-BASE: raise ValueError('Unexpected Rec Room overlay extent')
    if struct.unpack_from('<IHH',raw,0x020c2440-BASE)!=(0x020c21ec,5,8):
        raise ValueError('Rec Room native factory/profile metadata changed')
    relocations={}
    for line in (root/'config/arm9/overlays/ov005/relocs.txt').read_text().splitlines():
        m=re.fullmatch(r'from:0x([0-9a-f]+) kind:load to:0x([0-9a-f]+) module:overlay\(5\)',line)
        if not m: continue
        source,target=(int(v,16) for v in m.groups())
        if START<=source<END and START<=target<END:
            if struct.unpack_from('<I',raw,source-BASE)[0]!=target: raise ValueError('Rec Room relocation mismatch')
            relocations[source]=target
    # The eight character OAM records and their pointer table must remain one block.
    # 30 OAM references plus two original RTTI references in the copied metadata.
    if len(relocations)!=32: raise ValueError('Rec Room relocation inventory changed')
    return raw[START-BASE:],relocations

def generate(root,output):
    raw,relocations=image(root)
    labels={}
    for name,address in owned_names(root): labels.setdefault(address,[]).append(name)
    lines=['.386','.model flat','.data','ALIGN 4','PUBLIC _pc_minigame_menu_data','_pc_minigame_menu_data LABEL BYTE']
    address=START
    while address<END:
        for name in labels.get(address,[]): lines+=['PUBLIC _'+name,'_'+name+' LABEL BYTE']
        if address in relocations:
            lines.append('DD OFFSET _pc_minigame_menu_data + '+str(relocations[address]-START))
            address+=4
        else:
            count=min(16,END-address)
            for i in range(1,count):
                if address+i in labels or address+i in relocations: count=i;break
            lines.append('DB '+','.join(f'0{b:02x}h' for b in raw[address-START:address-START+count]))
            address+=count
    lines+=['END']
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text('\n'.join(lines)+'\n')
    output.with_suffix('.json').write_text(json.dumps({'start':hex(START),'end':hex(END),
        'labels':owned_names(root),'relocations':{hex(a):hex(b) for a,b in relocations.items()}},indent=2)+'\n')

if __name__=='__main__': generate(Path(sys.argv[1]),Path(sys.argv[2]))
