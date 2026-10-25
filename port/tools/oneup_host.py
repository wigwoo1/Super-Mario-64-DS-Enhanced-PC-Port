"""Host adaptation of the recovered mushroom actor's dispatch and startup."""
import re


def transform(text):
    # Use typed wrappers so the one int-returning handler is called through
    # its real signature. Preserve the original fourteen dispatch identities.
    old = 'typedef void (da1up_c::*PMF)();'
    assert text.count(old) == 1
    text = text.replace(old, 'typedef void (*PMF)(da1up_c *);')
    old = '(this->*data_ov002_0210dc00[mMushroomType])();'
    assert text.count(old) == 1
    text = text.replace(old, 'data_ov002_0210dc00[mMushroomType](this);')
    start = text.index('PMF data_ov002_0210dc00[14] = {')
    tail = text[start:]
    methods = re.findall(r'&da1up_c::(func_ov002_\w+)', tail)
    assert len(methods) == 14
    wrappers = ''.join(f'static void pc_oneup_dispatch_{i}(da1up_c *self) {{ self->{method}(); }}\n'
                       for i, method in enumerate(methods))
    text = text[:start] + wrappers + re.sub(r'(?:\(PMF\))?&da1up_c::func_ov002_\w+',
        lambda m: '&pc_oneup_dispatch_' + str(methods.index(m[0].split('::')[1])), tail)
    # File handles require the runtime file catalog, after process startup.
    signature = 'int da1up_c::InitResources()\n{'
    assert text.count(signature) == 1
    text = text.replace(signature, signature + '''
    extern SharedFilePtr data_ov002_0210d9d8;
    extern SharedFilePtr data_ov002_0210da30;
    static bool ready=false;
    if(!ready) {
        data_ov002_0210d9d8.Construct(0x9c0f);
        data_ov002_0210da30.Construct(0x49f);
        ready=true;
    }
''', 1)
    # The retail range helper's return register is unused by callers. Its
    # recovered void-to-int cast has no defined result in portable C++.
    old = 'return ((int (*)(void*))func_ov002_020aefa4)((void*)this);'
    assert text.count(old) == 1
    text = text.replace(old, 'func_ov002_020aefa4((char*)this);\n  return 1;')
    assert text.count('GiveLives(1);') == 2
    text = text.replace('GiveLives(1);', '''{
            const int before=data_0209f2f4;
            GiveLives(1);
            if(std::getenv("SM64DS_PC_ONEUP_TRACE"))
                std::fprintf(stderr,"[native oneup reward] tick=%llu uid=%u before=%d after=%d\\n",
                    static_cast<unsigned long long>(ntr::rt_frame()),unsigned(uniqueID),before,int(data_0209f2f4));
        }''')
    # Read-only lifecycle evidence for original placements and collection.
    trace = '''
    if(const char *path=std::getenv("SM64DS_PC_ONEUP_TRACE")) {
        extern signed char data_0209f2f4;
        static FILE *file=nullptr;
        if(!file) {file=std::fopen(path,"a");if(file)std::fputs("tick,uid,actor,param,type,state,x,y,z,shown,blink,unlock,hit,other,lives,alive\\n",file);}
        if(file && (ntr::rt_frame()%5==0 || mdCcAc_c.hitFlags)) {
            std::fprintf(file,"%llu,%u,%u,%u,%d,%d,%d,%d,%d,%u,%u,%d,%u,%u,%d,%u\\n",
                static_cast<unsigned long long>(ntr::rt_frame()),unsigned(uniqueID),unsigned(actorID),
                unsigned(param1),mMushroomType,mState,mPosX,mPosY,mPosZ,unsigned(mShown),
                unsigned(mBlinkOn),mUnlockCount,unsigned(mdCcAc_c.hitFlags),unsigned(mdCcAc_c.otherOwner),
                int(data_0209f2f4),unsigned(aliveState));
            std::fflush(file);
        }
    }
'''
    signature = 'int da1up_c::Behavior()\n{'
    assert text.count(signature) == 1
    text = text.replace(signature, signature + trace, 1)
    text = text.replace('        extern signed char data_0209f2f4;\n', '')
    return ('#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n'
            'extern "C" signed char data_0209f2f4;\n' + text)


def transform_logo(text):
    for kind in ('OneUpLogoModelFile', 'OneUpLogoTexSeqFilePtr'):
        pattern = rf'{kind}\(unsigned int fileID\) \{{ pc_sharedfile_handle_construct\(this, fileID\); \}}'
        text, count = re.subn(pattern, f'{kind}(unsigned int fileID) {{words[0]=fileID;words[1]=0;}}', text)
        assert count == 1, kind + ' constructor'
        old = f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old) == 1, kind + ' destructor'
        text = text.replace(old, f'~{kind}() {{}}', 1)
    signature = 's32 daObj1UpLogo_c::InitResources()\n{'
    assert text.count(signature) == 1
    text = text.replace(signature, signature + '''
    static bool ready=false;
    if(!ready) {
        data_ov002_02110aa4.Construct(0x436);
        data_ov002_02110a9c.Construct(0x437);
        ready=true;
    }
''', 1)
    signature = 's32 daObj1UpLogo_c::Render()\n{'
    assert text.count(signature) == 1
    text = text.replace(signature, signature + '''
    if(std::getenv("SM64DS_PC_ONEUP_TRACE") && ntr::rt_frame()%5==0)
        std::fprintf(stderr,"[native oneup logo] tick=%llu uid=%u model=%p state=%u delay=%u\\n",
            static_cast<unsigned long long>(ntr::rt_frame()),unsigned(uniqueID),static_cast<void *>(mModel.data.modelFile),
            unsigned(mState),unsigned(mDelay));
''', 1)
    return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n' + text
