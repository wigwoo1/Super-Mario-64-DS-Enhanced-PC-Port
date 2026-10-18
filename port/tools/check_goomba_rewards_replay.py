"""Check real egg-hit/stomp replays and their coin rewards."""
import argparse
import collections
import csv
import pathlib


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode',choices=('egg','stomp'))
    parser.add_argument('trace',type=pathlib.Path)
    parser.add_argument('exit_code',type=pathlib.Path)
    parser.add_argument('--require-collection',action='store_true')
    args=parser.parse_args()
    assert int(args.exit_code.read_text(encoding='utf-8-sig').strip())==0,'game did not exit cleanly'
    rows=list(csv.DictReader(args.trace.open()))
    groups=collections.defaultdict(list)
    for row in rows:groups[row['id']].append(row)
    end=max(int(r['tick']) for r in rows)
    death=5 if args.mode=='egg' else 1
    victims=[rs for rs in groups.values() if rs[0]['kind']=='200' and
             any(int(r['death'])==death for r in rs)]
    assert victims,f'no Goomba entered expected {args.mode} death state'
    assert any(int(rs[-1]['tick'])<end-100 for rs in victims),'defeated Goomba was not removed'
    initial_ids={r['id'] for r in rows if r['tick']==rows[0]['tick']}
    coins=[rs for uid,rs in groups.items() if uid not in initial_ids and rs[0]['kind']=='288']
    assert any(rs[0]['param']=='242' for rs in coins),'Goomba did not drop its original coin reward'
    if args.mode=='egg':
        eggs=[rs for rs in groups.values() if rs[0]['kind']=='9']
        assert eggs,'no swallowed-Goomba egg exists'
        assert any(any(r['state']=='0' for r in rs) and any(r['state']=='1' for r in rs) for rs in eggs),'egg never entered flight'
        assert any(max(int(r['payout_bits']) for r in rs)>0 for rs in eggs),'egg never paid its stored reward'
        assert any(rs[0]['param']=='2' for rs in coins),'egg payout coins were not spawned'
    states=args.trace.with_name(args.trace.name.replace('-rewards.csv','-state.csv'))
    samples=list(csv.DictReader(states.open()))
    if args.mode=='stomp':
        hit=min(int(r['tick']) for rs in victims for r in rs if int(r['death'])==death)
        assert any(hit<=int(r['tick'])<=hit+10 and int(r['player_vert_speed'])>0
                   for r in samples),'no player bounce after stomp'
    assert any(int(r['tick'])>=end-50 and r['player_state']=='St_Wait_Main' for r in samples),'Yoshi did not recover to idle'
    start_coins=int(rows[0]['course_coins'])
    final_coins=max(int(r['course_coins']) for r in rows)
    if args.mode=='egg' or args.require_collection:
        assert final_coins>start_coins,'no reward coin was collected'
    print(f'PASS: {args.mode}; defeated Goomba IDs {[rs[0]["id"] for rs in victims]}; '
          f'{len(coins)} new reward coins; course coins {start_coins}->{final_coins}; '
          f'normal control/idle restored, continuation to {end}, exit0')


if __name__=='__main__':main()
