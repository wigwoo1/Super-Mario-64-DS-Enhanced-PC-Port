"""The recovered Model::Render symbol is a direct base call, not a vcall."""
import argparse,json,re,struct
from pathlib import Path
from check_player_animation_data import pe_bytes
ROOT=Path(__file__).resolve().parents[2]
def audit(exe,map_path):
    symbols={m[1]:int(m[2],16) for m in re.finditer(r'^\s+\d{4}:[\da-f]+\s+(\S+)\s+([\da-f]{8})\s',map_path.read_text(),re.M)}
    address=symbols['__ZN5Model6RenderEPK7Vector3']
    targets={a for n,a in symbols.items() if n.startswith('?Render@Model@@')}
    assert len(targets)==1
    code=pe_bytes(exe,address,15);calls=[]
    for i,b in enumerate(code[:-4]):
        if b in (0xe8,0xe9):
            destination=address+i+5+struct.unpack_from('<i',code,i+1)[0]
            if destination in targets:calls.append({'offset':i,'destination':f'{destination:08x}'})
    assert len(calls)==1 and b'\xff\x50\x14' not in code,'Model renderer bridge dispatches through the animation vtable'
    return {'BridgeAddress':f'{address:08x}','CodeHex':code.hex(),'DirectBaseCalls':calls}
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--exe',type=Path,default=ROOT/'build/port/sm64ds_pc.exe');parser.add_argument('--map',type=Path,default=ROOT/'build/port/sm64ds_pc.map')
    args=parser.parse_args();report=audit(args.exe,args.map);print('PASS: native recovered Model::Render bridge directly calls the base renderer; no extra virtual animation update');print(json.dumps(report))
