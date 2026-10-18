"""Verify a real repeat Power Star pickup gives no additional save credit."""
import argparse
import csv
import json
from pathlib import Path
from check_power_star_replay import save_payload

def verify(rows, states, code, before, after, log):
    assert code == 0, 'game did not exit normally'
    assert before[0x14] == 2, 'seed must already contain star ID 1'
    total = sum(x.bit_count() for x in before[0x14:0x32])
    assert any(r['kind']=='178' and r['param']=='7' and r['state']=='4' for r in rows), 'original map star absent'
    star = [r for r in rows if r['kind']=='178' and r['param']=='1']
    assert len({r['id'] for r in star}) == 1, 'expected one repeat fixture'
    assert star[0]['star_id']=='1' and int(star[0]['payload']) & 4, 'star did not recognize prior collection'
    pickup = min(int(r['tick']) for r in star if r['state']=='5')
    assert any(int(r['tick']) < pickup and int(r['hit']) & 0x400000 and r['other']=='3' for r in star), 'missing real player contact'
    assert {'5','6','7'} <= {r['state'] for r in star}, 'incomplete celebration'
    assert all(r['save_star_mask']=='2' and int(r['power_star_total'])==total for r in rows), 'duplicate or lost live credit'
    assert before[0x14:0x32] == after[0x14:0x32], 'saved course progress changed'
    assert before[0x41] == after[0x41], 'saved character changed'
    assert '[native save] slot=0 write=committed bytes=68' in log, 'no save commit'
    assert 'Save failed' not in log, 'save failure'
    exits = [s for s in states if int(s['tick'])>pickup and s['level']=='1']
    assert exits and any(s['level']=='6' and pickup-20<=int(s['tick'])<=pickup for s in states), 'no course exit'
    assert any(s['level']=='1' and s['player_state']=='St_Jump_Main' and int(s['tick'])>pickup+150 for s in states), 'no later castle jump'
    final = states[-1]
    assert final['level']=='1' and final['player_state']=='St_Wait_Main' and final['player_no_control']==final['player_disabled']=='0', 'control did not return'
    return dict(pickup=pickup, course_exit=int(exits[0]['tick']), final_frame=int(final['tick']), total_before=total, total_after=sum(x.bit_count() for x in after[0x14:0x32]), course_mask=after[0x14])

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('prefix','seed','saved'): p.add_argument(name,type=Path)
    a=p.parse_args(); prefix=a.prefix
    rows=list(csv.DictReader(Path(str(prefix)+'-rewards.csv').open()))
    states=list(csv.DictReader(Path(str(prefix)+'-state.csv').open()))
    base=prefix.parent.parent/prefix.name
    result=verify(rows,states,int(Path(str(base)+'.exit-code.txt').read_text(encoding='utf-8-sig')),save_payload(a.seed),save_payload(a.saved),Path(str(base)+'.log').read_text())
    print(json.dumps(result,indent=2))

if __name__=='__main__': main()
