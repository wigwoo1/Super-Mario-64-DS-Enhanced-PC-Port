"""Verify a second native King Bob-omb reward collection adds no credit."""
import argparse
import csv
from pathlib import Path
from check_power_star_replay import save_payload


def verify(boss_rows,rewards,states,exit_code,before,after,log):
    assert exit_code==0,'game did not exit normally'
    assert before[0x14]==2 and sum(v.bit_count() for v in before[0x14:0x32])==7,'repeat test must start with this Star credited'
    assert boss_rows and rewards and states,'missing repeat observations'
    bosses=[r for r in boss_rows if r['kind']=='189']
    assert len({r['id'] for r in bosses})==1 and all(r['param']=='65281' for r in bosses),'wrong original boss'
    health=[]
    for r in bosses:
        if not health or r['health']!=health[-1]['health']:health.append(r)
    assert [r['health'] for r in health]==['3','2','1','0'],'boss did not complete the three-hit fight'
    hits=[]
    for change in health[1:]:
        tick=int(change['tick']);previous=next((r for r in bosses if int(r['tick'])==tick-1),None)
        assert previous and int(change['state'])==0x02126ffc,'missing original boss damage state'
        bomb_id=previous['other2']
        assert int(previous['hit2'])&0x4000 and bomb_id!='0','missing Bob-omb collision'
        bombs=[r for r in boss_rows if r['kind']=='206' and r['id']==bomb_id and r['param']=='2']
        assert bombs and any(r['mouth']==bomb_id and int(r['tick'])<tick for r in bosses),'Yoshi did not catch this spawned boss bomb'
        assert any(r['mouth']=='0' and r['other']==change['id'] and int(r['hit'])&0x200000 and tick-8<=int(r['tick'])<tick for r in bombs),'no spat-bomb hit'
        assert any(bomb_id in (r['spawn0'],r['spawn1']) for r in bosses),'bomb not linked to native boss spawn'
        hits.append(tick)
    defeat=hits[-1]
    assert any(int(r['state'])==0x0212705c for r in bosses if int(r['tick'])>defeat),'defeat state missing'
    stars=[r for r in rewards if r['kind']=='178' and r['star_id']=='1']
    assert len({r['id'] for r in stars})==1,'expected one repeat boss Star'
    assert all(r['param']=='65' and r['star_kind']=='4' for r in stars),'wrong original reward actor'
    assert int(stars[0]['payload'])&4,'boss reward did not recognize the previously saved Star'
    spawn=int(stars[0]['tick']);assert spawn>defeat,'reward did not follow the boss fight'
    collected=[r for r in stars if r['state']=='5'];assert collected,'repeat reward was not picked up'
    pickup=int(collected[0]['tick'])
    assert any(int(r['hit'])&0x400000 and r['other']!='0' and int(r['tick'])<pickup for r in stars),'no player contact before pickup'
    assert {'5','6','7'}<={r['state'] for r in stars},'repeat pickup celebration incomplete'
    assert all(int(r['power_star_total'])==7 and int(r['save_star_mask'])==2 for r in stars),'duplicate or lost live Star credit'
    assert before[0x14:0x32]==after[0x14:0x32],'saved course Star masks changed on repeat'
    assert before[0x41]==after[0x41],'saved character changed'
    assert sum(v.bit_count() for v in after[0x14:0x32])==7,'saved Star total changed on repeat'
    assert '[native save] slot=0 write=committed bytes=68' in log and '[native save] Save failed' not in log,'repeat save was not committed cleanly'
    exits=[r for r in states if r['level']=='1' and int(r['tick'])>pickup]
    assert exits and any(r['player_state']=='St_Jump_Main' and r['level']=='1' and int(r['tick'])>pickup+150 for r in states),'castle return/control regression'
    final=states[-1]
    assert final['level']=='1' and final['player_state']=='St_Wait_Main' and final['player_no_control']==final['player_disabled']=='0','normal castle idle did not return'
    assert final['power_star_total']=='7','repeat changed the displayed total'
    return dict(boss=bosses[0]['id'],hits=hits,star=stars[0]['id'],spawn=spawn,pickup=pickup,course_exit=int(exits[0]['tick']),saved_total=7)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('prefix',type=Path);p.add_argument('seed_save',type=Path);p.add_argument('saved_file',type=Path)
    a=p.parse_args();prefix=a.prefix
    def rows(suffix):
        with Path(str(prefix)+suffix).open(newline='') as f:return list(csv.DictReader(f))
    base=prefix.parent.parent/prefix.name
    log=base.with_suffix('.log')
    code=base.with_suffix('.exit-code.txt')
    if not log.is_file():log=Path(str(prefix)+'.log')
    if not code.is_file():code=Path(str(prefix)+'.exit-code.txt')
    print('PASS:',verify(rows('-boss.csv'),rows('-rewards.csv'),rows('-state.csv'),
        int(code.read_text(encoding='utf-8-sig')),save_payload(a.seed_save),
        save_payload(a.saved_file),log.read_text()))


if __name__=='__main__':main()
