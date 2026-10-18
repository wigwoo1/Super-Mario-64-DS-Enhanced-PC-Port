"""Verify Continue preserves live credit without writing it to disk."""
import csv,json,sys
from pathlib import Path
from check_power_star_replay import save_payload

def verify(rows,states,code,seed_raw,saved_raw,log,restarted,restart_code,restart_log):
    assert code==restart_code==0, 'game or restart failed'
    seed=save_payload_bytes(seed_raw)
    assert seed_raw==saved_raw, 'Continue wrote the progress save'
    assert seed[0x14]==0, 'seed already has the fixture star'
    total=sum(x.bit_count() for x in seed[0x14:0x32])
    star=[r for r in rows if r['kind']=='178' and r['param']=='1']
    assert len({r['id'] for r in star})==1,'missing unique fixture'
    pickup=min(int(r['tick']) for r in star if r['state']=='5')
    assert any(int(r['tick'])<pickup and int(r['hit'])&0x400000 and r['other']=='3' for r in star),'no real contact'
    assert {'5','6','7'} <= {r['state'] for r in star}, 'incomplete celebration'
    for r in star:
        taken=int(r['tick'])>=pickup
        assert int(r['save_star_mask'])==(2 if taken else 0),'wrong course credit'
        assert int(r['power_star_total'])==total+int(taken),'wrong live credit'
    post=[s for s in states if int(s['tick'])>pickup]
    assert post and all(int(s['power_star_total'])==total+1 for s in post), 'live credit lost after pickup'
    final=states[-1]
    assert final['level']=='1' and final['player_state']=='St_Wait_Main' and final['player_disabled']==final['player_no_control']=='0', 'no normal castle control'
    assert any(s['level']=='1' and s['player_state']=='St_Jump_Main' and int(s['tick'])>2700 for s in states), 'no later jump'
    assert 'write=committed' not in log and 'Save failed' not in log,'unexpected save attempt'
    assert restarted and all(int(r['power_star_total'])==total and r['save_star_mask']=='0' for r in restarted), 'unsaved credit survived restart'
    assert not any(r['kind']=='178' and r['param']=='1' for r in restarted), 'restart unexpectedly used fixture'
    assert 'slot=0 loaded=primary bytes=68' in restart_log, 'restart did not load primary save'
    return dict(pickup=pickup,final_frame=int(final['tick']),live_total=total+1,disk_total=total,restarted_total=total,save_bytes_unchanged=True)

def save_payload_bytes(raw):
    import struct,zlib
    data=bytearray(raw)
    assert len(data)==92 and data[:8]==b'SM64PC01','invalid native save'
    slot,size,checksum,reserved=struct.unpack_from('<4I',data,8)
    assert (slot,size,reserved)==(0,68,0)
    struct.pack_into('<I',data,16,0)
    assert zlib.crc32(data)==checksum,'bad save checksum'
    return bytes(data[24:])

def main():
    build=Path('build');p=build/'port/native-file-menu-baseline-power-star-continue';re=build/'port/native-file-menu-baseline-power-star-continue-restart'
    load=lambda path:list(csv.DictReader(path.open()))
    result=verify(load(Path(str(p)+'-rewards.csv')),load(Path(str(p)+'-state.csv')),int((build/(p.name+'.exit-code.txt')).read_text(encoding='utf-8-sig')),(build/'reward-unlocked-fixture-saves/slot1.sav').read_bytes(),(build/'power-star-continue-saves/slot1.sav').read_bytes(),(build/(p.name+'.log')).read_text(),load(Path(str(re)+'-rewards.csv')),int((build/(re.name+'.exit-code.txt')).read_text(encoding='utf-8-sig')),(build/(re.name+'.log')).read_text())
    print(json.dumps(result,indent=2))
if __name__=='__main__':main()
