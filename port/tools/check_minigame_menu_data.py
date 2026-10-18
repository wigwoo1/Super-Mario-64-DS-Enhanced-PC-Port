"""Check linked Rec Room tables, interior labels and profile against the ROM."""
from pathlib import Path
import re
import struct
from check_player_animation_data import pe_bytes
from minigame_menu_data import START,END,image,owned_names

root=Path(__file__).resolve().parents[2]
symbols={m[1]:int(m[2],16) for line in (root/'build/port/sm64ds_pc.map').read_text().splitlines()
         if (m:=re.match(r'\s+\d{4}:[\da-f]+\s+(\S+)\s+([\da-f]{8})\s',line))}
exe=root/'build/port/sm64ds_pc.exe'
base=symbols['_pc_minigame_menu_data']
expected,relocations=image(root)
expected=bytearray(expected)
for source,target in relocations.items(): struct.pack_into('<I',expected,source-START,base+target-START)
if pe_bytes(exe,base,END-START)!=expected: raise ValueError('Native Rec Room data differs from relocated ROM bytes')
for name,address in owned_names(root):
    if symbols['_'+name]!=base+address-START: raise ValueError('Rec Room interior label is detached: '+name)
profile=[address for name,address in symbols.items() if name=='_pc_minigame_menu_profile' or name.startswith('?pc_minigame_menu_profile@@')]
if len(profile)!=1: raise ValueError('Rec Room profile missing')
factory,behavior,render=struct.unpack('<IHH',pe_bytes(exe,profile[0],8))
if (factory,behavior,render)!=(symbols['_dScMiniGm_c_classInit'],5,8): raise ValueError('Incorrect Rec Room native profile')
print(f'PASS: Rec Room {END-START} data bytes, {len(relocations)} pointers, {len(owned_names(root))} interior labels and native profile')
