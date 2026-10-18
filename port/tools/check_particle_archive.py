"""Check the full embedded particle archive in the linked PC image against ROM."""
import pathlib, re, struct
from check_player_animation_data import pe_bytes
root=pathlib.Path(__file__).resolve().parents[2]
symbols={}
for line in (root/'build/port/sm64ds_pc.map').read_text().splitlines():
    m=re.match(r'\s+\S+\s+(\S+)\s+([\da-f]{8})\s',line)
    if m:symbols[m[1]]=int(m[2],16)
arm=(root/'extracted/arm9_dec.bin').read_bytes()
offset=0x02075f14-0x02004000
particle_bytes,texture_bytes,texture_offset=struct.unpack_from('<III',arm,offset+0x10)
size=32+particle_bytes+texture_bytes
expected=arm[offset:offset+size]
compiled=pe_bytes(root/'build/port/sm64ds_pc.exe',symbols['_data_02075f14'],size)
if compiled!=expected:raise ValueError('linked particle archive differs from ROM or is truncated')
particles,textures=struct.unpack_from('<HH',compiled,8)
if texture_offset!=32+particle_bytes:raise ValueError('invalid particle/texture boundary')
cursor=texture_offset
for i in range(textures):
    if compiled[cursor:cursor+4]!=b' TPS':raise ValueError(f'texture {i}: invalid record signature')
    tex_bytes=struct.unpack_from('<I',compiled,cursor+8)[0]
    record_bytes=struct.unpack_from('<I',compiled,cursor+28)[0]
    if record_bytes<32+tex_bytes or cursor+record_bytes>size:raise ValueError(f'texture {i}: invalid extent')
    cursor+=record_bytes
if cursor!=size:raise ValueError('texture records do not cover the complete archive')
print(f'PASS: {size} linked particle bytes match ROM; {particles} definitions and {textures} complete texture records')
