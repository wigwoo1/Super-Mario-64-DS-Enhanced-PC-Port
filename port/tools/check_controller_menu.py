"""Check the controller-only original title/file/options/back replay."""
import argparse
import csv
import json
from pathlib import Path


def verify(directory):
    assert int((directory/'exit-code.txt').read_text())==0,'controller replay did not exit cleanly'
    with (directory/'menu.csv').open() as file:menu=list(csv.DictReader(file))
    with (directory/'state.csv').open() as file:state=list(csv.DictReader(file))
    assert any(r['root']=='0' and r['sub']=='11' and r['touch']=='1' and r['x']=='128' and r['y']=='96' for r in menu),'Start did not leave the animated title'
    assert any(r['root']=='0' and r['sub']=='0' and r['touch']=='1' and r['y']=='168' for r in menu),'Adventure did not confirm'
    assert any(r['root']=='5' and r['sub']=='0' and r['focus']=='15' for r in menu),'File B did not receive controller focus'
    assert any(r['root']=='5' and r['sub']=='1' for r in menu),'File Options did not open'
    assert any(2400<int(r['tick'])<2600 and r['root']=='5' and r['sub']=='0' for r in menu),'B did not return from File Options'
    assert any(2700<int(r['tick'])<2900 and r['root']=='0' and r['sub']=='0' for r in menu),'B did not return to the title menu'
    assert any(3290<int(r['tick'])<3310 and r['focus']=='15' and r['touch']=='1' for r in menu),'File B did not confirm after returning'
    assert state[-1]['level']=='1' and state[-1]['player_character']=='3','File B did not enter the game'
    assert (directory/'menu-1770.bmp').is_file(),'File B focus screenshot missing'
    assert (directory/'menu-2200.bmp').is_file(),'File Options screenshot missing'
    return {'status':'PASS','title_start':True,'adventure_confirm':True,'file_b_navigation_and_confirm':True,
            'file_options_and_back':True,'title_return_and_reentry':True,'final_frame':int(state[-1]['tick']),
            'physical_xbox_tested':False}


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path)
    args=parser.parse_args();result=verify(args.directory)
    output=json.dumps(result,indent=2)
    (args.directory/'verification.json').write_text(output+'\n');print(output)
