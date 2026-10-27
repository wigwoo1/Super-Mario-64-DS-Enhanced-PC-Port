"""Typed native state dispatch for the recovered Power Flower actor."""

STATE_METHODS = (
    ('BeginLaunched', 'StateLaunched'),
    ('BeginOpening', 'StateOpening'),
    ('BeginResting', 'StateResting'),
)


def transform(text):
    old = 'typedef void (daObjPowerUpItem_c::*FlowerPMF)();'
    assert text.count(old) == 1
    text = text.replace(old, 'typedef void (*FlowerPMF)(daObjPowerUpItem_c *);')
    for column in (0, 1):
        old = f'(this->*data_ov002_021097bc[j].pmf[{column}])();'
        assert text.count(old) == 1
        text = text.replace(old, f'data_ov002_021097bc[j].pmf[{column}](this);')
    anchor = 'FlowerEntry data_ov002_021097bc[3] = {'
    assert text.count(anchor) == 1
    wrappers = ''.join(
        f'static void pc_flower_dispatch_{i}(daObjPowerUpItem_c *self) {{ self->{method}(); }}\n'
        for i, method in enumerate(method for pair in STATE_METHODS for method in pair))
    text = text.replace(anchor, wrappers + anchor)
    for i, method in enumerate(method for pair in STATE_METHODS for method in pair):
        old = f'&daObjPowerUpItem_c::{method}'
        assert text.count(old) == 1
        text = text.replace(old, f'&pc_flower_dispatch_{i}')
    # Observe actual pickup eligibility and original player power state, without
    # writing actor fields or invoking a reward from the test harness.
    anchor = 'typedef void (*FlowerPMF)(daObjPowerUpItem_c *);'
    observation = '''static void pc_flower_observe(const char *event, daObjPowerUpItem_c *self, Player *player=nullptr) {
    const char *path=std::getenv("SM64DS_PC_POWER_FLOWER_TRACE");
    if(!path)return;
    static FILE *file=nullptr;
    if(!file) {
        file=std::fopen(path,"w");
        if(file)std::fputs("event,tick,uid,param,state,x,y,z,life,other,player,character,player_flags,vanish,power_timer,metal\\n",file);
    }
    if(file) {
        std::fprintf(file,"%s,%llu,%u,%u,%d,%d,%d,%d,%u,%u,%u,%u,%u,%u,%u,%u\\n",event,
            static_cast<unsigned long long>(ntr::rt_frame()),unsigned(self->uniqueID),unsigned(self->param1),
            self->mState,self->mPosX,self->mPosY,self->mPosZ,unsigned(self->mLifeTimer),
            unsigned(self->mdCcAc_c.otherOwner),player?unsigned(player->uniqueID):0,
            player?unsigned(player->param1):255,player?unsigned(player->mStateFlags):0,
            player?unsigned(player->mIsVanish):0,player?unsigned(player->mPowerupTimer):0,
            player?unsigned(player->mIsMetal):0);
        std::fflush(file);
    }
}
'''
    text = text.replace(anchor, observation + anchor, 1)
    anchor = '    switch (player->param1) {'
    assert text.count(anchor) == 1
    text = text.replace(anchor, '    pc_flower_observe("pickup",this,player);\n' + anchor)
    anchor = '        player->InitFireYoshi();'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor+'\n        pc_flower_observe("fire_yoshi",this,player);')
    anchor = '        player->InitVanishLuigi();'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor+'\n        pc_flower_observe("vanish_luigi",this,player);')
    anchor = '        player->InitMetalWario();'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor+'\n        pc_flower_observe("metal_wario",this,player);')
    anchor = 'int daObjPowerUpItem_c::Behavior()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor+'\n    if(ntr::rt_frame()%5==0)pc_flower_observe("behavior",this);')
    text += '''
static_assert(sizeof(daObjPowerUpItem_c)==0x3cc,"native Power Flower layout");
static_assert(offsetof(daObjPowerUpItem_c,mdCcAc_c)==0x1cc,"native flower contact collider");
static_assert(offsetof(daObjPowerUpItem_c,mWithMeshClsn)==0x200,"native flower mesh collision");
static_assert(offsetof(daObjPowerUpItem_c,mState)==0x3c0,"native flower state");
static_assert(sizeof(FlowerPMF)==4 && sizeof(FlowerEntry)==12,"native flower table stride");
'''
    return '#include <cstddef>\n#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n' + text
