"""Check repeated Buddy talks, original castle save, reentry and fresh-process reload."""
import csv
import re
import struct
import sys
from pathlib import Path

from check_power_star_replay import save_payload


def read_case(case, expect_course=True):
    assert (case / 'exit-code.txt').read_text().strip() == '0', 'game did not exit cleanly'
    log = (case / 'run.log').read_text(errors='replace')
    states = list(csv.DictReader((case / 'state.csv').open()))
    actors = list(csv.DictReader((case / 'buddy.csv').open()))
    final = states[-1]
    if expect_course:
        assert final['level'] == '6', 'replay did not finish in Battlefield'
        assert final['player_no_control'] == final['player_disabled'] == final['player_airborne'] == '0', 'normal grounded controls missing'
    assert actors and all(int(r['cannon_flags']) & 1 for r in actors[-9:]), 'cannon flag missing at end'
    return log, states, actors


def check(case, reload_case, seed):
    log, states, actors = read_case(case, expect_course=False)
    events = re.findall(r'\[native Buddy repeat\] visit=(\d+) interaction=(\d+) complete tick=(\d+)', log)
    assert {(v, n) for v, n, _ in events} >= {('1', '1'), ('1', '2')}, 'repeat conversations missing'
    assert log.count('confirmed mission=2 level=6') == 1, 'original mission selection required'
    assert '[native save] slot=0 write=committed bytes=68' in log, 'original save did not reach disk'
    assert '[native Buddy persistence] saved; awaiting fresh-process reload' in log, 'save-and-quit flow missing'
    first_end = int(next(t for v, n, t in events if (v, n) == ('1', '2')))
    assert any(r['level'] == '2' and int(r['tick']) > first_end for r in states), 'castle visit missing'
    assert states[-1]['player_state'] == 'none', 'Save & Quit did not leave gameplay'
    # The shutter must start closed in the first run and open when recreated
    # in a fresh process, before any new conversation can affect it.
    first_shutters = [r for r in actors if r['kind'] == '14' and r['param'] == '65281']
    groups = {}
    for r in first_shutters:
        groups.setdefault(r['id'], []).append(r)
    assert len(groups) == 1, 'unexpected course visits before saving'
    old = next(iter(groups.values()))
    assert old[0]['opened'] == '0' and old[-1]['opened'] == '1', 'first shutter did not open'
    before = save_payload(seed)
    saved = save_payload(case / 'saves/slot1.sav')
    assert struct.unpack_from('<I', before, 0x10)[0] == 0, 'seed cannon already open'
    assert struct.unpack_from('<I', saved, 0x10)[0] == 1, 'cannon flag not saved'
    assert before[0x14:0x32] == saved[0x14:0x32], 'star progress changed'
    assert before[0x41] == saved[0x41], 'saved character changed'
    restart_log, _, restart_actors = read_case(reload_case)
    assert 'confirmed mission=2 level=6' in restart_log, 'reload did not enter original mission 2'
    assert 'loaded=primary bytes=68' in restart_log, 'fresh process did not load the save'
    assert 'visit=1 interaction=2 complete' in restart_log, 'fresh-process repeat talk missing'
    restart_shutters = [r for r in restart_actors if r['kind'] == '14' and r['param'] == '65281']
    assert restart_shutters and all(r['opened'] == '1' for r in restart_shutters), 'disk-reloaded cannon did not stay open'
    assert all(r['state'] != '2' for r in restart_actors if r['kind'] == '181' and r['variant'] == '1'), 'already-open cannon replayed its opening cutscene'
    assert save_payload(reload_case / 'saves/slot1.sav') == saved, 'reload/repeat unexpectedly changed saved progress'
    print('PASS: two talks per visit, original save, course reentry, fresh-process load, open shutter and restored controls')


if __name__ == '__main__':
    check(*(Path(p) for p in sys.argv[1:]))
