"""Native ABI/startup adaptation for the original ov002 floor switches."""
import re


def transform(text, name):
    if name in ('d_a_obj_switch_hanswitch.c', 'd_a_obj_switch_star_switch.c'):
        factory = ('daObjSwitch_c_classInit_HANSWITCH' if 'hanswitch' in name
                   else 'daObjSwitch_c_classInit_STAR_SWITCH')
        assert factory in text and '_ZN7fBase_cnwEj(852)' in text
        # A host subclass can use different base tail padding. Allocate and
        # construct the real native class, including its native virtual table.
        return ('#include "common.h"\n#include "daObjSwitch_c.h"\n'
                f'extern "C" void *{factory}() {{ return new daObjSwitch_c(); }}\n')

    if name == 'func_ov002_020baa2c.c':
        for clause in ('0x706', '0x6f9', '0x703', '0x350', '0x98000'):
            assert clause in text, clause
        # The callback's owner is a derived class: its last byte does not stay
        # at the ARM offset when MSVC retains dBgActor_c's tail padding.
        return '''#include "common.h"
#include "daObjSwitch_c.h"
#include "Player.h"
extern "C" s32 Vec3_Dist(const Vector3 *, const Vector3 *);
extern "C" void func_ov002_020baa2c(void *self, void *other) {
    auto *actor=static_cast<dActor_c *>(other);
    if(actor->actorID!=191)return;
    auto *player=static_cast<Player *>(actor);
    if(player->mIsUnderwater && !player->mIsMetal && !player->mIsMega)return;
    auto *sw=static_cast<daObjSwitch_c *>(self);
    if(Vec3_Dist(reinterpret_cast<const Vector3 *>(&sw->mPosX),
                 reinterpret_cast<const Vector3 *>(&player->mPosX))<0x98000)
        sw->mPlayerNearby=1;
}
'''

    assert name == 'daObjSwitch_c.cpp', name
    for kind in ('SwitchModelFilePtr', 'SwitchClsnFileHandle'):
        pattern = rf'{kind}\(unsigned int (\w+)\) \{{ pc_sharedfile_handle_construct\(this, \1\); \}}'
        text, count = re.subn(pattern, lambda m:
                             f'{kind}(unsigned int {m[1]}) {{ words[0]={m[1]};words[1]=0; }}', text)
        assert count == 1, kind
        old = f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old) == 1, kind
        text = text.replace(old, f'~{kind}() {{}}', 1)

    anchor = 'extern daObjSwitch_c::Resources data_ov002_021098e8[];'
    assert text.count(anchor) == 1
    text = text.replace(anchor, 'static daObjSwitch_c::Resources pc_floor_switch_resources[2];')
    text = text.replace('data_ov002_021098e8[idx]', 'pc_floor_switch_resources[idx]')
    text = text.replace('data_ov002_021098e8[mResourceIdx]', 'pc_floor_switch_resources[mResourceIdx]')
    signature = 'int daObjSwitch_c::InitResources()\n{'
    assert text.count(signature) == 1
    text = text.replace(signature, 'extern "C" void pc_floor_switch_init_resources();\n'
                        + signature + '\n    pc_floor_switch_init_resources();', 1)
    # This field is named in daStar_c.h; retain the original null-player gate
    # without assuming its native offset equals the cartridge's offset.
    old = '*reinterpret_cast<int *>(reinterpret_cast<char *>(p) + 0x438) == 0'
    assert text.count(old) == 1
    text = text.replace(old, 'static_cast<daStar_c *>(p)->mPlayer == 0', 1)
    text += '''
extern "C" unsigned char data_ov002_0210d854[], data_ov002_0210d894[];
extern "C" void pc_floor_switch_init_resources() {
    static bool ready=false;if(ready)return;
    data_ov002_0210dfd4.Construct(0x45c);
    data_ov002_0210dfc4.Construct(0x45d);
    data_ov002_0210dfcc.Construct(0x48f);
    data_ov002_0210dfbc.Construct(0x490);
    pc_floor_switch_resources[0]={&data_ov002_0210dfd4,&data_ov002_0210dfc4,
                                 reinterpret_cast<CLPS_Block *>(data_ov002_0210d854)};
    pc_floor_switch_resources[1]={&data_ov002_0210dfcc,&data_ov002_0210dfbc,
                                 reinterpret_cast<CLPS_Block *>(data_ov002_0210d894)};
    ready=true;
}
'''
    trace_anchor = '    mPlayerNearby = 0;\n    return 1;'
    assert text.count(trace_anchor) == 1
    trace = '''    if(const char *path=std::getenv("SM64DS_PC_FLOOR_SWITCH_TRACE")) {
        static FILE *file=nullptr;
        if(!file){file=std::fopen(path,"w");if(file)std::fputs("tick,uid,actor,state,timer,limit,event,event_on,nearby,scale_y,displacement,collision,x,y,z\\n",file);}
        if(file && ntr::rt_frame()%5==0){
            std::fprintf(file,"%llu,%u,%u,%d,%u,%u,%u,%d,%u,%d,%d,%u,%d,%d,%d\\n",
                static_cast<unsigned long long>(ntr::rt_frame()),unsigned(uniqueID),unsigned(actorID),
                mState,unsigned(mTimer),unsigned(mTimeLimit),unsigned(mEventBit),
                actorID==11?_ZN5Event6GetBitEj(mEventBit):0,unsigned(mPlayerNearby),
                mDrawScale.y,mDisplacementY,unsigned(mMeshCollider.IsEnabled()),mPosX,mPosY,mPosZ);
            std::fflush(file);
        }
    }
'''
    text = text.replace(trace_anchor, trace + trace_anchor, 1)
    render = 'int daObjSwitch_c::Render()\n{'
    assert text.count(render) == 1
    text = text.replace(render, render + '\n    pc_render_probe::Scope probe(this,0,&mModel.mat4x3);', 1)
    return ('#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n'
            '#include "hal/actor_render_probe.h"\n'
            'extern "C" int _ZN5Event6GetBitEj(unsigned int);\n' + text)
