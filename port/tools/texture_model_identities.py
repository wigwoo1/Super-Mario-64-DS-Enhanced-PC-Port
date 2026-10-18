"""Generate model identities from the user's ROM, without copying asset bytes."""
import argparse,csv,hashlib,json,re,struct
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
def unpack(data):
    if data.startswith(b'LZ77'):data=data[4:]
    elif not data or data[0]!=0x10:return data
    if len(data)<4 or data[0]!=0x10:raise ValueError('Invalid model compression')
    size=int.from_bytes(data[1:4],'little')
    if size>64*1024*1024:raise ValueError('Excessive decompressed size')
    out=bytearray();pos=4
    while len(out)<size:
        flags=data[pos];pos+=1
        for bit in range(7,-1,-1):
            if len(out)==size:break
            if flags&(1<<bit):
                pair=int.from_bytes(data[pos:pos+2],'big');pos+=2
                count=(pair>>12)+3;distance=(pair&0xfff)+1
                if distance>len(out):raise ValueError('Invalid model back-reference')
                for _ in range(min(count,size-len(out))):out.append(out[-distance])
            else:out.append(data[pos]);pos+=1
    return bytes(out)
def fnv(data):
    value=14695981039346656037
    for byte in data:value=((value^byte)*1099511628211)&0xffffffffffffffff
    return value
def archive_members(data):
    if data[:4]!=b'NARC':return []
    position=struct.unpack_from('<H',data,12)[0];fat=[];image=b''
    while position+8<=len(data):
        tag=data[position:position+4];size=struct.unpack_from('<I',data,position+4)[0]
        if size<8 or position+size>len(data):raise ValueError('Invalid archive block')
        if tag==b'BTAF':
            count=struct.unpack_from('<H',data,position+8)[0]
            fat=[struct.unpack_from('<II',data,position+12+i*8) for i in range(count)]
        if tag==b'GMIF':image=data[position+8:position+size]
        position+=size
    return [(index,unpack(image[start:end])) for index,(start,end) in enumerate(fat)]
def is_model(data):
    if len(data)<0x38:return False
    nt,pt,np,pp,nm,pm=struct.unpack_from('<6I',data,0x14)
    return (struct.unpack_from('<I',data)[0]<=15 and 0<nt<1024 and np<1024 and
        0<nm<1024 and pt+nt*20<=len(data) and pp+np*16<=len(data) and pm+nm*48<=len(data))
def generate(rom_path,output):
    rom=rom_path.read_bytes();fat_offset,fat_size=struct.unpack_from('<II',rom,0x48)
    if fat_offset+fat_size>len(rom) or fat_size%8:raise ValueError('Invalid FAT')
    catalog=list(csv.DictReader((ROOT/'build/assets/files.tsv').open(),delimiter='\t'))
    models=[]
    for row in catalog:
        if not row['path'].lower().endswith(('.bmd','.narc')):continue
        file_id=int(row['file_id']);start,end=struct.unpack_from('<II',rom,fat_offset+file_id*8)
        if end<start or end>len(rom) or end-start!=int(row['size']):raise ValueError(row['path'])
        if row['path'].lower().endswith('.bmd'):
            models.append((row['path'],file_id,unpack(rom[start:end])))
        else:
            for index,data in archive_members(rom[start:end]):
                if is_model(data):models.append((f'{row["path"]}#{index}',file_id,data))
    roles={m[1]:m[2] for m in re.finditer(r'PC_TEXTURE_MODEL\("([^"]+)", (\w+),',
        (ROOT/'port/ntr/texture_model_roles.inc').read_text())}
    if set(roles)!={path for path,_,_ in models}:raise ValueError('Role manifest does not exactly cover named/archive models')
    lines=['// Generated from the local ROM; model fingerprints and names only.']
    records=[]
    for path,file_id,data in models:
        key=fnv(data)
        lines.append(f'PC_TEXTURE_MODEL_IDENTITY(0x{key:016x}ull, {len(data)}, "{path}")')
        records.append({'file_id':file_id,'path':path,'role':roles[path],
            'bytes':len(data),'fnv64':f'{key:016x}','sha256':hashlib.sha256(data).hexdigest()})
    output.parent.mkdir(parents=True,exist_ok=True);output.write_text('\n'.join(lines)+'\n')
    report=ROOT/'build/texture-model-identities.json'
    report.write_text(json.dumps({'rom_sha256':hashlib.sha256(rom).hexdigest(),'models':records},indent=2)+'\n')
    print(f'PASS: {len(records)} model identities; exact role/catalog coverage; {report}')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--rom',type=Path,default=ROOT/'build/assets/user-europe-rev0.nds')
    p.add_argument('--output',type=Path,default=ROOT/'port/ntr/texture_model_identities.inc')
    a=p.parse_args();generate(a.rom,a.output)
