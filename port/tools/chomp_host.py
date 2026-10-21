"""Native ABI adapters for the recovered Chain Chomp, stump and fence.

Retail source stays untouched. ROM records are inspected locally; callbacks
and profiles in the generated build use native pointers, never ARM addresses.
"""
import re
import struct


ACTORS = {
    'd_a_wanwan.cpp': (14, 0x02114750, 0x02112d1c, 'WANWAN', 'daWanwan_c', 'chomp'),
    'daObjPile_c.cpp': (91, 0x02135298, 0x02133938, 'PILE', 'daObjPile_c', 'pile'),
    'd_a_obj_wanwan_shutter.cpp': (14, 0x0211488c, 0x0211307c, 'WANWAN_SHUTTER', 'daObjWanwanShutter_c', 'wanwan_fence'),
}
CALLBACKS = (
    (0x021146e0, 0x02111e74), (0x021146b8, 0x02111e14),
    (0x021146f0, 0x02111dc4), (0x021146c0, 0x02111ca8),
    (0x021146a8, 0x02111b70), (0x021146a0, 0x02111af0),
    (0x021146f8, 0x02111a6c), (0x021146b0, 0x021115ec),
    (0x021146d8, 0x021115c0), (0x021146c8, 0x0211150c),
    (0x021146e8, 0x021114d8), (0x021146d0, 0x02111484),
)


def overlay(root, number):
    config = (root / f'config/arm9/overlays/ov{number:03d}/delinks.txt').read_text()
    base = int(re.search(r'\.text\s+start:0x([\da-f]+)', config)[1], 16)
    return (root / f'extracted/overlays/overlay_{number:04d}.bin').read_bytes(), base


def inventory(root, name):
    number, address, factory, *_ = ACTORS[name]
    raw, base = overlay(root, number)
    profile = struct.unpack_from('<IHHIiiii', raw, address - base)
    assert profile[0] == factory, 'Chain Chomp family profile factory changed'
    if name == 'd_a_wanwan.cpp':
        for descriptor, target in CALLBACKS:
            assert struct.unpack_from('<II', raw, descriptor-base) == (target, 0), 'Chomp callback changed'
    return profile[1:]


