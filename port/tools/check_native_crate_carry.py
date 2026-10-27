"""Check Mario's normal-input crate grab, carried walk, throw and three-coin reward."""
import argparse
import csv
import hashlib
import json
import math
import re
from pathlib import Path


def rows(path):
    with path.open(newline='', encoding='utf-8-sig') as stream:
        return list(csv.DictReader(stream))


def distance(a, b, prefix=''):
    return math.hypot(int(a[prefix + 'x']) - int(b[prefix + 'x']),
                      int(a[prefix + 'z']) - int(b[prefix + 'z'])) / 4096


def check(case):
    config = json.loads((case / 'test-config.json').read_text(encoding='utf-8-sig'))
    assert config['mode'] == 'carry-throw' and config['level'] == 13
    assert config['development_picker'] and config['authored_actors'] and config['output_muted']
    assert (case / 'exit-code.txt').read_text(encoding='utf-8-sig').strip() == '0'
    digest = hashlib.sha256((case / 'sm64ds_pc.exe').read_bytes()).hexdigest()
    assert digest == config['exe_sha256'].lower()
    log = (case / 'run.log').read_text(errors='replace')
    assert '[native mission selection] confirmed mission=1 level=13' in log
    assert '[native audio] submission output_muted=1 nonzero_samples=0' in log
    assert re.search(r'\[native audio\] shutdown .* errors=0 ', log)
    assert 'grab-timeout' not in log and 'throw-timeout' not in log
    assert all(event in log for event in ('press-grab-attack', 'carried', 'carry-walk-finished',
                                         'press-throw-attack', 'crate-in-flight', 'throw-broken'))
    states = [r for r in rows(case / 'state.csv') if r['level'] == '13' and r['player_state'] != 'none']
    assert states and all(r['player_parameter'] == '0' for r in states)
    assert any(r['player_state'] == 'St_HoldLight_Main' for r in states)
    assert any(r['player_state'] == 'St_Throw_Main' for r in states)
    final = states[-1]
    assert int(final['tick']) == config['frames'] and final['player_state'] == 'St_Wait_Main'
    assert final['player_disabled'] == final['player_no_control'] == '0'

    crate = rows(case / 'crate.csv')
    entries = [r for r in crate if r['event'] == 'state_enter']
    assert [r['state'] for r in entries[:4]] == ['0', '1', '2', '6']
    assert [r['state'] for r in entries[4:]] in ([], ['0'])
    origin, grab, thrown, broken = entries[:4]
    if len(entries)==5:
        respawn=entries[4]
        assert int(respawn['flags']) & 8 and respawn['coins_paid']=='1'
        assert math.dist([int(respawn[k]) for k in ('x','y','z')],
                         [int(respawn['player_'+k]) for k in ('x','y','z')]) > 2000*4096
    uid = origin['uid']
    assert all(r['uid'] == uid and r['param'] == origin['param'] for r in crate)
    assert int(grab['hit']) & 0x1000 and grab['other'] == grab['player_uid']
    assert grab['holder'] != '0' and grab['player_held_is_crate'] == '1'
    carried = [r for r in crate if r['event'] == 'behavior' and r['state'] == '1' and r['player_held_is_crate'] == '1']
    assert len(carried) >= 10 and all(r['holder'] == grab['holder'] for r in carried)
    moved = max(distance(r, carried[0], 'player_') for r in carried)
    assert moved > 60 and max(distance(r, carried[0]) for r in carried) > 60
    settled = [r for r in carried if r['player_state_step'] == '1']
    assert settled and all(math.hypot(int(r['x']) - int(r['player_x']), int(r['z']) - int(r['player_z'])) < 150 * 4096 for r in settled)
    assert thrown['holder'] == '0' and thrown['prev_holder'] == grab['holder']
    assert thrown['player_held_is_crate'] == '0'
    assert int(thrown['horz_speed']) == 40 * 4096 and int(thrown['vert_speed']) == 20 * 4096
    flight = [r for r in crate if r['state'] == '2']
    assert len(flight) >= 3 and max(distance(r, thrown) for r in flight) > 20
    assert all(r['holder'] == '0' and r['prev_holder'] == grab['holder'] for r in flight)
    assert not any(r['event'] in ('ground_pound', 'egg') for r in crate)
    kills = [r for r in crate if r['event'] == 'kill']
    assert len(kills) == 1 and kills[0]['tick'] == broken['tick']
    assert all(r['coins_paid'] == '0' for r in entries[:3])
    assert all(r['coins_paid'] == '1' for r in entries[3:])
    assert all(r['player_held_is_crate'] == '0' for r in crate if int(r['tick']) >= int(broken['tick']))

    particles = rows(case / 'particles.csv')
    creates = [r for r in particles if r['phase'] == 'create' and r['id'] == '14']
    assert len(creates) == 1 and creates[0]['tick'] == broken['tick']
    assert sum(int(r['clipped_triangles']) for r in particles if r['phase'] == 'render' and r['id'] == '14') > 0
    tick = int(broken['tick'])
    rewards = rows(case / 'rewards.csv')
    assert max(int(r['course_coins']) for r in rewards if int(r['tick']) < tick) == 0
    assert int(rewards[-1]['course_coins']) == 3, 'All three thrown-crate coins were not collected'
    first, removed = {}, {}
    for row in rewards:
        if row['kind'] == '288':
            first.setdefault(row['id'], row)
            if row['alive'] == '2':
                removed.setdefault(row['id'], row)
    coins = [r for r in first.values() if int(r['tick']) == tick + 1]
    assert len(coins) == 3
    pickups = []
    for coin in coins:
        end = removed[coin['id']]
        pickup = int(end['tick'])
        assert tick < pickup < config['frames']
        prior = max(int(r['course_coins']) for r in rewards if tick <= int(r['tick']) < pickup)
        assert int(end['course_coins']) > prior, 'Coin removal was not collection'
        if config.get('pickup_after_two'):
            assert int(end['coin_timer'])==0, 'Removal was not the original collection timer reset'
        pickups.append(dict(uid=coin['id'], tick=pickup))
    seed = Path(config['save_seed'])
    saves = sorted(seed.glob('*.sav'))
    assert saves and all(p.read_bytes() == (case / 'saves' / p.name).read_bytes() for p in saves)
    return dict(status='VERIFIED_SELECTED_MARIO_CRATE_CARRY_THROW_BREAK_AND_THREE_COINS',
                executable_sha256=digest, crate_uid=uid, grab_tick=int(grab['tick']),
                carried_player_displacement=moved, throw_tick=int(thrown['tick']), break_tick=tick,
                launch_speed_units=dict(horizontal=40, vertical=20), ownership_released=True,
                coins_collected=pickups, final_course_coins=3, normal_control=True,
                original_distance_respawn=len(entries)==5,
                isolated_saves_unchanged=True, audio_muted=True, audio_errors=0, exit=0,
                limitations=['One authored HMC crate with Mario and a synthetic prerequisite save.',
                             'Diagnostic course picker; other characters, other placements and wider campaign remain unverified.'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('case', type=Path)
    case = parser.parse_args().case.resolve()
    result = check(case)
    (case / 'crate-carry-acceptance.json').write_text(json.dumps(result, indent=2))
    print(json.dumps(result, indent=2))
