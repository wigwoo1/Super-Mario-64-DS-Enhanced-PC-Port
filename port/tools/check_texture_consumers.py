"""Verify shared draw policy sources and the native Player/Coin adaptations."""
import hashlib,json,re,struct
from pathlib import Path
from texture_consumer_hooks import HOOKS,transform
from check_texture_source_roles import overlay2_from_rom
ROOT=Path(__file__).resolve().parents[2]
def audit():
    allow=re.findall(r'PC_TEXTURE_SHARED_MODEL\("([^"]+)", (0x[0-9a-f]+), "([^"]+)"\)',
        (ROOT/'port/ntr/texture_shared_models.inc').read_text())
    assert len(allow)==9 and len({p for p,_,_ in allow})==9
    roles={p:r for p,r in re.findall(r'PC_TEXTURE_MODEL\("([^"]+)", (\w+),',
        (ROOT/'port/ntr/texture_model_roles.inc').read_text())}
    common=(ROOT/'src/unnamed/ov002/__sinit_ov002_02100560.c').read_text()
    syms={int(h,16):symbol for _,h,symbol in allow}
    for path,h,symbol in allow:
        assert roles[path]=='Unknown'
        assert re.search(r'func_02017acc\(\s*&?'+symbol+r'\s*,\s*'+h+r'\s*\)',common),path
    base,image=overlay2_from_rom((ROOT/'build/assets/user-europe-rev0.nds').read_bytes())
    tables={}
    for address,handles in [(0x020ff060,[0x8006,0x8008,0x9c07]),(0x020ff06c,[0x8005,0x8007,0x9c06])]:
        words=struct.unpack_from('<3I',image,address-base)
        assert [f'data_ov002_{a:08x}' for a in words]==[syms[h] for h in handles]
        tables[f'{address:08x}']=[f'{a:08x}' for a in words]
    source_paths={
        '_ZN6Player6RenderEv.cpp':'src/_ZN6Player6RenderEv.cpp',
        'daTrs_c.cpp':'src/actors/daTrs_c.cpp',
        'daCoin_c.cpp':'src/actors/daCoin_c.cpp',
        'daObjMarioCap_c.cpp':'src/actors/daObjMarioCap_c.cpp'}
    hashes={}
    for name,path in source_paths.items():
        text=(ROOT/path).read_text();adapted,count=transform(text,name);assert count==1
        signature,role=HOOKS[name]
        start=adapted.index(signature);assert 'TextureRole::'+role in adapted[start:start+180]
        hashes[path]=hashlib.sha256((ROOT/path).read_bytes()).hexdigest()
    # These two consumers are in the current native link; others are verified
    # transformations for future actor coverage, not claims of game execution.
    player=(ROOT/'build/port/host-src/player_render.cpp').read_text()
    coin=(ROOT/'build/port/pc-boot-src/src/actors/daCoin_c.cpp').read_text()
    assert player==transform((ROOT/source_paths['_ZN6Player6RenderEv.cpp']).read_text(),'_ZN6Player6RenderEv.cpp')[0]
    assert 'TextureConsumerScope texture_consumer_scope(ntr::TextureRole::Object);' in coin
    link=(ROOT/'build/port/sm64ds_pc.map').read_text()
    assert re.search(r'\?Render@Player@@.*player_render.cpp.obj',link)
    assert re.search(r'\?Render@daCoin_c@@.*daCoin_c.cpp.obj',link)
    report={'SharedModelAllowlist':allow,'CoinTablesFromUserROM':tables,'SourceSHA256':hashes,
        'NativeConsumers':['Player','daCoin_c'],'TransformsReviewed':['Player','daTrs_c','daCoin_c','daObjMarioCap_c'],
        'Limitations':['Enemy disguise and cap actor native gameplay still require actor/course coverage; their transforms are checked, not executed here.',
        'Other callers keep Unknown base roles. This is not full per-consumer coverage.']}
    (ROOT/'build/yoshi-session-consumer-audit.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS: nine shared model constructors, six user-ROM coin-table slots, four exact hook transforms, linked native Player/Coin scopes')
if __name__=='__main__':audit()
