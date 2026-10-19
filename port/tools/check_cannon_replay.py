"""Check original course entry, native cannon rise/aim/launch/reset and landing."""
import csv
import math
import re
import sys
from pathlib import Path
from check_power_star_replay import save_payload


def angle_difference(a,b):
    return abs((int(a)-int(b)+32768)%65536-32768)


def check(case,seed):
    assert (case/'exit-code.txt').read_text().strip()=='0','game did not exit cleanly'
    log=(case/'run.log').read_text(errors='replace')
    assert 'confirmed mission=2 level=6' in log,'original mission selection missing'
    selected=re.search(r'\[native cannon replay\] target=(\d+)',log)
    assert selected,'live cannon not approached'
    target=selected[1]
    rows=list(csv.DictReader((case/'cannon.csv').open()))
    buddy_path=case/'buddy.csv'
    if buddy_path.is_file():
        actors=list(csv.DictReader(buddy_path.open()))
        shutter=[r for r in actors if r['kind']=='14' and r['param']=='65281']
        if shutter and shutter[0]['opened']=='0':
            buddy=[r for r in actors if r['kind']=='181' and r['variant']=='1']
            assert {0,1,2,3,4}.issubset({int(r['step']) for r in buddy if r['state']=='2'}),'original Buddy opening sequence incomplete'
            assert shutter[-1]['opened']=='1' and buddy[-1]['state']=='0' and buddy[-1]['opened']=='1','Buddy/shutter did not finish opening'
    cannon=[r for r in rows if r['level']=='6' and r['id']==target and r['kind']=='207']
    assert cannon and all(r['variant']=='2' for r in cannon),'rideable native cannon missing'
    assert cannon[0]['state']=='2','cannon did not begin waiting in the hole'
    load=[r for r in cannon if r['state']=='3']
    assert {0,1,2,3,4}.issubset({int(r['step']) for r in load}),'load/rise/turn/aim/sink sequence incomplete'
    assert all(r['player_state']=='St_Cannon_Main' and r['control_disabled']=='1' for r in load if int(r['step'])<4),'rider was not held in the original cannon state'
    aim=[r for r in load if r['step']=='3']
    assert len(aim)>=6,'aiming view was not held long enough'
    assert angle_difference(aim[0]['camera_yaw'],aim[-1]['camera_yaw'])>0x200,'horizontal aim input had no effect'
    assert int(aim[-1]['camera_pitch'])<int(aim[0]['camera_pitch'])-0x400,'vertical aim input had no effect'
    assert angle_difference(aim[-1]['yaw'],int(aim[-1]['camera_yaw'])+0x4000)<0x100,'cannon barrel is not following the chosen camera heading'
    launch=next((r for r in cannon if r['player_state']=='St_Cannon_Main' and r['player_step']=='2'),None)
    assert launch and int(launch['vy'])>0 and int(launch['speed'])>0 and launch['control_disabled']=='0','launch velocity or control handoff missing'
    flight=[r for r in cannon if r['player_state']=='St_Cannon_Main' and r['player_step']=='2' and int(r['tick'])<=int(launch['tick'])+20]
    assert len(flight)>=2,'flight trajectory missing'
    dx=int(flight[-1]['px'])-int(flight[0]['px']);dz=int(flight[-1]['pz'])-int(flight[0]['pz'])
    assert math.hypot(dx,dz)>150*4096,'player did not travel out of the cannon'
    travel_heading=round(math.atan2(dx,dz)*65536/(2*math.pi))
    assert angle_difference(travel_heading,int(aim[-1]['camera_yaw'])+0x8000)<0x400,'flight does not follow the chosen heading'
    after=[r for r in cannon if int(r['tick'])>int(launch['tick'])]
    assert any(r['state']=='2' and int(r['pitch'])==0 for r in after),'cannon did not return to its waiting pose'
    assert '[native cannon replay] landed' in log,'no grounded landing observed'
    final=cannon[-1]
    assert final['airborne']==final['control_disabled']=='0' and final['player_state']=='St_Wait_Main','normal grounded idle not restored'
    assert save_payload(case/'saves/slot1.sav')==save_payload(seed),'cannon use changed saved progression'
    print('PASS: native cannon rise, camera/barrel aim, aimed launch trajectory, reset, grounded controls and unchanged save')


if __name__=='__main__':check(*(Path(p) for p in sys.argv[1:]))
