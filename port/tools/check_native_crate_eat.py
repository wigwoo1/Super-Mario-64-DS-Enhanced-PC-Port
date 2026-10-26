"""Verify an isolated normal-input Yoshi crate capture and egg conversion."""
import argparse
import csv
import hashlib
import json
import re
from pathlib import Path


def rows(path):
    with path.open(newline='', encoding='utf-8-sig') as stream:
        return list(csv.DictReader(stream))


def check(case):
    config = json.loads((case / 'test-config.json').read_text(encoding='utf-8-sig'))
    assert config['mode'] == 'eat' and config['authored_actors']
    assert config['development_picker'] and config['output_muted']
    assert (case / 'exit-code.txt').read_text(encoding='utf-8-sig').strip() == '0'
    digest = hashlib.sha256((case / 'sm64ds_pc.exe').read_bytes()).hexdigest()
    assert digest == config['exe_sha256'].lower()
    log = (case / 'run.log').read_text(errors='replace')
    assert re.search(r'\[native audio\] shutdown .* errors=0 ', log)
    assert '[native audio] submission output_muted=1 nonzero_samples=0' in log
    crate = rows(case / 'crate.csv')
    conversions = [r for r in crate if r['event'] == 'egg']
    assert len(conversions) == 1, 'expected exactly one crate-to-egg callback'
    converted = conversions[0]
    uid, tick = converted['uid'], int(converted['tick'])
    trace = [r for r in crate if r['uid'] == uid]
    entered = [r for r in trace if r['event'] == 'state_enter']
    assert [r['state'] for r in entered] == ['0', '5', '6']
    assert not any(r['event'] in ('kill', 'ground_pound') for r in trace)
    held = [r for r in trace if r['state'] == '5' and r['player_mouth_is_crate'] == '1']
    assert held and max(int(r['tick']) for r in held) - min(int(r['tick']) for r in held) > 100
    assert converted['player_param'] == '3' and converted['player_mouth_is_crate'] == '1'
    assert converted['coins_paid'] == '0'
    broken = entered[-1]
    assert int(broken['tick']) == tick and broken['coins_paid'] == '1'
    assert broken['player_egg_params'] == '12', 'crate did not bank its original three coins'
    late = [r for r in trace if int(r['tick']) > tick]
    assert late and all(r['state'] == '6' and r['coins_paid'] == '1' for r in late)
    assert late[-1]['player_mouth_is_crate'] == '0'
    states = [r for r in rows(case / 'state.csv')
              if r['level'] == str(config['level']) and r['player_state'] != 'none']
    assert states and all(r['player_parameter'] == '3' for r in states)
    assert any(r['player_state'] == 'St_YoshiPower_Main' for r in states)
    assert any(r['player_state'] == 'St_Swallow_Main' for r in states)
    final = states[-1]
    assert int(final['tick']) > tick + 500 and final['player_state'] == 'St_Wait_Main'
    assert final['player_disabled'] == final['player_no_control'] == '0'
    rewards = rows(case / 'rewards.csv')
    eggs = [r for r in rewards if r['kind'] == '9']
    assert eggs and len({r['id'] for r in eggs}) == 1
    assert int(eggs[0]['tick']) == tick + 1 and int(eggs[-1]['tick']) > tick + 500
    assert all(r['alive'] == '1' and r['state'] == '0' and
               (int(r['param']) >> 6) & 15 == 3 and
               int(r['payload']) == int(r['param']) >> 4 and
               not int(r['param']) & 0xc00 for r in eggs)
    assert all(r['course_coins'] == '0' for r in rewards), 'coin credit was awarded before egg payout'
    initial_coins = {r['id'] for r in rewards if r['kind'] == '288' and int(r['tick']) < tick}
    assert not any(r['kind'] == '288' and int(r['tick']) >= tick and
                   r['id'] not in initial_coins
                   for r in rewards), 'swallow incorrectly spawned loose coins'
    seed = Path(config['save_seed'])
    saved = sorted(seed.glob('*.sav'))
    assert saved and all((case / 'saves' / p.name).read_bytes() == p.read_bytes() for p in saved)
    result = {
        'status': 'VERIFIED_SELECTED_CRATE_CAPTURE_AND_THREE_COIN_EGG',
        'case': case.name, 'executable_sha256': digest, 'level': config['level'],
        'diagnostic_course_picker': True, 'crate_uid': uid,
        'capture_tick': int(entered[1]['tick']), 'egg_callback_tick': tick,
        'egg_uid': eggs[0]['id'], 'egg_param': int(eggs[0]['param']),
        'egg_coin_payload': 3, 'course_coins': 0, 'normal_control': True,
        'audio_muted': True, 'audio_errors': 0, 'exit': 0,
        'preserved_save_files': [p.name for p in saved],
        'limitations': ['One authored Hazy Maze Cave crate via diagnostic course picker.',
                        'Spitting, carrying, throwing and original course entry remain unverified.',
                        'Egg launch and reward release are outside this capture/creation check.'],
    }
    (case / 'crate-eat-acceptance.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('case', type=Path)
    print(json.dumps(check(parser.parse_args().case), indent=2))
