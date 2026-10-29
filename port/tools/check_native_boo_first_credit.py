"""Check one genuine first-mission Boo defeat and the main-hall boss credit."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import re


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('case',type=Path)
    args=parser.parse_args()
    case=args.case.resolve()
    config=json.loads((case/'test-config.json').read_text(encoding='utf-8-sig'))
    log=(case/'run.log').read_text()
    assert (case/'exit-code.txt').read_text(encoding='utf-8-sig').strip()=='0'
    assert config['boo_trace'] and config['output_muted']
    assert 'confirmed mission=1 level=12' in log
    assert '[native audio] submission output_muted=1 nonzero_samples=0' in log
    assert re.search(r'\[native audio\] shutdown .* errors=0 ',log)
    assert not re.search(r'unhandled exception|FATAL:',log,re.I)
    rows=list(csv.DictReader((case/'boo.csv').open()))
    ready=[r for r in rows if r['event']=='ready']
    scene_tick=min(int(r['tick']) for r in ready)
    ready=[r for r in ready if int(r['tick'])==scene_tick]
    first_mission_ghosts=[r for r in ready if r['actor']=='209' and r['variant']=='2']
    assert len(first_mission_ghosts)==5
    assert all(int(r['model']) and r['hp']=='1' for r in first_mission_ghosts)
    boss=next(r for r in ready if r['actor']=='210' and r['variant']=='14')
    assert boss['hp']=='3' and int(boss['model'])
    target=next(r for r in first_mission_ghosts if r['x']==str(2750*4096) and r['z']==str(2150*4096))
    target_rows=[r for r in rows if r['uid']==target['uid']]
    assert any(r['state']=='1' and r['boss']==boss['uid'] for r in target_rows)
    assert any(r['state']=='5' and r['children']=='2' for r in target_rows)
    boss_rows=[r for r in rows if r['uid']==boss['uid'] and r['event']=='behavior']
    assert any(r['gate']=='0' for r in boss_rows)
    assert boss_rows[-1]['gate']=='1' and boss_rows[-1]['hp']=='3' and boss_rows[-1]['state']=='0'
    assert boss_rows[-1]['visible']=='0'
    render=list(csv.DictReader((case/'actor-render.csv').open()))
    drawn=[r for r in render if r['actor']=='209' and int(r['triangles'])>0]
    assert drawn
    states=list(csv.DictReader((case/'state.csv').open()))
    last=states[-1]
    assert last['level']=='12' and last['player_character']=='0'
    assert last['player_disabled']=='0' and last['player_no_control']=='0'
    assert last['player_state']=='St_Wait_Main'
    rewards=list(csv.DictReader((case/'rewards.csv').open()))
    initial_tick=min(int(r['tick']) for r in rewards)
    first_seen={}
    for r in rewards:
        first_seen.setdefault(r['id'],int(r['tick']))
    dropped={r['id'] for r in rewards if r['kind']=='290' and first_seen[r['id']]>initial_tick}
    assert len(dropped)==1
    payout=[r for r in rewards if r['id'] in dropped]
    assert any(r['alive']=='1' and r['course_coins']=='0' for r in payout)
    assert any(r['alive']=='2' and r['course_coins']=='5' for r in payout)
    seed=Path(config['save_seed'])
    if not seed.is_absolute():
        seed=Path.cwd()/seed
    saves={}
    for original in seed.glob('*.sav'):
        actual=case/'saves'/original.name
        assert original.read_bytes()==actual.read_bytes(),original.name
        saves[original.name]=hashlib.sha256(actual.read_bytes()).hexdigest()
    assert saves
    stairs=list(csv.DictReader((case/'merry.csv').open()))
    hidden={r['step'] for r in stairs if r['actor']=='65' and r['event']=='behavior'
            and r['visible']=='0' and r['trigger']=='0'}
    assert hidden=={'0','1','2'}
    report=dict(status='VERIFIED',scope='Authored Boo resources/render/chase, one attack defeat, five-coin reward, one Big Boo prerequisite credit and hidden mission1 staircase',
                exe_sha256=hashlib.sha256((case/'sm64ds_pc.exe').read_bytes()).hexdigest(),
                boss_uid=boss['uid'],boo_uid=target['uid'],boss_gate=1,boss_hp=3,
                coin_reward=5,final_coins=int(rewards[-1]['course_coins']),render_samples=len(drawn),saves_unchanged=saves,muted=True,
                limitations=['Diagnostic entry and prepared save','Four prerequisite Boos remain','Active Big Boo fight, Star and staircase rise unverified','Other variants and course lifecycle unverified'])
    (case/'boo-first-credit-acceptance.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':
    main()
