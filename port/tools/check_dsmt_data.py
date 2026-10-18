"""Check original-menu storage against the supplied ROM and actual native PE."""
import argparse,hashlib,json,re,struct
from pathlib import Path
from dsmt_host import START,BSS,END,BASE,SUMMARY_START,SUMMARY_END,SAVE_START,MESSAGE_IDS_START,MESSAGE_IDS_END,owned_names,self_relocations,tail_veneers
from check_texture_source_roles import overlay_from_rom
from check_player_animation_data import pe_bytes
ROOT=Path(__file__).resolve().parents[2]
def audit(exe,map_path):
    symbols={m[1]:int(m[2],16) for m in re.finditer(r'^\s+\d{4}:[\da-f]+\s+(\S+)\s+([\da-f]{8})\s',map_path.read_text(),re.M)}
    rom=(ROOT/'build/assets/user-europe-rev0.nds').read_bytes();base,raw=overlay_from_rom(rom,7)
    assert base==BASE and len(raw)==BSS-BASE
    origin=symbols['_pc_dsmt_rom_data'];summary=symbols['_pc_dsmt_save_summary'];save=symbols['_pc_native_live_save']
    message_ids=symbols['_pc_message_id_table']
    labels=owned_names(ROOT)
    for name,address in labels:
        expected=(origin+address-START if START<=address<END else summary+address-SUMMARY_START if SUMMARY_START<=address<SUMMARY_END else message_ids+address-MESSAGE_IDS_START if MESSAGE_IDS_START<=address<MESSAGE_IDS_END else save+address-SAVE_START)
        assert symbols['_'+name]==expected,(name,'canonical label drift')
        aliases=[(n,a) for n,a in symbols.items() if n.startswith('?'+name+'@@')]
        assert all(a==expected for n,a in aliases),(name,'independent C++ storage remains',aliases)
    # Slot identities are independently read from the original ROM. Eight
    # C-callable callbacks are the only intentional changes to the full image.
    fields={0x02103254:'func_ov007_020c3e4c',0x02103258:'func_ov007_020c3e64',
            0x02103290:'func_ov007_020ccab4',0x02103294:'func_ov007_020cca98',
            0x02103298:'func_ov007_020cca80',0x0210329c:'func_ov007_020cca74',
            0x021032a0:'func_ov007_020cca68',0x021032a4:'func_ov007_020cc600'}
    function_addresses={n:int(a,16) for n,a in re.findall(r'^(\S+) kind:function.*? addr:0x([0-9a-f]+)',(ROOT/'config/arm9/overlays/ov007/symbols.txt').read_text(),re.M)}
    expected=bytearray(raw[START-BASE:]+bytes(END-BSS))
    self_refs=self_relocations(ROOT,raw)
    for address,target in self_refs.items():struct.pack_into('<I',expected,address-START,origin+target-START)
    for address,name in fields.items():
        assert struct.unpack_from('<I',raw,address-BASE)[0]==function_addresses[name]
        struct.pack_into('<I',expected,address-START,symbols['_'+name])
    assert pe_bytes(exe,origin,len(expected))==expected,'Unreviewed original-menu data changed'
    assert pe_bytes(exe,summary,SUMMARY_END-SUMMARY_START)==bytes(SUMMARY_END-SUMMARY_START)
    assert pe_bytes(exe,save,0x400)==bytes(0x400)
    arm=(ROOT/'extracted/arm9_dec.bin').read_bytes()
    original_ids=arm[MESSAGE_IDS_START-0x02004000:MESSAGE_IDS_END-0x02004000]
    native_ids=pe_bytes(exe,message_ids,len(original_ids))
    assert native_ids==original_ids,'Message ID table truncated or changed'
    pairs=list(struct.iter_unpack('<HH',native_ids))
    assert pairs[0]==(0,0) and pairs[-1]==(8000,630)
    assert symbols['_data_0208eeee']==message_ids+2
    # Dialog IDs from the original menu must map within the final message range.
    ids=struct.unpack_from('<13H',raw,0x020d7a68-BASE)
    for value in ids[1:]:
        base_id,base_index=max((p for p in pairs if p[0]<=value),key=lambda p:p[0])
        assert base_index+value-base_id==630+value-8000

    veneers=tail_veneers(ROOT)
    for name,to in veneers.items():
        if '_'+name not in symbols:continue # Unreferenced code may be removed.
        address=symbols['_'+name];code=pe_bytes(exe,address,5)
        assert code[0]==0xe9,(name,'veneer has a call frame instead of a transparent jump')
        assert address+5+struct.unpack_from('<i',code,1)[0]==symbols['_'+to],(name,'wrong native jump target')
    # The generic file-local helper must not be coalesced with another TU's
    # incompatible inline_fn (the observed native link selected daDemo's).
    assert '_inline_fn' not in symbols,'Conflicting external C inline helpers remain linked'
    # Verify the original allocator's load and all three install targets.
    assert struct.unpack_from('<I',raw,0x020c3e18-BASE)[0]==0x02103258
    assert struct.unpack_from('<I',raw,0x020c3e28-BASE)[0]==0x02104c14
    assert struct.unpack_from('<I',raw,0x020c3e38-BASE)[0]==0x02103254
    assert struct.unpack_from('<I',raw,0x020c3e48-BASE)[0]==0x02103258
    return {'CanonicalLabels':len(labels),'OriginalDataBytesChecked':len(expected),'ReviewedCallbacks':8,'SourceDeclaredSelfRelocations':len(self_refs),'ROMIdentifiedTailVeneers':len(veneers),'ExternalInlineHelperConflictAbsent':True,
        'ROM_SHA256':hashlib.sha256(rom).hexdigest().upper(),'SaveMinigamesOffset':hex(symbols['_data_0209cae4']-save),
        'Limits':['Unreviewed embedded pointer fields remain original; full UI behavior is not established by this byte audit']}
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,default=ROOT/'build/port/sm64ds_pc.exe');p.add_argument('--map',type=Path,default=ROOT/'build/port/sm64ds_pc.map');args=p.parse_args()
    report=audit(args.exe,args.map);print('PASS: actual user-ROM DSMT extraction, canonical interior labels/live save, eight native callbacks, all other original data bytes');print(json.dumps(report,indent=2))
