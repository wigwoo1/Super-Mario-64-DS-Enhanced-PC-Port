"""Verify native rabbit capture, castle-key reward and persisted QA progress."""
import argparse
import csv
import json
from pathlib import Path
import struct
import zlib


def rows(path):
    with path.open() as file:return list(csv.DictReader(file))


def verify(directory,reload_directory=None):
    assert int((directory/'exit-code.txt').read_text())==0,'capture replay did not exit cleanly'
    rabbit=rows(directory/'rabbit.csv');state=rows(directory/'state.csv')
    assert any(r['rabbit_id']=='7' and r['disabled']=='1' and not int(r['flags2'],16)&0x20000 for r in rabbit),'opening rabbit gate missing'
    caught=[r for r in rabbit if r['eaten']=='1' and int(r['state'])==0x021306ac]
    assert caught,'Yoshi did not catch the rabbit'
    sources={r['unique'] for r in caught}
    key=[r for r in rabbit if r['key_alive']=='1' and r['key_source'] in sources]
    assert key and int(key[0]['tick'])>int(caught[0]['tick']),'captured rabbit did not spawn its native key'
    reward=[r for r in key if int(r['flags1'],16)&0x40]
    assert reward,'native key did not award the castle key'
    assert any(r['player_state']=='St_YoshiPower_Main' for r in state),'no normal tongue action observed'
    final=state[-1]
    assert final['level']=='1' and final['player_character']=='3','final scene is not Yoshi outside the castle'
    assert final['player_disabled']==final['player_no_control']=='0','player remained locked after key reward'
    assert rabbit[-1]['key_alive']=='0' and int(rabbit[-1]['flags1'],16)&0x40,'key did not retire after collection'
    data=bytearray((directory/'saves/slot1.sav').read_bytes())
    assert data[:8]==b'SM64PC01' and len(data)==92,'QA save format invalid'
    crc=struct.unpack_from('<I',data,16)[0];struct.pack_into('<I',data,16,0)
    assert zlib.crc32(data)==crc,'QA save checksum invalid'
    assert struct.unpack_from('<I',data,28)[0]&0x40,'castle key missing from persisted save'
    for frame in (7000,7400):assert (directory/f'gate-{frame}.bmp').is_file(),f'missing screenshot {frame}'
    result={'status':'PASS','caught_tick':int(caught[0]['tick']),'key_spawn_tick':int(key[0]['tick']),
            'key_award_tick':int(reward[0]['tick']),'normal_control':True,'save_checksum_valid':True,'castle_key_saved':True,'final_tick':int(final['tick'])}
    if reload_directory:
        assert int((reload_directory/'exit-code.txt').read_text())==0,'reload replay did not exit cleanly'
        log=(reload_directory/'run.log').read_text()
        assert '[native save] slot=0 loaded=primary' in log,'native save reader did not load File A'
        assert any(int(r['flags1'],16)&0x40 for r in rows(reload_directory/'rabbit.csv')),'key flag absent after reload'
        assert any(r['level']=='2' for r in rows(reload_directory/'state.csv')),'reloaded key did not allow original castle entry'
        result['reloaded_key_and_entered_castle']=True
    return result


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    parser.add_argument('--reload',type=Path)
    args=parser.parse_args()
    result=verify(args.directory,args.reload)
    output=json.dumps(result,indent=2)
    (args.directory/'verification.json').write_text(output+'\n')
    print(output)
