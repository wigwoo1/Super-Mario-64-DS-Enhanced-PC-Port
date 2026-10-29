"""Check original rear-door entry and both area6 Boo defeats in one QA replay."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import re


def rows(path):
    with path.open(newline='') as stream:
        return list(csv.DictReader(stream))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('case', type=Path)
    case = parser.parse_args().case.resolve()
    config = json.loads((case/'test-config.json').read_text(encoding='utf-8-sig'))
    log = (case/'run.log').read_text()
    assert (case/'exit-code.txt').read_text(encoding='utf-8-sig').strip() == '0'
    assert re.search(r'confirmed mission=1 level=12(?:\r?\n|$)', log)
    assert config['boo_trace'] and config['output_muted']
    assert '[native audio] submission output_muted=1 nonzero_samples=0' in log
    assert re.search(r'\[native audio\] shutdown .* errors=0 ', log)
    assert not re.search(r'unhandled exception|FATAL:', log, re.I)
    for target in ('-1525,0,-1750', '-1525,0,-1250', '-1520,0,-1200'):
        assert re.search(r'reached waypoint=\d+ level=12 target='+target+r' tick=\d+', log), target
    history = rows(case/'boo.csv')
    ready = [r for r in history if r['event']=='ready']
    first = min(int(r['tick']) for r in ready)
    initial = [r for r in ready if int(r['tick'])==first]
    ghosts = [r for r in initial if r['actor']=='209' and r['variant']=='2']
    assert len(ghosts)==5
    boss = next(r for r in initial if r['actor']=='210' and r['variant']=='14')
    targets = []
    for x,z,param in ((-1800,-800,0x2f02),(-1520,-1200,0x0102)):
        ghost = next(r for r in ghosts if int(r['x'])==x*4096 and int(r['z'])==z*4096)
        assert ghost['area']=='6' and int(ghost['param'])==param and int(ghost['model'])
        samples = [r for r in history if r['uid']==ghost['uid']]
        assert any(r['state']=='1' and r['boss']==boss['uid'] for r in samples)
        hit = next(r for r in samples if r['state']=='3')
        death = next(r for r in samples if r['state']=='5' and r['children']=='2')
        assert int(death['tick'])>int(hit['tick'])
        targets.append((ghost,hit,death))
    coins = {}
    first_reward = None
    last_reward = None
    with (case/'rewards.csv').open(newline='') as stream:
        for r in csv.DictReader(stream):
            if first_reward is None:
                first_reward = int(r['tick'])
            last_reward = r
            if r['kind']!='290':
                continue
            item = coins.setdefault(r['id'], {'spawn':r})
            if r['alive']=='1' and 'pickup' not in item:
                item['before'] = r
            elif r['alive']=='2':
                item.setdefault('pickup',r)
    dropped = {uid:item for uid,item in coins.items() if int(item['spawn']['tick'])>first_reward}
    assert len(dropped)==2
    defeats=[]
    used=set()
    for ghost,hit,death in targets:
        matches=[]
        for uid,item in dropped.items():
            spawn=item['spawn']
            distance=sum((int(spawn[k])-int(hit[k]))**2 for k in ('x','y','z'))
            if abs(int(spawn['tick'])-int(hit['tick']))<=20 and distance<(100*4096)**2:
                matches.append((uid,item))
        assert len(matches)==1
        uid,item=matches[0]
        assert uid not in used and 'pickup' in item
        used.add(uid)
        assert int(item['pickup']['course_coins'])-int(item['before']['course_coins'])==5
        defeats.append(dict(boo_uid=ghost['uid'], hit_tick=int(hit['tick']),
                            death_tick=int(death['tick']), coin_uid=uid,
                            pickup_tick=int(item['pickup']['tick']), coins=5))
    assert int(last_reward['course_coins'])==10
    final=rows(case/'state.csv')[-1]
    assert final['level']=='12' and final['player_character']=='0'
    assert final['player_state']=='St_Wait_Main' and final['player_airborne']=='0'
    assert final['player_disabled']=='0' and final['player_no_control']=='0'
    assert int(final['tick'])==config['frames']
    saves={}
    for original in Path(config['save_seed']).glob('*.sav'):
        actual=case/'saves'/original.name
        assert actual.read_bytes()==original.read_bytes(), original.name
        saves[original.name]=hashlib.sha256(actual.read_bytes()).hexdigest()
    assert saves
    digest=hashlib.sha256((case/'sm64ds_pc.exe').read_bytes()).hexdigest()
    assert digest==config['exe_sha256'].lower()
    report=dict(status='VERIFIED', scope='Original rear-door entry, two area6 Boo defeats and two collected five-coin rewards',
                exe_sha256=digest, boss_uid=boss['uid'], defeats=defeats,
                final_course_coins=10, final_state=final['player_state'],
                output_muted=True, audio_errors=0, saves_unchanged=saves,
                limitations=['Prepared QA save and diagnostic original Mission1 selection',
                             'The earlier three-Boo replay is a separate course lifetime',
                             'One continuous five-clear run, Big Boo fight/Star/stairs and lifecycle remain unverified'])
    (case/'boo-rear-entry-acceptance.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':
    main()
