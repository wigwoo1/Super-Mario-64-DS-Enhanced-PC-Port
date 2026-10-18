"""Compile the production Back handler without pulling in the full game scene."""
from pathlib import Path
import sys
import hostgen
source,output=map(Path,sys.argv[1:])
text,_=hostgen.transform(source.read_text())
signature='void func_ov005_020c0250(char *self) {'
if text.count(signature)!=1: raise ValueError('Rec Room Back handler changed')
start=text.index(signature)
end=text.index('{',start)+1
depth=1
while depth:
    depth+=(text[end]=='{')-(text[end]=='}')
    end+=1
declarations=('struct TouchRec { u8 held; u8 edge; u8 x; u8 y; };',
              'extern dFader_c *data_0209f5bc;','extern TouchRec gTouchHeld[];',
              'extern u8 gActivePlayerSlot;','int  func_02012790(int);')
for declaration in declarations:
    if text.count(declaration)!=1: raise ValueError('Rec Room test interface changed: '+declaration)
output.parent.mkdir(parents=True,exist_ok=True)
output.write_text('// Actual generated production function; no replacement handler.\n'
    '#include "dScMiniGm_c.h"\n#include "dFader_c.h"\nextern "C" {\n'
    +'\n'.join(declarations)+'\n'+text[start:end]+'\n}\n')
