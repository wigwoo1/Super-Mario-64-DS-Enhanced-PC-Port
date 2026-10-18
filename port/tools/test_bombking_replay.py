"""Reject missing/altered evidence using a completed real boss replay."""
import argparse
from pathlib import Path
from check_bombking_replay import load,verify


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('prefix','seed','saved'):p.add_argument(name,type=Path)
    a=p.parse_args();original=load(a.prefix,a.seed,a.saved)
    print('positive:',verify(*original))
    cases=[]
    def change(index,fn):
        v=list(original);v[index]=fn(v[index]);return v
    cases.append(('missing boss',change(0,lambda r:[x for x in r if x['kind']!='189'])))
    cases.append(('missing hit',change(0,lambda r:[dict(x,hit2='0') if x['kind']=='189' else x for x in r])))
    cases.append(('missing catch',change(0,lambda r:[dict(x,mouth='0') for x in r])))
    cases.append(('missing bomb impact',change(0,lambda r:[dict(x,hit='0') if x['kind']=='206' else x for x in r])))
    cases.append(('wrong reward',change(1,lambda r:[dict(x,param='1') if x['kind']=='178' and x['star_id']=='1' else x for x in r])))
    cases.append(('no collection',change(1,lambda r:[dict(x,state='4') if x['kind']=='178' and x['state']=='5' else x for x in r])))
    cases.append(('duplicate credit',change(1,lambda r:[dict(x,power_star_total='8') if x['kind']=='178' and x['state']=='5' else x for x in r])))
    cases.append(('lost control',change(2,lambda r:r[:-1]+[dict(r[-1],player_no_control='1')])))
    cases.append(('unsaved reward',change(5,lambda _:original[4])))
    cases.append(('artificial star',change(6,lambda s:s+'\n[native QA fixture] Power Star')))
    cases.append(('crash',change(3,lambda _:1)))
    for name,v in cases:
        try:verify(*v)
        except AssertionError:pass
        else:raise AssertionError('invalid evidence accepted: '+name)
    print(f'PASS: {len(cases)} negative evidence checks')


if __name__=='__main__':main()
