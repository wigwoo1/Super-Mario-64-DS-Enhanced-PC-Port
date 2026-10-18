"""Check linked minigame callbacks, graphics relocations, aliases and profile."""
import pathlib
import re
import struct
from check_player_animation_data import pe_bytes
import minigame_host
import minigame_data

ROOT=pathlib.Path(__file__).resolve().parents[2]


def main():
    exe=ROOT/'build/port/sm64ds_pc.exe'
    symbols={m[1]:int(m[2],16) for line in (ROOT/'build/port/sm64ds_pc.map').read_text().splitlines()
             if (m:=re.match(r'\s+\d{4}:[\da-f]+\s+(\S+)\s+([\da-f]{8})\s',line))}
    def named(name):
        values={a for n,a in symbols.items() if n=='_'+name or n.startswith('?'+name+'@@')}
        if len(values)!=1: raise ValueError(f'{name}: missing/divergent native aliases: {values}')
        return values.pop()
    entries=minigame_host.descriptors(ROOT)
    for name,(cls,method,_) in entries.items():
        expected=(named('pc_mg_'+cls+'_'+method),0)
        actual=struct.unpack('<II',pe_bytes(exe,named(name),8))
        if actual!=expected: raise ValueError('Wrong callback: '+name)
    for name,slots in minigame_host.tables(ROOT).items():
        for slot,constant in slots.items():
            actual=pe_bytes(exe,named(name)+slot*8,8)
            if actual!=pe_bytes(exe,named(constant),8): raise ValueError('Wrong table: '+name)
    labels,relocations=minigame_data.inventory(ROOT)
    size=0
    for ov,ranges in minigame_data.RANGES.items():
        image=(ROOT/f'extracted/overlays/overlay_{ov:04}.bin').read_bytes()
        base=0x020ad660 if ov==4 else 0x020bfec0
        for low,high in ranges:
            block=named(f'pc_mg_data_{ov}_{low:x}')
            expected=bytearray(image[low-base:high-base].ljust(high-low,b'\0'))
            for (module,address),record in relocations.items():
                if module!=ov or not low<=address<high: continue
                if record[0]=='function': target=named(record[1])
                else:
                    mod,start,_=record[1]
                    target=named(f'pc_mg_data_{mod}_{start:x}')+record[2]-start
                struct.pack_into('<I',expected,address-low,target)
            if pe_bytes(exe,block,high-low,allow_zero_tail=True)!=expected:
                raise ValueError(f'Wrong resource bytes: {ov}:{low:x}')
            for label,(module,address) in labels.items():
                if module==ov and low<=address<high and named(label)!=block+address-low:
                    raise ValueError('Broken interior alias: '+label)
            size+=high-low
    profile=struct.unpack('<IHH',pe_bytes(exe,named('pc_wanted_profile'),8))
    if profile!=(named('dScMgLuigi_c_classInit'),366,366): raise ValueError('Wrong Wanted profile')
    print(f'PASS: {len(entries)} callbacks, 23 table slots, {len(labels)} labels, '
          f'{len(relocations)} resource relocations, {size} resource bytes, native profile366')


if __name__=='__main__': main()
