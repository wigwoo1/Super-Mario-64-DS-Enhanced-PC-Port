"""Compare isolated native dialogue replays with replacement mode On and Off."""
import argparse
import csv
import hashlib
import json
import re
import struct
import zlib
from pathlib import Path


def rows(case,name):
    return list(csv.DictReader((case/name).open()))


def save(case,name='slot1.sav'):
    raw=bytearray((case/'saves'/name).read_bytes())
    assert raw[:8]==b'SM64PC01' and len(raw)>=24
    slot,size,crc,reserved=struct.unpack_from('<4I',raw,8)
    assert reserved==0 and len(raw)==size+24
    struct.pack_into('<I',raw,16,0)
    assert zlib.crc32(raw)==crc
    assert (slot,size)==((0,68) if name=='slot1.sav' else (3,740))
    return bytes(raw[24:])


def common(on,off,expect_save=True):
    digests=[]
    for case in (on,off):
        assert (case/'exit-code.txt').read_text(encoding='utf-8-sig').strip()=='0',case
        config=json.loads((case/'test-config.json').read_text(encoding='utf-8-sig'))
        assert config['muted'] is True
        digest=hashlib.sha256((case/'sm64ds_pc.exe').read_bytes()).hexdigest()
        assert digest==config['exe_sha256'].lower()
        digests.append(digest)
        assert 'unhandled exception' not in (case/'run.log').read_text()
    assert digests[0]==digests[1]
    if expect_save:
        assert save(on)==save(off),'saved progress differs between text modes'
        assert save(on,'minigames.sav')==save(off,'minigames.sav')
    else:
        for case in (on,off):
            assert not (case/'saves/slot1.sav').exists(),'declining save created adventure progress'
            assert not (case/'saves/minigames.sav').exists(),'declining save wrote minigame progress'
    return digests[0]


def sequence(case,ids):
    return [tuple(r[k] for k in ('message','state','line','selection','answer','choice_mode'))
            for r in rows(case,'dialogue.csv') if int(r['message']) in ids]


def check_buddy(on,off):
    digest=common(on,off)
    assert sequence(on,{143,144})==sequence(off,{143,144}),'page/control sequence differs'
    for case in (on,off):
        log=(case/'run.log').read_text()
        assert 'confirmed mission=2 level=6' in log
        events=re.findall(r'\[native Buddy repeat\] visit=1 interaction=(\d+) complete tick=(\d+)',log)
        assert {n for n,_ in events}=={'1','2'}
        assert 'original save dialog tick=' in log and 'saved; awaiting fresh-process reload' in log
        assert '[native save] slot=0 write=committed bytes=68' in log
        dialogue=rows(case,'dialogue.csv')
        assert sum(r['message']=='143' and r['state']=='7' for r in dialogue)==1
        assert sum(r['message']=='144' and r['state']=='7' for r in dialogue)==6
        actors=rows(case,'buddy.csv')
        buddies=[r for r in actors if r['level']=='6' and r['kind']=='181' and r['variant']=='1']
        assert buddies[0]['opened']=='0' and buddies[-1]['opened']=='1'
        assert {0,1,2,3,4}.issubset({int(r['step']) for r in buddies})
        assert buddies[-1]['state']=='0' and int(buddies[-1]['cannon_flags'])&1
        shutters=[r for r in actors if r['level']=='6' and r['kind']=='14' and r['id']==next(b['shutter'] for b in buddies if b['shutter']!='0')]
        assert shutters[0]['opened']=='0' and shutters[-1]['opened']=='1'
        assert int(shutters[-1]['y'])<int(shutters[0]['y'])
        assert abs(int(shutters[-1]['z'])-int(shutters[0]['z']))>200*4096
        states=rows(case,'state.csv');end=int(events[-1][1])
        assert any(int(r['tick'])>=end and r['level']=='6' and r['player_no_control']==r['player_disabled']==r['player_airborne']=='0' for r in states)
        assert states[-1]['player_state']=='none','Save & Quit did not finish'
        assert struct.unpack_from('<I',save(case),0x10)[0]&1
    on_log=(on/'run.log').read_text();off_log=(off/'run.log').read_text()
    assert 'id=143 replacement=1' in on_log and on_log.count('id=144 replacement=1')==2
    assert '[dialogue mod]' not in off_log
    print(f'PASS Buddy: original entry, actual On/Off selection, identical multi-page/control sequence, cannon opening, repeat talk, control return, Save & Quit and identical valid saves; EXE {digest}')


def check_rabbit(on,off,answer):
    digest=common(on,off,answer==1)
    assert sequence(on,{396})==sequence(off,{396}),'choice/page sequence differs between modes'
    for case in (on,off):
        log=(case/'run.log').read_text()
        assert 'reached waypoint=3 level=1 target=-75,2500' in log,'route did not finish before rabbit handoff'
        animals=rows(case,'rabbit.csv');states=rows(case,'state.csv')
        caught=[r for r in animals if r['eaten']=='1' and int(r['state'])==0x021306ac]
        assert caught,'native rabbit was not caught'
        ids={r['unique'] for r in caught}
        keys=[r for r in animals if r['key_alive']=='1' and r['key_source'] in ids]
        assert keys and int(keys[0]['tick'])>int(caught[0]['tick'])
        assert any(int(r['flags1'],16)&0x40 for r in keys),'native castle key was not awarded'
        assert int(animals[-1]['flags1'],16)&0x40 and animals[-1]['key_alive']=='0'
        final=states[-1]
        assert final['level']=='1' and final['player_character']=='3'
        assert final['player_disabled']==final['player_no_control']=='0','control did not return'
        dialogue=rows(case,'dialogue.csv')
        choices=[r for r in dialogue if r['message']=='396' and r['state']=='5' and r['choice_mode']=='3']
        assert choices and int(choices[-1]['selection'])==answer-1,'wrong choice selected'
        assert sum(r['message']=='396' and r['state']=='7' for r in dialogue)==2,'choice prompt page progression incomplete'
        if answer==1:
            assert '[native save] slot=0 write=committed bytes=68' in log
            assert any(r['message']=='661' for r in dialogue),'Save branch did not reach original saving notice'
            assert struct.unpack_from('<I',save(case),4)[0]&0x40,'key not saved'
        else:
            assert 'write=committed' not in log,'declined save still committed'
            assert not any(r['message']=='661' for r in dialogue),'declined save entered saving notice'
    assert 'id=396 replacement=1' in (on/'run.log').read_text()
    assert '[dialogue mod]' not in (off/'run.log').read_text()
    print(f'PASS Rabbit: actual catch/key, identical On/Off pages and choice {answer}, correct save/no-save outcome, key retirement and normal control; EXE {digest}')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--kind',choices=['buddy','rabbit'],default='buddy')
    p.add_argument('--answer',type=int,choices=[1,2],default=1)
    p.add_argument('on',type=Path);p.add_argument('off',type=Path)
    a=p.parse_args()
    if a.kind=='buddy':check_buddy(a.on,a.off)
    else:check_rabbit(a.on,a.off,a.answer)


if __name__=='__main__':main()