def transform(root, text, name):
    if name == 'dActor_c.cpp':
        old = '    if (flags & 0x20)\n        goto ret0;'
        assert text.count(old) == 1
        text = text.replace(old, '''    if (flags & 0x20) {
        // These three original actors start with the distance-hide bit set
        // until first entering retail range. Permit a PC distant first draw
        // only in a showing area, without advancing their gameplay.
        const bool firstChompView=(actorID==219 || actorID==27 || actorID==41) &&
            ntr::video_get_settings().render_distance!=ntr::RenderDistance::Minimum &&
            (flags&0x18) && (mAreaId<0 || IsAreaShowing(mAreaId)) &&
            mCamSpacePosZ!=0x7fffffff;
        if(!firstChompView)goto ret0;
    }''')
        old = '((flags&8)?0x40:0)|((flags&0x10)?0x20:0));'
        assert text.count(old) == 1
        text = text.replace(old, '((flags&8)?0x40:0)|((flags&0x10)?0x20:0)|((flags&0x20)?0x10:0));')
        old = '                mFlags&=~0x18;'
        assert text.count(old) == 1
        text = text.replace(old, '                mFlags&=~0x38;')
        old = '        mFlags&=~0x18;\n        if(mClipResult&0x40)'
        assert text.count(old) == 1
        text = text.replace(old, '        mFlags&=~0x18;\n        if(mClipResult&0x10)mFlags|=0x20;\n        if(mClipResult&0x40)')
        return text
    if name not in ACTORS:
        return text
    _, _, _, profile_name, actor, prefix = ACTORS[name]
    values = inventory(root, name)
    # hostgen has supplied wrapper constructors. Delay actual filesystem work
    # until the native registry is initialized, as for Buddy and the cannon.
    kinds = re.findall(r'struct (\w+) : SharedFilePtr', text)
    for kind in kinds:
        pattern = rf'{kind}\(unsigned int (\w+)\) \{{ pc_sharedfile_handle_construct\(this, \1\); \}}'
        text, count = re.subn(pattern, lambda m: f'{kind}(unsigned int {m[1]}) {{ words[0]={m[1]};words[1]=0; }}', text)
        assert count == 1, kind+' constructor changed'
        text = text.replace(f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}', f'~{kind}() {{}}')
    handles = re.findall(r'^(\w+) (data_\w+)\((0x[\da-f]+|\d+)\);$', text, re.M)
    assert len(handles) == (4 if name == 'd_a_wanwan.cpp' else 2)
    suffix = ['\nstruct PcChompProfile { void *factory; unsigned short behaviorPriority, renderPriority; unsigned flags; int clipOffsetY, clipRadius, clipDistance, farDistance; };',
              'static_assert(sizeof(PcChompProfile)==28,"Chomp profile ABI");',
              f'extern "C" PcChompProfile g_profile_{profile_name} = {{(void *)&{actor}_classInit,' + ','.join(str(v) for v in values) + '};',
              f'extern "C" void pc_{prefix}_init_resources() {{',
              'static bool ready=false;if(ready)return;ready=true;']
    suffix += [f'{handle}.Construct({file_id});' for _, handle, file_id in handles]
    suffix += ['}']
    if name == 'd_a_wanwan.cpp':
        text = text.replace('#include "dCc_c.h"', '#include "dCc_c.h"\n#include "hal/actor_render_probe.h"', 1)
        text = text.replace('int daWanwan_c::Render()\n{', 'int daWanwan_c::Render()\n{\n    pc_render_probe::Scope renderProbe(this,0,&mModelAnim.mat4x3);')
        start = text.index('extern "C" daWanwan_c *daWanwan_c_classInit()\n{')
        end = text.index('\n// @symbol', start)
        text = text[:start] + 'extern "C" daWanwan_c *daWanwan_c_classInit() { return new daWanwan_c(); }\n' + text[end:]
        text = text.replace('typedef void (WanwanState::*WanwanPmf)();', 'struct WanwanPmf { void (*call)(WanwanState *); int adjustment; };')
        for field in ('enter', 'update'):
            old = f'(c->*data_ov014_0211476c[j].{field})();'
            assert text.count(old) == 1
            text = text.replace(old, f'data_ov014_0211476c[j].{field}.call(c);')
        # Define descriptors immediately before the existing state table.
        marker = 'extern "C" WanwanStateRow data_ov014_0211476c[6] = {'
        native = []
        for descriptor, target in CALLBACKS:
            kind = 'u8' if target == 0x021115ec else 'char'
            native += [f'static void pc_chomp_{target:08x}(WanwanState *self) {{ func_ov014_{target:08x}(reinterpret_cast<{kind} *>(self)); }}',
                       f'extern "C" WanwanPmf data_ov014_{descriptor:08x} = {{pc_chomp_{target:08x},0}};']
        assert text.count(marker) == 1
        text = text.replace(marker, '\n'.join(native)+'\n'+marker)
        suffix += ['static_assert(sizeof(WanwanPmf)==8 && sizeof(WanwanStateRow)==20,"Chomp callbacks");',
                   'static_assert(sizeof(daWanwan_c)==0x620 && offsetof(daWanwan_c,mState)==0x610,"Chomp raw-offset ABI");',
                   'static_assert(offsetof(daWanwan_c,mLinkPos)==0x524 && offsetof(daWanwan_c,mLinkDelta)==0x578,"Chomp chain ABI");']
        suffix += [r'''
// Read-only release target. The original camera clears its one-frame target
// pointer at the end of Behavior, so resolve the live actor instead of keeping
// a pointer across actor destruction or course changes.
extern "C" int pc_chomp_release_target(int *position) {
    for(auto *a=dActor_c::FindWithActorID(219,nullptr);a;
        a=dActor_c::FindWithActorID(219,a)) {
        auto *c=static_cast<daWanwan_c *>(a);
        if(c->aliveState!=1 || c->mState!=3 || !c->mChainBroken || c->mReleaseStep>8)continue;
        position[0]=c->mPosX;position[1]=c->mPosY;position[2]=c->mPosZ;
        return 1;
    }
    return 0;
}
''']
    elif name == 'daObjPile_c.cpp':
        # MSVC does not reuse dBgActor_c's two bytes of tail padding. These
        # fields are only accessed by their native C++ names, including Chomp.
        suffix += ['static_assert(sizeof(daObjPile_c)==0x334 && offsetof(daObjPile_c,mStepsLeft)==0x320 && offsetof(daObjPile_c,mBusy)==0x322,"native stump tail padding");']
    else:
        suffix += ['static_assert(sizeof(daObjWanwanShutter_c)==0x324 && offsetof(daObjWanwanShutter_c,mDisabled)==0x320,"native fence tail padding");']
    return '#include <stddef.h>\n' + text + '\n' + '\n'.join(suffix) + '\n'
