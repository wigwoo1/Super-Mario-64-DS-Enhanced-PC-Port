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
offset=32
extras={8:12,9:12,10:8,11:12,16:20,24:8,25:8,26:16,27:4,28:8,29:16}
for effect in range(particles):
    flags=struct.unpack_from('<I',compiled,offset)[0]
    kind=(flags>>4)&3
    child_offset=offset+56+sum(extras[bit] for bit in (8,9,10,11) if flags&(1<<bit))
    child_kind=(struct.unpack_from('<H',compiled,child_offset)[0]>>7)&3 if flags&(1<<16) else None
    if kind>2 or (child_kind is not None and child_kind>2):
        raise ValueError(f'effect {effect}: unsupported original draw kind')
    if (kind==2 or child_kind==2) and ((flags>>17)&3)>1:
        raise ValueError(f'effect {effect}: rotation index exceeds two original callbacks')
    offset+=56+sum(length for bit,length in extras.items() if flags&(1<<bit))
if offset!=texture_offset:raise ValueError('effect records do not cover complete definition span')
print(f'PASS: {size} linked particle bytes match ROM; {particles} definitions and {textures} complete texture records')

# These original data words are indirect CODE addresses, not effect payload.
# The native versions must use native targets even though their ARM labels lie
# in the broad BSS address range used by the old generic data generator.
targets=struct.unpack_from('<4I',arm,0x02099fb4-0x02004000)
if targets!=(0x0204c24c,0x0204c194,0x0204c0a8,0x0204c0e8):
    raise ValueError('Original particle draw/rotation callback table changed')
for table,names in (
        ('data_02099fb4',('pc_particle_quad_normal','pc_particle_quad_offset')),
        ('data_02099fbc',('func_0204c0a8','func_0204c0e8'))):
    actual=struct.unpack('<2I',pe_bytes(root/'build/port/sm64ds_pc.exe',symbols['_'+table],8))
    if actual!=tuple(symbols['_'+name] for name in names):
        raise ValueError('Particle callbacks are not bound to native workers: '+table)
stage=[address for name,address in symbols.items() if name.startswith('?stageGraphVtable@?')]
if len(stage)!=1:raise ValueError('Native Stage graphics table missing or ambiguous')
slots=struct.unpack('<4I',pe_bytes(root/'build/port/sm64ds_pc.exe',stage[0],16))
if slots[1]!=symbols['_pc_stage_graph_draw_particles'] or slots[2]!=symbols['_pc_stage_graph_apply']:
    raise ValueError('Stage graphics callbacks do not bind particle draw/minimap workers')
if '?GraphCallback1@dScStage_c@@QAEHXZ' not in symbols:
    raise ValueError('Original Stage particle callback is not compiled')
print('PASS: Stage slot1 draws original particles; four indirect particle callbacks bind native workers')
