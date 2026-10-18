"""Verify a real input replay captured a Goomba, swallowed it and made an egg."""
import argparse
import csv
import pathlib


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('trace',type=pathlib.Path)
    parser.add_argument('exit_code',type=pathlib.Path)
    args=parser.parse_args()
    assert int(args.exit_code.read_text(encoding='utf-8-sig').strip())==0, 'game did not exit cleanly'
    rows=list(csv.DictReader(args.trace.open()))
    frames={}
    for row in rows: frames.setdefault(int(row['tick']),[]).append(row)
    held=[(tick,rs[0]) for tick,rs in frames.items() if int(rs[0]['mouth'])]
    assert held,'tongue never captured an actor'
    first,record=held[0]
    target=record['mouth']
    assert any(r['goomba']==target for r in frames[first]),'captured actor was not a Goomba'
    assert len(held)>10,'capture did not persist through tongue retraction'
    swallowed=[t for t,rs in frames.items() if t>first and rs[0]['state']=='St_Swallow_Main']
    assert swallowed,'swallow animation never ran'
    completed=[(t,rs[0]) for t,rs in frames.items() if t>swallowed[0] and
               rs[0]['mouth']=='0' and int(rs[0]['egg_id']) and
               all(r['goomba']!=target for r in rs)]
    assert completed,'swallow did not remove the Goomba and create an egg'
    done,egg=completed[0]
    assert max(frames)>done+100,'no sustained play after swallowing'
    assert any(t>done+30 and rs[0]['state'] in ('St_Wait_Main','St_Walk_Main')
               for t,rs in frames.items()),'Yoshi did not recover normal control'
    assert not any('Hurt' in rs[0]['state'] for t,rs in frames.items() if first<=t<=done), 'Yoshi was hurt during capture/swallow'
    print(f'PASS: Goomba {target} captured at {first}, held {len(held)} frames; '
          f'swallow at {swallowed[0]}, Goomba removed/egg {egg["egg_id"]} created at {done}; '
          f'normal control recovered and play continued through {max(frames)}, exit0')


if __name__=='__main__':main()
