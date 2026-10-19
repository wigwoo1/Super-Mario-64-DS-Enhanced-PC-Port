"""Native callbacks and deferred resources for Bob-omb Buddy/cannon shutter."""
import re
import struct


def inventory(root):
    bounds=(root/'config/arm9/overlays/ov084/delinks.txt').read_text()
    base=int(re.search(r'\.text\s+start:0x([\da-f]+)',bounds)[1],16)
    rom=(root/'extracted/overlays/overlay_0084.bin').read_bytes()
    # Original initializer copies these six eight-byte descriptors in this order.
    order=(0x021309dc,0x021309ec,0x021309cc,0x021309e4,0x021309c4,0x021309d4)
    targets=[]
    for address in order:
        target,adjust=struct.unpack_from('<II',rom,address-base)
        if adjust or target not in (0x0212c92c,0x0212c8b0,0x0212c89c,0x0212c508,0x0212c4a0,0x0212c1a0):
            raise ValueError('Buddy callback changed')
        targets.append(target)
    return targets


def transform(root,text,name):
    queries={
        'func_0203842c.cpp':('o->v7(self)', 'dBgCh_Lin', 'o', 'self'),
        'func_0203859c.cpp':('e->v7(obj)', 'dBgCh_Lin', 'e', 'obj'),
    }
    if name in queries:
        old,kind,collider,query=queries[name]
        assert text.count(old)==1, name+' collision dispatch changed'
        return ('#include "dBgW.h"\n#include "'+kind+'.h"\n'+text.replace(old,
            f'reinterpret_cast<dBgW *>({collider})->DetectClsn(*reinterpret_cast<{kind} *>({query}))'))
    if name=='func_02038324.cpp':
        old='obj->m9(arg, func_020393b4(data_020a0c80[idx]), b, c, d);'
        assert text.count(old)==1
        return '#include "dBgW.h"\n'+text.replace(old,'''reinterpret_cast<dBgW *>(obj)->BeforeClsn(
        *static_cast<dBgPi *>(arg), reinterpret_cast<dActor_c *>(func_020393b4(data_020a0c80[idx])),
        *reinterpret_cast<Vector3 *>(b), reinterpret_cast<Vector3_16 *>(c), reinterpret_cast<Vector3_16 *>(d));''')
    if name=='dBgW_KcMbg.cpp':
        for kind in ('Gnd', 'Lin'):
            signature=f'int dBgW_KcMbg::DetectClsn(dBgCh_{kind} &ray)\n{{'
            assert text.count(signature)==1
            text=text.replace(signature,signature+f'''
    if(std::getenv("SM64DS_PC_MOVING_TRACE")) {{
        static unsigned calls=0;
        if(calls++<200)std::fprintf(stderr,"[moving entry {kind}] tick=%llu this=%p query=%p\\n",
            static_cast<unsigned long long>(ntr::rt_frame()),this,&ray);
    }}
''')
        anchor='    r = dBgW_Kc::DetectClsn(loc);'
        assert text.count(anchor)==1
        text=text.replace('        ray.clsnY = worldPos.y;', '''
        if(std::getenv("SM64DS_PC_MOVING_TRACE")) {
            static unsigned ground_traces=0;
            if(ground_traces++<100)std::fprintf(stderr,"[moving ground] tick=%llu probe=%d,%d,%d local=%d,%d,%d world=%d,%d,%d mat=%d,%d,%d inv=%d,%d,%d scale=%d\\n",
                static_cast<unsigned long long>(ntr::rt_frame()),probePos.x,probePos.y,probePos.z,
                localStart.x,localStart.y,localStart.z,worldPos.x,worldPos.y,worldPos.z,
                mat.t.x,mat.t.y,mat.t.z,invScaledMat.t.x,invScaledMat.t.y,invScaledMat.t.z,scale);
        }
        ray.clsnY = worldPos.y;''')
        return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n'+text.replace(anchor,anchor+'''
    if(std::getenv("SM64DS_PC_MOVING_TRACE")) {
        static unsigned traces=0;
        if(traces++<200)std::fprintf(stderr,"[moving sphere] tick=%llu owner=%p center=%d,%d,%d local=%d,%d,%d radius=%d local_radius=%d scale=%d inverse=%d hit=%d mat=%d,%d,%d inv=%d,%d,%d\\n",
            static_cast<unsigned long long>(ntr::rt_frame()),owner,
            sphere.centre.x,sphere.centre.y,sphere.centre.z,pos.x,pos.y,pos.z,
            sphere.radius,loc.radius,scale,invScale,r,mat.t.x,mat.t.y,mat.t.z,
            invScaledMat.t.x,invScaledMat.t.y,invScaledMat.t.z);
    }
