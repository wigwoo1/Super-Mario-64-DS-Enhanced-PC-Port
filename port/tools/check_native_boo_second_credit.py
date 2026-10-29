"""Check two distinct first-mission Boo defeats and the main-hall boss counter."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import re


def read_csv(path):
    with path.open(newline='') as stream:
        return list(csv.DictReader(stream))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('case', type=Path)
    case = parser.parse_args().case.resolve()
    config = json.loads((case / 'test-config.json').read_text(encoding='utf-8-sig'))
    log = (case / 'run.log').read_text()
    assert (case / 'exit-code.txt').read_text(encoding='utf-8-sig').strip() == '0', 'Runtime did not exit cleanly'
    assert re.search(r'confirmed mission=1 level=12(?:\r?\n|$)', log), 'Original Mission 1 was not confirmed'
    assert config['boo_trace'] and config['output_muted']
    assert '[native audio] submission output_muted=1 nonzero_samples=0' in log
    assert re.search(r'\[native audio\] shutdown .* errors=0 ', log)
    assert not re.search(r'unhandled exception|FATAL:', log, re.I)
    rows = read_csv(case / 'boo.csv')
    ready = [row for row in rows if row['event'] == 'ready']
    scene_tick = min(int(row['tick']) for row in ready)
    initial = [row for row in ready if int(row['tick']) == scene_tick]
    ghosts = [row for row in initial if row['actor'] == '209' and row['variant'] == '2']
    assert len(ghosts) == 5 and all(row['hp'] == '1' and int(row['model']) for row in ghosts)
    boss = next(row for row in initial if row['actor'] == '210' and row['variant'] == '14')
    assert boss['hp'] == '3' and int(boss['model'])
    targets = []
    for x, z in ((2750, 2150), (-100, -500)):
        target = next(row for row in ghosts if (int(row['x']), int(row['z'])) == (x * 4096, z * 4096))
        history = [row for row in rows if row['uid'] == target['uid']]
        assert any(row['state'] == '1' and row['boss'] == boss['uid'] for row in history), 'Boo did not chase/link to the main-hall boss'
        hit = next(row for row in history if row['state'] == '3')
        death = next(row for row in history if row['state'] == '5' and row['children'] == '2')
        assert int(death['tick']) > int(hit['tick'])
        targets.append((target, hit, death))
    assert targets[0][0]['uid'] != targets[1][0]['uid']
    assert int(targets[0][2]['tick']) < int(targets[1][1]['tick'])
    boss_history = [row for row in rows if row['uid'] == boss['uid'] and row['event'] == 'behavior']
    gates = [int(row['gate']) for row in boss_history]
    assert {0, 1, 2}.issubset(gates) and max(gates) == 2, 'Boss did not progress through exactly 0, 1, 2 credits'
    assert boss_history[-1]['gate'] == '2' and boss_history[-1]['hp'] == '3'
    assert boss_history[-1]['state'] == '0' and boss_history[-1]['visible'] == '0'
    assert int(boss_history[-1]['tick']) > int(targets[1][2]['tick']), 'Boss credit was not observed after the second death'
    coins = {}
    final_reward = None
    initial_reward_tick = None
    with (case / 'rewards.csv').open(newline='') as stream:
        for row in csv.DictReader(stream):
            if initial_reward_tick is None:
                initial_reward_tick = int(row['tick'])
            final_reward = row
            if row['kind'] != '290':
                continue
            item = coins.setdefault(row['id'], {'spawn': row})
            if row['alive'] == '1' and 'pickup' not in item:
                item['before'] = row
            elif row['alive'] == '2':
                item.setdefault('pickup', row)
    # Rewards are sampled on the next render tick after actor initialization.
    # Compare within this stream so original stage coins are not counted as drops.
    dropped = {uid: item for uid, item in coins.items() if int(item['spawn']['tick']) > initial_reward_tick}
    assert len(dropped) == 2, 'Expected precisely two newly dropped blue coins'
    rewards = []
    for target, hit, death in targets:
        matches = []
        for uid, item in dropped.items():
            spawn = item['spawn']
            distance = sum((int(spawn[axis]) - int(hit[axis])) ** 2 for axis in ('x', 'y', 'z'))
            if abs(int(spawn['tick']) - int(hit['tick'])) <= 20 and distance < (100 * 4096) ** 2:
                matches.append((uid, item))
        assert len(matches) == 1, 'No unique blue coin at this Boo hit position/time'
        uid, item = matches[0]
        assert 'pickup' in item and 'before' in item, 'Dropped blue coin was not collected'
        gain = int(item['pickup']['course_coins']) - int(item['before']['course_coins'])
        assert gain == 5, 'Blue coin did not award exactly five course coins'
        rewards.append(dict(boo_uid=target['uid'], area=int(target['area']),
                            hit_tick=int(hit['tick']), death_tick=int(death['tick']),
                            coin_uid=uid, collected_tick=int(item['pickup']['tick']), coins=gain))
    assert rewards[0]['coin_uid'] != rewards[1]['coin_uid']
    states = read_csv(case / 'state.csv')
    last = states[-1]
    assert last['level'] == '12' and last['player_character'] == '0'
    assert last['player_state'] == 'St_Wait_Main' and last['player_airborne'] == '0'
    assert last['player_disabled'] == '0' and last['player_no_control'] == '0'
    final_waypoint = len(config['route'].split(',')) - 1
    assert re.search(rf'reached waypoint={final_waypoint} level=12 target=950,0,1400 tick=\d+', log), 'Player did not return to the hall'
    stairs = read_csv(case / 'merry.csv')
    assert {row['step'] for row in stairs if row['actor'] == '65' and row['event'] == 'behavior'
            and row['visible'] == '0' and row['trigger'] == '0'} == {'0', '1', '2'}
    seed = Path(config['save_seed'])
    saves = {}
    for original in seed.glob('*.sav'):
        actual = case / 'saves' / original.name
        assert actual.read_bytes() == original.read_bytes(), f'{original.name} changed'
        saves[original.name] = hashlib.sha256(actual.read_bytes()).hexdigest()
    assert saves
    report = dict(status='VERIFIED', scope='Two distinct authored Mission 1 Boo defeats, two five-coin rewards and main-hall Big Boo prerequisite counter 0 to 1 to 2',
                  exe_sha256=hashlib.sha256((case / 'sm64ds_pc.exe').read_bytes()).hexdigest(),
                  boss_uid=boss['uid'], boss_gate=2, boss_hp=3, defeats=rewards,
                  final_coins=int(final_reward['course_coins']), saves_unchanged=saves, muted=True,
                  limitations=['Diagnostic entry and prepared save', 'Three prerequisite Boos remain',
                               'Active boss, Star, staircase rise and course lifecycle still unverified'])
    (case / 'boo-second-credit-acceptance.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
