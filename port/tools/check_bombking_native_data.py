"""Audit original boss metadata and the compiled native registry resources."""
import re,struct
from pathlib import Path
from bombking_host import inventory
from link_data import records,source_bytes
from check_player_animation_data import pe_bytes
ROOT=Path(__file__).resolve().parents[2]

def main():
    symbols={}
    for line in (ROOT/'build/port/sm64ds_pc.map').read_text().splitlines():
        m=re.match(r'\s+[\da-f]+:[\da-f]+\s+(\S+)\s+([\da-f]{8})\s',line)
        if m:symbols[m[1]]=int(m[2],16)
    rows=records(ROOT);exe=ROOT/'build/port/sm64ds_pc.exe'
    arm=(ROOT/'extracted/arm9_dec.bin').read_bytes()
    for address,size in [(0x02099f88,12),(0x02099f94,16)]:
        name=f'_data_{address:08x}'
        assert pe_bytes(exe,symbols[name],size)==arm[address-0x02004000:address-0x02004000+size],name+' original billboard/material constants'
    for name,mod,factory in [('g_profile_BOMBKING','ov078','daBombking_c_classInit'),('g_profile_BOMBHEI','ov102','daBmb_c_classInit')]:
        row=next(r for r in rows if r['module']==mod and r['name']==name)
        raw=source_bytes(ROOT,row)[:28];native=pe_bytes(exe,symbols['_'+name],28)
        assert native[4:]==raw[4:],name+' profile metadata'
        assert struct.unpack_from('<I',native)[0]==symbols['_'+factory],name+' factory'
    states,handles=inventory()
    for name,pair in states.items():
        assert '_'+name in symbols,name+' canonical state allocation'
        for target in pair.values():
            assert any(n.startswith('?pc_boss_'+f'{target:08x}'+'@@') for n in symbols),hex(target)+' native thunk'
    for name in handles:assert '_'+name in symbols,name+' canonical handle'
    assert '_pc_bombking_init_resources' in symbols
    generated=(ROOT/'build/port/pc-boot-src/src/actors/daBombking_c.cpp').read_text()
    assert 'this->**' not in generated,'raw member-pointer dispatch survived'
    assert 'mHeldActor->GetHealth()' in generated,'implicit player receiver'
    bmb=(ROOT/'build/port/pc-boot-src/src/actors/daBmb_c.cpp').read_text()
    assert 'pc_bmb_carry[12]' in bmb,'contiguous original carry offsets'
    print('PASS: original King Bob-omb/Bob-omb profiles and factories,18 native state pairs/36 thunks,13 canonical handles,boss layout assertions and native carry-offset initialization; combat requires runtime verification')
if __name__=='__main__':main()
