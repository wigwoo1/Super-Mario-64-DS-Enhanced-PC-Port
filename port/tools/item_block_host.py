"""Native dispatch/lifetime adapter for the recovered item-block class.

The original callback identities are symbol-only code bindings. Private ROM
audits verify them; no cartridge data or ARM function address is emitted here.
"""
import re

CALLBACK_GROUPS = {
    '02149d80': ('0214e278',),
    '02149ccc': ('0214e260',),
    '02149c78': ('0214e268',),
    '021498e0': ('0214e258',),
    '021498c4': ('0214e3a0',),
    '02149878': ('0214e398',),
    '021494cc': ('0214e388','0214e380','0214e378','0214e370'),
    '02149478': ('0214e368','0214e360','0214e358','0214e350'),
    '02149428': ('0214e348','0214e340','0214e320','0214e288'),
    '02149384': ('0214e338','0214e330','0214e280','0214e318'),
    '021493dc': ('0214e2a8','0214e2b0','0214e270','0214e298'),
    '021492d4': ('0214e2a0','0214e310','0214e308','0214e300','0214e290'),
    '02149288': ('0214e2f8','0214e2f0','0214e2e8','0214e390',
                 '0214e2d8','0214e2d0','0214e2c8','0214e2b8','0214e2e0','0214e328'),
    '02149220': ('0214e2c0',),
}
BINDINGS = {'data_ov102_'+record: 'func_ov102_'+method
            for method, records in CALLBACK_GROUPS.items() for record in records}
assert len(BINDINGS) == 42


def transform(text):
    original = ('struct C;\ntypedef void (C::*PMF)();\n'
                'struct Entry { PMF pmf[2]; };\nstruct C {};')
    assert text.count(original) == 1, 'item-block dispatch types changed'
    text = text.replace(original, 'using C = daObjHatenaBlock_c;\n'
                        'using PMF = void (*)(daObjHatenaBlock_c *);\n'
                        'struct Entry { PMF pmf[2]; };', 1)
    # Typed free-function adapters preserve the exact original target method,
    # including spawners whose ignored result is a pointer. No member-pointer
    # representation or incompatible calling convention is assumed.
    names = re.findall(r'^extern PMF (\w+);', text, re.M)
    assert set(names) == set(BINDINGS), 'item-block callback inventory changed'
    for name, method in BINDINGS.items():
        declaration = f'extern PMF {name};'
        code = (f'static void pc_item_{name}(daObjHatenaBlock_c *self) {{ self->{method}(); }}\n'
                f'static PMF {name} = &pc_item_{name};')
        text = text.replace(declaration, code, 1)
    for column in (0, 1):
        old = f'(((C *)this)->*data_ov102_0214e890[j].pmf[{column}])();'
        assert text.count(old) == 1
        text = text.replace(old, f'data_ov102_0214e890[j].pmf[{column}](this);')
    assert text.count('(self->*tbl[content][0])();') == 1
    assert text.count('(self->*tbl[content][ch])();') == 2
    text = text.replace('(self->*tbl[content][0])();','tbl[content][0](self);')
    text = text.replace('(self->*tbl[content][ch])();','tbl[content][ch](self);')
    for kind in ('HatenaBlockModelFilePtr','HatenaBlockAnimFileHandle','HatenaBlockClsnFileHandle'):
        old=f'{kind}(unsigned int id) {{ pc_sharedfile_handle_construct(this, id); }}'
        assert text.count(old) == 1, kind+' host constructor changed'
        text = text.replace(old,f'{kind}(unsigned int id) {{ unk0=id;file=nullptr; }}',1)
        old=f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old)==1, kind+' host destructor changed'
        text = text.replace(old,f'~{kind}() {{}}',1)
    handles = re.findall(r'^HatenaBlock\w+\s+(\w+)\((0x[0-9a-f]+)\)',text,re.M)
    assert len(handles) == 8
    initialization = '\n    static bool ready=false;\n    if(!ready) {\n'
    initialization += ''.join(f'        {name}.Construct({number});\n' for name,number in handles)
    initialization += '        ready=true;\n    }\n'
    signature = 'int daObjHatenaBlock_c::InitResources()\n{'
    assert text.count(signature) == 1
    text = text.replace(signature,signature+initialization,1)
    # Recovered return values are discarded by the retail hit dispatch. Give
    # their host-only copies a defined value without changing hit eligibility.
    for method,arguments in (('OnAttacked1','dActor_c &other'),
                             ('OnHitFromUnderneath','dActor_c &other'),
                             ('func_ov102_02149878','')):
        signature = f'int daObjHatenaBlock_c::{method}({arguments})'
        start = text.index(signature)
        end = text.find('// @symbol',start+len(signature))
        if end < 0: end = len(text)
        body=text[start:end];close=body.rfind('}')
        body=body[:close].replace('return;','return 0;')+'    return 0;\n'+body[close:]
        text=text[:start]+body+text[end:]
    trace = '''
    if(const char *path=std::getenv("SM64DS_PC_ITEM_BLOCK_TRACE")) {
        static FILE *file=nullptr;
        if(!file) {file=std::fopen(path,"a");if(file)std::fputs("tick,uid,actor,x,y,z,state,timer,content,star,tracked,collider\\n",file);}
        if(file && ntr::rt_frame()%5==0) {
            std::fprintf(file,"%llu,%u,%u,%d,%d,%d,%d,%u,%u,%u,%u,%u\\n",
                static_cast<unsigned long long>(ntr::rt_frame()),unsigned(uniqueID),unsigned(actorID),
                mPosX,mPosY,mPosZ,mState,unsigned(mBounceTimer),unsigned(mContentType),
                unsigned(mStarId),unsigned(mStarTracked),unsigned(mMeshCollider.IsEnabled()));
            std::fflush(file);
        }
    }
'''
    signature='int daObjHatenaBlock_c::Behavior()\n{'
    assert text.count(signature)==1
    text=text.replace(signature,signature+trace,1)
    signature='int daObjHatenaBlock_c::OnHitFromUnderneath(dActor_c &other)\n{'
    assert text.count(signature)==1
    text=text.replace(signature,signature+'''
    if(std::getenv("SM64DS_PC_POWER_FLOWER_TRACE"))
        std::fprintf(stderr,"[native item underside] tick=%llu uid=%u attacker=%u state=%d underwater=%u\\n",
            static_cast<unsigned long long>(ntr::rt_frame()),unsigned(uniqueID),unsigned(other.param1),
            mState,unsigned(ClosestPlayer()->mIsUnderwater));
''',1)
    text+='''
static_assert(sizeof(daObjHatenaBlock_c)==0x3f8,"native item-block layout");
static_assert(offsetof(daObjHatenaBlock_c,mModelAnim)==0x320,"native item-block animation");
static_assert(offsetof(daObjHatenaBlock_c,mShadowModel)==0x384,"native item-block shadow");
static_assert(offsetof(daObjHatenaBlock_c,mState)==0x3e8,"native item-block state");
static_assert(offsetof(daObjHatenaBlock_c,mHeldActor)==0x3f4,"native item-block held actor");
static_assert(sizeof(PMF)==4 && sizeof(Entry)==8,"native typed dispatch stride");
static_assert(sizeof(HatenaBlockModelFilePtr)==8,"native shared model handle");
static_assert(sizeof(HatenaBlockAnimFileHandle)==8,"native shared animation handle");
static_assert(sizeof(HatenaBlockClsnFileHandle)==8,"native shared collision handle");
'''
    return '#include <cstddef>\n#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n'+text


