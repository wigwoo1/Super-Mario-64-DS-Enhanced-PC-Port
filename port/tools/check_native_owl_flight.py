"""Check a muted diagnostic Owl flight replay; not campaign acceptance."""
import argparse
import csv
import hashlib
import json
import math
import pathlib
import re


def check(case):
    config = json.loads((case / 'test-config.json').read_text(encoding='utf-8-sig'))
    log = (case / 'run.log').read_text(errors='replace')
    assert (case / 'exit-code.txt').read_text().strip() == '0', 'Replay did not exit normally'
    assert config['owl_ride'] and config['owl_flight'], 'Full-flight pilot was not enabled'
    expected_level = int(config.get('expected_level', 7))
    expected_mission = int(config.get('expected_mission', 3))
    assert f'[native mission selection] confirmed mission={expected_mission} level={expected_level}' in log
    assert 'output_muted=1 nonzero_samples=0' in log, 'Audible output was not muted'
    assert re.search(r'\[native audio\] shutdown .* errors=0\b', log)
    def event(name):
        found = re.search(r'\[native owl ride\] ' + re.escape(name) + r' tick=(\d+)', log)
        assert found, 'Missing Owl event: ' + name
        return int(found[1])
    attached = event('attached')
    flight = event('flight-ready')
    release = event('release-input')
    landed = event('landed')
    walking = event('movement-recovered')
    assert attached < flight < release < landed <= walking
    with (case / 'state.csv').open(newline='') as f:
        rows = list(csv.DictReader(f))
    flying = [r for r in rows if flight <= int(r['tick']) < release]
    assert len(flying) >= 8
    assert all(r['player_state'] == 'St_Owl_Main' and r['player_state_step'] == '1' for r in flying)
    assert any(int(r['reader_magnitude']) > 0 and int(r['game_held']) & 2 for r in flying)
    first, last = flying[0], flying[-1]
    distance = math.hypot(int(last['player_x'])-int(first['player_x']),
                          int(last['player_z'])-int(first['player_z'])) / 4096
    assert distance > 100, 'No meaningful lateral flight displacement'
    assert any(int(r['tick']) >= walking and r['player_state'] == 'St_Walk_Main' for r in rows)
    final = rows[-1]
    assert int(final['level']) == expected_level and final['player_state'] == 'St_Wait_Main'
    assert all(final[k] == '0' for k in ('player_airborne', 'player_disabled', 'player_no_control'))
    assert re.search(r'\[native owl\] state tick=\d+ uid=\d+ state=4 talk=2', log)
    seed = pathlib.Path(config['save_seed'])
    hashes = {}
    for original in seed.glob('*.sav'):
        copy = case / 'saves' / original.name
        assert copy.read_bytes() == original.read_bytes(), 'Copied save changed: ' + original.name
        hashes[original.name] = hashlib.sha256(copy.read_bytes()).hexdigest()
    assert len(hashes) >= 2, 'Save preservation was not checked'
    return dict(status='VERIFIED_DIAGNOSTIC_OWL_FLIGHT_RELEASE_AND_WALK',
                attached_tick=attached, flight_tick=flight, release_tick=release,
                landed_tick=landed, walking_tick=walking, lateral_distance=distance,
                frames=int(final['tick']), exe_sha256=config['exe_sha256'], saves=hashes,
                character=int(final['player_character']),
                limitations='Prepared character save; no campaign, timeout or course-reentry acceptance. Dialogue appearance requires separate screenshot review.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('case', type=pathlib.Path)
    args = parser.parse_args()
    result = check(args.case)
    (args.case / 'hoot-flight-acceptance.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
