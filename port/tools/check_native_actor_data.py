#!/usr/bin/env python3
"""Audit native EXIT/DOOR descriptors and callback targets against the local ROM."""
import argparse
import pathlib
import re
import struct
from check_player_animation_data import pe_bytes
from link_data import records, source_bytes, message_text_callbacks
from lakitu_host import callback_bindings
from camera_host import owned_names, MODE_START, MODE_END

ROOT = pathlib.Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--map', type=pathlib.Path, default=ROOT / 'build/port/sm64ds_pc.map')
    parser.add_argument('--exe', type=pathlib.Path, default=ROOT / 'build/port/sm64ds_pc.exe')
    args = parser.parse_args()
    symbols = {}
    for line in args.map.read_text().splitlines():
        match = re.match(r'\s+\d{4}:[\da-f]+\s+(\S+)\s+([\da-f]{8})\s', line)
        if match:
            symbols[match[1]] = int(match[2], 16)

    def named(prefix):
        matches = {address for name, address in symbols.items()
                   if name == '_' + prefix or name.startswith('?' + prefix + '@@')}
        if len(matches) != 1:
            raise ValueError(f'{prefix}: missing or divergent native symbols: {matches}')
        return matches.pop()

    def overlay(number):
        text = (ROOT / f'config/arm9/overlays/ov{number:03d}/delinks.txt').read_text()
        base = int(re.search(r'\.text\s+start:0x([\da-f]+)', text)[1], 16)
        return base, (ROOT / f'extracted/overlays/overlay_{number:04d}.bin').read_bytes()

    from sound_host import STAGE_SOUND_START,STAGE_SOUND_END,STAGE_SOUND_LABELS
    sound_base=named(STAGE_SOUND_LABELS[0])
    for i,name in enumerate(STAGE_SOUND_LABELS):
        if named(name)!=sound_base+i:raise ValueError('Stage group/bank/music labels do not share their original byte lanes')
    arm=(ROOT/'extracted/arm9_dec.bin').read_bytes()
    if pe_bytes(args.exe,sound_base,STAGE_SOUND_END-STAGE_SOUND_START)!=arm[STAGE_SOUND_START-0x02004000:STAGE_SOUND_END-0x02004000]:
        raise ValueError('Native level sound table differs from original 52 records')
    print('PASS: all 52 original stage sound records and +1/+2 bank/music labels share their correct native allocation')

    # Dialogue cleanup must reach the original camera restore worker. A legacy
    # actor-smoke no-op previously hid its alternate C symbol in shipping builds.
    restore = named('func_0200d81c')
    owners = [line for line in args.map.read_text().splitlines()
              if re.search(r'\s_func_0200d81c\s', line)]
    if len(owners) != 1 or not owners[0].rstrip().endswith('dCamera_c.cpp.obj'):
        raise ValueError('Dialogue camera restore is not owned by the original camera TU')
    if pe_bytes(args.exe, restore, 1) == b'\xc3':
        raise ValueError('Dialogue camera restore still links an empty placeholder')

    message_targets = message_text_callbacks(ROOT)
    actual = struct.unpack('<3I', pe_bytes(args.exe, named('MSG_GEN_TEXT_FUNCS'), 12))
    if actual != tuple(named(target) for target in message_targets):
        raise ValueError('Message text formatters do not bind to native callback workers')

    for name, factory, module, guest in (
            ('pc_exit_profile', 'daChScene_c_classInit', 2, 0x02108690),
            ('pc_picture_gate_profile', 'daPicGate_c_classInit', 80, 0x02128290),
            ('pc_player_profile', 'daPly_c_classInit', 2, 0x0210a704),
            ('pc_warp_profile', 'daWarpkun_c_classInit', 2, 0x0210ac98),
            ('pc_demo_profile', 'daDemo_c_classInit', 2, 0x0210ba60),
            ('pc_lakitu_profile', 'daC_Jugem_c_classInit', 85, 0x02130320),
            ('pc_buddy_profile', 'daRedBombhei_c_classInit', 84, 0x02130a14),
            ('pc_cannon_profile', 'daCnn_c_classInit', 98, 0x0213c67c),
            ('pc_waterbomb_profile', 'daWbm_c_classInit', 98, 0x0213c74c),
            ('g_profile_CANNON_SHUTTER', 'daObjCannonShutter_c_classInit', 2, 0x02109d14),
            ('pc_rabbit_profile', 'daMip_c_classInit', 85, 0x021300d4),
            ('pc_rabbit_key_profile', 'daObj_Mip_Key_c_classInit', 85, 0x021301b4),
            ('g_profile_COIN', 'daCoin_c_classInit_COIN', 2, 0x02108790),
            ('g_profile_RED_COIN', 'daCoin_c_classInit_RED_COIN', 2, 0x021087ac),
            ('g_profile_BLUE_COIN', 'daCoin_c_classInit_BLUE_COIN', 2, 0x021087c8),
            ('g_profile_WANWAN', 'daWanwan_c_classInit', 14, 0x02114750),
            ('g_profile_PILE', 'daObjPile_c_classInit', 91, 0x02135298),
            ('g_profile_FL_LONDON', 'daObjFl_London_c_classInit', 22, 0x02113f4c),
            ('pc_fl_seesaw_profile', 'daObjFl_Seesaw_c_classInit', 22, 0x02114010),
            ('pc_floor_switch_profile', 'daObjSwitch_c_classInit_HANSWITCH', 2, 0x02109900),
            ('pc_star_switch_profile', 'daObjSwitch_c_classInit_STAR_SWITCH', 2, 0x0210991c),
            ('pc_updown_lift_profile', 'daObjRotateUpdownLift_c_classInit_UPDOWN_LIFT',91,0x02134bf8),
            ('pc_battlefield_seesaw_profile','daObjSeesaw_c_classInit_BOMB_SEESAW',95,0x02137484),
            ('pc_battlefield_gate_profile','daObjBSwdoor_c_classInit',14,0x021145e4),
            ('pc_koopa_race_profile','daRNk_c_classInit',62,0x0211db78),
            ('pc_koopa_flag_profile','daRFlag_c_classInit',62,0x0211dc30),
            ('g_profile_WANWAN_SHUTTER', 'daObjWanwanShutter_c_classInit', 14, 0x0211488c),
            ('g_profile_SBIRD', 'daSBird_c_classInit', 9, 0x02113934),
            ('g_profile_KINOPIO', 'daKinopio_c_classInit', 85, 0x0212fe6c),
            ('g_profile_STAR_GATE', 'daStarGate_c_classInit', 100, 0x021483a8),
            ('pc_door_profile', 'daDoor_c_classInit', 100, 0x02148164)):
        base, rom = overlay(module)
        native = pe_bytes(args.exe, named(name), 28)
        if struct.unpack_from('<I', native)[0] != named(factory):
            raise ValueError(f'{name}: factory does not point at the native constructor')
        if native[4:] != rom[guest - base + 4:guest - base + 28]:
            raise ValueError(f'{name}: priority/flags/ranges differ from the local ROM')

    base62,rom62=overlay(62)
    koopa_states=((0x0211db48,0x0211a9c4),(0x0211db30,0x0211a740),
                  (0x0211db58,0x0211a1f4),(0x0211db50,0x0211a168),
                  (0x0211db40,0x0211a0f0),(0x0211db38,0x02119be0))
    koopa_targets=[]
    for record,guest_target in koopa_states:
        if struct.unpack_from('<II',rom62,record-base62)!=(guest_target,0):
            raise ValueError('Original Koopa state record changed')
        matches={address for name,address in symbols.items()
                 if name.startswith(f'?func_ov062_{guest_target:08x}@daRNk_c@@')}
        if len(matches)!=1:raise ValueError('Missing/divergent native Koopa state')
        koopa_targets.append(matches.pop())
    if struct.unpack('<6I',pe_bytes(args.exe,named('data_ov062_0211e0a4'),24))!=tuple(koopa_targets):
        raise ValueError('Koopa state table does not bind to the six original handlers')

    switch_methods=('020ba3fc','020ba3a8','020ba4c0','020ba2d0',
                    '020ba2ac','020ba1ac','020ba4c0','020ba0f8','020ba4c0','020ba0bc')
    targets=[]
    for method in switch_methods:
        prefix=f'?func_ov002_{method}@daObjSwitch_c@@'
        matches={address for name,address in symbols.items() if name.startswith(prefix)}
        if len(matches)!=1:raise ValueError('Missing/divergent floor-switch state method '+method)
        targets.append(matches.pop())
    if struct.unpack('<10I',pe_bytes(args.exe,named('data_ov002_0210e00c'),40))!=tuple(targets):
        raise ValueError('Floor switch state pairs do not bind to ten native member pointers')
    base2,rom2=overlay(2)
    for address in (0x0210d854,0x0210d894):
        # Each selected CLPS block ends at the next32-byte symbol boundary.
        if pe_bytes(args.exe,named(f'data_ov002_{address:08x}'),32)!=rom2[address-base2:address-base2+32]:
            raise ValueError('Floor switch collision-material bytes differ from original')

    base64,rom64=overlay(64)
    # Complete movement paths and collision materials, rather than only their
    # first named coordinate/word. Interior labels must not truncate a table.
    for number,address,size in (
            (91,0x02134cdc,360),(91,0x021344e8,12),
            (91,0x02134bac,12),(91,0x02134bd0,12),
            (91,0x02134bb8,12),(91,0x02134ba0,12),
            (95,0x02136f58,16),(14,0x02114578,32),
            (14,0x02114598,32),(2,0x0210d834,32)):
        base,rom=overlay(number)
        name=f'data_ov{number:03d}_{address:08x}'
        if pe_bytes(args.exe,named(name),size)!=rom[address-base:address-base+size]:
            raise ValueError('Battlefield platform data differs from original: '+name)
    if pe_bytes(args.exe,named('data_ov064_0211bb2c'),32)!=rom64[0x0211bb2c-base64:0x0211bb4c-base64]:
        raise ValueError('Lava bridge collision properties differ from original')
    if pe_bytes(args.exe,named('data_ov064_0211bacc'),32)!=rom64[0x0211bacc-base64:0x0211bacc-base64+32]:
        raise ValueError('Lethal Lava Land seesaw collision properties differ from original')
    base100,rom100=overlay(100)
    if pe_bytes(args.exe,named('data_ov100_02148390'),24)!=rom100[0x02148390-base100:0x02148390-base100+24]:
        raise ValueError('Star Gate thresholds/save flags/message table differs from original')

    # Native Toad's member-function pointers are four bytes on MSVC x86;
    # its two rows are12bytes each, not the original ARM20byte layout.
    toad_states=[]
    for state in ('Idle','Talk'):
        for phase in ('Init','Main'):
            prefix=f'?St_{state}_{phase}@daKinopio_c@@'
            matches={address for name,address in symbols.items() if name.startswith(prefix)}
            if len(matches)!=1:raise ValueError('Missing/divergent native Toad callback '+prefix)
            toad_states.append(matches.pop())
        toad_states.append(0)
    if struct.unpack('<6I',pe_bytes(args.exe,named('data_ov085_0212fe88'),24))!=tuple(toad_states):
        raise ValueError('Toad state table does not bind to native methods')
    base85,rom85=overlay(85)
    requirements=rom85[0x0212f27c-base85:0x0212f27c-base85+3]
    if requirements!=bytes((20,40,60)) or pe_bytes(args.exe,named('data_ov085_0212f27c'),3)!=requirements:
        raise ValueError('Native Toad Star requirements differ from original 20/40/60 thresholds')

    from chomp_host import CALLBACKS
    for descriptor,target in CALLBACKS:
        native=struct.unpack('<II',pe_bytes(args.exe,named(f'data_ov014_{descriptor:08x}'),8))
        if native != (named(f'pc_chomp_{target:08x}'),0):
            raise ValueError('Chain Chomp native callback target/adjustment mismatch')
    from buddy_host import inventory as buddy_inventory
    base98, rom98 = overlay(98)
    if pe_bytes(args.exe, named('data_ov098_0213bf90'), 10) != rom98[0x0213bf90-base98:0x0213bf90-base98+10]:
        raise ValueError('Water-bomb fragment angle table is not five complete original entries')
    from cannon_host import inventory as cannon_inventory
    for table,bomb in (('data_ov098_0213c8fc',False),('data_ov098_0213c930',True)):
        targets=cannon_inventory(ROOT,bomb)
        actual=struct.unpack('<'+str(len(targets)*2)+'I',pe_bytes(args.exe,named(table),len(targets)*8))
        expected=tuple(word for target in targets for word in (named(f'pc_cannon_{target:08x}'),0))
        if actual!=expected:raise ValueError('Cannon callback target/stride mismatch: '+table)
    buddy_targets=buddy_inventory(ROOT)
    buddy_callbacks=struct.unpack('<12I',pe_bytes(args.exe,named('data_ov084_02130dc4'),48))
    expected=tuple(value for target in buddy_targets for value in (named(f'pc_buddy_{target:08x}'),0))
    if buddy_callbacks!=expected:raise ValueError('Buddy callback stride or native target mismatch')
    for handle in ('data_ov084_02130da4','data_ov084_02130d9c','data_ov002_0210e12c','data_ov002_0210e124'):
        named(handle)

    from rabbit_host import inventory
    rabbit_bindings,key_bindings=inventory(ROOT)
    rabbit_source=(ROOT/'src/actors/daMip_c.cpp').read_text()
    for record,entry,execute in re.findall(r'MipStatePair (data_ov085_[\da-f]+) = \{(data_ov085_[\da-f]+), (data_ov085_[\da-f]+)\};',rabbit_source):
        actual=struct.unpack('<4I',pe_bytes(args.exe,named(record),16))
        expected=(named('pc_mip_'+rabbit_bindings[entry]),0,named('pc_mip_'+rabbit_bindings[execute]),0)
        if actual!=expected:raise ValueError('Rabbit state has a non-native callback: '+record)
    for record,entry,execute in (('data_ov085_0213071c','data_ov085_02130184','data_ov085_0213018c'),('data_ov085_0213072c','data_ov085_0213017c','data_ov085_02130174')):
        if struct.unpack('<2I',pe_bytes(args.exe,named(record),8))!=(named(key_bindings[entry]),named(key_bindings[execute])):
            raise ValueError('Rabbit key state has a non-native callback: '+record)
    for address in (0x021305d8,0x021305e0,0x021305b8,0x021305d0,0x021305b0,0x021305c8,0x021305c0):
        named(f'data_ov085_{address:08x}')

    base, map_rom = overlay(2)
    lakitu_bindings, lakitu_pairs = callback_bindings(ROOT)
    for record, init, main in lakitu_pairs:
        actual = struct.unpack('<II', pe_bytes(args.exe, named(record), 8))
        expected = []
        for constant in (init, main):
            method = lakitu_bindings[constant][0]
            native_name = f'?{method}@daC_Jugem_c@@QAEHXZ'
            if native_name not in symbols:
                raise ValueError(f'Lakitu method {method}: missing native symbol')
            expected.append(symbols[native_name])
        if actual != tuple(expected):
            raise ValueError(f'Lakitu state {record}: callback pair does not match the ROM/native methods')
    for address in (0x0213074c, 0x02130744, 0x0213073c):
        named(f'data_ov085_{address:08x}')
    arm9 = (ROOT / 'extracted/arm9_dec.bin').read_bytes()
    touch_base=named('gTouchHeld')
    for name,offset in (('gTouchEdge',1),('gTouchX',2),('gTouchY',3),('pc_touch_state_end',16)):
        if named(name)!=touch_base+offset:
            raise ValueError('Native touch labels do not share the four original byte records: '+name)
    input_base = named('data_020a0e58')
    raw_base=named('data_020a1040')
    for suffix,offset in (('1044',4),('1046',6),('1048',8),('104c',12),('104e',14),('1050',16),('1052',18)):
        if named('data_020a'+suffix)!=raw_base+offset:raise ValueError('raw input interior label diverged: '+suffix)
    if named('pc_raw_input_end')!=raw_base+0x24:raise ValueError('raw input record extent')
    history_base=named('data_020a1154')
    for suffix,offset in (('1160',12),('1162',14),('1164',16),('1166',18),('1178',36),('117c',40),('11a0',76),('11c4',112)):
        if named('data_020a'+suffix)!=history_base+offset:raise ValueError('input history interior label diverged: '+suffix)
    if named('pc_input_history_end')!=history_base+4*0x24:raise ValueError('four input history record extents')
    if named('data_020a0e5a') != input_base + 2 or named('pc_input_state_end') != input_base + 16:
        raise ValueError('Native held/pressed input labels do not share the complete four-player record')
    controller_base = named('data_0209f498')
    for address in (0x0209f49c,0x0209f49e,0x0209f4a0,0x0209f4a2,0x0209f4a4,
                    0x0209f4a6,0x0209f4a8,0x0209f4a9,0x0209f4ab,0x0209f4ac,0x0209f4ae):
        if named(f'data_{address:08x}') != controller_base+address-0x0209f498:
            raise ValueError(f'Processed controller field {address:08x} is not canonical')
    if named('pc_controller_state_end') != controller_base+4*24:
        raise ValueError('Processed controller block does not contain all four records')
    oam_base = named('data_0209e674')
    for name, offset in (('data_0209e67c', 8), ('data_0209e694', 32), ('data_0209ea74', 0x400)):
        if named(name) != oam_base + offset:
            raise ValueError(f'Native OAM interior label {name} is not canonical')
    camera_states, camera_modes = owned_names(ROOT)
    camera_base = named('data_0209b008')
    for record, slots in camera_states.items():
        address = named(record)
        if address - camera_base != int(record[5:], 16) - 0x0209b008:
            raise ValueError(f'Camera state {record}: record stride differs from original')
        actual = struct.unpack('<4I', pe_bytes(args.exe, address, 16))
        expected = []
        for slot in range(2):
            worker = slots[slot]
            # void workers use typed adapters; every other callback is a
            # native cdecl worker with one 32-bit camera address argument.
            target = ('pc_camera_' + worker if worker in
                      {'func_0200987c', 'func_020094c0', 'func_02005000'} else worker)
            expected += [named(target), 0]
            named(worker)
        if actual != tuple(expected):
            raise ValueError(f'Camera state {record}: wrong native callback or adjustment')
    mode_base = named(f'data_{MODE_START:08x}')
    for name, guest in camera_modes.items():
        if named(name) - mode_base != guest - MODE_START:
            raise ValueError(f'Camera mode {name}: interior address lost')
    if pe_bytes(args.exe, mode_base, MODE_END - MODE_START) != arm9[
            MODE_START - 0x02004000:MODE_END - 0x02004000]:
        raise ValueError('Camera mode table differs from the original 34 complete records')
    control_maps = struct.unpack_from('<3I', arm9, 0x0209214c - 0x02004000)
    native_maps = struct.unpack('<3I', pe_bytes(args.exe, named('data_0209214c'), 12))
    for guest, native in zip(control_maps, native_maps):
        if native != named(f'data_{guest:08x}'):
            raise ValueError('Control mode mapping still contains a DS pointer')
        if pe_bytes(args.exe, native, 32) != arm9[guest - 0x02004000:guest - 0x02004000 + 32]:
            raise ValueError('Control mode key map differs from the original 16 halfwords')
    for address in (0x020ff060, 0x020ff06c):
        original = struct.unpack_from('<3I', map_rom, address - base)
        native = struct.unpack('<3I', pe_bytes(args.exe, named(f'data_ov002_{address:08x}'), 12))
        expected = tuple(named(f'data_ov002_{target:08x}') for target in original)
        if native != expected:
            raise ValueError(f'Coin resource table {address:08x}: handles are not canonical native objects')
    tongue_rom = struct.unpack_from('<2I', map_rom, 0x0210a084 - base)
    tongue_native = struct.unpack('<2I', pe_bytes(args.exe, named('data_ov002_0210a084'), 8))
    if tongue_rom != (0x0210e6c0, 0x0210eb98):
        raise ValueError('Yoshi animation handle manifest changed')
    if tongue_native != tuple(named(f'data_ov002_{target:08x}') for target in tongue_rom):
        raise ValueError('Yoshi tongue/retract pointers are not canonical native handles')
    # Player's character-specific jump table is populated by the ov002
    # initializer from four distinct descriptors, not one contiguous range.
    for descriptor, target in ((0x02109f0c, 0x020e200c),
                               (0x0210a47c, 0x020e1e70),
                               (0x0210a10c, 0x020e200c),
                               (0x0210a45c, 0x020e1c20)):
        actual = struct.unpack_from('<II', map_rom, descriptor - base)
        if actual != (target, 0):
            raise ValueError(f'Player jump descriptor {descriptor:08x}: target/adjustment changed')
        named(f'func_ov002_{target:08x}')
    named('pc_player_jump_callback')
    sequence_rom = map_rom[0x0210a7a0 - base:0x0210a7e8 - base]
    sequence_native = pe_bytes(args.exe, named('data_ov002_0210a7a0'), 18 * 4)
    available = records(ROOT)
    for guest, native in zip(struct.unpack('<18I', sequence_rom),
                             struct.unpack('<18I', sequence_native)):
        matches = [row for row in available
                   if row['module'] == 'ov002' and row['address'] == guest]
        if len(matches) != 1 or native == guest:
            raise ValueError(f'Player animation sequence {guest:08x}: missing native target')
        expected = source_bytes(ROOT, matches[0])
        if pe_bytes(args.exe, native, len(expected), allow_zero_tail=True) != expected:
            raise ValueError(f'Player animation sequence {guest:08x}: list bytes differ')
    for table_address, count in ((0x0210be1c, 4), (0x0210b998, 4), (0x0210c3e0, 2)):
        original = map_rom[table_address - base:table_address - base + count * 4]
        compiled = pe_bytes(args.exe, named(f'data_ov002_{table_address:08x}'), count * 4)
        for guest, native in zip(struct.unpack(f'<{count}I', original), struct.unpack(f'<{count}I', compiled)):
            matches = [row for row in available
                       if row['module'] == 'ov002' and row['address'] == guest]
            if len(matches) != 1 or native == guest:
                raise ValueError(f'DEMO sprite {guest:08x}: missing native target')
            expected = source_bytes(ROOT, matches[0])
            if pe_bytes(args.exe, native, len(expected)) != expected:
                raise ValueError(f'DEMO sprite {guest:08x}: OAM bytes differ')
    preload_row = next(row for row in available if row['name'] == 'data_020756f0')
    original = source_bytes(ROOT, preload_row)
    compiled = pe_bytes(args.exe, named('data_020756f0'), 12 * 4)
    if len(original) != 12 * 4:
        raise ValueError('Stage preload/release table extent changed')
    for guest, native in zip(struct.unpack('<12I', original), struct.unpack('<12I', compiled)):
        if native != named(f'data_ov002_{guest:08x}'):
            raise ValueError(f'Stage handle {guest:08x}: not bound to the initialized native object')
    stage_row = next(row for row in available if row['name'] == 'g_profile_STAGE')
    original = source_bytes(ROOT, stage_row)
    compiled = pe_bytes(args.exe, named('pc_stage_profile'), 8)
    if len(original) != 8 or compiled[4:] != original[4:]:
        raise ValueError('Stage native profile priorities differ from the original descriptor')
    if struct.unpack_from('<I', compiled)[0] != named('dScStage_c_classInit'):
        raise ValueError('Stage native profile has no native factory')
    star_select_row = next(row for row in available if row['name'] == 'g_profile_STAR_SELECT')
    original_select = source_bytes(ROOT, star_select_row)
    native_select = pe_bytes(args.exe, named('pc_star_select_profile'), 8)
    if native_select[4:] != original_select[4:8]:
        raise ValueError('Star selection priorities differ from the original descriptor')
    if struct.unpack_from('<I', native_select)[0] != named('dScStarSel_c_classInit'):
        raise ValueError('Star selection profile lacks its native factory')
    map_profile = pe_bytes(args.exe, named('pc_map_profile'), 8)
    if struct.unpack_from('<I', map_profile)[0] != named('dMap_c_classInit'):
        raise ValueError('MAP factory does not point at the native constructor')
    if map_profile[4:] != map_rom[0x0210c160 - base + 4:0x0210c160 - base + 8]:
        raise ValueError('MAP priorities differ from the original eight-byte profile')

    base, rom = overlay(100)
    raw_variants = rom[0x02148204 - base:0x02148334 - base]
    if pe_bytes(args.exe, named('data_ov100_02148204'), len(raw_variants)) != raw_variants:
        raise ValueError('DOOR variant table extent or metadata differs from the local ROM')
    for offset in range(0, len(raw_variants), 16):
        for pointer in struct.unpack_from('<II', raw_variants, offset):
            if pointer:
                named(f'data_ov100_{pointer:08x}')

    key_base, key_rom = overlay(89)
    for guest in (0x02132894, 0x021328b4):
        raw = key_rom[guest - key_base:guest - key_base + 32]
        if pe_bytes(args.exe, named(f'data_ov089_{guest:08x}'), 32) != raw:
            raise ValueError('key-resource table differs from the original relocation input')
    for line in (ROOT / 'config/arm9/overlays/ov089/relocs.txt').read_text().splitlines():
        match = re.match(r'from:0x([\da-f]+) kind:load to:0x([\da-f]+) module:overlay\((\d+)\)', line)
        if match and 0x02132894 <= int(match[1], 16) < 0x021328d4:
            named(f'data_ov{int(match[3]):03d}_{int(match[2], 16):08x}')

    # State records are built by overlay initialization. Check each native
    # callback against its original two-word method descriptor independently.
    states = {
        0x021488b4: (0, 0x0214813c), 0x021488c4: (0x021480dc, 0x021480e4),
        0x021488d4: (0x021480f4, 0x021480ec), 0x021488e4: (0x0214810c, 0x021480fc),
        0x021488f4: (0x0214811c, 0x02148104), 0x02148904: (0x02148124, 0x0214812c),
        0x02148914: (0x02148144, 0x021480d4), 0x02148924: (0, 0x02148134),
        0x021488a4: (0, 0x02148114),
    }
    for address, descriptors in states.items():
        actual = struct.unpack('<4I', pe_bytes(args.exe, named(f'data_ov100_{address:08x}'), 16))
        expected = []
        for descriptor in descriptors:
            if not descriptor:
                expected += [0, 0]
                continue
            method, adjustment = struct.unpack_from('<II', rom, descriptor - base)
            if adjustment:
                raise ValueError(f'unsupported DOOR this-adjustment at {descriptor:08x}')
            expected += [named(f'pc_door_{method:08x}'), 0]
        if actual != tuple(expected):
            raise ValueError(f'DOOR state {address:08x}: incorrect callback or stride')

    scratch = named('data_020a0e68')
    next_object = min(address for address in symbols.values() if address > scratch)
    if next_object - scratch < 48:
        raise ValueError('shared Matrix4x3 allocation overlaps the next native object')
    print('PASS: original Toad 20/40/60 Star thresholds verified; 3 native message formatters verified; all 4 native held/pressed input pairs share original interior labels; '
          'all 4 processed-controller records share the original 24-byte layout and interior labels; '
          'all 4 touch records share their original held/edge/x/y byte lanes; '
          'paired OAM buffers retain +8/+32/+1024 reset labels; '
          'native profiles match ROM metadata/factories; 7 cannon/water-bomb callbacks, 5 fragment angles and shared raw/history input labels verified; '
          '3 Buddy state pairs and 4 shared handles verified; 8 rabbit state pairs and 2 key callback pairs verified; 11 Lakitu state '
          'pairs bind to 22 ROM-identified native methods; shared Lakitu handles '
          'have canonical storage; 19 camera state pairs bind to 38 native targets '
          'with original record strides; 34 camera mode records retain complete '
          'ROM bytes and interior labels; 3 control-mode maps have native pointers '
          'and ROM-matching key entries; 6 coin resource '
          'pointers bind to canonical native handles; 19 DOOR variants and '
          '9 native callback records verified; shared key-resource targets resolve; '
          'Matrix4x3 scratch has 48 bytes; 4 Player jump descriptors and '
          'their 3 native workers verified; 2 Yoshi tongue/retract handles verified; 18 animation-sequence pointers '
          'resolve to complete ROM-matching native lists; 2 camera warning icon '
          'pointers and 8 DEMO sprite '
          'pointers resolve to ROM-matching OAM data; 12 stage preload/release '
          'pointers bind to canonical native handles; native STAR_SELECT profile matches ROM priorities and factory')


if __name__ == '__main__':
    main()

