"""Verify original course exit/reentry while native water bombs are active."""
import csv
from pathlib import Path
import re
import sys

from check_power_star_replay import save_payload


def check(case, slot_seed, minigame_seed):
    assert (case / 'exit-code.txt').read_text().strip() == '0', 'unclean game exit'
    log = (case / 'run.log').read_text(errors='replace')
    assert log.count('confirmed mission=1 level=6') == 2, 'mission1 not confirmed twice'
    events = dict((name, int(tick)) for name, tick in re.findall(
        r'\[native waterbomb replay\] event=([\w-]+) tick=(\d+)', log))
    names = ('observe', 'damage-and-fragments', 'exit-request', 'pause-request', 'castle-return',
             'course-reentry', 'reentry-complete')
    assert all(n in events for n in names), 'original exit/reentry incomplete'
    assert [events[n] for n in names] == sorted(events[n] for n in names), 'event order'
    with (case / 'waterbomb.csv').open(newline='') as f:
        rows = [{k: v if k == 'player_state' else int(v) for k, v in r.items()}
                for r in csv.DictReader(f)]
    start, castle, back = (events[n] for n in ('exit-request', 'castle-return', 'course-reentry'))
    observations = [(int(t), int(selected), int(phase)) for t, selected, phase in re.findall(
        r'\[native mission observation\] tick=(\d+) selected=(\d+) phase=(\d+) mode=0 confirm=0 ready=1', log)
        if castle < int(t) < back]
    assert any(selected > 0 for _, selected, _ in observations), 'reentry default mission was not observed'
    assert any(selected == 0 and phase == 2 for _, selected, phase in observations), 'mission1 highlight/navigation readiness not observed'
    assert any(castle < int(t) < back for t in re.findall(
        r'\[native mission replay\] tick=(\d+) selected=\d+ phase=\d+ buttons=20', log)), 'ordinary Left input not observed on reentry'
    first = [r for r in rows if r['level'] == 6 and r['tick'] < castle]
    active = [r for r in first if r['tick'] == start and r['kind'] == 208 and r['variant'] in (1, 2, 3)]
    assert active, 'exit did not begin with a moving bomb or fragment'
    assert any(r['tick'] == events['pause-request'] and r['kind'] == 208 and r['variant'] in (1, 2, 3)
               for r in first), 'no moving bomb or fragment when the pause input was sent'
    assert any(r['kind'] == 208 and r['variant'] == 2 for r in first), 'no original fragments before exit'
    players = {r['tick']: r for r in first if r['kind'] == 191}
    hits = []
    for r in first:
        if r['kind'] != 208 or r['variant'] not in (1, 2) or not r['other'] or r['other'] != r['player_id']:
            continue
        for t in range(r['tick'], r['tick'] + 4):
            if t in players and t - 1 in players and players[t - 1]['health'] - players[t]['health'] == 1:
                hits.append((r['variant'], t))
    assert {1, 2} <= {v for v, t in hits}, 'body and fragment damage not measured before exit'
    old_ids = {r['id'] for r in first if r['kind'] in (207, 208)}
    castle_rows = [r for r in rows if castle <= r['tick'] < back and r['level'] == 2]
    assert castle_rows and any(r['kind'] == 191 for r in castle_rows), 'no castle player samples'
    assert not any(r['kind'] in (207, 208) for r in castle_rows), 'old course enemies survived in castle'
    second = [r for r in rows if r['tick'] >= back]
    assert all(r['level'] == 6 for r in second), 'unexpected course change after reentry'
    assert not any(r['id'] in old_ids for r in second), 'old course actor ID survived reentry'
    droppers = {r['id'] for r in second if r['kind'] == 208 and r['variant'] == 0}
    cannons = {r['id'] for r in second if r['kind'] == 207 and r['variant'] == 0}
    assert len(droppers) == len(cannons) == 2, 'original mission1 enemies not recreated'
    launches = [r for r in second if r['kind'] == 208 and r['variant'] == 3]
    assert launches and any(r['vertical_speed'] > 0 for r in launches), 'recreated cannon did not fire'
    final = next(r for r in reversed(second) if r['kind'] == 191)
    assert final['tick'] >= events['reentry-complete'] + 300, 'insufficient gameplay after reentry'
    assert final['player_state'] == 'St_Wait_Main' and final['health'] > 0, 'final player did not recover'
    assert final['airborne'] == final['disabled'] == final['no_control'] == 0, 'final grounded control missing'
    with (case / 'state.csv').open(newline='') as f:
        state = list(csv.DictReader(f))
    assert any(r['pause_active'] == '1' and r['pause_selection'] == '3' for r in state), 'original Exit Course menu not used'
    assert log.count('[native painting] texture cache reset') >= 2, 'painting cache did not reset'
    assert log.count('[native painting] actor created live=6') >= 2, 'castle paintings not recreated'
    assert log.count('[native painting] actor destroyed live=0') >= 2, 'painting actors not released'
    assert save_payload(case / 'saves/slot1.sav') == save_payload(slot_seed), 'saved progression changed'
    assert (case / 'saves/slot1.sav').read_bytes() == slot_seed.read_bytes(), 'slot container changed'
    assert (case / 'saves/minigames.sav').read_bytes() == minigame_seed.read_bytes(), 'minigame save changed'
    print(f'PASS: active-projectile exit tick{start}, castle cleanup tick{castle}, reentry tick{back}')
    print('PASS: fresh droppers/cannons, cannon firing, painting reload, grounded control and unchanged saves')


if __name__ == '__main__':
    check(*(Path(p) for p in sys.argv[1:]))
