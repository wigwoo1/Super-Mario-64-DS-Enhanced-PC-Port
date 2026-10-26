"""Check the normal-input crate egg flight and its one-time three-coin payout."""
import argparse
import csv
import hashlib
import json
import re
from collections import defaultdict
from pathlib import Path


def rows(path):
    with path.open(newline='', encoding='utf-8-sig') as stream:
        return list(csv.DictReader(stream))


def check(case):
    config = json.loads((case / 'test-config.json').read_text(encoding='utf-8-sig'))
    assert config['mode'] == 'eat-spit' and config['authored_actors']
    assert config['development_picker'] and config['output_muted']
    assert (case / 'exit-code.txt').read_text(encoding='utf-8-sig').strip() == '0'
    digest = hashlib.sha256((case / 'sm64ds_pc.exe').read_bytes()).hexdigest()
    assert digest == config['exe_sha256'].lower()
    log = (case / 'run.log').read_text(errors='replace')
    assert re.search(r'\[native audio\] shutdown .* errors=0 ', log)
    assert '[native audio] submission output_muted=1 nonzero_samples=0' in log
    pilot = [line for line in log.splitlines() if '[native crate pound replay]' in line]
    assert any('captured-in-mouth' in line for line in pilot)
    assert any('turned-into-egg' in line for line in pilot)
    assert any('press-egg-attack' in line for line in pilot)
    assert any('egg-in-flight' in line for line in pilot)

    crate = rows(case / 'crate.csv')
    converted = [r for r in crate if r['event'] == 'egg']
    assert len(converted) == 1
    crate_id = converted[0]['uid']
    crate_rows = [r for r in crate if r['uid'] == crate_id]
    entered = [r for r in crate_rows if r['event'] == 'state_enter']
    assert [r['state'] for r in entered] == ['0', '5', '6']
    assert converted[0]['coins_paid'] == '0' and entered[-1]['coins_paid'] == '1'
    assert entered[-1]['player_egg_params'] == '12'
    assert not any(r['event'] in ('kill', 'ground_pound') for r in crate_rows)

    states = [r for r in rows(case / 'state.csv')
              if r['level'] == str(config['level']) and r['player_state'] != 'none']
    assert states and all(r['player_parameter'] == '3' for r in states)
    assert any(r['player_state'] == 'St_YoshiPower_Main' for r in states)
    assert any(r['player_state'] == 'St_Swallow_Main' for r in states)
    final = states[-1]
    assert int(final['tick']) == config['frames'] and final['player_state'] == 'St_Wait_Main'
    assert final['player_disabled'] == final['player_no_control'] == '0'

    rewards = rows(case / 'rewards.csv')
    eggs = [r for r in rewards if r['kind'] == '9']
    assert eggs and len({r['id'] for r in eggs}) == 1
    egg_id = eggs[0]['id']
    egg_rows = [r for r in eggs if r['id'] == egg_id]
    assert int(egg_rows[0]['param']) == 224 and int(egg_rows[0]['payload']) == 14
    assert (int(egg_rows[0]['param']) >> 6) & 15 == 3
    flight = [r for r in egg_rows if int(r['tick']) >= 2730 and r['state'] == '1']
    assert flight and any(int(r['flags']) & 0x400 for r in flight)
    burst = [r for r in egg_rows if int(r['alive']) == 2]
    assert burst and int(burst[0]['payout_index']) == 1
    assert int(burst[0]['payout_bits']) == 1, 'egg burst reward was not recorded once'
    assert int(burst[0]['course_coins']) == 0
    payout_tick = int(burst[0]['tick'])
    counters = [int(r['course_coins']) for r in rewards if int(r['tick']) >= payout_tick]
    assert min(counters) == 0 and max(counters) == 3 and counters[-1] == 3

    before = [int(r['id']) for r in rewards if r['kind'] == '288' and
              int(r['tick']) < int(flight[0]['tick'])]
    assert before
    new = defaultdict(list)
    for row in rewards:
        if row['kind'] == '288' and int(row['id']) > max(before) and int(row['tick']) >= int(flight[0]['tick']):
            new[row['id']].append(row)
    assert len(new) == 3, f'expected exactly three coins from egg, saw {sorted(new)}'
    for timeline in new.values():
        assert timeline[0]['alive'] == '1'
        assert timeline[-1]['alive'] == '2'
        assert int(timeline[-1]['tick']) > int(timeline[0]['tick'])
        assert int(timeline[-1]['course_coins']) == 3

    seed = Path(config['save_seed'])
    saved = sorted(seed.glob('*.sav'))
    assert saved and all((case / 'saves' / p.name).read_bytes() == p.read_bytes() for p in saved)
    result = {
        'status': 'VERIFIED_SELECTED_CRATE_EGG_FLIGHT_AND_ONE_TIME_THREE_COIN_PAYOUT',
        'case': case.name, 'executable_sha256': digest, 'level': config['level'],
        'diagnostic_course_picker': True, 'crate_uid': crate_id,
        'egg_uid': egg_id, 'egg_param': int(egg_rows[0]['param']),
        'egg_flight_tick': int(flight[0]['tick']), 'egg_burst_tick': int(burst[0]['tick']),
        'new_coin_uids': sorted(new, key=int), 'course_coin_counter': 3,
        'payout_count': 1, 'normal_control': True, 'audio_muted': True,
        'audio_errors': 0, 'exit': 0, 'preserved_save_files': [p.name for p in saved],
        'limitations': ['One authored Hazy Maze Cave crate and nearby wall via diagnostic picker.',
                        'This does not verify long-distance targeting or coin collection routes.'],
    }
    (case / 'crate-spit-acceptance.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('case', type=Path)
    print(json.dumps(check(parser.parse_args().case), indent=2))
