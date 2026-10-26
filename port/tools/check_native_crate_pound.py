"""Check a completed, isolated native Yoshi crate-pound replay (private CSVs)."""
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
    assert (case / 'exit-code.txt').read_text().strip() == '0'
    assert config['authored_actors'] and config['development_picker']
    repeat = config['mode'] == 'pound-reset'
    assert config['mode'] in ('pound', 'pound-reset') and config['output_muted']
    digest = hashlib.sha256((case / 'sm64ds_pc.exe').read_bytes()).hexdigest()
    assert digest == config['exe_sha256'].lower()
    log = (case / 'run.log').read_text(errors='replace')
    assert re.search(r'\[native audio\] shutdown .* errors=0 ', log)
    assert '[native audio] submission output_muted=1 nonzero_samples=0' in log
    states = [r for r in rows(case / 'state.csv')
              if r['level'] == str(config['level']) and r['player_state'] != 'none']
    assert states and all(r['player_parameter'] == '3' for r in states)
    assert any(r['player_state'] == 'St_Jump_Main' for r in states)
    assert any(r['player_state'] == 'St_GroundPound_Main' for r in states)
    final = states[-1]
    assert final['player_state'] == 'St_Wait_Main'
    assert final['player_disabled'] == final['player_no_control'] == '0'
    crate = rows(case / 'crate.csv')
    pounds = [r for r in crate if r['event'] == 'ground_pound']
    kills = [r for r in crate if r['event'] == 'kill']
    assert len(pounds) == len(kills) == (2 if repeat else 1)
    uid, tick = pounds[0]['uid'], int(pounds[0]['tick'])
    assert all(k['uid'] == p['uid'] == uid and k['tick'] == p['tick']
               for k, p in zip(kills, pounds))
    entered = [r for r in crate if r['event'] == 'state_enter' and r['uid'] == uid]
    assert [r['state'] for r in entered] == (['0', '6', '0', '6'] if repeat else ['0', '6'])
    assert entered[1]['coins_paid'] == '1' and int(entered[1]['tick']) == tick
    late = [r for r in crate if r['uid'] == uid and int(r['tick']) > tick]
    assert late and all(r['coins_paid'] == '1' for r in late)
    assert late[-1]['state'] == '6'
    if not repeat:
        assert all(r['state'] == '6' for r in late)
    else:
        respawn = entered[2]
        assert int(respawn['flags']) & 8  # Original off-screen requirement.
        distance = math.dist([int(respawn[a]) for a in ('x', 'y', 'z')],
                             [int(respawn['player_' + a]) for a in ('x', 'y', 'z')]) / 4096
        assert distance > 2000 and respawn['coins_paid'] == '1'
        assert all(r['player_param'] == '3' for r in pounds)
        assert all(r['other'] == r['player_uid'] for r in pounds)
    rewards = rows(case / 'rewards.csv')
    first, last, removed = {}, {}, {}
    for r in rewards:
        if r['kind'] == '288':
            first.setdefault(r['id'], r)
            last[r['id']] = r
            if r['alive'] == '2':
                removed.setdefault(r['id'], r)
    origin = pounds[0]
    released = [r for r in first.values() if int(r['tick']) == tick + 1 and
                math.hypot(int(r['x']) - int(origin['x']),
                           int(r['z']) - int(origin['z'])) < 200 * 4096]
    assert len(released) == 3
    assert max(int(r['course_coins']) for r in rewards if int(r['tick']) < tick) == 0
    assert int(rewards[-1]['course_coins']) == 3
    collected = []
    for coin in released:
        end = last[coin['id']]
        assert end['alive'] == '2' and 0 < int(end['tick']) - tick < 400
        # Coin expiry must not masquerade as collection: the original counter
        # rises at each observed removal tick (two coins may be simultaneous).
        removal_tick = int(removed[coin['id']]['tick'])
        prior = max(int(r['course_coins']) for r in rewards if int(r['tick']) < removal_tick)
        assert int(end['course_coins']) > prior
        collected.append({'uid': coin['id'], 'tick': removal_tick})
    particles = rows(case / 'particles.csv')
    created = [r for r in particles if r['phase'] == 'create' and r['id'] == '14']
    renders = [r for r in particles if r['phase'] == 'render' and r['id'] == '14']
    assert len(created) == len(pounds)
    assert [r['tick'] for r in created] == [r['tick'] for r in pounds]
    assert renders and sum(int(r['clipped_triangles']) for r in renders) > 0
    for i, create in enumerate(created):
        begin = int(create['tick'])
        end = int(created[i + 1]['tick']) if i + 1 < len(created) else math.inf
        assert sum(int(r['clipped_triangles']) for r in renders if begin <= int(r['tick']) < end) > 0
    if repeat:
        second_tick = int(pounds[1]['tick'])
        assert int(rewards[-1]['course_coins']) == 3
        assert all(int(r['course_coins']) == 3 for r in rewards if int(r['tick']) >= second_tick)
        assert not [r for r in first.values() if int(r['tick']) >= second_tick and
                    math.hypot(int(r['x']) - int(origin['x']),
                               int(r['z']) - int(origin['z'])) < 200 * 4096]
    saved = []
    for original in Path(config['save_seed']).glob('*.sav'):
        assert original.read_bytes() == (case / 'saves' / original.name).read_bytes()
        saved.append(original.name)
    assert saved
    result = dict(status='VERIFIED_SELECTED_YOSHI_CRATE_POUND_AND_THREE_COIN_PICKUP',
                executable_sha256=digest, case=case.name, level=config['level'],
                diagnostic_course_picker=True, crate_uid=uid, pound_tick=tick,
                coins_collected=collected, final_course_coins=3,
                debris_render_updates=len(renders),
                debris_clipped_triangles=sum(int(r['clipped_triangles']) for r in renders),
                final_player_state=final['player_state'], normal_control=True,
                preserved_save_files=saved, audio_muted=True, audio_errors=0, exit=0,
                limitations=['Selected authored Hazy Maze Cave crate only.',
                             'Carry, throw, eat, respawn and original castle entry are unverified.'])
    if repeat:
        result.update(status='VERIFIED_SELECTED_CRATE_RESPAWN_AND_NO_DUPLICATE_PAYOUT',
                      respawn_tick=int(respawn['tick']), respawn_distance=distance,
                      second_pound_tick=second_tick, coins_after_second_break=3,
                      same_actor_uid=True, limitations=[
                          'Same-session distance/off-screen respawn for one Hazy Maze Cave crate.',
                          'Carry, throw, eat, course re-entry and original castle entry are unverified.'])
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('case', type=Path)
    args = parser.parse_args()
    result = check(args.case.resolve())
    (args.case / 'crate-pound-acceptance.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
