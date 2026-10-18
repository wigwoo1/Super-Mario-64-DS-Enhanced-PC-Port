"""Check original first-course entry using the isolated, naturally earned key save."""
import argparse
import csv
import json
from pathlib import Path

from check_power_star_replay import save_payload


def verify(case, seed):
    log = (case / 'run.log').read_text()
    assert int((case / 'exit-code.txt').read_text()) == 0, 'game did not exit cleanly'
    states = list(csv.DictReader((case / 'state.csv').open()))
    assert states and states[0]['player_state'] == 'none', 'original menu was skipped'
    assert 'slot=0 loaded=primary' in log, 'earned save was not loaded'
    assert '[native QA fixture]' not in log, 'forced gameplay fixture was used'
    first = {}
    for row in states:
        if row['player_state'] != 'none':
            first.setdefault(int(row['level']), int(row['tick']))
            assert row['power_star_total'] == '0', 'expected zero collected stars'
    assert all(level in first for level in (1, 2, 6)), 'exterior, castle, or first course missing'
    assert first[1] < first[2] < first[6], 'incorrect progression order'
    assert 'original transition 1 -> 2' in log, 'original front door did not run'
    assert 'flags1=00000041' in log and 'flags1=00002041' in log, 'key/unlock flags missing'
    assert '[native mission selection] confirmed mission=1 level=6' in log, 'original mission was not confirmed'
    assert any(r['level'] == '2' and r['player_state'] == 'St_Jump_Main' and
               int(r['player_x']) < -4700 * 4096 for r in states), 'no jump into painting'
    assert any(r['level'] == '6' and r['player_state'] == 'St_Jump_Main' and
               int(r['tick']) >= 5800 for r in states), 'later course jump missing'
    final = states[-1]
    assert final['level'] == '6' and final['player_state'] == 'St_Wait_Main', 'not idle in course'
    assert final['player_disabled'] == final['player_no_control'] == '0', 'controls locked'
    assert final['player_character'] == '3', 'expected Yoshi'
    before = save_payload(seed)
    after_path = case / 'saves' / 'slot1.sav'
    after = save_payload(after_path)
    assert before[0x14:0x32] == after[0x14:0x32], 'saved star credit changed'
    assert seed.read_bytes() == after_path.read_bytes(), 'entry unexpectedly wrote save data'
    return dict(status='PASS', first_playable_ticks=first, final_tick=int(final['tick']),
                character='Yoshi', stars=0, saved_bytes_unchanged=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('case', type=Path)
    parser.add_argument('seed', type=Path)
    args = parser.parse_args()
    print(json.dumps(verify(args.case, args.seed), indent=2))
