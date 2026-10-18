"""Audit reviewed archive roles against source constructors and original ROM tables."""
import collections,hashlib,json,re,struct
from pathlib import Path
from link_data import records,source_bytes
ROOT=Path(__file__).resolve().parents[2]
def overlay_from_rom(rom,number):
    table,length=struct.unpack_from('<II',rom,0x50)
    rows=[struct.unpack_from('<8I',rom,p) for p in range(table,table+length,32)]
    row=next(r for r in rows if r[0]==number);_,base,size,_,_,_,file_id,flags=row
    fat=struct.unpack_from('<I',rom,0x48)[0];start,end=struct.unpack_from('<II',rom,fat+file_id*8)
    stored=rom[start:end]
    if flags>>24&1:
        # BLZ runs backwards. Check the decoded overlay independently against
        # the extraction used by source_bytes before trusting any model table.
        for appended in range(0,32,4):
            footer=len(stored)-appended-8
            packed,extra=struct.unpack_from('<II',stored,footer)
            header,compressed=packed>>24,packed&0xffffff
            if 8<=header<=footer+8 and compressed<=footer+8 and all(x==255 for x in stored[footer+8-header:footer]):break
        else:raise AssertionError('BLZ footer')
        assert extra and header<=compressed
        stop=len(stored)-appended-compressed
        stream=stored[stop:len(stored)-appended-header][::-1]
        out=bytearray();pos=0;target=compressed+extra
        while len(out)<target:
            control=stream[pos];pos+=1
            for bit in range(7,-1,-1):
                if len(out)==target:break
                if control&(1<<bit):
                    a,b=stream[pos:pos+2];pos+=2
                    count=(a>>4)+3;distance=((a&15)<<8|b)+3
                    if distance>len(out):
                        assert len(out)>=2;distance=2
                    assert len(out)+count<=target
                    for _ in range(count):out.append(out[-distance])
                else:out.append(stream[pos]);pos+=1
        image=stored[:stop]+bytes(out[::-1])+stored[len(stored)-appended:] if appended else stored[:stop]+bytes(out[::-1])
    else:image=stored
    assert len(image)==size
    assert image==(ROOT/f'extracted/overlays/overlay_{number:04d}.bin').read_bytes()
    return base,image

def overlay2_from_rom(rom):return overlay_from_rom(rom,2)
def audit():
    evidence=json.loads((ROOT/'port/docs/texture-model-source-evidence.json').read_text())
    roles={p:(r,b) for p,r,b in re.findall(r'PC_TEXTURE_MODEL\("([^"]+)", (\w+), "([^"]+)"\)',
        (ROOT/'port/ntr/texture_model_roles.inc').read_text())}
    ids=json.loads((ROOT/'build/texture-model-identities.json').read_text())
    identities={r['path']:r for r in ids['models']}
    rom=ROOT/'build/assets/user-europe-rev0.nds'
    rom_bytes=rom.read_bytes()
    assert ids['rom_sha256']==hashlib.sha256(rom_bytes).hexdigest()
    overlay_base,overlay=overlay2_from_rom(rom_bytes)
    assert set(identities)==set(roles) and len(roles)==505
    ranges=[(int(a,16),int(b,16),p) for a,b,p in re.findall(
        r'\{(0x[0-9a-f]+), (0x[0-9a-f]+), "(ARCHIVE/[^\"]+)"\}',(ROOT/'port/hal/fs.cpp').read_text())]
    original={r['name']:r for r in records(ROOT)}
    source_hashes={};verified=[]
    def read(path):
        b=(ROOT/path).read_bytes();source_hashes[path]=hashlib.sha256(b).hexdigest();return b.decode()
    for item in evidence['confirmed']:
        path=item['path'];handle=int(item['handle'],16);symbol=item['symbol']
        matching=[f'{p}#{handle-start}' for start,end,p in ranges if start<=handle<end]
        assert matching==[path],(path,matching)
        assert roles[path][0]==identities[path]['role']==item['role'],path
        initializer=read(item['initializer'])
        constructor=re.search(r'func_02017acc\(\s*&?'+re.escape(symbol)+r'\s*,\s*0x'+item['handle']+r'\s*\)',initializer)
        if not constructor:
            constructor=re.search(r'\b'+re.escape(symbol)+r'\s*\(\s*0x'+item['handle']+r'\s*\)',initializer)
        assert constructor,(path,'resource constructor absent')
        consumer=read(item['consumer'])
        for needle in item['consumer_needles']:assert needle in consumer,(path,needle)
        if item['table']:
            table=item['table'];row=original[table['name']].copy();row['size']=16
            table_bytes=source_bytes(ROOT,row)
            offset=row['address']-overlay_base
            assert table_bytes==overlay[offset:offset+16]
            words=struct.unpack('<4I',table_bytes)
            assert words[table['index']]==int(table['address'],16),path
            assert symbol==f'data_ov002_{words[table["index"]]:08x}',path
            # Confirm that the host rebasing uses the same slot, not a guessed
            # neighboring model with similar texture colors.
            host=read('port/hal/actor_vtables.cpp')
            start=host.index('void **modelHandles = (void **)data_ov002_0210a68c;')
            for name in ['data_ov002_0210a68c','data_ov002_0210a69c','data_ov002_0210a6ac']:
                a=host.index(name+';',start);b=host.index('modelHandles[3]',a)+150
                if name==table['name']:
                    assert f'modelHandles[{table["index"]}] = {symbol};' in host[a:b],path
        verified.append({'path':path,'role':item['role'],'handle':item['handle'],'model_sha256':identities[path]['sha256']})
    for item in evidence['deferred']:
        source=read(item['source'])
        for needle in item['needles']:assert needle in source,needle
        for path in item['paths']:assert roles[path][0]=='Unknown',path
    assert len(verified)==29 and len({r['path'] for r in verified})==29
    report={'ReviewedRoleChanges':verified,'RoleCounts':dict(collections.Counter(r for r,_ in roles.values())),
            'SourceSHA256':source_hashes,'ROM_SHA256':ids['rom_sha256'],'ROM_Overlay2_SHA256':hashlib.sha256(overlay).hexdigest(),
            'DeferredSharedModels':evidence['deferred'],
            'Limitations':['29 reviewed role changes do not establish every model role, animated variant or complete game coverage.',
            'Shared coin/cap models still require per-draw ownership before replacement can be enabled safely.']}
    (ROOT/'build/archive-role-source-audit.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS: 29 archive roles; exact archive handle ranges, resource constructors, source consumers, 12 original-ROM/host player-table slots; shared uses remain excluded')
    print(json.dumps(report['RoleCounts'],sort_keys=True))
if __name__=='__main__':audit()
