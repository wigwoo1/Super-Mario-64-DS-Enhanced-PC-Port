"""Verify a real large-Goomba ground pound, blue-coin drop and five-coin pickup."""
import argparse
import collections
import csv
import pathlib


def verify(rows, states, exit_code):
    assert exit_code == 0, 'game did not exit cleanly'
    assert rows and states, 'missing gameplay telemetry'
    groups = collections.defaultdict(list)
    timeline = {}
    for row in rows:
        groups[row['id']].append(row)
        timeline[int(row['tick'])] = int(row['course_coins'])
    end = max(timeline)
    initial = {r['id'] for r in rows if r['tick'] == rows[0]['tick']}
    victims = [rs for rs in groups.values() if rs[0]['kind'] == '202' and
               any(r['death'] == '1' and r['payload'] == '3' for r in rs)]
    assert victims, 'no large Goomba selected its ground-pound blue reward'
    coins = [rs for uid, rs in groups.items() if uid not in initial and
             rs[0]['kind'] == '290' and rs[0]['param'] == '242']
    assert coins, 'no new Goomba blue-coin reward'
    collected = []
    for coin in coins:
        spawn = int(coin[0]['tick'])
        parents = [enemy for enemy in victims if any(
            r['death'] == '1' and r['payload'] == '3' and
            0 <= spawn - int(r['tick']) <= 1 and
            abs(int(r['x']) - int(coin[0]['x'])) < 4096 and
            abs(int(r['z']) - int(coin[0]['z'])) < 4096
            for r in enemy)]
        if not parents:
            continue
        hit = min(int(r['tick']) for r in parents[0] if r['death'] == '1')
        assert any(hit - 20 <= int(s['tick']) <= hit + 10 and
                   s['player_state'] == 'St_GroundPound_Main' for s in states), 'no real player ground pound'
        last = int(coin[-1]['tick'])
        # A timed-out coin disappears too. Require its disappearance to coincide
        # with an exact +5 transition, and the player to be within pickup range.
        for tick in (last, last + 1):
            if tick not in timeline or tick - 1 not in timeline:
                continue
            if timeline[tick] - timeline[tick - 1] != 5:
                continue
            near = [s for s in states if abs(int(s['tick']) - tick) <= 10 and
                    sum((int(s[f'player_{axis}']) - int(coin[-1][axis])) ** 2
                        for axis in ('x', 'y', 'z')) <= (250 * 4096) ** 2]
            assert near, 'player was not near the collected blue coin'
            assert last < end - 100, 'no continuation after pickup'
            assert int(parents[0][-1]['tick']) < end - 100, 'defeated Goomba was not removed'
            collected.append((coin[0]['id'], tick))
    assert collected, 'blue coin was not collected for exactly five coins'
    level = next(s['level'] for s in states if int(s['tick']) >= int(rows[0]['tick']))
    tail = [s for s in states if int(s['tick']) >= end - 50]
    assert tail and all(s['level'] == level for s in tail), 'player left the test course'
    assert any(s['player_state'] == 'St_Wait_Main' and s['player_no_control'] == '0'
               and s['player_disabled'] == '0' for s in tail), 'normal control/idle did not return'
    pickup = min(t for _, t in collected)
    assert any(pickup + 100 < int(s['tick']) < end - 20 and
               s['player_state'] == 'St_Jump_Main' and s['player_no_control'] == '0'
               for s in states), 'no successful player jump after collection'
    return {'blue_coins': collected, 'course_coins_before': timeline[min(timeline)],
            'course_coins_after': timeline[end], 'frames': end}


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
    result = verify(rows, states, int(args.exit_code.read_text(encoding='utf-8-sig').strip()))
    print(f'PASS: large-Goomba ground pound, linked blue reward {result["blue_coins"]}, '
          f'coins {result["course_coins_before"]}->{result["course_coins_after"]}, '
          f'pickup +5, removal, later jump, same-course idle/control, {result["frames"]}frames exit0')


if __name__ == '__main__':
    main()
