"""Verify real special-Goomba reward release and collection replays."""
import argparse
import collections
import csv
import pathlib


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('mode',choices=('cap','cap-eaten','cap-placed','silver','silver-egg'))
    p.add_argument('trace',type=pathlib.Path)
    p.add_argument('exit_code',type=pathlib.Path)
    args=p.parse_args()
    assert int(args.exit_code.read_text(encoding='utf-8-sig').strip())==0,'game did not exit cleanly'
    rows=list(csv.DictReader(args.trace.open()))
    groups=collections.defaultdict(list)
    for row in rows:groups[row['id']].append(row)
    initial={r['id'] for r in rows if r['tick']==rows[0]['tick']}
    end=max(int(r['tick']) for r in rows)
    state=args.trace.with_name(args.trace.name.replace('-rewards.csv','-state.csv'))
    states=list(csv.DictReader(state.open()))
    initial_level=next(r['level'] for r in states if int(r['tick'])>=1500)
    tail=[r for r in states if int(r['tick'])>=end-50]
    assert any(r['player_state']=='St_Wait_Main' and r['player_no_control']=='0'
               and r['level']==initial_level for r in tail),'normal control/idle was not restored in the same course'
    victims=[rs for rs in groups.values() if rs[0]['kind']=='200' and
             any(int(r['death'])==1 for r in rs)]
    if args.mode in ('cap','silver'):assert victims,'no original Goomba was stomped'
    if args.mode.startswith('cap'):
        if args.mode in ('cap','cap-eaten'):
            rewards=[rs for uid,rs in groups.items() if uid not in initial and rs[0]['kind']=='269'
                     and int(rs[0]['param'])&0xff==0x12]
            assert rewards,'no new enemy-released cap (type18)'
            if args.mode=='cap-eaten':
                tongue=args.trace.with_name(args.trace.name.replace('-rewards.csv','-yoshi.csv'))
                mouth={r['mouth'] for r in csv.DictReader(tongue.open()) if r['mouth']!='0'}
                assert any(rs[0]['id'] in mouth for rs in rewards),'released cap did not enter Yoshi mouth'
                assert any(rs[0]['kind']=='200' and int(rs[0]['payout_index'])<3 and
                           any(int(r['payout_index'])&0x80 for r in rs)
                           for rs in groups.values()),'no capped Goomba lost its cap'
        else:
            rewards=[rs for rs in groups.values() if rs[0]['kind']=='269' and
                     any(r['other']!='0' for r in rs)]
            assert rewards,'no placed cap had a pickup contact'
        collected=[]
        for rs in rewards:
            character=int(rs[0]['payload'])
            first=int(rs[0]['tick'])
            if args.mode!='cap-placed':
                parents=[]
                for enemy in groups.values():
                    if enemy[0]['kind']!='200':continue
                    for before,after in zip(enemy,enemy[1:]):
                        old=int(before['payout_index']);new=int(after['payout_index'])
                        if old==character and new&0x80 and abs(int(after['tick'])-first)<=1:
                            parents.append(enemy)
                assert parents,'no matching Goomba lost this cap when it spawned'
                if args.mode=='cap':
                    assert any(any(r['death']=='1' and int(r['tick'])<=first for r in e)
                               for e in parents),'cap parent was not stomped'
            assert any(int(r['tick'])>=1500 and int(r['tick'])<first+1 and
                       int(r['player_body_model'])!=character for r in states), 'player already had this disguise'
            if any(first<=int(r['tick']) and int(r['player_hat_character'])==character
                   and int(r['player_body_model'])==character for r in states):
                assert int(rs[-1]['tick'])<end-100,'collected cap was not removed'
                collected.append(rs[0]['id'])
        assert collected,'no released cap changed the player hat/body model'
        print(f'PASS: {args.mode} IDs {collected}, character transformation, '
              f'cap removal, same-course idle/normal control, {end}frames exit0')
    else:
        if args.mode=='silver':
            assert any((int(rs[0]['param'])>>4)&15==1 for rs in victims),'stomped Goomba did not carry a silver star'
        else:
            eaten=[rs for rs in groups.values() if rs[0]['kind']=='200' and
                   (int(rs[0]['param'])>>4)&15==1 and int(rs[-1]['tick'])<end-100]
            assert eaten,'no silver-star-carrying Goomba was removed'
            tongue=args.trace.with_name(args.trace.name.replace('-rewards.csv','-yoshi.csv'))
            mouth={r['mouth'] for r in csv.DictReader(tongue.open()) if r['mouth']!='0'}
            assert any(rs[0]['id'] in mouth for rs in eaten),'silver-star Goomba did not enter Yoshi mouth'
            eggs=[rs for rs in groups.values() if rs[0]['kind']=='9' and int(rs[0]['payload'])&128]
            assert eggs and any({r['state'] for r in rs}>={'0','1'} for rs in eggs),'silver reward egg did not enter flight'
        stars=[rs for uid,rs in groups.items() if uid not in initial and rs[0]['kind']=='179']
        assert stars,'no new silver-star reward'
        linked=[]
        for rs in stars:
            if any(r['target'] in groups and groups[r['target']][0]['kind']=='180' and
                   any(m['target']==rs[0]['id'] for m in groups[r['target']]) for r in rs):
                linked.append(rs)
        assert linked,'silver star and original marker were not linked both ways'
        assert any(any(5<=int(r['state'])<=7 for r in rs) for rs in linked),'silver-star collection pose did not run'
        before=int(rows[0]['silver_stars']);after=max(int(r['silver_stars']) for r in rows)
        assert after>before,'silver-star counter did not increase'
        assert any(int(rs[-1]['tick'])<end-100 for rs in linked),'collected silver star was not removed'
        print(f'PASS: silver-star drop IDs {[rs[0]["id"] for rs in linked]}, reciprocal marker link, '
              f'collection pose/counter {before}->{after}, removal, same-course idle/normal control, {end}frames exit0')


if __name__=='__main__':main()
