"""Compare the native sound actor profile and complete row array to local ROM data."""
import pathlib
import re
import struct
from check_player_animation_data import pe_bytes

ROOT=pathlib.Path(__file__).resolve().parents[2]


def main():
    symbols={}
    for line in (ROOT/'build/port/sm64ds_pc.map').read_text().splitlines():
        match=re.match(r'\s+[\da-f]+:[\da-f]+\s+(\S+)\s+([\da-f]{8})\s',line)
        if match:symbols[match[1]]=int(match[2],16)
    image=(ROOT/'extracted/overlays/overlay_0002.bin').read_bytes()
    exe=ROOT/'build/port/sm64ds_pc.exe'
    base=0x020ad660
    address=0x0210c080
    expected=image[address-base:address-base+7*12]
    actual=pe_bytes(exe,symbols['_data_ov002_0210c080'],7*12)
    assert actual==expected,'sound actor row array is truncated or differs from the local ROM'
    for row in range(7):
        sound,volume,limit,loop,pad=struct.unpack_from('<iiHBB',actual,row*12)
        assert 0<=sound<256 and 0<=volume<=127 and loop in (0,1),(row,'invalid row fields')
    profiles=[value for name,value in symbols.items() if name=='_pc_sound_obj_profile' or name.startswith('?pc_sound_obj_profile@')]
    assert len(profiles)==1,'missing native sound actor profile'
    profile=pe_bytes(exe,profiles[0],28)
    assert struct.unpack_from('<I',profile)[0]==symbols['_daSoundObj_c_classInit'],'wrong sound actor factory'
    assert profile[4:]==image[0x0210c064-base+4:0x0210c064-base+28],'sound actor descriptor differs'
    print('PASS: all seven sound actor rows and native profile match the local ROM; native factory is bound')


if __name__=='__main__':main()
