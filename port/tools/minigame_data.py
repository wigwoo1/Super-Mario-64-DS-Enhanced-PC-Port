"""Canonical minigame graphics/configuration storage and pointer relocation.

Only resource/state regions are included. Original vtables and member-function
records are deliberately outside these ranges; native C++ owns those instead.
"""
import re
import struct

RANGES = {
    4: [(0x020b944c,0x020b948c),(0x020b9e00,0x020bbf4c),
        (0x020bbf94,0x020bc038),(0x020bc070,0x020bc0bc),
        (0x020bc150,0x020bc8bc),(0x020bcaa0,0x020beb74),
        (0x020bf3e8,0x020bf428),(0x020bf9e4,0x020bfec0)],
    6: [(0x0212e848,0x0212e8e0),(0x02133600,0x0213ac48),
        (0x0213cdec,0x0213cdf4),(0x0213ce70,0x0213cf08),
        (0x0213cfa0,0x0213cfc0)],
}


def region(ov,address):
    return next(((ov,a,b) for a,b in RANGES.get(ov,[]) if a<=address<b),None)


def inventory(root):
    labels, relocations, functions = {}, {}, {}
    for ov in RANGES:
        text=(root/f'config/arm9/overlays/ov{ov:03}/symbols.txt').read_text()
        functions[ov]={int(a,16):n for n,a in re.findall(
            r'^(\S+) kind:function\([^\n]*?addr:0x([\da-fA-F]+)',text,re.M)}
        for name,address in re.findall(r'^(data_ov\d+_\w+) kind:(?:data\([^)]*\)|bss) addr:0x([0-9a-f]+)',text,re.M):
            address=int(address,16)
            if region(ov,address): labels[name]=(ov,address)
    for ov in RANGES:
        for line in (root/f'config/arm9/overlays/ov{ov:03}/relocs.txt').read_text().splitlines():
            m=re.fullmatch(r'from:0x([0-9a-f]+) kind:load to:0x([0-9a-f]+) module:(.*)',line)
            if not m: continue
            source,target=int(m[1],16),int(m[2],16)
            if not region(ov,source): continue
            candidates=[int(n) for n in re.findall(r'\d+',m[3])]
            data=[r for candidate in candidates if (r:=region(candidate,target))]
            if len(data)==1:
                relocations[ov,source]=('data',data[0],target)
            elif candidates==[ov] and target in functions[ov] and functions[ov][target].startswith('func_'):
                relocations[ov,source]=('function',functions[ov][target],target)
            else: raise ValueError('Unresolved minigame resource relocation: '+line)
    return labels,relocations


def owned_names(root):
    return set(inventory(root)[0]) | {'data_ov004_' + a for a in (
        '020bf428','020bf490','020bf4f8','020bf560','020bf5d4','020bf648')}


def function_targets(root):
    return {v[1] for v in inventory(root)[1].values() if v[0]=='function'}


def write_sources(root,directory):
    labels,relocations=inventory(root)
    lines=['.386','.model flat','.data']
    lines += [f'EXTERN _{name}:PROC' for name in sorted(function_targets(root))]
    for ov,ranges in RANGES.items():
        raw=(root/f'extracted/overlays/overlay_{ov:04}.bin').read_bytes()
        base=0x020ad660 if ov==4 else 0x020bfec0
        for start,end in ranges:
            block=f'pc_mg_data_{ov}_{start:x}'
            lines+=['ALIGN 4',f'PUBLIC _{block}',f'_{block} LABEL BYTE']
            address=start
            while address<end:
                for name,(module,value) in labels.items():
                    if module==ov and value==address: lines += [f'PUBLIC _{name}',f'_{name} LABEL BYTE']
                item=relocations.get((ov,address))
                if item:
                    if address-base+4<=len(raw) and struct.unpack_from('<I',raw,address-base)[0]!=item[2]:
                        raise ValueError('Resource relocation does not match supplied image')
                    if item[0]=='function': lines.append(f'DD OFFSET _{item[1]}')
                    else:
                        module,low,_=item[1]
                        lines.append(f'DD OFFSET _pc_mg_data_{module}_{low:x} + {item[2]-low}')
                    address+=4
                else:
                    # Stop at interior labels and relocation sites, preserving aliasing.
                    count=min(16,end-address)
                    for n in range(1,count):
                        if (ov,address+n) in relocations or (ov,address+n) in labels.values(): count=n;break
                    values=raw[address-base:address-base+count] if address-base<len(raw) else bytes(count)
                    values=values.ljust(count,b'\0')
                    lines.append('DB '+','.join(f'0{v:02x}h' for v in values))
                    address+=count
    lines.append('END')
    directory.mkdir(parents=True,exist_ok=True)
    output=directory/'minigame_resources.asm'
    text='\n'.join(lines)+'\n'
    if not output.exists() or output.read_text()!=text: output.write_text(text)
    return [output.as_posix()]
