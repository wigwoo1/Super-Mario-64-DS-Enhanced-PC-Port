"""Deferred native resources and read-only probes for original HEART297."""
import re


def transform(text):
    for kind in ('HeartModelFilePtr', 'HeartAnimationFilePtr'):
        pattern = rf'{kind}\(unsigned int (\w+)\) \{{ pc_sharedfile_handle_construct\(this, \1\); \}}'
        text, count = re.subn(pattern, lambda m:
            f'{kind}(unsigned int {m[1]}) {{ words[0]={m[1]};words[1]=0; }}', text)
        assert count == 1, kind + ' constructor'
        old = f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old) == 1, kind + ' destructor'
        text = text.replace(old, f'~{kind}() {{}}', 1)
    helper = r'''
static void pc_heart_observe(const char *event,daObjHeart_c *heart,int amount=0) {
    const char *path=std::getenv("SM64DS_PC_HEART_TRACE");
    if(!path || !*path)return;
    static FILE *file=nullptr;
    if(!file) {file=std::fopen(path,"w");if(file)std::fputs("event,tick,uid,x,y,z,timer,touched,anim_speed,anim_frame,other,hit,player,px,py,pz,player_speed,health,amount,model\n",file);}
    if(file) {
        auto *p=static_cast<Player *>(dActor_c::FindWithActorID(191,nullptr));
        std::fprintf(file,"%s,%llu,%u,%d,%d,%d,%u,%u,%d,%d,%u,%u,%u,%d,%d,%d,%d,%d,%d,%u\n",event,
            static_cast<unsigned long long>(ntr::rt_frame()),unsigned(heart->uniqueID),
            heart->mPosX,heart->mPosY,heart->mPosZ,unsigned(heart->mHealTimer),unsigned(heart->mWasTouched),
            heart->mAnimSpeed,heart->mModelAnim.currFrame,unsigned(heart->mdCcAc_c.otherOwner),
            unsigned(heart->mdCcAc_c.hitFlags),p?unsigned(p->uniqueID):0,p?p->mPosX:0,p?p->mPosY:0,
            p?p->mPosZ:0,p?p->mHorzSpeed:0,p?p->GetHealth():-1,amount,
            unsigned(reinterpret_cast<std::uintptr_t>(heart->mModelAnim.data.modelFile)));
        std::fflush(file);
    }
}
'''
    anchor = '// @symbol _ZN12daObjHeart_cD1Ev'
    assert text.count(anchor) == 1
    text = text.replace(anchor, helper + '\n' + anchor, 1)
    anchor = 's32 daObjHeart_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '''
    static bool ready=false;
    if(!ready) {
        data_ov002_0210e104.Construct(0x470);
        data_ov002_0210e0fc.Construct(0x471);
        ready=true;
    }
    pc_heart_observe("init",this);
''', 1)
    old = '    func_ov002_020bc664(this);\n    return 1;'
    assert text.count(old) == 1
    text = text.replace(old, '    func_ov002_020bc664(this);\n    pc_heart_observe("ready",this);\n    return 1;', 1)
    for method, event in (('CleanupResources','cleanup'), ('Behavior','behavior')):
        anchor = f's32 daObjHeart_c::{method}()\n{{'
        assert text.count(anchor) == 1
        condition = 'if(ntr::rt_frame()%5==0)' if method == 'Behavior' else ''
        text = text.replace(anchor, anchor + f'\n    {condition}pc_heart_observe("{event}",this);', 1)
    old = '                    ((Player *)other)->Heal(amount);'
    assert text.count(old) == 1
    text = text.replace(old, '                    pc_heart_observe("heal-before",this,amount);\n' + old +
                        '\n                    pc_heart_observe("heal-after",this,amount);', 1)
    anchor = 's32 daObjHeart_c::Render()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '\n    pc_render_probe::Scope probe(this,0,&mModelAnim.mat4x3);', 1)
    text += '''
static_assert(sizeof(HeartModelFilePtr)==8 && sizeof(HeartAnimationFilePtr)==8,"native heart file handles");
static_assert(sizeof(daObjHeart_c)==0x174,"native healing heart layout");
static_assert(offsetof(daObjHeart_c,mModelAnim)==0xd4,"native healing heart model");
static_assert(offsetof(daObjHeart_c,mdCcAc_c)==0x138,"native healing heart cylinder");
static_assert(offsetof(daObjHeart_c,mHealTimer)==0x170,"native healing heart timer");
'''
    return ('#include <cstddef>\n#include <cstdint>\n#include <cstdio>\n#include <cstdlib>\n'
            '#include "ntr/rt.h"\n#include "hal/actor_render_probe.h"\n' + text)
