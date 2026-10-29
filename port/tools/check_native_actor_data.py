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

    # ROM-free kits rename prebuilt objects to stable numbered paths. Resolve
    # those map owners back to their reviewed build inputs for source ownership
    # checks; ordinary source-build maps keep their original object filenames.
    object_origins = {}
    kit_manifest = ROOT / '.native-kit/manifest.json'
    if kit_manifest.is_file():
        import json
        metadata = json.loads(kit_manifest.read_text(encoding='utf-8'))
        object_origins = {
            pathlib.PurePosixPath(item['path']).name:
                pathlib.PurePosixPath(item['origin_object']).name
            for item in metadata.get('prebuilt', [])
            if item.get('path', '').endswith('.obj')
        }

    def owner_is(line, expected_object):
        owner = line.rsplit(maxsplit=1)[-1]
        owner = object_origins.get(pathlib.PurePath(owner).name,
                                   pathlib.PurePath(owner).name)
        return owner.endswith(expected_object)

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

    # The original common-model BSS cache occupies 0x0209cefc..0x0209d3ac:
    # 100 twelve-byte entries. A legacy 64-entry host buffer placed its count
    # inside that span, so registering more models corrupted adjacent state.
    common_cache=named('data_0209cefc')
    common_count=named('data_0209cef8')
    if common_cache<=common_count<common_cache+(0x0209d3ac-0x0209cefc):
        raise ValueError('Native common-model cache is smaller than the original 100-entry BSS span')
    print('PASS: original 100-entry common-model cache span does not overlap its native counter')

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
    if len(owners) != 1 or not owner_is(owners[0], 'dCamera_c.cpp.obj'):
        raise ValueError('Dialogue camera restore is not owned by the original camera TU')
    if pe_bytes(args.exe, restore, 1) == b'\xc3':
        raise ValueError('Dialogue camera restore still links an empty placeholder')

    message_targets = message_text_callbacks(ROOT)
    actual = struct.unpack('<3I', pe_bytes(args.exe, named('MSG_GEN_TEXT_FUNCS'), 12))
    if actual != tuple(named(target) for target in message_targets):
        raise ValueError('Message text formatters do not bind to native callback workers')

    ambient_base,ambient_rom=overlay(2)
    ambient_table=named('data_ov002_0210b498')
    if pe_bytes(args.exe,ambient_table,44)!=ambient_rom[0x0210b498-ambient_base:0x0210b4c4-ambient_base]:
        raise ValueError('Native ambient sound table must contain all eleven original IDs')
    print('PASS: all eleven original ambient sound IDs share their native table')
    if pe_bytes(args.exe,named('data_020821c0'),72)!=arm[0x020821c0-0x02004000:0x02082208-0x02004000]:
        raise ValueError('Native sound distance presets differ from the original nine pairs')
    print('PASS: all nine original sound distance presets retained')

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
            ('pc_item_block_profile','daObjHatenaBlock_c_classInit_ITEM_BLOCK',102,0x0214e404),
            ('pc_question_block_profile','daObjHatenaBlock_c_classInit_HATENA_BLOCK',102,0x0214e3e8),
            ('g_profile_POWER_UP_ITEM','daObjPowerUpItem_c_classInit',2,0x021097a0),
            ('pc_feather_profile','daFeather_c_classInit',2,0x02108884),
            ('g_profile_HATENA_SWITCH','daObjHatenaSwitch_c_classInit',2,0x02108e38),
            ('pc_castle_light_profile','daObjC1Hikari_c_classInit',10,0x02112b84),
            ('pc_ambient_sound_profile','daSetSE_c_classInit',2,0x0210b47c),
            ('pc_healing_heart_profile','daObjHeart_c_classInit',2,0x02109c50),
            ('pc_arrow_left_profile','daObjYajirusi_c_classInit_YAJIRUSI_L',98,0x0213c398),
            ('pc_arrow_right_profile','daObjYajirusi_c_classInit_YAJIRUSI_R',98,0x0213c3b4),
            ('pc_th_fall_block_profile','daObjTh_Fall_Block_c_classInit',63,0x0211eb34),
            ('pc_kaidan_profile','daTrsTrap_c_classInit_KAIDAN',63,0x0211ea10),
            ('pc_boo_profile','daTrs_c_classInit_TERESA',63,0x0211e770),
            ('pc_big_boo_profile','daTrs_c_classInit_BOSS_TERESA',63,0x0211e78c),
            ('pc_boo_basket_profile','daTBasket_c_classInit',63,0x0211e7a8),
            ('pc_boo_icon_profile','daTrsIcon_c_classInit',63,0x0211e7c4),
            ('pc_merry_go_round_profile','daTrsTrap_c_classInit_MERRYGOROUND',63,0x0211ea48),
            ('pc_teresapit_profile','daTrsTrap_c_classInit_TERESAPIT',63,0x0211ea64),
            ('pc_oneup_profile','da1up_c_classInit_ONEUPKINOKO',2,0x02108388),
            ('pc_oneup_logo_profile','daObj1UpLogo_c_classInit',2,0x0210b188),
            ('pc_bookshelf_profile','daTrsTrap_c_classInit_BOOKSHELF',63,0x0211ea2c),
            ('pc_book_switch_profile','daBook_c_classInit_BOOK_SWITCH',20,0x02114898),
            ('pc_shoot_book_profile','daBook_c_classInit_SHOOT_BOOK',20,0x021148b4),
            ('pc_signpost_profile','daObjTatefuda_c_classInit',2,0x02109ad4),
            ('pc_crate_profile','daObjBlockS_c_classInit',98,0x0213c510),
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
    print('PASS: original KAIDAN65 native factory and full profile descriptor match overlay 63')
    for suffix in ('edc4','edcc','edd4','eddc','ede4','edec','edf4'):
        label='data_ov063_0211'+suffix
        named(label)
        owners=[line for line in args.map.read_text().splitlines()
                if re.search(r'\s[?_]'+label+r'(?:@@|\s)',line)]
        if not owners or any(not owner_is(line,'daTrs_c.cpp.obj') for line in owners):
            raise ValueError('Boo handle must belong to the native recovered actor: '+label)
    def boo_method(prefix):
        candidates={address for name,address in symbols.items() if name.startswith(prefix)}
        if len(candidates)!=1:raise ValueError('Missing/ambiguous native Boo callback: '+prefix)
        return candidates.pop()
    boo_vtable=struct.unpack('<31I',pe_bytes(args.exe,boo_method('??_7daTrs_c@@'),124))
    # MSVC has one deleting-destructor slot: ROM slots18/29 become17/28.
    for slot,method in ((0,'InitResources'),(3,'CleanupResources'),(6,'Behavior'),
                        (9,'Render'),(17,'OnYoshiTryEat'),(28,'OnAimedAtWithEgg')):
        if boo_vtable[slot]!=boo_method('?'+method+'@daTrs_c@@'):
            raise ValueError('Boo callback does not use its compiler-owned native method: '+method)
    if boo_vtable[16]!=boo_method('??_GdaTrs_c@@'):
        raise ValueError('Boo deleting destructor is not native')
    print('PASS: original Boo/Big Boo/icon/basket profiles, seven canonical resources and seven native callbacks')

    base2,rom2=overlay(2)
    for label in ('data_ov002_0210e104','data_ov002_0210e0fc'):
        named(label)
        owners=[line for line in args.map.read_text().splitlines()
                if re.search(r'\s_'+label+r'\s',line)]
        if len(owners)!=1 or not owner_is(owners[0], 'daObjHeart_c.cpp.obj'):
            raise ValueError('Healing-heart resources must belong to the native actor: '+label)
    print('PASS: original HEART profile/native factory and both canonical native resource handles')
    models=named('pc_arrow_models');collisions=named('pc_arrow_collisions')
    expected_resources=(models,collisions,named('data_ov002_0210d914'),
                        models+8,collisions+8,named('data_ov002_0210d734'))
    if struct.unpack('<6I',pe_bytes(args.exe,named('pc_arrow_resources'),24))!=expected_resources:
        raise ValueError('Arrow resources have wrong eight-byte handle or twelve-byte row strides')
    if max(models,collisions)<min(models,collisions)+16:
        raise ValueError('Arrow model/collision handle arrays overlap')
    arrow_rows=records(ROOT)
    for label in ('data_ov002_0210d914','data_ov002_0210d734'):
        row=next(r for r in arrow_rows if r['name']==label)
        original=source_bytes(ROOT,row)
        if pe_bytes(args.exe,named(label),len(original))!=original:
            raise ValueError('Arrow collision-material bytes differ from original: '+label)
    def arrow_method(prefix):
        candidates={address for name,address in symbols.items() if name.startswith(prefix)}
        if len(candidates)!=1:raise ValueError('Missing/ambiguous native arrow callback: '+prefix)
        return candidates.pop()
    arrow_vtable=struct.unpack('<31I',pe_bytes(args.exe,arrow_method('??_7PcNativeArrowSign@@'),124))
    for slot,method in ((0,'InitResources'),(3,'CleanupResources'),(6,'Behavior'),(9,'Render'),
                        (21,'OnAttacked1'),(26,'OnHitByMegaChar'),(30,'Kill')):
        if arrow_vtable[slot]!=arrow_method('?'+method+'@PcNativeArrowSign@@'):
            raise ValueError('Incorrect compiler-owned arrow callback slot: '+method)
    if arrow_vtable[16]!=arrow_method('??_GPcNativeArrowSign@@'):
        raise ValueError('Arrow cleanup does not use its native deleting destructor')
    print('PASS: both original arrow profiles/factories; four eight-byte handles, resource row stride, original CLPS bytes and eight native virtual callbacks')
    fall_model=named('data_ov063_0211ef60');fall_collision=named('data_ov063_0211ef58')
    if abs(fall_model-fall_collision)<8:
        raise ValueError('Falling-block native file handles overlap')
    if struct.unpack('<3I',pe_bytes(args.exe,named('data_ov063_0211eb10'),12))!=(
            fall_model,fall_collision,named('data_ov063_0211ec98')):
        raise ValueError('Falling-block descriptor still contains DS addresses or incorrect native resource identities')
    fall_clps=next(row for row in records(ROOT) if row['name']=='data_ov063_0211ec98')
    original=source_bytes(ROOT,fall_clps)
    if pe_bytes(args.exe,named('data_ov063_0211ec98'),len(original))!=original:
        raise ValueError('Falling-block collision properties differ from original')
    fall_vtable=struct.unpack('<31I',pe_bytes(args.exe,arrow_method('??_7daObjTh_Fall_Block_c@@'),124))
    for slot,method,owner in ((0,'InitResources','daObjTh_Fall_Block_c'),
                              (3,'CleanupResources','daObjTh_Fall_Block_c'),
                              (6,'Behavior','daObjFallBlock_c'),(9,'Render','daObjFallBlock_c'),
                              (26,'OnHitByMegaChar','daObjFallBlock_c'),(30,'Kill','daObjFallBlock_c')):
        if fall_vtable[slot]!=arrow_method('?'+method+'@'+owner+'@@'):
            raise ValueError('Incorrect native falling-block virtual callback: '+method)
    if fall_vtable[16]!=arrow_method('??_GdaObjTh_Fall_Block_c@@'):
        raise ValueError('Falling block lacks its native deleting destructor')
    print('PASS: original falling-block profile/factory, canonical eight-byte handles/native descriptor/CLPS and seven virtual callbacks')
    for label,suffixes in (('data_ov063_0211e27c',('eeb0','eea8','eeb8','ee98')),
                           ('data_ov063_0211e28c',('eea0','eed0','eec0','eec8')),
                           ('data_ov063_0211e9e8',('ec58','ebf8','ec78','ec18'))):
        if struct.unpack('<4I',pe_bytes(args.exe,named(label),16))!=tuple(named('data_ov063_0211'+s) for s in suffixes):
            raise ValueError('Spinning-floor resource row contains incorrect native identities: '+label)
    for suffix in ('ec58','ebf8','ec78','ec18'):
        label='data_ov063_0211'+suffix
        row=next(r for r in records(ROOT) if r['name']==label);original=source_bytes(ROOT,row)
        if pe_bytes(args.exe,named(label),len(original))!=original:raise ValueError('Furniture CLPS differs from original: '+label)
    merry_targets=tuple(arrow_method('?func_ov063_'+address+'@daTrsTrap_c@@') for address in ('0211cc18','0211cb54','0211c89c','0211c7b0'))
    if struct.unpack('<4I',pe_bytes(args.exe,named('data_ov063_0211ef38'),16))!=merry_targets:
        raise ValueError('Spinning-floor dispatch does not use four native member callbacks')
    merry_vtable=struct.unpack('<28I',pe_bytes(args.exe,arrow_method('??_7daTrsTrap_c@@'),112))
    for slot,method in ((0,'InitResources'),(3,'CleanupResources'),(6,'Behavior'),(9,'Render'),(12,'OnPendingDestroy')):
        if merry_vtable[slot]!=arrow_method('?'+method+'@daTrsTrap_c@@'):
            raise ValueError('Spinning-floor native virtual callback mismatch: '+method)
    if merry_vtable[16]!=arrow_method('??_GdaTrsTrap_c@@'):raise ValueError('Spinning floor lacks native deleting destructor')
    print('PASS: original spinning-floor159 and trapdoor160 profiles/factories, three canonical four-row resources/CLPS, four native state dispatches and six virtual callbacks')
    switch_records=records(ROOT)
    for label in ('data_ov002_0210d8b4','data_ov002_0210d774'):
        row=next(item for item in switch_records if item['name']==label)
        original=source_bytes(ROOT,row)
        if pe_bytes(args.exe,named(label),len(original))!=original:
            raise ValueError('Question switch collision properties differ from original data: '+label)
    print('PASS: original question switch profile/native factory and both collision-property blocks')
    sign_targets = tuple(named(f'pc_sign_dispatch_{i}') for i in range(10))
    if struct.unpack('<10I', pe_bytes(args.exe, named('data_ov002_0210e084'), 40)) != sign_targets:
        raise ValueError('Signpost state dispatch does not use the ten typed native handlers')
    for name in ('data_ov002_020ff0d0','data_ov002_020ff0d4','data_ov002_020ff0d8','data_ov002_020ff0dc'):
        address=int(name.rsplit('_',1)[1],16)
        length=4 if address==0x020ff0d0 else 0x020ff0ec-address
        if pe_bytes(args.exe,named(name),length)!=rom2[address-base2:address-base2+length]:
            raise ValueError('Signpost attachment constants differ from original data: '+name)
    print('PASS: original signpost descriptor, typed state handlers and attachment constants')
    from crate_host import STATE_METHODS
    crate_base,crate_rom=overlay(98)
    crate_descriptors=(0x0213c4a8,0x0213c488,0x0213c490,0x0213c468,
                       0x0213c498,0x0213c470,0x0213c4b0,0x0213c480,
                       0x0213c4a0,0x0213c478,0x0213c458,0x0213c460,
                       0x0213c4c0,0x0213c4b8)
    for descriptor,method in zip(crate_descriptors,(m for pair in STATE_METHODS for m in pair)):
        target,adjustment=struct.unpack_from('<II',crate_rom,descriptor-crate_base)
        if adjustment or target!=int(method.rsplit('_',1)[1],16):
            raise ValueError('Original crate state identity/adjustment changed')
    expected=tuple(named(f'pc_crate_dispatch_{i}') for i in range(14))
    if struct.unpack('<14I',pe_bytes(args.exe,named('data_ov098_0213c878'),56))!=expected:
        raise ValueError('Crate dispatch does not use fourteen typed native callbacks')
    files=struct.unpack('<3I',pe_bytes(args.exe,named('pc_crate_files'),12))
    if files!=tuple(named(n) for n in ('data_ov098_0213c850','data_ov098_0213c858','data_ov002_0210d934')):
        raise ValueError('Crate resource table contains noncanonical native pointers')
    for address,length in ((0x0213bf60,48),(0x0213bf64,44),(0x0213bf68,40),
                           (0x0213c4e0,16),(0x0213c4f0,16)):
        if pe_bytes(args.exe,named(f'data_ov098_{address:08x}'),length)!=crate_rom[address-crate_base:address-crate_base+length]:
            raise ValueError('Crate attachment/throw constants differ from original data')
    if pe_bytes(args.exe,named('data_ov002_0210d934'),16)!=rom2[0x0210d934-base2:0x0210d944-base2]:
        raise ValueError('Crate CLPS differs from original data')
    print('PASS: original crate descriptor, fourteen typed callbacks, resources, four carry poses and throw constants')
    mushroom_targets=(0x020aff10,0x020afe4c,0x020afd10,0x020afc44,0x020afbb4,
                      0x020afa98,0x020afa6c,0x020af950,0x020af924,0x020af838,
                      0x020af7cc,0x020afa50,0x020af908,0x020af724)
    # The compiler stores the fourteen PMF constants in a different order
    # from the dispatch array initialized from them. Check their identities;
    # the recovered source initializer specifies the dispatch order below.
    original_mushroom=[struct.unpack_from('<II',rom2,0x02108300+i*8-base2) for i in range(14)]
    if sorted(original_mushroom)!=sorted((target,0) for target in mushroom_targets):
        raise ValueError('Original mushroom behavior identities changed')
    native_mushroom=struct.unpack('<14I',pe_bytes(args.exe,named('data_ov002_0210dc00'),56))
    if native_mushroom!=tuple(named('pc_oneup_dispatch_'+str(i)) for i in range(14)):
        raise ValueError('Mushroom behavior dispatch does not use its fourteen typed native targets')
    for address in (0x020ff040,0x020ff050):
        if pe_bytes(args.exe,named(f'data_ov002_{address:08x}'),14)!=rom2[address-base2:address-base2+14]:
            raise ValueError('Mushroom type flags differ from the original fourteen entries')
    print('PASS: 1-Up and logo profiles, fourteen mushroom behavior identities/native adapters and original type flags')

    # The recovered item block uses ARM member-pointer constants. Its host
    # tables use typed native adapters instead. Check both original identities
    # and actual linked pointer storage; no guest code address may survive.
    from item_block_host import BINDINGS
    base102,rom102=overlay(102)
    config102=(ROOT/'config/arm9/overlays/ov102/symbols.txt').read_text()
    functions102={name:int(address,16) for name,address in re.findall(
        r'^(\S+) kind:function\([^)]*\) addr:0x([0-9a-fA-F]+)',config102,re.M)}
    data102={name:int(address,16) for name,address in re.findall(
        r'^(\S+) kind:data\([^)]*\) addr:0x([0-9a-fA-F]+)',config102,re.M)}
    for constant,method in BINDINGS.items():
        original=functions102['_ZN18daObjHatenaBlock_c19'+method+'Ev']
        target,adjustment=struct.unpack_from('<II',rom102,data102[constant]-base102)
        if (target,adjustment)!=(original,0):
            raise ValueError('Original item-block callback changed: '+constant)
        actual=struct.unpack('<I',pe_bytes(args.exe,named(constant),4))[0]
        if actual!=named('pc_item_'+constant):
            raise ValueError('Item-block callback is not its native typed adapter: '+constant)
    print('PASS: item-block profile and all42 original callback identities bind to native typed adapters')

    flower_table=pe_bytes(args.exe,named('data_ov002_021097bc'),36)
    for row in range(3):
        actual=struct.unpack_from('<2I',flower_table,row*12)
        expected=tuple(named('pc_flower_dispatch_'+str(row*2+i)) for i in range(2))
        if actual!=expected:
            raise ValueError('Power Flower state table does not use its six typed native handlers')
    print('PASS: original question-block/Power Flower profiles and six typed flower handlers')

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

