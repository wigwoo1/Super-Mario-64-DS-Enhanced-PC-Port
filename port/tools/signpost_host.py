"""Host dispatch and deferred resource initialization for the original signpost."""
import re


def transform(text):
    for old, new in (
        ('_ZN6Player14IsFrontSlidingEv(self->mHoldingPlayer)', 'self->mHoldingPlayer->IsFrontSliding()'),
        ('_ZN6Player17LostGrabbedObjectEv(self->mHoldingPlayer)', 'self->mHoldingPlayer->LostGrabbedObject()'),
        ('_ZN8dActor_c19DisappearPoofDustAtERK7Vector3(this, &vec2)', 'DisappearPoofDustAt(vec2)'),
    ):
        assert text.count(old) == 1, old
        text = text.replace(old, new)
    for kind in ('SignPostModelFilePtr', 'SignPostCollisionFilePtr'):
        pattern = rf'{kind}\(unsigned int fileID\) \{{ pc_sharedfile_handle_construct\(this, fileID\); \}}'
        text, count = re.subn(pattern, f'{kind}(unsigned int fileID) {{words[0]=fileID;words[1]=0;}}', text)
        assert count == 1, kind
        old = f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old) == 1
        text = text.replace(old, f'~{kind}() {{}}')
    old = 'typedef void (daObjTatefuda_c::*PMF)();'
    assert text.count(old) == 1
    text = text.replace(old, 'typedef void (*PMF)(daObjTatefuda_c *);')
    for index in (0, 1):
        old = f'(this->*data_ov002_0210e084[j].pmf[{index}])();'
        assert text.count(old) == 1
        text = text.replace(old, f'data_ov002_0210e084[j].pmf[{index}](this);')
    start = text.index('Entry data_ov002_0210e084[5] = {')
    tail = text[start:]
    methods = re.findall(r'&daObjTatefuda_c::(\w+)', tail)
    assert len(methods) == 10
    wrappers = ''.join(f'static void pc_sign_dispatch_{i}(daObjTatefuda_c *self) {{ self->{method}(); }}\n'
                       for i, method in enumerate(methods))
    text = text[:start] + wrappers + re.sub(r'&daObjTatefuda_c::(\w+)',
        lambda m: '&pc_sign_dispatch_' + str(methods.index(m[1])), tail)
    signature = 'int daObjTatefuda_c::InitResources()\n{'
    assert text.count(signature) == 1
    text = text.replace(signature, signature + '''
    static bool ready=false;
    if(!ready) {
        SignPost_ModelFile.Construct(0x491);
        SignPost_ClsnFile.Construct(0x492);
        ready=true;
    }
''', 1)
    signature = 'int daObjTatefuda_c::Behavior()'
    at = text.index('{', text.index(signature)) + 1
    text = text[:at] + '''
    if(const char *path=std::getenv("SM64DS_PC_SIGN_TRACE")) {
        static FILE *file=nullptr;
        if(!file) {file=std::fopen(path,"a");if(file)std::fputs("tick,uid,param,state,talk_step,x,y,z,hidden,pounds,hit,other,alive\\n",file);}
        if(file && (ntr::rt_frame()%5==0 || mdCcAc_c.hitFlags)) {
            std::fprintf(file,"%llu,%u,%u,%d,%u,%d,%d,%d,%u,%u,%u,%u,%u\\n",
                static_cast<unsigned long long>(ntr::rt_frame()),unsigned(uniqueID),unsigned(param1),
                int(mState),unsigned(mTalkStep),mPosX,mPosY,mPosZ,unsigned(mHidden),
                unsigned(mPoundsLeft),unsigned(mdCcAc_c.hitFlags),unsigned(mdCcAc_c.otherOwner),unsigned(aliveState));
            std::fflush(file);
        }
    }
''' + text[at:]
    return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n' + text
