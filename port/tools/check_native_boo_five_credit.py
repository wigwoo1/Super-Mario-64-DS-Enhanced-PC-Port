"""Accept five original Boos in a single uninterrupted, muted course visit."""
import csv, hashlib, json, re, sys
from pathlib import Path

case = Path(sys.argv[1]).resolve()
def rows(name):
    with (case/name).open(newline='') as f:
        return list(csv.DictReader(f))
cfg = json.loads((case/'test-config.json').read_text(encoding='utf-8-sig'))
log = (case/'run.log').read_text()
assert (case/'exit-code.txt').read_text(encoding='utf-8-sig').strip() == '0'
assert cfg['output_muted'] and cfg['expected_mission'] == 1
assert 'confirmed mission=1 level=12' in log
assert '[native audio] submission output_muted=1 nonzero_samples=0' in log
assert re.search(r'\[native audio\] shutdown .* errors=0 ', log)
assert not re.search(r'unhandled exception|FATAL:', log, re.I)
b = rows('boo.csv')
ready = [r for r in b if r['event'] == 'ready']
start = min(int(r['tick']) for r in ready)
initial = [r for r in ready if int(r['tick']) == start]
ghosts = [r for r in initial if r['actor'] == '209' and r['variant'] == '2']
assert len(ghosts) == 5
boss = next(r for r in initial if r['actor'] == '210' and r['variant'] == '14')
assert len({r['uid'] for r in ready if r['actor'] == '210' and r['variant'] == '14'}) == 1
states = [r for r in rows('state.csv') if int(r['tick']) >= start]
assert states and all(r['level'] == '12' for r in states)
assert not any('St_Dead' in r['player_state'] for r in states)
coins = {}
initial_reward_tick = None
with (case/'rewards.csv').open(newline='') as reward_file:
    for r in csv.DictReader(reward_file):
        if initial_reward_tick is None: initial_reward_tick = int(r['tick'])
        if r['kind'] != '290': continue
        item = coins.setdefault(r['id'], {'spawn': r})
        if r['alive'] == '1': item.setdefault('before', r)
        if r['alive'] == '2': item.setdefault('pickup', r)
dropped = {k:v for k,v in coins.items() if int(v['spawn']['tick']) > initial_reward_tick}
assert len(dropped) == 5, f'Expected 5 newly dropped blue coins, got {len(dropped)}'
defeats = []
for target in ghosts:
    h = [r for r in b if r['uid'] == target['uid']]
    assert any(r['state'] == '1' and r['boss'] == boss['uid'] for r in h)
    hit = next(r for r in h if r['state'] == '3')
    death = next(r for r in h if r['state'] == '5' and r['children'] == '2')
    assert int(hit['tick']) < int(death['tick'])
    # Actor trace samples every ten ticks. The first hit sample can already
    # contain knockback; the coin spawns at contact, near the preceding sample.
    contact_samples = [r for r in h if int(hit['tick'])-10 <= int(r['tick']) <= int(hit['tick'])]
    matches = [(uid,item) for uid,item in dropped.items()
               if abs(int(item['spawn']['tick'])-int(hit['tick'])) <= 20
               and any(sum((int(item['spawn'][k])-int(r[k]))**2 for k in ('x','y','z')) < (100*4096)**2 for r in contact_samples)]
    assert len(matches) == 1
    uid,item = matches[0]
    assert 'pickup' in item and 'before' in item
    assert int(item['pickup']['course_coins'])-int(item['before']['course_coins']) == 5
    defeats.append(dict(boo_uid=target['uid'],hit_tick=int(hit['tick']),death_tick=int(death['tick']),coin_uid=uid,pickup_tick=int(item['pickup']['tick'])))
assert len({d['coin_uid'] for d in defeats}) == 5
bh = [r for r in b if r['uid'] == boss['uid'] and r['event'] == 'behavior']
gates = sorted({int(r['gate']) for r in bh})
assert 5 in gates and max(gates) == 5, gates
assert any(int(r['gate']) == 5 and int(r['tick']) > max(d['death_tick'] for d in defeats) for r in bh)
activation = next(r for r in bh if r['gate'] == '5' and r['state'] == '1' and r['visible'] == '1' and r['hp'] == '3')
last = states[-1]
assert last['player_airborne'] == '0' and last['player_disabled'] == '0' and last['player_no_control'] == '0'
assert last['player_state'] == 'St_Wait_Main'
save_hashes = {}
for original in Path(cfg['save_seed']).glob('*.sav'):
    actual = case/'saves'/original.name
    assert actual.read_bytes() == original.read_bytes()
    save_hashes[original.name] = hashlib.sha256(actual.read_bytes()).hexdigest()
assert save_hashes
sha = hashlib.sha256((case/'sm64ds_pc.exe').read_bytes()).hexdigest()
assert sha.upper() == cfg['exe_sha256'].upper()
report = dict(status='VERIFIED',scope='Five original Boos defeated and rewards collected in one visit; same Big Boo counter reaches five and activates',executable_sha256=sha,defeats=sorted(defeats,key=lambda d:d['hit_tick']),boss_uid=boss['uid'],observed_gates=gates,boss_activation_tick=int(activation['tick']),final_tick=int(last['tick']),final_player_state=last['player_state'],saves_unchanged=save_hashes,audio_muted=True,limitations=['Prepared isolated QA save and diagnostic mission selector. Big Boo fight, Star and staircase are not accepted by this check.','Replay stops after activation; leaving Mario idle longer allowed the active boss to defeat him in the exploratory run.'])
(case/'boo-five-acceptance.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
