"""Verify three input-only cannon rides across an original course exit/reentry."""
import csv
import math
import re
import sys
from pathlib import Path

from check_cannon_replay import angle_difference
from check_power_star_replay import save_payload


def read_rows(path):
    with path.open(newline='') as stream:
        return list(csv.DictReader(stream))


def check(case, seed):
    assert (case / 'exit-code.txt').read_text().strip() == '0', 'unclean runtime exit'
    log = (case / 'run.log').read_text(errors='replace')
    pattern = r'\[native cannon cycle\] visit=(\d+) ride=(\d+) event=([\w-]+) tick=(\d+) id=(\d+)'
    events = [(int(v), int(r), name, int(t), ident)
              for v, r, name, t, ident in re.findall(pattern, log)]
    rows = read_rows(case / 'cannon.csv')
    state = read_rows(case / 'state.csv')
    landed = [e for e in events if e[2] == 'grounded-idle']
    assert [(e[0], e[1]) for e in landed] == [(1, 1), (1, 2), (2, 1)], 'three rides not completed'
    assert landed[0][4] == landed[1][4], 'repeat did not use the same cannon'
    assert landed[2][4] != landed[0][4], 'course did not recreate its cannon actor'
    previous = 0
    for visit, ride, _, end, ident in landed:
        def event_tick(name):
            matches = [e[3] for e in events if e[:3] == (visit, ride, name)]
            assert len(matches) == 1, f'ride {visit}/{ride}: missing/duplicate {name}'
            return matches[0]
        aim_tick, launch_tick = event_tick('aim'), event_tick('launch')
        assert previous < aim_tick < launch_tick < end, 'ride event order'
        cannon = [r for r in rows if r['id'] == ident and r['kind'] == '207'
                  and previous < int(r['tick']) <= end]
        load = [r for r in cannon if r['state'] == '3']
        assert {0, 1, 2, 3, 4} <= {int(r['step']) for r in load}, 'incomplete cannon sequence'
        assert all(r['player_state'] == 'St_Cannon_Main' and r['control_disabled'] == '1'
                   for r in load if int(r['step']) < 4), 'loading control handoff'
        aim = [r for r in load if r['step'] == '3']
        assert len(aim) >= 6, 'aim view too short'
        assert angle_difference(aim[0]['camera_yaw'], aim[-1]['camera_yaw']) > 0x200, 'horizontal aim'
        assert int(aim[-1]['camera_pitch']) < int(aim[0]['camera_pitch']) - 0x400, 'vertical aim'
        assert angle_difference(aim[-1]['yaw'], int(aim[-1]['camera_yaw']) + 0x4000) < 0x100, 'barrel heading'
        flight = [r for r in cannon if r['player_state'] == 'St_Cannon_Main' and r['player_step'] == '2']
        assert flight and int(flight[0]['vy']) > 0 and int(flight[0]['speed']) > 0, 'launch velocity'
        assert flight[0]['control_disabled'] == '0', 'launch controls'
        early = [r for r in flight if int(r['tick']) <= int(flight[0]['tick']) + 20]
        assert len(early) >= 2, 'missing trajectory samples'
        dx = int(early[-1]['px']) - int(early[0]['px'])
        dz = int(early[-1]['pz']) - int(early[0]['pz'])
        assert math.hypot(dx, dz) > 150 * 4096, 'no flight travel'
        heading = round(math.atan2(dx, dz) * 65536 / (2 * math.pi))
        assert angle_difference(heading, int(aim[-1]['camera_yaw']) + 0x8000) < 0x400, 'aimed trajectory'
        assert any(int(r['tick']) > launch_tick and r['state'] == '2' and r['pitch'] == '0'
                   for r in cannon), 'cannon did not reset'
        idle = [r for r in state if end <= int(r['tick']) <= end + 20]
        assert any(r['player_state'] == 'St_Wait_Main' and
                   r['player_airborne'] == r['player_disabled'] == r['player_no_control'] == '0'
                   for r in idle), 'grounded controls not restored'
        print(f'PASS: visit {visit}, ride {ride}, cannon {ident}, aim/launch/reset/landing')
        previous = end
    castle = next(e[3] for e in events if e[2] == 'castle-return')
    reentry = next(e[3] for e in events if e[2] == 'course-reentry')
    assert landed[1][3] < castle < reentry < landed[2][3], 'exit/reentry order'
    assert any(r['level'] == '2' and castle <= int(r['tick']) < reentry for r in state), 'no castle visit'
    assert any(r['pause_active'] == '1' and r['pause_selection'] == '3' for r in state), 'original Exit Course not selected'
    assert log.count('confirmed mission=2 level=6') == 2, 'original mission selection was not used twice'
    assert '[native painting] texture cache reset' in log, 'painting cache not reset on return'
    assert log.count('[native painting] actor created live=6') >= 2, 'painting actors not recreated'
    assert log.count('[native painting] actor destroyed live=0') >= 2, 'painting actors not cleaned up'
    assert all(r['id'] != landed[0][4] for r in rows if int(r['tick']) >= reentry), 'old actor survives reentry'
    buddy = read_rows(case / 'buddy.csv')
    shutters = [r for r in buddy if r['kind'] == '14' and r['param'] == '65281']
    assert shutters and all(r['opened'] == '1' for r in shutters), 'saved cannon shutter did not stay open'
    assert all(int(r['cannon_flags']) & 1 for r in buddy), 'saved cannon flag lost'
    assert any(e[2] == 'complete' for e in events), 'cycle incomplete'
    assert state[-1]['level'] == '6' and state[-1]['player_state'] == 'St_Wait_Main', 'final gameplay not idle'
    assert save_payload(case / 'saves/slot1.sav') == save_payload(seed), 'saved progression changed'
    print('PASS: original exit/reentry, fresh actor, persistent open cannon, unchanged save')


if __name__ == '__main__':
    check(*(Path(p) for p in sys.argv[1:]))
