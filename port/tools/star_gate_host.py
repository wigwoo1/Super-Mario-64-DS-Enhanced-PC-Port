"""PC adaptation of the recovered large Star Gate; keep DS source intact."""
import re
def transform(text):
    def replace(old,new):
        nonlocal text
        assert text.count(old)==1, 'Star Gate source changed: '+old
        text=text.replace(old,new)
    replace('    StarGateModelFilePtr(unsigned int fileID) { pc_sharedfile_handle_construct(this, fileID); }\n    ~StarGateModelFilePtr() { pc_sharedfile_handle_destroy(this); }','')
    replace('StarGateModelFilePtr data_ov100_02148934(0x4a1);',
            'StarGateModelFilePtr data_ov100_02148934={};')
    def state(match):
        name,init,main=match.groups()
        return ('daStarGate_c::State daStarGate_c::'+name+' = [] { State s={}; '+
                (('s.init='+init+'; ') if init.strip()!='0' else '')+
                's.main='+main+'; return s; }();')
    text,count=re.subn(r'daStarGate_c::State daStarGate_c::(ST_\w+) = \{\s*(0|&daStarGate_c::\w+),\s*(&daStarGate_c::\w+)\s*\};',state,text)
    assert count==5,'Star Gate state definitions changed'
    replace('mState->init == 0','mState->init.address == 0')
    replace('mState->main != 0','mState->main.address != 0')
    replace('extern Player *data_0209f394[];','extern "C" Player *data_0209f394[];')
    replace('extern char data_020a0ebc[];','extern "C" char data_020a0ebc[];')
    replace('_ZN9dCamera_c14GoBehindPlayerEj(data_0209f318, data_0209f250);',
            'data_0209f318->GoBehindPlayer(data_0209f250);')
    replace('int daStarGate_c::InitResources()\n{', '''int daStarGate_c::InitResources()
{
    static bool ready=false;
    if(!ready){data_ov100_02148934.Construct(0x4a1);ready=true;}
    if(std::getenv("SM64DS_PC_STAR_GATE_TRACE"))std::fprintf(stderr,
        "[native Star Gate] init packed=%08x pos=%d,%d,%d areas=%d,%d\\n",param1,mPosX,mPosY,mPosZ,mAngleX,mAngleZ);''')
    replace('    mState = nextState;', '''    if(std::getenv("SM64DS_PC_STAR_GATE_TRACE"))std::fprintf(stderr,
        "[native Star Gate] state tick=%llu variant=%u state=%d pos=%d,%d,%d relative=%d,%d,%d\\n",
        static_cast<unsigned long long>(ntr::rt_frame()),param1,
        nextState==&ST_WAIT?0:nextState==&ST_TALKING_TO_PLAYER?1:nextState==&ST_UNLOCKING?2:nextState==&ST_OPEN_CLOSE?3:4,
        mPosX,mPosY,mPosZ,mScaleX,mScaleY,mScaleZ);
    mState = nextState;''')
    return '''#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include "ntr/rt.h"
#include "daStarGate_c.h"
#include "dCamera_c.h"
static_assert(sizeof(daStarGate_c)==0x118,"Star Gate actor layout");
static_assert(sizeof(daStarGate_c::State)==16,"Star Gate callback stride");
static_assert(offsetof(daStarGate_c,mState)==0x110,"Star Gate state offset");
'''+text
