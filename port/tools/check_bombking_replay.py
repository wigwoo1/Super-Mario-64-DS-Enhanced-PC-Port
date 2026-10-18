"""Verify the original summit boss, three caught/spat bombs and saved reward.

The labelled QA travel moves Yoshi to the summit once. All boss damage,
defeat, reward spawning, collection and saving must follow game code.
"""
import argparse
import csv
from pathlib import Path
from check_power_star_replay import save_payload


def verify(boss_rows, rewards, states, exit_code, before, after, log):
    assert exit_code == 0, 'game did not finish normally'
    assert 'summit travel:' in log, 'missing labelled summit travel'
    assert '[native QA fixture] Power Star' not in log, 'reward was artificially placed'
    assert boss_rows and rewards and states, 'missing observations'
    bosses=[r for r in boss_rows if r['kind']=='189']
    assert len({r['id'] for r in bosses})==1, 'expected one original map boss'
    assert all(r['param']=='65281' for r in bosses), 'wrong original mission boss'
    health=[]
    for r in bosses:
        if not health or r['health']!=health[-1]['health']:health.append(r)
    assert [r['health'] for r in health]==['3','2','1','0'], 'fight did not take three hits without resetting'
    hits=[]
    for change in health[1:]:
        tick=int(change['tick'])
        assert int(change['state'])==0x02126ffc, 'damage did not enter original hit state'
        previous=next((r for r in bosses if int(r['tick'])==tick-1),None)
        assert previous, 'missing pre-hit observation'
        bomb_id=previous['other2']
        assert int(previous['hit2'])&0x4000 and bomb_id!='0', 'no real Bob-omb collision before damage'
        bombs=[r for r in boss_rows if r['kind']=='206' and r['id']==bomb_id and r['param']=='2']
        assert bombs, 'hit did not come from a native boss Bob-omb'
        assert any(r['mouth']==bomb_id and int(r['tick'])<tick for r in bosses), 'Yoshi never caught this bomb'
        assert any(r['mouth']=='0' and r['other']==change['id'] and int(r['hit'])&0x200000 and tick-8<=int(r['tick'])<tick
                   for r in bombs), 'no spat-bomb impact'
        assert any(bomb_id in (r['spawn0'],r['spawn1']) for r in bosses), 'boss never spawned this bomb'
        hits.append(tick)
    defeat=hits[-1]
    assert any(int(r['state'])==0x0212705c and r['message_active']=='1' for r in bosses if int(r['tick'])>defeat), 'defeat dialogue did not run'
    stars=[r for r in rewards if r['kind']=='178' and r['star_id']=='1']
    assert len({r['id'] for r in stars})==1, 'expected one boss reward'
    assert all(r['param']=='65' and r['star_kind']=='4' for r in stars), 'wrong original boss reward kind'
    spawn=int(stars[0]['tick'])
    assert spawn>defeat and int(bosses[-1]['tick'])<=spawn, 'reward not linked to boss defeat/removal'
    collected=[r for r in stars if r['state']=='5']
    assert collected, 'reward was never collected'
    pickup=int(collected[0]['tick'])
    assert any(int(r['hit'])&0x400000 and r['other']!='0' and int(r['tick'])<pickup for r in stars), 'no player contact before pickup'
    assert {'5','6','7'}<={r['state'] for r in stars}, 'collection sequence did not finish'
    for r in stars:
        taken=int(r['tick'])>=pickup
        assert int(r['save_star_mask'])==(2 if taken else 0), 'wrong mission reward bit'
        assert int(r['power_star_total'])==6+int(taken), 'incorrect total star credit'
    assert before[0x14]==0 and after[0x14]==2, 'boss reward not saved'
    assert before[0x15:0x32]==after[0x15:0x32], 'other course progress changed'
    assert before[0x41]==after[0x41], 'saved character changed'
    assert '[native save] slot=0 write=committed bytes=68' in log, 'save never committed'
    assert '[native save] Save failed' not in log, 'save failed'
    exits=[r for r in states if r['level']=='1' and int(r['tick'])>pickup]
    assert exits, 'course never completed'
    assert any(r['player_state']=='St_Jump_Main' and r['level']=='1' and int(r['tick'])>pickup+150 for r in states), 'later castle jump missing'
    final=states[-1]
    assert final['level']=='1' and final['player_state']=='St_Wait_Main' and final['player_no_control']==final['player_disabled']=='0', 'normal castle control did not return'
    assert final['power_star_total']=='7', 'final reward credit missing'
    return dict(boss=bosses[0]['id'],hits=hits,star=stars[0]['id'],spawn=spawn,pickup=pickup,
                course_exit=int(exits[0]['tick']),final_tick=int(final['tick']),saved_total=7)


def load(prefix,seed,save):
    def rows(suffix):
        with Path(str(prefix)+suffix).open(newline='') as f:return list(csv.DictReader(f))
    # prefix points at build/port/native-file-menu-...; logs are in build/.
    log=prefix.parent.parent/(prefix.name+'.log')
    code=prefix.parent.parent/(prefix.name+'.exit-code.txt')
    if not log.is_file():log=Path(str(prefix)+'.log')
    if not code.is_file():code=Path(str(prefix)+'.exit-code.txt')
    return (rows('-boss.csv'),rows('-rewards.csv'),rows('-state.csv'),
            int(code.read_text(encoding='utf-8-sig').strip()),save_payload(seed),save_payload(save),log.read_text())


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('prefix',type=Path)
    parser.add_argument('seed_save',type=Path)
    parser.add_argument('saved_file',type=Path)
    args=parser.parse_args()
    print('PASS:',verify(*load(args.prefix,args.seed_save,args.saved_file)))


if __name__=='__main__':main()
