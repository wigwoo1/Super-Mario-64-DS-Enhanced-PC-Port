"""Check Power Star collection/save before returning to a live menu scene.

The final title artwork is also inspected in actual captured screenshots.
"""
import csv,json,sys,re
from pathlib import Path
from check_power_star_replay import save_payload

def verify(rows, states, code, before, after, log):
    assert code==0, 'game failed to exit normally'
    star=[r for r in rows if r['kind']=='178' and r['param']=='1']
    assert len({r['id'] for r in star})==1, 'missing single Power Star fixture'
    assert before[0x14]==0 and after[0x14]==2, 'star credit not saved'
    assert before[0x15:0x32]==after[0x15:0x32], 'other course progress changed'
    total=sum(x.bit_count() for x in before[0x14:0x32])
    pickup=min(int(r['tick']) for r in star if r['state']=='5')
    assert any(int(r['tick'])<pickup and int(r['hit'])&0x400000 and r['other']=='3' for r in star), 'no player contact'
    assert {'5','6','7'}<={r['state'] for r in star}, 'missing celebration'
    for r in star:
        taken=int(r['tick'])>=pickup
        assert int(r['save_star_mask'])==(2 if taken else 0), 'wrong live course mask'
        assert int(r['power_star_total'])==total+int(taken), 'wrong live star total'
    assert '[native save] slot=0 write=committed bytes=68' in log and 'Save failed' not in log, 'save not committed'
    assert any(s['level']=='1' and int(s['tick'])>pickup for s in states), 'no course return'
    final=states[-1];end=int(final['tick'])
    menu=[s for s in states if int(s['tick'])>=end-100]
    assert len(menu)>=5 and all(s['actors']=='0' and s['player_character']=='255' for s in menu), 'player/stage actors survived quit'
    captures=re.findall(r'\[native capture\] tick=(\d+).*triangles=(\d+)',log)
    assert any(int(t)>=3000 and int(n)>0 for t,n in captures), 'no rendered title after quit'
    assert log.count('[native save] slot=0 loaded=primary bytes=68')>=2, 'title did not reload saved file'
    assert end>=3000, 'replay ended before title could settle'
    return dict(pickup=pickup,final_frame=end,total_before=total,total_after=total+1,course_mask=after[0x14],final_actors=int(final['actors']),screenshot_inspection_required=True)

def main():
    prefix=Path(sys.argv[1]);base=prefix.parent.parent/prefix.name
    r=list(csv.DictReader(Path(str(prefix)+'-rewards.csv').open()));s=list(csv.DictReader(Path(str(prefix)+'-state.csv').open()))
    result=verify(r,s,int(Path(str(base)+'.exit-code.txt').read_text(encoding='utf-8-sig')),save_payload(Path(sys.argv[2])),save_payload(Path(sys.argv[3])),Path(str(base)+'.log').read_text())
    print(json.dumps(result,indent=2))
if __name__=='__main__':main()
