"""Verify two authored crate rewards across original Exit Course and HMC re-entry."""
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


def check(case):
    config = json.loads((case / 'test-config.json').read_text(encoding='utf-8-sig'))
    assert config['mode'] == 'pound-reentry' and config['level'] == 13
    assert config['output_muted'] and config['authored_actors']
    assert config['initial_diagnostic_course_entry'] and config['synthetic_unlocks']
    assert (case / 'exit-code.txt').read_text(encoding='utf-8-sig').strip() == '0'
    digest = hashlib.sha256((case / 'sm64ds_pc.exe').read_bytes()).hexdigest()
    assert digest == config['exe_sha256'].lower()
    log = (case / 'run.log').read_text(errors='replace')
    assert '[native audio] submission output_muted=1 nonzero_samples=0' in log
    assert re.search(r'\[native audio\] shutdown .* errors=0 ', log)
    assert log.count('[native mission selection] confirmed mission=1 level=13') == 2
    events = dict((name, int(tick)) for name, tick in re.findall(
        r'\[native lava bridge lifecycle\] ([\w-]+) tick=(\d+)', log))
    required = ('exit-request', 'castle-return', 'original-course-reentry', 'done')
    assert all(event in events for event in required) and 'timeout' not in events
    assert [events[event] for event in required] == sorted(events[event] for event in required)
    assert '[native course route] original transition 2 -> 4' in log
    states = rows(case / 'state.csv')
    visits = [[r for r in states if r['level'] == '13' and r['player_state'] != 'none'
               and (int(r['tick']) < events['castle-return'] if visit == 0 else
                    int(r['tick']) >= events['original-course-reentry'])] for visit in (0, 1)]
    assert all(visits)
    assert all(r['player_parameter'] == '3' for visit in visits for r in visit)
    assert any(r['pause_active'] == '1' for r in visits[0]), 'Original pause menu was not observed'
    final = states[-1]
    assert int(final['tick']) == config['frames'] and final['level'] == '13'
    assert final['player_state'] == 'St_Wait_Main'
    assert final['player_disabled'] == final['player_no_control'] == '0'

    crate = rows(case / 'crate.csv')
    pounds = [r for r in crate if r['event'] == 'ground_pound']
    kills = [r for r in crate if r['event'] == 'kill']
    assert len(pounds) == len(kills) == 2
    assert int(pounds[0]['tick']) < events['exit-request'] < events['original-course-reentry'] < int(pounds[1]['tick'])
    assert pounds[0]['uid'] != pounds[1]['uid'], 'The old actor identity survived scene teardown'
    assert all(p['uid'] == k['uid'] and p['tick'] == k['tick'] for p, k in zip(pounds, kills))
    assert all(p['player_param'] == '3' and p['other'] == p['player_uid'] for p in pounds)
    assert [pounds[0][key] for key in ('param', 'x', 'y', 'z')] == [pounds[1][key] for key in ('param', 'x', 'y', 'z')]
    rewards = rows(case / 'rewards.csv')
    first, removed = {}, {}
    for row in rewards:
        if row['kind'] == '288':
            first.setdefault(row['id'], row)
            if row['alive'] == '2':
                removed.setdefault(row['id'], row)
    results = []
    for visit, pound in zip(visits, pounds):
        uid, tick = pound['uid'], int(pound['tick'])
        entries = [r for r in crate if r['event'] == 'state_enter' and r['uid'] == uid]
        assert [r['state'] for r in entries] == ['0', '6']
        assert entries[0]['coins_paid'] == '0' and entries[1]['coins_paid'] == '1'
        assert int(entries[1]['tick']) == tick
        assert any(r['player_state'] == 'St_Jump_Main' and int(r['tick']) < tick for r in visit)
        assert any(r['player_state'] == 'St_GroundPound_Main' and abs(int(r['tick']) - tick) < 20 for r in visit)
        coins = [r for r in first.values() if int(r['tick']) == tick + 1 and
                 math.hypot(int(r['x']) - int(pound['x']), int(r['z']) - int(pound['z'])) < 200 * 4096]
        assert len(coins) == 3
        visit_end = int(visit[-1]['tick'])
        visit_rewards = [r for r in rewards if tick <= int(r['tick']) <= visit_end]
        assert int(visit_rewards[0]['course_coins']) == 0
        assert int(visit_rewards[-1]['course_coins']) == 3
        pickups = []
        for coin in coins:
            end = removed[coin['id']]
            pickup = int(end['tick'])
            assert tick < pickup < visit_end
            prior = max(int(r['course_coins']) for r in visit_rewards if int(r['tick']) < pickup)
            assert int(end['course_coins']) > prior, 'Coin removal was not a collection'
            player = min(visit, key=lambda r: abs(int(r['tick']) - pickup))
            assert math.hypot(int(player['player_x']) - int(end['x']), int(player['player_z']) - int(end['z'])) < 400 * 4096
            pickups.append(dict(uid=coin['id'], tick=pickup))
        results.append(dict(crate_uid=uid, pound_tick=tick, collected=pickups, final_course_coins=3))
    particles = rows(case / 'particles.csv')
    creates = [r for r in particles if r['phase'] == 'create' and r['id'] == '14']
    assert [r['tick'] for r in creates] == [r['tick'] for r in pounds]
    for index, pound in enumerate(pounds):
        begin = int(pound['tick'])
        end = int(pounds[index + 1]['tick']) if index == 0 else config['frames'] + 1
        assert sum(int(r['clipped_triangles']) for r in particles if r['phase'] == 'render' and
                   r['id'] == '14' and begin <= int(r['tick']) < end) > 0
    seed = Path(config['save_seed'])
    provenance = json.loads((seed / 'fixture-provenance.json').read_text())
    for name, expected in provenance['fixture_hashes'].items():
        assert hashlib.sha256((seed / name).read_bytes()).hexdigest() == expected
        assert (case / 'saves' / name).read_bytes() == (seed / name).read_bytes()
    return dict(status='VERIFIED_SELECTED_HMC_CRATE_RESET_AND_REWARD_AFTER_ORIGINAL_REENTRY',
                executable_sha256=digest, lifecycle=events, rounds=results, normal_control=True,
                isolated_saves_unchanged=True, audio_muted=True, audio_errors=0, exit=0,
                limitations=['One authored HMC crate with Yoshi and synthetic unlock prerequisites.',
                             'Initial course entry used the diagnostic picker; re-entry used real castle doors and the pool.',
                             'Does not verify earned key progression, other crate placements or a complete campaign.'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('case', type=Path)
    args = parser.parse_args()
    case = args.case.resolve()
    result = check(case)
    (case / 'crate-reentry-acceptance.json').write_text(json.dumps(result, indent=2))
    print(json.dumps(result, indent=2))
