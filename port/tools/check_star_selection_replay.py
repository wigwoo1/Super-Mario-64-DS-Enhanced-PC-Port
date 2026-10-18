"""Check mission confirmation, real stage control, and untouched test saves."""
import argparse
import csv
from pathlib import Path

def verify(prefix, before, after, frames=2400):
    rows=list(csv.DictReader(Path(str(prefix)+'-state.csv').open()))
    log=Path(str(prefix)+'.log').read_text()
    assert Path(str(prefix)+'.exit-code.txt').read_text().strip()=='0', 'abnormal game exit'
    assert '[native mission selection] confirmed mission=1 level=6' in log, 'original scene did not confirm mission 1'
    assert rows and int(rows[-1]['tick'])==frames, 'replay did not finish'
    last=rows[-1]
    assert last['level']=='6' and last['player_character']=='3', 'Yoshi did not enter the selected course'
    assert last['player_state']=='St_Wait_Main' and last['player_no_control']=='0', 'normal control did not return'
    assert any(int(r['tick'])>2100 and r['level']=='6' and r['player_airborne']=='1' for r in rows), 'later jump not observed'
    assert last['power_star_total']=='7' and last['story_star_mask']=='2', 'progress changed'
    assert before.read_bytes()==after.read_bytes(), 'test save changed'
    return {'mission':1,'level':6,'character':'Yoshi','frames':frames,'save':'unchanged'}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('prefix',type=Path)
    p.add_argument('before',type=Path)
    p.add_argument('after',type=Path)
    p.add_argument('--frames',type=int,default=2400)
    a=p.parse_args()
    print('PASS:',verify(a.prefix,a.before,a.after,a.frames))
