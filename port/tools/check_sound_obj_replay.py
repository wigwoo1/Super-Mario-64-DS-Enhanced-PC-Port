"""Verify native SOUND_OBJ creation, callback execution and cleanup during Star pickup."""
import argparse
import csv
import json
import pathlib
import re
import struct
from check_power_star_repeat_replay import verify as verify_repeat
from check_power_star_replay import save_payload

ROOT=pathlib.Path(__file__).resolve().parents[2]


def verify_lifecycle(log, row):
    assert '[native sound_obj] callback-table initialized host-slot-size=4' in log,'host callback table was not initialized'
    init=re.findall(r'\[native sound_obj\] init-ok ptr=(\S+) actor=359 unique=(\d+) param=4 sound=(-?\d+) volume=(-?\d+) limit=(\d+) loop=(\d+)',log)
    assert len(init)==1,'expected exactly one Star sound actor'
    ptr,unique,*fields=init[0]
    assert tuple(map(int,fields))==row,'native actor read invalid or truncated sound table fields'
    events=(
        rf'spawn-request actor=359 param=4 parent=\S+ area=-?\d+',
        rf'factory ptr={ptr} actor=359 unique={unique} alive=0 param=4',
        rf'init-begin ptr={ptr} actor=359 unique={unique} alive=0 param=4',
        rf'init-ok ptr={ptr} actor=359 unique={unique} param=4 ',
        rf'spawn-result ptr={ptr} actor=359 unique={unique} alive=\d+ param=4',
        rf'behavior ptr={ptr} param=4 sound=34 counter=\d+ limit=\d+ done=\d+ current=-1',
        rf'mark-destroy ptr={ptr} param=4 ',
        rf'cleanup-begin ptr={ptr} unique={unique} alive=2 param=4',
        rf'cleanup-end ptr={ptr} unique={unique} alive=2 param=4 success=2',
    )
    last=0
    for event in events:
        match=re.search(r'\[native sound_obj\] '+event,log[last:])
        assert match,'missing or out-of-order sound lifecycle event: '+event
        last+=match.end()
    assert log.count(f'cleanup-end ptr={ptr} unique={unique} ')==1,'sound actor cleaned up more than once'
    return int(unique)


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('case',type=pathlib.Path)
    ap.add_argument('seed',type=pathlib.Path)
    args=ap.parse_args()
    case=args.case
    log=(case/'run.log').read_text(encoding='utf-8-sig')
    image=(ROOT/'extracted/overlays/overlay_0002.bin').read_bytes()
    sound,volume,limit,loop,_=struct.unpack_from('<iiHBB',image,0x0210c080-0x020ad660+4*12)
    actor=verify_lifecycle(log,(sound,volume,limit,loop))
    rows=list(csv.DictReader((case/'rewards.csv').open()))
    states=list(csv.DictReader((case/'state.csv').open()))
    result=verify_repeat(rows,states,int((case/'exit-code.txt').read_text(encoding='utf-8-sig')),
        save_payload(args.seed/'slot1.sav'),save_payload(case/'saves/slot1.sav'),log)
    result['sound_actor']=actor
    result['audible_playback_verified']=False
    print('PASS:',json.dumps(result,sort_keys=True))


if __name__=='__main__':main()
