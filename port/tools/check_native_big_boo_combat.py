"""Check the original five-Boo prerequisites, three boss hits and Star spawn.

This checks combat only. Star collection and stair traversal require separate
runtime evidence; spawning a Star does not establish that it was collected.
"""
import csv
import json
import subprocess
import sys
from pathlib import Path

case = Path(sys.argv[1]).resolve()
subprocess.run([sys.executable, str(Path(__file__).with_name('check_native_boo_five_credit.py')), str(case)], check=True, stdout=subprocess.DEVNULL)
prerequisites = json.loads((case/'boo-five-acceptance.json').read_text())
with (case/'boo.csv').open(newline='') as f:
    boss = [r for r in csv.DictReader(f) if r['uid'] == prerequisites['boss_uid'] and r['event'] == 'behavior']
hits = []
for hp in (3, 2, 1, 0):
    row = next(r for r in boss if r['hp'] == str(hp) and r['gate'] == '5')
    hits.append({'health': hp, 'tick': int(row['tick']), 'state': int(row['state'])})
assert all(a['tick'] < b['tick'] for a, b in zip(hits, hits[1:]))
assert any(r['hp'] == '0' and r['state'] == '4' for r in boss)
stars = {}
with (case/'rewards.csv').open(newline='') as f:
    for r in csv.DictReader(f):
        if r['kind'] != '178':
            continue
        item = stars.setdefault(r['id'], {'first_tick': int(r['tick']), 'star_id': int(r['star_id']), 'states': {}})
        item['states'].setdefault(r['state'], int(r['tick']))
reward = [(uid, s) for uid, s in stars.items() if s['first_tick'] > hits[-1]['tick'] and s['star_id'] == 1 and '4' in s['states']]
assert len(reward) == 1, reward
report = dict(status='VERIFIED', scope='Five original Boos, three Big Boo hits, death and original Star spawn', executable_sha256=prerequisites['executable_sha256'], boss_uid=prerequisites['boss_uid'], health_transitions=hits, star_uid=reward[0][0], star=reward[0][1], audio_muted=True, limitations=['Diagnostic mission selection and isolated prepared save.', 'This checker does not accept Star collection or staircase traversal.'])
(case/'big-boo-combat-acceptance.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps(report, indent=2))
