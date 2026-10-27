"""Read-only opt-in evidence for original Feather pickup and player wings."""


def transform(text):
    anchor = 'int daFeather_c::Behavior()\n{'
    assert text.count(anchor) == 1
    helper = r'''
static void pc_feather_observe(const char *event, daFeather_c *self, Player *player=nullptr) {
    const char *path=std::getenv("SM64DS_PC_FEATHER_TRACE");
    if(!path || !*path)return;
    static FILE *file=nullptr;
    if(!file) {
        file=std::fopen(path,"w");
        if(file)std::fputs("event,tick,uid,x,y,z,life,other,player,character,wings,power_timer\n",file);
    }
    if(file) {
        std::fprintf(file,"%s,%llu,%u,%d,%d,%d,%u,%u,%u,%u,%u,%u\n",event,
            static_cast<unsigned long long>(ntr::rt_frame()),unsigned(self->uniqueID),
            self->mPosX,self->mPosY,self->mPosZ,unsigned(self->mLifeTimer),
            unsigned(self->mdCcAc_c.otherOwner),player?unsigned(player->uniqueID):0,
            player?unsigned(player->param1):255,player?unsigned(player->mHasWings):0,
            player?unsigned(player->mPowerupTimer):0);
        std::fflush(file);
    }
}
'''
    text = text.replace('// @symbol _ZN11daFeather_c8BehaviorEv',
                        helper+'\n// @symbol _ZN11daFeather_c8BehaviorEv', 1)
    text = text.replace(anchor, anchor+'\n    if(ntr::rt_frame()%5==0)pc_feather_observe("behavior",this);', 1)
    anchor = '                ((Player *)a)->InitWingFeathers(true);'
    assert text.count(anchor) == 1
    text = text.replace(anchor, '                pc_feather_observe("pickup",this,(Player *)a);\n'+anchor+
                        '\n                pc_feather_observe("wings",this,(Player *)a);', 1)
    return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n'+text
