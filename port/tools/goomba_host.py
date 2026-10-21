"""Native Goomba callback/resource adaptation; cartridge sources stay intact."""
import re


def transform(text):
    for actor, hidden, matrix in (
            ('daCoin_c', '!(mCoinFlags&1)', '&mCommonModel1.mat4x3'),
            ('daBmb_c', '!mShouldRender', '&mModelAnim.mat4x3')):
        render='int '+actor+'::Render()\n{'
        if render in text and 'pc_render_probe::Scope renderProbe' not in text:
            if text.count(render)!=1:raise ValueError(actor+' render entry changed')
            text=text.replace(render,render+'\n    pc_render_probe::Scope renderProbe(this,'+hidden+','+matrix+');')
            text=text.replace('#include "common.h"','#include "common.h"\n#include "hal/actor_render_probe.h"',1)
    if 'int func_ov002_020d674c(char* c)' in text and 'o->vt[0x12](o)' in text:
        return ('extern int pc_actor_yoshi_try_eat(void *);\n'+
                text.replace('o->vt[0x12](o)', 'pc_actor_yoshi_try_eat(o)'))
    if 'int daYegg_c::InitResources()' in text:
        text=text.replace('struct Vec3 tmp = ((struct VObj *)o)->v30();',
                          'Vector3 tmp = o->OnAimedAtWithEggReturnVec();')
        text=text.replace('((VObjQuery *)actor)->s20()', 'actor->Virtual50()')
        return '#include <stddef.h>\n'+text+'\nstatic_assert(sizeof(daYegg_c)==0x42c && offsetof(daYegg_c,mPlayer)==0x38c,"native egg layout");\n' if 'native egg layout' not in text else text
    if 'int Player::St_Swallow_Init()' in text:
        text=text.replace('int(*f)(void*)=*(int(**)(void*))(*(char**)o+0x48);',
                          'dActor_c *actor = static_cast<dActor_c *>(o);')
        return text.replace('if(f(o)==4)', 'if(actor->OnYoshiTryEat()==4)')
    if 'int Player::St_Swallow_Main()' in text:
        text=text.replace('(*(void (**)(void*, char*))(*(char**)p360 + 0x4c))(p360, ((char*)this));',
                          'static_cast<dActor_c *>(p360)->OnTurnIntoEgg(*this);')
        return text.replace('(*(int (**)(void*))(*(char**)q + 0x48))(q)',
                            'static_cast<dActor_c *>(q)->OnYoshiTryEat()')
    if 'int func_ov002_020d6998(char* self)' in text and '((VObj*)other)->V18()' in text:
        return ('extern "C" int pc_actor_yoshi_try_eat(void *);\n' +
                text.replace('((VObj*)other)->V18()', 'pc_actor_yoshi_try_eat(other)'))
    if 'int func_ov002_020d6790(char* self)' in text and 'o->v18()' in text:
        text=text.replace('o->v18()', 'pc_actor_yoshi_try_eat(o)')
        text=text.replace('(*(Obj**)(self + 0x360))->v19(self);',
                          'pc_actor_turn_into_egg(*(void **)(self + 0x360), self);')
        return ('extern "C" int pc_actor_yoshi_try_eat(void *);\n'
                'extern "C" void pc_actor_turn_into_egg(void *, void *);\n' + text)
    egg_call='(*(Obj**)(&mObjInMouth))->v19(c);'
    if 'int Player::St_YoshiPower_Main()' in text and egg_call in text:
        text,count=re.subn(r'struct Obj \{[\s\S]*?\n\};','using Obj = dActor_c;',text,count=1)
        if count!=1: raise ValueError('Yoshi target virtual layout changed')
        return text.replace(egg_call,'(*(Obj**)(&mObjInMouth))->OnTurnIntoEgg(*this);').replace(
            '->v18()', '->OnYoshiTryEat()')
    for veneer, method in (
            ('func_02038414','UpdateDiscreteNoLava_2'),
            ('dBgCh_Actr_UpdateDiscreteNoLava_veneer','UpdateDiscreteNoLava')):
        if 'void '+veneer+'(void)' in text:
            bridge='_ZN10dBgCh_Actr'+str(len(method))+method+'Ev'
            return ('extern void '+bridge+'(void *self);\nvoid '+veneer+
                    '(void *self) { '+bridge+'(self); }\n')
    anchor = 'typedef void (daKrb_c::*StateHandler)();'
    if anchor not in text:
        return text
    render='int daKrb_c::Render()\n{'
    if text.count(render)!=1:raise ValueError('Goomba render entry changed')
    text='#include "hal/actor_render_probe.h"\n'+text.replace(render,render+
        '\n    pc_render_probe::Scope renderProbe(this,mIsDormant,&mModelAnim.mat4x3);')
    init='int daKrb_c::InitResources()\n{'
    if text.count(init)!=1:raise ValueError('Goomba resource initializer changed')
    text='#include "hal/pc_session.h"\n'+text.replace(init,init+
        '\n    param1=pc_test_conditional_goomba_param(param1, data_0209f2f8);')
    text = text.replace(anchor, '''struct PcGoombaState {
    void (*fn)(daKrb_c *);
    int adjustment;
};
typedef PcGoombaState StateHandler;
static_assert(sizeof(StateHandler)==8,"Goomba state descriptor stride");
static void pc_goomba_airborne(daKrb_c *self) { self->func_ov084_0212aab0(); }
static_assert(sizeof(daKrb_c)==0x478,"Goomba native layout");
static_assert(offsetof(daKrb_c,mState)==0x434,"Goomba state offset");''')
    text = text.replace('union KrbFreeHandler {\n    void (*fn)(daKrb_c *goomba);\n    StateHandler pmf;\n};',
                        'struct KrbFreeHandler { StateHandler pmf; };')
    text = text.replace('(this->*data_ov084_02130d74[st])();',
                        'data_ov084_02130d74[st].fn(this);')
    for name in ('Tumble', 'HopStart', 'Walk', 'Pause'):
        pattern = r'(KrbFreeHandler krbStateDesc_'+name+r' = )\{ (\w+) \};'
        text, count = re.subn(pattern, r'\1{ { \2, 0 } };', text)
        if count != 1: raise ValueError('Goomba free state descriptor changed: '+name)
    text = text.replace('&daKrb_c::func_ov084_0212aab0,', '{pc_goomba_airborne, 0},')
    # Defer handle construction until the native overlay registry is ready.
    for kind in ('KrbModelFilePtr', 'KrbAnimationFileHandle'):
        text = text.replace(kind+'(u32 fileID);', kind+'(u32 fileID) : id(fileID), ptr(0) {}')
        text = text.replace('~'+kind+'();', '~'+kind+'() {}')
    handles = re.findall(r'(Krb(?:ModelFilePtr|AnimationFileHandle)) (data_ov084_[0-9a-f]+)\((0x[0-9a-f]+)\);', text)
    if len(handles) != 9: raise ValueError('Goomba resource handle manifest changed')
    text += '\nextern "C" void *_ZN13SharedFilePtr9ConstructEj(void *, unsigned);\n'
    text += 'extern "C" void pc_goomba_init_resources() {\nstatic bool ready=false; if(ready)return; ready=true;\n'
    for _, name, file_id in handles:
        text += f'_ZN13SharedFilePtr9ConstructEj(&{name}, {file_id});\n'
    text += '}\n'
    text += 'static_assert(sizeof(KrbModelFilePtr)==8 && offsetof(KrbModelFilePtr,ptr)==4,"Goomba model handle layout");\n'
    text += 'static_assert(sizeof(KrbAnimationFileHandle)==8 && offsetof(KrbAnimationFileHandle,ptr)==4,"Goomba animation handle layout");\n'
    return '#include <stddef.h>\n' + text
