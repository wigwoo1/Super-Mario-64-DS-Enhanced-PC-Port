"""Check the original-menu, first-Buddy talk/retreat collision regression."""
import csv
import sys
from pathlib import Path


def check(case):
    assert (case / 'exit-code.txt').read_text().strip() == '0', 'game did not exit cleanly'
    log = (case / 'run.log').read_text(errors='replace')
    assert 'confirmed mission=1 level=6' in log, 'original mission selection missing'
    assert '[native Buddy replay] completed' in log, 'conversation did not finish'
    rows = list(csv.DictReader((case / 'state.csv').open()))
    ground = [r for r in rows if int(r['tick']) >= 4500 and r['level'] == '6']
    assert len(ground) >= 150, 'need 1500 ticks of grounded course coverage'
    assert all(int(r['player_y']) == 0 for r in ground), 'Yoshi floated above the starting field'
    assert any(r['player_state'] == 'St_Talk_Main' for r in ground), 'no real dialogue state'
    last = ground[-1]
    assert last['player_no_control'] == last['player_disabled'] == '0', 'control did not return'
    assert last['player_airborne'] == '0', 'Yoshi did not finish grounded'
    print('PASS: original course entry, grounded Buddy conversation, retreat, and restored controls')


if __name__ == '__main__':
    check(Path(sys.argv[1]))
