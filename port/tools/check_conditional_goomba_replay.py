"""Check a confirmed ground-pound hit and the live conditional silver-star gate."""
import argparse
import collections
import csv
import pathlib


def verify(rows, states, exit_code, mode, star_id):
    assert exit_code == 0, 'game did not exit cleanly'
    assert rows and states, 'missing gameplay evidence'
    groups = collections.defaultdict(list)
    for row in rows:
        groups[row['id']].append(row)
    end = max(int(r['tick']) for r in rows)
    initial = {r['id'] for r in rows if r['tick'] == rows[0]['tick']}
    enemies = [rs for rs in groups.values() if rs[0]['kind'] == '200' and
               rs[0]['payout_bits'] == '2' and int(rs[0]['target']) == star_id]
    assert len(enemies) == 1, 'expected exactly one conditional Goomba'
    enemy = enemies[0]
    hits = [r for r in enemy if r['death'] == '1' and int(r['last_hit']) & 0x20]
    assert hits, 'no confirmed spin/ground-pound hit on the conditional Goomba'
    hit = int(hits[0]['tick'])
    active = int(hits[0]['active_star_id'])
    assert active >= 0, 'active VS star was not observed'
    assert all(int(r['active_star_id']) == active for r in enemy if int(r['tick']) <= hit), 'active star changed before the hit'
    assert (star_id == active) == (mode == 'match'), 'live star IDs do not satisfy the requested case'
    assert any(hit - 10 <= int(s['tick']) <= hit + 10 and
               s['player_state'] == 'St_GroundPound_Main' for s in states), 'no player ground pound near enemy hit'
    stars = [rs for uid, rs in groups.items() if uid not in initial and rs[0]['kind'] == '179']
    markers = [rs for uid, rs in groups.items() if uid not in initial and rs[0]['kind'] == '180']
    if mode == 'match':
        assert len(stars) == len(markers) == 1, 'matching hit did not release exactly one star and marker'
        star, marker = stars[0], markers[0]
        for reward in (star, marker):
            assert int(reward[0]['param']) == 0x30 | star_id, 'wrong conditional reward parameter'
            spawn = int(reward[0]['tick'])
            assert hit <= spawn <= hit + 100, 'reward spawn unrelated to hit'
            assert any(abs(int(r['tick']) - spawn) <= 1 and
                       abs(int(r['x']) - int(reward[0]['x'])) <= 4096 and
                       abs(int(r['z']) - int(reward[0]['z'])) <= 4096 for r in enemy), 'reward did not spawn at its parent'
        assert any(r['payout_bits'] == '3' and int(r['param']) == int(enemy[0]['param']) & 0xff0f
                   for r in enemy), 'Goomba did not mark the conditional reward spent'
        assert any(r['target'] == marker[0]['id'] for r in star), 'silver star did not find its marker'
        assert any(r['target'] == star[0]['id'] for r in marker), 'marker did not link back to its star'
    else:
        assert not stars and not markers, 'non-matching hit released a silver reward'
        assert all(r['payout_bits'] == '2' and r['param'] == enemy[0]['param'] for r in enemy), 'non-matching reward was incorrectly spent'
        assert int(enemy[-1]['tick']) < end - 100, 'no continuation after enemy defeat'
    level = next(s['level'] for s in states if int(s['tick']) >= int(rows[0]['tick']))
    tail = [s for s in states if int(s['tick']) >= end - 50]
    assert tail and all(s['level'] == level for s in tail), 'test left the encounter course'
    final=tail[-1]
    assert final['player_state'] == 'St_Wait_Main' and final['player_no_control'] == '0' and \
        final['player_disabled'] == '0', 'normal control/idle did not return'
    assert any(int(s['tick']) > hit + 150 and s['player_state'] == 'St_Jump_Main' for s in states), 'no later jump after reward processing'
    return dict(mode=mode, goomba=enemy[0]['id'], hit=hit, active_star=active,
                goomba_star=star_id, new_stars=[rs[0]['id'] for rs in stars], frames=end)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('mode', choices=('match', 'mismatch'))
    p.add_argument('star_id', type=int)
    p.add_argument('trace', type=pathlib.Path)
    p.add_argument('exit_code', type=pathlib.Path)
    args = p.parse_args()
    state = args.trace.with_name(args.trace.name.replace('-rewards.csv', '-state.csv'))
    with args.trace.open(newline='') as stream:
        rows = list(csv.DictReader(stream))
    with state.open(newline='') as stream:
        states = list(csv.DictReader(stream))
    result = verify(rows, states, int(args.exit_code.read_text(encoding='utf-8-sig').strip()), args.mode, args.star_id)
    print('PASS:', result)


if __name__ == '__main__':
    main()
