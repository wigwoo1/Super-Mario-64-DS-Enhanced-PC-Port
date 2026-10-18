"""Check the original File A -> castle doors -> painting -> mission -> course replay.

The isolated seed already has the castle key. The route helper supplies only
stick/buttons, so the original door unlock, painting and scene code must run.
"""
import argparse
import csv
import json
from pathlib import Path
import re
from check_power_star_replay import save_payload


def verify(prefix, before, after):
    log = prefix.with_suffix('.log').read_text()
    assert int(prefix.with_suffix('.exit-code.txt').read_text()) == 0, 'game did not exit cleanly'
    states_path = prefix.with_name(prefix.name + '-state.csv')
    if not states_path.exists():
        states_path = prefix.parent / 'port' / (prefix.name + '-state.csv')
    states = list(csv.DictReader(states_path.open()))
    assert states and states[0]['player_state'] == 'none', 'replay did not begin in original menu'
    assert '[native DSMT] native factory' in log and 'slot=0 loaded=primary' in log, 'original File A was not loaded'
    first = {}
    for row in states:
        if row['player_state'] != 'none':
            first.setdefault(int(row['level']), int(row['tick']))
    assert all(level in first for level in (1, 2, 6)), 'exterior, castle or Bob-omb Battlefield missing'
    assert first[1] < first[2] < first[6], 'wrong course-entry order'
    assert '[native course route] original transition 1 -> 2' in log, 'original castle entrance not observed'
    assert re.search(r'level=1 player=.*flags1=00000041', log), 'wrong castle-key fixture'
    assert re.search(r'level=2 player=.*flags1=00002041', log), 'original castle unlock did not set its flag'
    assert '[native mission selection] confirmed mission=1 level=6' in log, 'mission selection was bypassed or wrong mission'
    assert '[native QA fixture]' not in log, 'a placement fixture was used'
    reached = {int(n) for n in re.findall(r'reached waypoint=(\d+)', log)}
    assert {0, 2, 3, 4, 5, 6, 7} <= reached, 'stairs/door route was not completed'
    assert any(r['level'] == '2' and r['player_state'] == 'St_Jump_Main' and
               int(r['player_x']) < -4700*4096 for r in states), 'no normal jump toward the painting'
    assert any(r['level'] == '6' and r['player_state'] == 'St_Jump_Main' and
               int(r['tick']) >= 5800 for r in states), 'course control was not verified by the later jump'
    final = states[-1]
    assert final['level'] == '6' and final['player_state'] == 'St_Wait_Main', 'course did not finish idle'
    assert final['player_disabled'] == final['player_no_control'] == '0', 'course controls remained locked'
    assert final['player_character'] == '3', 'wrong playable character'
    assert all(r['power_star_total'] == '7' for r in states if r['player_state'] != 'none'), 'star total changed during entry'
    seed, saved = save_payload(before), save_payload(after)
    assert seed[0x14:0x32] == saved[0x14:0x32], 'course-entry test changed saved star credit'
    return dict(status='PASS', first_playable_frames=first, final_frame=int(final['tick']),
                final_character='Yoshi', power_stars=7, saved_bytes_unchanged=before.read_bytes()==after.read_bytes())


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('prefix', type=Path)
    p.add_argument('seed', type=Path)
    p.add_argument('save', type=Path)
    args = p.parse_args()
    print(json.dumps(verify(args.prefix, args.seed, args.save), indent=2))
