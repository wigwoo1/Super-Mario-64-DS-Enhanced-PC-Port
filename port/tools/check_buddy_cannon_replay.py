"""Verify original mission 2 Buddy dialogue, shutter motion, and control return."""
import csv
import sys
from pathlib import Path


def check(case):
    assert (case / 'exit-code.txt').read_text().strip() == '0', 'game did not exit cleanly'
    log = (case / 'run.log').read_text(errors='replace')
    assert 'confirmed mission=2 level=6' in log, 'wrong mission'
    assert '[native Buddy replay] completed' in log and 'cannon=1' in log, 'cannon conversation did not finish'
    rows = list(csv.DictReader((case / 'buddy.csv').open()))
    buddy = [r for r in rows if r['kind'] == '181' and r['variant'] == '1']
    assert buddy and buddy[0]['opened'] == '0', 'cannon was already open'
    assert any(r['message'] != '0' for r in buddy), 'no original dialogue'
    assert {0, 1, 2, 3, 4}.issubset({int(r['step']) for r in buddy}), 'cutscene steps missing'
    shutters = [r for r in rows if r['kind'] == '14' and r['id'] == next(r['shutter'] for r in buddy if r['shutter'] != '0')]
    assert shutters[0]['opened'] == '0' and shutters[-1]['opened'] == '1', 'shutter never opened'
    assert int(shutters[-1]['y']) < int(shutters[0]['y']), 'shutter did not descend'
    assert abs(int(shutters[-1]['z']) - int(shutters[0]['z'])) > 200 * 4096, 'shutter did not slide clear'
    assert buddy[-1]['opened'] == '1' and buddy[-1]['state'] == '0', 'Buddy did not return to idle'
    assert int(buddy[-1]['cannon_flags']) & 1, 'live course cannon flag missing'
    states = list(csv.DictReader((case / 'state.csv').open()))
    last = states[-1]
    assert last['level'] == '6' and last['player_no_control'] == last['player_disabled'] == '0', 'player control did not return'
    assert last['player_airborne'] == '0', 'player did not finish grounded'
    print('PASS: mission 2, original dialogue, shutter descent/slide, camera return, live cannon flag, player controls')


if __name__ == '__main__':
    check(Path(sys.argv[1]))
