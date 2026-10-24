"""Native resource lifetime and observations for the original Koopa race.

The recovered actor, state handlers, path following and reward rules remain
in the matched translation units. Only startup handles need host adaptation.
"""
import re


def transform(text, name):
    race = name == 'daRNk_c.cpp'
    assert race or name == 'daRFlag_c.cpp', name
    handles = ('RNkModelFileHandle', 'RNkAnimationFileHandle') if race else (
        'RFlagModelFileHandle', 'RFlagAnimResFileHandle')
    for kind in handles:
        pattern = rf'{kind}\(unsigned int (\w+)\) \{{ pc_sharedfile_handle_construct\(this, \1\); \}}'
        fields = 'fileId={};file=nullptr;' if race else 'words[0]={};words[1]=0;'
        text, count = re.subn(pattern, lambda m:
            f'{kind}(unsigned int {m[1]}) {{ {fields.format(m[1])} }}', text)
        assert count == 1, kind + ' constructor changed'
        old = f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old) == 1, kind + ' destructor changed'
        text = text.replace(old, f'~{kind}() {{}}', 1)

    cls = 'daRNk_c' if race else 'daRFlag_c'
    records = [('0211e00c', 0x3ba), ('0211e014', 0x3c6),
               ('0211e024', 0x3c4), ('0211e01c', 0x3c5),
               ('0211e034', 0x3c2), ('0211e03c', 0x3c3),
               ('0211e02c', 0x3bf), ('0211e004', 0x3be)] if race else [
               ('0211e0d4', 0x488), ('0211e0dc', 0x489)]
    init = f'int {cls}::InitResources()\n{{'
    assert text.count(init) == 1
    code = '\n    static bool ready=false;\n    if(!ready){\n'
    for suffix, resource in records:
        assert re.search(r'data_ov062_' + suffix + r'\(0x' + f'{resource:x}' + r'\)', text)
        code += f'        data_ov062_{suffix}.Construct(0x{resource:x});\n'
    code += '        ready=true;\n    }\n'
    text = text.replace(init, init+code, 1)

    behavior = f'int {cls}::Behavior()\n{{'
    assert text.count(behavior) == 1
    # Capture original state/contact before it is consumed, without changing it.
    if race:
        fields = 'state,step,node,nodes,speed,racing,won,cannon,player,flag,star'
        values = ('mState,int(mStep),mCurPathPt,mNumPathPts,mHorzSpeed,'
                  'int(mIsRacing),int(mPlayerWon),int(mHasPlayerUsedCannon),'
                  'mPlayer?int(mPlayer->uniqueID):0,mFlagID,int(mStarID)')
    else:
        fields = 'touched,victory_timer,contact'
        values = 'int(mHasTouchedFlag),int(mVictoryTimer),int(mdCcAc_c.otherOwner)'
    trace = f'''
    if(const char *path=std::getenv("SM64DS_PC_KOOPA_TRACE")){{
        static FILE *file=nullptr;
        if(!file){{file=std::fopen(path,"a");if(file)std::fputs("# {cls}:tick,uid,x,y,z,{fields}\\n",file);}}
        if(file && ntr::rt_frame()%5==0){{
            std::fprintf(file,"%llu,%u,%d,%d,%d,{','.join(['%d']*len(fields.split(',')))}\\n",
                static_cast<unsigned long long>(ntr::rt_frame()),unsigned(uniqueID),mPosX,mPosY,mPosZ,{values});
            std::fflush(file);
        }}
    }}
'''
    text = text.replace(behavior, behavior+trace, 1)
    text += f'\nstatic_assert(sizeof({cls})==0x' + ('3e0' if race else '174') + ',"native race actor layout");\n'
    if race:
        text += 'static_assert(sizeof(StateFunc)==4,"native Koopa state table stride");\n'
    return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n'+text
