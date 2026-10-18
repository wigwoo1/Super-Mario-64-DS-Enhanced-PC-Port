"""Audit native Power Star/silver-star/cap profiles, callbacks and resources."""
import pathlib
import re
import struct
from check_player_animation_data import pe_bytes
from link_data import records, source_bytes
from reward_host import CAP_METHODS, NO_CONTROL_DESCRIPTORS, NO_CONTROL_TARGETS, NO_CONTROL_FREE

ROOT=pathlib.Path(__file__).resolve().parents[2]


def main():
    exe=ROOT/'build/port/sm64ds_pc.exe'
    symbols={}
    for line in (ROOT/'build/port/sm64ds_pc.map').read_text().splitlines():
        m=re.match(r'\s+[\da-f]+:[\da-f]+\s+(\S+)\s+([\da-f]{8})\s',line)
        if m:symbols[m[1]]=int(m[2],16)
    def c(name):return symbols['_'+name]
    def cpp(prefix):
        if '_'+prefix in symbols:return symbols['_'+prefix]
        matches=[value for name,value in symbols.items() if name.startswith('?'+prefix+'@')]
        assert len(matches)==1,(prefix,matches)
        return matches[0]
    available=records(ROOT)
    for name,original,factory in (
            ('pc_power_star_profile','g_profile_STAR',cpp('pc_power_star_factory')),
            ('pc_silver_star_profile','g_profile_SILVER_STAR',cpp('pc_silver_star_factory')),
            ('pc_star_marker_profile','g_profile_STARBASE',cpp('pc_star_marker_factory')),
            ('pc_cap_reward_profile','g_profile_OBJ_MARIO_CAP',c('daObjMarioCap_c_classInit'))):
        row=next(r for r in available if r['module']=='ov002' and r['name']==original)
        actual=pe_bytes(exe,cpp(name),28)
        assert actual[4:]==source_bytes(ROOT,row)[4:28],name
        assert struct.unpack_from('<I',actual)[0]==factory,name+' factory'
    for address,method in zip(range(0x0210951c,0x021095ac,8),CAP_METHODS):
        actual=struct.unpack('<II',pe_bytes(exe,c(f'data_ov002_{address:08x}'),8))
        assert actual==(cpp('pc_cap_'+method),0),(method,actual)
    handlers=('020ea9d0','020ea90c','020ea824','020ea7ac','020ea420','020ea100',
              '020ea06c','020e9d18','020e99e8','020e9840','020ea410','020e9af4',
              '020e9804','020e96a0')
    actual=struct.unpack('<14I',pe_bytes(exe,c('data_ov002_021109d8'),56))
    expected=tuple(symbols[f'?func_ov002_{h}@daStar_c@@QAEXXZ'] for h in handlers)
    assert actual==expected,'native star state callbacks'
    for address in (0x020ff0a0,0x020ff0ac,0x020ff0b8,0x020ff0c4):
        row=next(r for r in available if r['module']=='ov002' and r['address']==address)
        original=struct.unpack('<3I',source_bytes(ROOT,row))
        expected=tuple(c(f'data_ov002_{x:08x}') for x in original)
        actual=struct.unpack('<3I',pe_bytes(exe,c(row['name']),12))
        assert actual==expected,row['name']
    for descriptor,target in zip(NO_CONTROL_DESCRIPTORS,NO_CONTROL_TARGETS):
        row=next(r for r in available if r['module']=='ov002' and r['address']==descriptor)
        assert struct.unpack_from('<II',source_bytes(ROOT,row))==(target,0),'original no-control descriptor'
        assert struct.unpack('<II',pe_bytes(exe,c(row['name']),8))==(target,0),'compiled no-control descriptor'
        # Optimized native calls may inline these small workers into the
        # dispatcher. Source tests verify every typed call; standalone worker
        # symbols are not required by the native ABI.
    c('pc_player_no_control_init')
    row=next(r for r in available if r['module']=='arm9' and r['name']=='data_020876e4')
    original=struct.unpack('<6I',source_bytes(ROOT,row))
    expected=tuple(c(f'data_{address:08x}') for address in original)
    assert struct.unpack('<6I',pe_bytes(exe,c('data_020876e4'),24))==expected,'native Power Star camera scripts'
    row=next(r for r in available if r['module']=='ov001' and r['name']=='_ZN3OAM7NUMBERSE')
    targets=struct.unpack('<20I',source_bytes(ROOT,row))
    actual=struct.unpack('<20I',pe_bytes(exe,c('_ZN3OAM7NUMBERSE'),80))
    for digit,(target,native) in enumerate(zip(targets,actual)):
        original=next(r for r in available if r['module']=='ov001' and r['address']==target)
        assert native==c(f'pc_oam_number_{digit}'),'native course-clear numeral pointer'
        assert pe_bytes(exe,native,8)==source_bytes(ROOT,original)[:8],'original numeral OAM bytes'
    print('PASS: 4 reward profiles/factories, 18 native cap callbacks, '
          '14 native star callbacks, 12 canonical model/animation pointers '
          '20 original no-control descriptors with a native dispatcher, 6 native Power Star camera scripts and 20 course-clear numeral descriptors; '
          'runtime collection remains a separate gameplay check')


if __name__=='__main__':main()
