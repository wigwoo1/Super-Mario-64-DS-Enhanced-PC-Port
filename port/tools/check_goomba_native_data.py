"""Audit Goomba profiles, resource handles and embedded material pointers."""
import pathlib
import re
import struct
from check_player_animation_data import pe_bytes
from link_data import records, source_bytes

ROOT=pathlib.Path(__file__).resolve().parents[2]


def main():
    exe=ROOT/'build/port/sm64ds_pc.exe'
    symbols={}
    for line in (ROOT/'build/port/sm64ds_pc.map').read_text().splitlines():
        match=re.match(r'\s+[\da-f]+:[\da-f]+\s+(\S+)\s+([\da-f]{8})\s',line)
        if match: symbols[match[1]]=int(match[2],16)
    def named(name):
        return symbols['_'+name]
    def words(name,count):
        return struct.unpack(f'<{count}I',pe_bytes(exe,named(name),count*4))
    available=records(ROOT)
    height=next(r for r in available if r['module']=='ov084' and r['address']==0x02130208)
    expected_heights=struct.pack('<4i',40<<12,100<<12,224<<12,100<<12)
    assert source_bytes(ROOT,height)==expected_heights, 'original Goomba cylinder heights'
    assert pe_bytes(exe,named(height['name']),16)==expected_heights, 'complete native Goomba cylinder heights'
    coin_actors=next(r for r in available if r['module']=='ov002' and r['address']==0x020ff014)
    expected_coins=struct.pack('<4H',288,289,290,0)
    assert source_bytes(ROOT,coin_actors)[:8]==expected_coins, 'original enemy coin actor IDs'
    assert pe_bytes(exe,symbols['?data_ov002_020ff014@@3GA'],8)==expected_coins, 'native enemy coin actor IDs'
    assert words('data_02075238',3)==(1,2,5), 'native yellow/red/blue coin values'
    for suffix in ('','_S','_L'):
        name='g_profile_KURIBO'+suffix
        row=next(r for r in available if r['module']=='ov084' and r['name']==name)
        original=source_bytes(ROOT,row)[:28]
        actual=pe_bytes(exe,named(name),28)
        assert len(original)==28 and actual[4:]==original[4:],name
        assert struct.unpack_from('<I',actual)[0]==named('daKrb_c_classInit_KURIBO'+suffix),name
    egg=next(r for r in available if r['module']=='ov002' and r['name']=='g_profile_YOSHI_EGG')
    actual=pe_bytes(exe,named('pc_yoshi_egg_profile'),28)
    assert actual[4:]==source_bytes(ROOT,egg)[4:28], 'egg profile metadata'
    assert struct.unpack_from('<I',actual)[0]==named('daYegg_c_classInit'), 'egg factory'
    # This table is owned/dispatched entirely by native C++; each MSVC member
    # pointer is one word. Audit every entry against its original state worker.
    table=symbols['?data_ov002_02110a5c@@3PAUEntry@@A']
    handlers=('020ed5b0','020ed0d4','020ecfc8','020ecf94','020ecad4','020ec9c4','020ec978','020ec938')
    expected=tuple(symbols[f'?func_ov002_{h}@daYegg_c@@QAEXXZ'] for h in handlers)
    assert struct.unpack('<8I',pe_bytes(exe,table,32))==expected, 'egg state callbacks'
    for module,address,count in [('ov084',0x02130278,7),('ov002',0x020ff028,6),('ov002',0x021000a0,2)]:
        row=next(r for r in available if r['module']==module and r['address']==address)
        original=struct.unpack(f'<{count}I',source_bytes(ROOT,row))
        expected=tuple(named(f'data_{module}_{value:08x}') if value else 0 for value in original)
        assert words(row['name'],count)==expected,row['name']
    relocations={0x0213088c:{4:0x02130178,12:0x0213018c},
                 0x0213089c:{4:0x02130174,12:0x021301c8},
                 0x0213018c:{4:0x02130880},0x021301c8:{4:0x02130880}}
    for address,fields in relocations.items():
        row=next(r for r in available if r['module']=='ov084' and r['address']==address)
        expected=bytearray(source_bytes(ROOT,row))
        for offset,target in fields.items():struct.pack_into('<I',expected,offset,named(f'data_ov084_{target:08x}'))
        assert pe_bytes(exe,named(row['name']),len(expected))==expected,row['name']
    print('PASS: 3 native Goomba profiles/factories, 7 canonical animation handles, '
          '6 cap table slots, 6 embedded material pointers and original material bytes; '
          'native egg profile/factory, 2 model handles, all 8 state callbacks, all 4 Goomba collision heights, '
          'enemy coin actor IDs and original yellow/red/blue coin values 1/2/5')


if __name__=='__main__':main()
