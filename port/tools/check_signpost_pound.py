"""Verify two input-driven sign ground pounds against native runtime evidence."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import re


def rows(path):
    with path.open(newline='', encoding='utf-8-sig') as stream:
        return list(csv.DictReader(stream))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check(case):
    config = json.loads((case / 'test-config.json').read_text(encoding='utf-8-sig'))
    assert config['sign_pound_replay'] and not config['development_menu']
    assert config['output_muted'] and config['engine_enabled']
    assert sha(case / 'sm64ds_pc.exe') == config['exe_sha256'].lower()
    signs = [r for r in rows(case / 'sign.csv') if r['param'] == '1800']
    ids = {r['uid'] for r in signs}
    assert len(ids) == 1, 'selected sign was replaced during the test'
    uid = next(iter(ids))
    transitions = []
    for row in signs:
        if not transitions or row['pounds'] != transitions[-1]['pounds']:
            transitions.append(row)
    assert [int(r['pounds']) for r in transitions] == [2, 1, 0], 'not exactly two normal hits'
    for before, after in zip(transitions, transitions[1:]):
        assert int(after['y']) - int(before['y']) == -45 * 4096, 'wrong original sink distance'
    state = rows(case / 'state.csv')
    contacts = rows(case / 'contact.csv')
    for hit in transitions[1:]:
        tick = int(hit['tick'])
        assert any(abs(int(r['tick']) - tick) <= 10 and r['player_state'] == 'St_GroundPound_Main'
                   and r['player_character'] == '3' for r in state), 'hit not made by ground-pounding Yoshi'
        assert any(abs(int(r['tick']) - tick) <= 10 and r['grounded'] == '1'
                   and r['owner_kind'] == '184' and r['owner_uid'] == uid
                   for r in contacts), 'no native sign floor contact at hit'
    assert int(transitions[2]['tick']) - int(transitions[1]['tick']) > 30
    final = state[-1]
    assert final['player_state'] == 'St_Wait_Main'
    assert all(final[k] == '0' for k in ('player_disabled', 'player_no_control', 'player_airborne'))
    assert not any(r['player_state'] == 'St_Talk_Main' for r in state), 'dialogue intercepted the jump'
    assert int(final['tick']) - int(transitions[-1]['tick']) > 200, 'no stable final observation'
    log = (case / 'run.log').read_text(errors='replace')
    assert len(re.findall(r'\[native sign pound replay\] press-R ', log)) == 2
    assert '[native sign pound replay] complete tick=' in log
    assert re.search(r'\[native audio\] shutdown .* errors=0 ', log)
    assert 'submission output_muted=1 nonzero_samples=0' in log
    assert (case / 'exit-code.txt').read_text().strip() == '0'
    seed = Path(config['save_seed'])
    for path in seed.iterdir():
        if path.is_file():
            assert sha(path) == sha(case / 'saves' / path.name), 'QA save changed'
    return dict(status='VERIFIED', scope='Yoshi pounds one original castle sign twice',
                exe_sha256=config['exe_sha256'], actor_uid=int(uid),
                sink_observation_ticks=[int(r['tick']) for r in transitions[1:]],
                pounds_remaining=[2, 1, 0], sink_units_per_hit=45,
                native_floor_contact=True, normal_control_restored=True,
                saves_unchanged=True, output_muted=True, mixer_errors=0, exit_code=0,
                respawn_carrying_throwing_other_signs_verified=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('case', type=Path)
    args = parser.parse_args()
    result = check(args.case)
    (args.case / 'signpost-pound-acceptance.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
