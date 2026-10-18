"""Verify an actual conditional-star pickup and advancement of the live VS sequence.

Uses controller-replay observations, never injects a pickup or sequence value.
The encounter is the explicitly labelled, matching unused-map QA fixture.
"""
import argparse
import csv
import pathlib

from check_conditional_goomba_replay import verify as verify_release


def verify(rows, states, exit_code):
    result = verify_release(rows, states, exit_code, 'match', 0)
    star_id = result['new_stars'][0]
    star = [r for r in rows if r['id'] == star_id]
    collecting = [r for r in star if r['state'] in ('5', '6', '7')]
    assert collecting, 'released star never entered its collection states'
    pickup = int(collecting[0]['tick'])
    assert any(int(r['tick']) < pickup and int(r['hit']) & 0x400000 and
               int(r['other']) != 0 for r in star), 'no player contact before pickup'
    before = [r for r in rows if int(r['tick']) < pickup]
    after = [r for r in rows if int(r['tick']) >= pickup]
    assert all(r['active_star_index'] == '0' for r in before), 'sequence advanced before pickup'
    assert after and all(r['active_star_index'] == '1' for r in after), 'sequence did not advance exactly once on pickup'
    next_ids = {int(r['active_star_id']) for r in after}
    assert len(next_ids) == 1 and next(iter(next_ids)) in (1, 2, 3), 'invalid next ID for an original VS spawn-order row'
    assert all(r['silver_stars'] == '0' for r in before), 'silver-star credit predates pickup'
    credited = [r for r in after if r['silver_stars'] == '1']
    assert credited and all(int(r['silver_stars']) in (0, 1) for r in after), 'silver-star count did not increase exactly once'
    credit = min(int(r['tick']) for r in credited)
    assert all(r['silver_stars'] == '1' for r in after if int(r['tick']) >= credit), 'silver-star credit was lost'
    assert int(star[-1]['tick']) < result['frames'] - 100, 'collected star did not finish and disappear'
    assert any(pickup <= int(s['tick']) < credit and s['player_no_control'] != '0'
               for s in states), 'no original collection pose/control state'
    assert any(int(s['tick']) > credit + 100 and s['player_state'] == 'St_Jump_Main'
               for s in states), 'no controller jump after collection completed'
    return dict(result, pickup=pickup, credit=credit, next_star=next(iter(next_ids)),
                last_star_frame=int(star[-1]['tick']))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('trace', type=pathlib.Path)
    parser.add_argument('exit_code', type=pathlib.Path)
    args = parser.parse_args()
    state = args.trace.with_name(args.trace.name.replace('-rewards.csv', '-state.csv'))
    with args.trace.open(newline='') as stream:
        rows = list(csv.DictReader(stream))
    with state.open(newline='') as stream:
        states = list(csv.DictReader(stream))
    print('PASS:', verify(rows, states, int(args.exit_code.read_text(encoding='utf-8-sig').strip())))


if __name__ == '__main__':
    main()
