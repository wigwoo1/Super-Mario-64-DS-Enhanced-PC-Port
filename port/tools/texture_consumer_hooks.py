"""PC-only draw ownership hooks; original retail source remains unchanged."""
import re,sys
from pathlib import Path
HOOKS={
    '_ZN6Player6RenderEv.cpp':('int Player::Render()','Character'),
    'daTrs_c.cpp':('int daTrs_c::Render()','Character'),
    'daCoin_c.cpp':('int daCoin_c::Render()','Object'),
    'daObjMarioCap_c.cpp':('int daObjMarioCap_c::Render()','Object'),
}
def transform(text,name):
    if name not in HOOKS:return text,0
    signature,role=HOOKS[name]
    pattern=re.escape(signature)+r'\s*\{'
    text,count=re.subn(pattern,lambda m:m[0]+'\n    ntr::TextureConsumerScope texture_consumer_scope(ntr::TextureRole::'+role+');',text,count=1)
    if count!=1:raise ValueError('Texture consumer hook changed: '+signature)
    return '#include "ntr/texture_assets.h"\n'+text,count
if __name__=='__main__':
    source,output=map(Path,sys.argv[1:]);text,count=transform(source.read_text(),source.name)
    if count!=1:raise ValueError('No requested draw hook: '+str(source))
    output.parent.mkdir(parents=True,exist_ok=True);output.write_text(text)
