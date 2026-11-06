"""Native resources for the reviewed Dark World sliding-platform variant."""
import re,struct
from battlefield_platform_host import lazy_handles,observe

def transform(root,text):
    cfg=(root/'config/arm9/overlays/ov091/delinks.txt').read_text()
    base=int(re.search(r'\.text\s+start:0x([\da-f]+)',cfg)[1],16)
    raw=(root/'extracted/overlays/overlay_0091.bin').read_bytes()
    assert struct.unpack_from('<IHHIiiii',raw,0x02134f60-base)==(0x02132908,138,179,2,0,0x400000,0x2000000,0)
    assert struct.unpack_from('<3I',raw,0x02135024-base)==(0x02135520,0x02135538,0x02111be0)
    assert struct.unpack_from('<H',raw,0x02134504-base)[0]==69
    assert struct.unpack_from('<H',raw,0x02134514-base)[0]==16384
    text=lazy_handles(text,('SimpleLiftModelFilePtr','SimpleLiftClsnFileHandle'))
    text,n=re.subn(r'\*\(SharedFilePtr \*\*\)\(data_ov091_02135024 \+ mVariant \* 0xc\)', '&data_ov091_02135520',text);assert n==2,n
    text,n=re.subn(r'\*\(SharedFilePtr \*\*\)\(data_ov091_02135028 \+ (?:mVariant \* 0xc|tableOffset)\)', '&data_ov091_02135538',text);assert n==3,n
    text=text.replace('*(void **)(data_ov091_0213502c + tableOffset)','data_ov043_02111be0')
    text=text.replace('extern char data_ov091_0213502c[];','extern unsigned char data_ov043_02111be0[];')
    anchor='int daObjSimpleLift_c::InitResources()\n{'
    text=text.replace(anchor,anchor+"\n    assert(actorID==138);\n    static bool files_ready=false;\n    if(!files_ready){data_ov091_02135520.Construct(0x647);data_ov091_02135538.Construct(0x648);files_ready=true;}",1)
    text+='\nstatic_assert(sizeof(SimpleLiftModelFilePtr)==8 && sizeof(SimpleLiftClsnFileHandle)==8,"native simple lift handles");\n'
    return observe(text,'daObjSimpleLift_c','variant,timer,pause,speed,heading',['int(mVariant)','int(mMoveTimer)','int(mPauseTimer)','int(mHorzSpeed)','int(mPrevAngleY)'])