def trace_ceiling(text):
    signature='int func_ov002_020eeca8(void *c, int arg)\n{'
    assert text.count(signature)==1
    text=text.replace(signature,signature+'''
    if(((dActor_c *)arg)->actorID==191)pc_trace_jump_input((void *)arg,"ceiling-entry");
    if(std::getenv("SM64DS_PC_POWER_FLOWER_TRACE")) {
        const bool hit=func_02035638(c)!=0;
        const int owner=hit?_ZNK5dBgPi9GetClsnIDEv((void *)func_0203567c((int)c)):-1;
        const auto *mover=(dActor_c *)arg;
        const auto *target=owner!=-1?dActor_c::FindWithID(owner):nullptr;
        std::fprintf(stderr,"[native ceiling dispatch] tick=%llu mover=%u offset=%d hit=%d owner=%d kind=%d\\n",
            static_cast<unsigned long long>(ntr::rt_frame()),unsigned(mover->actorID),
            int((char *)c-(char *)mover),int(hit),owner,target?int(target->actorID):-1);
    }
''',1)
    return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\nextern "C" void pc_trace_jump_input(const void *,const char *);\n'+text


def trace_player_collision(text):
    # The short head ray is separate from the actor's sphere ceiling result.
    # Retail invokes slot 28 with r0/r1; an x86 raw function pointer instead
    # passes both arguments on the stack and loses the native method's ECX.
    # Preserve the original owner lookup and eligibility, and let C++ dispatch
    # the same virtual method using the host ABI.
    raw_hit = '(*(void (**)(void *, char *))(*(int *)a + 0x70))(a, self);'
    assert text.count(raw_hit)==1, 'Player short head-ray callback changed'
    text=text.replace(raw_hit, '''{
                    if(std::getenv("SM64DS_PC_POWER_FLOWER_TRACE"))
                        std::fprintf(stderr,"[native head dispatch] tick=%llu hit=1 owner=%u kind=%u\\n",
                            static_cast<unsigned long long>(ntr::rt_frame()),
                            unsigned(((dActor_c *)a)->uniqueID),unsigned(((dActor_c *)a)->actorID));
                    ((dActor_c *)a)->OnHitFromUnderneath(*(dActor_c *)self);
                }''',1)
    for anchor,label in (
        ('func_020383e4(c + 0x380);','collision-query'),
        ('((Player *)(c))->func_ov002_020c1234();','collision-return'),
        ('((Player *)(c))->func_ov002_020c61ac();','collision-post-state'),
    ):
        assert text.count(anchor)==1, label
        text=text.replace(anchor,anchor+'\n    pc_trace_jump_input(c,"'+label+'");',1)
    return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\nextern "C" void pc_trace_jump_input(const void *,const char *);\n'+text
