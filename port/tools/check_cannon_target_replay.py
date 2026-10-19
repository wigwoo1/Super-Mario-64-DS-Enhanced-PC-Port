"""Verify entry, aim, firing and landing from a selected native Battlefield cannon."""
import csv
import math
import re
import sys
from pathlib import Path

from check_cannon_replay import angle_difference
from check_power_star_replay import save_payload


def check(case, seed, expected_id, require_cap_intro=False):
    assert (case / 'exit-code.txt').read_text().strip() == '0', 'unclean runtime exit'
    log = (case / 'run.log').read_text(errors='replace')
    pattern = r'\[native cannon cycle\] visit=(\d+) ride=(\d+) event=([\w-]+) tick=(\d+) id=(\d+)'
    events = [(int(v), int(r), name, int(t), int(ident))
              for v, r, name, t, ident in re.findall(pattern, log)]
    aim = [e for e in events if e[2] == 'aim']
    launch = [e for e in events if e[2] == 'launch']
    landed = [e for e in events if e[2] == 'grounded-idle']
    assert len(aim) == len(launch) == len(landed) == 1, 'cannon ride did not complete exactly once'
    assert aim[0][4] == launch[0][4] == landed[0][4] == expected_id, 'wrong cannon selected'
    assert aim[0][3] < launch[0][3] < landed[0][3], 'ride event order'
    rows = list(csv.DictReader((case / 'cannon.csv').open(newline='')))
    cannon = [r for r in rows if r['kind'] == '207' and int(r['id']) == expected_id]
    assert cannon and all(r['variant'] == '2' for r in cannon), 'selected actor is not rideable'
    assert all(r['level'] == '6' for r in cannon), 'selected cannon was not in Battlefield'
    loading = [r for r in cannon if r['state'] == '3']
    assert {0, 1, 2, 3, 4} <= {int(r['step']) for r in loading}, 'cannon loading steps incomplete'
    assert all(r['player_state'] == 'St_Cannon_Main' and r['control_disabled'] == '1'
               for r in loading if int(r['step']) < 4), 'loading control handoff failed'
    aiming = [r for r in loading if r['step'] == '3']
    assert len(aiming) >= 6, 'aiming view was not held'
    assert angle_difference(aiming[0]['camera_yaw'], aiming[-1]['camera_yaw']) > 0x200, 'horizontal aiming input had no effect'
    assert int(aiming[-1]['camera_pitch']) < int(aiming[0]['camera_pitch']) - 0x400, 'vertical aiming input had no effect'
    assert angle_difference(aiming[-1]['yaw'], int(aiming[-1]['camera_yaw']) + 0x4000) < 0x100, 'cannon barrel aim mismatch'
    flight = [r for r in cannon if r['player_state'] == 'St_Cannon_Main' and r['player_step'] == '2']
    assert flight and int(flight[0]['vy']) > 0 and int(flight[0]['speed']) > 0 and flight[0]['control_disabled'] == '0', 'launch handoff missing'
    early = [r for r in flight if int(r['tick']) <= int(flight[0]['tick']) + 20]
    assert len(early) >= 2, 'flight trajectory missing'
    dx = int(early[-1]['px']) - int(early[0]['px'])
    dz = int(early[-1]['pz']) - int(early[0]['pz'])
    assert math.hypot(dx, dz) > 150 * 4096, 'Yoshi did not travel from selected cannon'
    heading = round(math.atan2(dx, dz) * 65536 / (2 * math.pi))
    assert angle_difference(heading, int(aiming[-1]['camera_yaw']) + 0x8000) < 0x400, 'shot did not follow aimed direction'
    assert any(int(r['tick']) > launch[0][3] and r['state'] == '2' and r['pitch'] == '0' for r in cannon), 'cannon did not reset'
    state = list(csv.DictReader((case / 'state.csv').open(newline='')))
    cap_form = [r for r in state if r['save_character'] == r['player_character'] == '3'
                and r['selected_character'] == r['player_body_model'] == r['player_hat_character'] == '0']
    if require_cap_intro:
        assert cap_form, 'Yoshi Mario-cap pickup not exercised'
    if cap_form:
        assert any(r['player_state'] == 'St_NoControl_Main' for r in cap_form), 'Mario cap intro not observed'
        assert any(r['player_no_control'] == r['player_disabled'] == '0'
                   and int(r['tick']) < aim[0][3] for r in cap_form), 'Mario cap intro did not return control before boarding'
        print('PASS: Yoshi Mario-cap intro completes and returns control before the cannon ride')
    assert any(r['player_state'] == 'St_Wait_Main' and r['player_airborne'] == r['player_disabled'] == r['player_no_control'] == '0'
               for r in state if int(r['tick']) >= landed[0][3]), 'normal grounded control not restored'
    assert save_payload(case / 'saves/slot1.sav') == save_payload(seed), 'game progress changed during ride'
    final = state[-1]
    assert final['level'] == '6' and final['player_state'] == 'St_Wait_Main' and final['player_no_control'] == final['player_disabled'] == '0', 'final grounded gameplay not restored'
    print(f'PASS: cannon {expected_id} approach, aim, fire, trajectory, reset, landing, and unchanged save')


if __name__ == '__main__':
    check(Path(sys.argv[1]), Path(sys.argv[2]), int(sys.argv[3]), '--cap-intro' in sys.argv[4:])