''')
    if name=='d_a_red_bombhei.cpp':
        targets=inventory(root)
        old='typedef void (daRedBombhei_c::*RedBombheiStatePMF)();'
        assert text.count(old)==1
        text=text.replace(old,'struct RedBombheiStatePMF { void (*call)(daRedBombhei_c *); int adjustment; };')
        for field in ('update','enter'):
            old=f'(this->*data_ov084_02130dc4[j].{field})();'
            assert text.count(old)==1
            text=text.replace(old,f'data_ov084_02130dc4[j].{field}.call(this);')
        text=text.replace('extern SharedFilePtr data_ov084_02130da4;',
            'struct PcBuddyFile : SharedFilePtr { unsigned words[2]; };\nextern "C" PcBuddyFile data_ov084_02130da4;')
        text=text.replace('extern SharedFilePtr data_ov084_02130d9c;',
            'extern "C" PcBuddyFile data_ov084_02130d9c;')
        suffix=['\nstatic_assert(sizeof(daRedBombhei_c)==0x1f0,"native Buddy layout");',
                'static_assert(sizeof(daObjCannonShutter_c)==0x330,"Buddy shutter layout");',
                'static_assert(offsetof(daObjCannonShutter_c,mCannonOpen)==0x32e,"Buddy shutter flag offset");',
                'static_assert(sizeof(RedBombheiState)==16,"native Buddy callback stride");']
        for target in targets:
            suffix.append(f'static void pc_buddy_{target:08x}(daRedBombhei_c *self) {{ self->func_ov084_{target:08x}(); }}')
        suffix+=['extern "C" {','PcBuddyFile data_ov084_02130da4={},data_ov084_02130d9c={};',
                 'RedBombheiState data_ov084_02130dc4[3] = {']
        for i in range(0,6,2):
            suffix.append(f'{{{{pc_buddy_{targets[i]:08x},0}},{{pc_buddy_{targets[i+1]:08x},0}}}},')
        suffix+=['};','void pc_buddy_init_resources() {','static bool ready=false;if(ready)return;ready=true;',
                 'data_ov084_02130da4.Construct(0x2b9);data_ov084_02130d9c.Construct(0x2ba);','}','}']
        return text+'\n'+'\n'.join(suffix)+'\n'
    if name=='d_a_obj_cannon_shutter.cpp':
        for kind in ('CannonShutterModelFilePtr','CannonShutterClsnFilePtr'):
            pattern=rf'{kind}\(unsigned int (\w+)\) \{{ pc_sharedfile_handle_construct\(this, \1\); \}}'
            text,count=re.subn(pattern,lambda m:f'{kind}(unsigned int {m[1]}) {{ words[0]={m[1]};words[1]=0; }}',text)
            assert count==1,kind+' constructor changed'
            text=text.replace(f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}',f'~{kind}() {{}}')
        return text+'''
static_assert(sizeof(daObjCannonShutter_c)==0x330,"native shutter layout");
extern "C" void pc_shutter_init_resources() {
    static bool ready=false;if(ready)return;ready=true;
    data_ov002_0210e12c.Construct(0x45f);data_ov002_0210e124.Construct(0x460);
}
'''
    return text
