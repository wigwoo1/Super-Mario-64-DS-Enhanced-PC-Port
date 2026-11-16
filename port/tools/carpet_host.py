"""Host adaptation of the original Rainbow Ride flying carpet and path base."""
import re
import struct

from battlefield_platform_host import observe


def transform(root, text, name):
    if name == 'dPathLiftActor_c.cpp':
        # Both callers refer to the same eight-byte ROM resource handle.
        text = text.replace('extern PathStuff data_ov002_0210d9f0;',
                            'extern "C" PathStuff data_ov002_0210d9f0;')
        # At the first node the authored source and destination coincide.
        # Retail _s32_div_f branches to _L1f8 for denominator zero and
        # returns the numerator; x86 integer division would raise an exception.
        for numerator in ('pitchDiff','yawDiff'):
            old=f'{numerator} / segFrames'
            assert text.count(old)==1
            text=text.replace(old,f'pc_path_div({numerator},segFrames)')
        helper='static int pc_path_div(int n,int d){if(!d)return n;if(n==(-2147483647-1)&&d==-1)return n;return n/d;}\n'
        return helper + text + '\nstatic_assert(sizeof(dPathLiftActor_c)==0x450,"native path lift layout");\n'
    if name == 'daObjPathLift_c.cpp':
        return transform_path_lift(root, text)
    raw = (root/'extracted/overlays/overlay_0036.bin').read_bytes()
    base = 0x021111a0
    assert struct.unpack_from('<IHHIiiii', raw, 0x02113f78-base) == (
        0x02112538,130,260,2,0,0x300000,0x1000000,0)
    # The recovered factory writes cartridge vtable address points manually.
    # Let the host compiler construct all members and its native vtable.
    text,n = re.subn(r'(extern "C" daObjRcCarpet_c \*daObjRcCarpet_c_classInit\(\)\n)\{.*?\n\}',
                     r'\1{ return new daObjRcCarpet_c(); }',text,flags=re.S)
    assert n==1
    for kind in ('CarpetModelFilePtr','CarpetAnimationFilePtr','CarpetCollisionFilePtr'):
        pattern = rf'{kind}\((?:unsigned int|u32) (\w+)\) \{{ pc_sharedfile_handle_construct\(this, \1\); \}}'
        text,n = re.subn(pattern,lambda m:f'{kind}(u32 {m[1]}) {{words[0]={m[1]};words[1]=0;}}',text)
        assert n == 1,kind
        old = f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old)==1
        text = text.replace(old,f'~{kind}() {{}}')
    anchor = 'int daObjRcCarpet_c::InitResources()\n{'
    assert text.count(anchor)==1
    text = text.replace(anchor,anchor+'''
    static bool files=false;
    if(!files){
        data_ov002_0210d9f0.Construct(0x9c0c);
        data_ov036_021141a4.Construct(1687);
        data_ov036_0211419c.Construct(1689);
        data_ov036_021141ac.Construct(1688);
        files=true;
    }
''')
    anchor = 'int daObjRcCarpet_c::Render()\n{'
    assert text.count(anchor)==1
    text = text.replace(anchor,anchor+'\n    pc_render_probe::Scope probe(this,false,&mModelAnim.mat4x3);')
    helper = r'''
static void pc_carpet_observe(daObjRcCarpet_c *a){
 const char *prefix=std::getenv("SM64DS_PC_UDLIFT_TRACE");if(!prefix||ntr::rt_frame()%5)return;
 static FILE *file=nullptr;
 if(!file){char path[2048];std::snprintf(path,sizeof(path),"%s.carpets.csv",prefix);file=std::fopen(path,"w");if(file)std::fputs("tick,uid,param,state,node,nodes,ridden,x,y,z,collider,area,alive\n",file);}
 if(file){std::fprintf(file,"%llu,%u,%u,%d,%d,%u,%u,%d,%d,%d,%d,%d,%u\n",static_cast<unsigned long long>(ntr::rt_frame()),unsigned(a->uniqueID),unsigned(a->param1),a->mState,a->mCurrPathNode,unsigned(a->mPath.NumNodes()),unsigned(a->mAfterClsnRan),a->mPosX,a->mPosY,a->mPosZ,int(a->mMeshCollider.IsEnabled()),int(a->mAreaId),unsigned(a->aliveState));std::fflush(file);}
}
'''
    anchor='// @symbol daObjRcCarpet_c_classInit'
    text=text.replace(anchor,helper+anchor)
    anchor='int daObjRcCarpet_c::Behavior()\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    pc_carpet_observe(this);')
    text += '\nstatic_assert(sizeof(daObjRcCarpet_c)==0x4c0,"native carpet allocation");\n'
    return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n#include "hal/actor_render_probe.h"\n'+text



def transform_path_lift(root, text):
    raw = (root / 'extracted/overlays/overlay_0100.bin').read_bytes()
    profile = struct.unpack_from('<IHHIiiii', raw, 0x02148558 - 0x02140d80)
    assert profile == (0x02147328, 31, 299, 2, 0, 0x300000, 0x1000000, 0x1000000)
    text, count = re.subn(
        r'(extern "C" daObjPathLift_c \*daObjPathLift_c_classInit\(\)\n)\{.*?\n\}',
        r'\1{ return new daObjPathLift_c(); }', text, flags=re.S)
    assert count == 1
    for kind, symbol in (
        ('PathLiftModelFile', 'data_ov100_02148a54'),
        ('PathLiftCollisionFilePtr', 'data_ov100_02148a5c'),
    ):
        text, count = re.subn(rf'struct {kind} : SharedFilePtr \{{.*?\n\}};',
                              f'struct {kind} : SharedFilePtr {{ u32 words[2]; }};',
                              text, flags=re.S)
        assert count == 1
        # Explicit lazy setup owns these two handle allocations.
        text, count = re.subn(rf'^{kind} {symbol}\([^\n]*\);$', f'{kind} {symbol};',
                              text, flags=re.M)
        assert count == 1
    assert text.count('unsigned int PlayLong(') == 1
    text = text.replace('unsigned int PlayLong(', 'int PlayLong(')
    old = 'extern SharedFilePtr data_ov002_0210d9f0;'
    assert text.count(old) == 1
    text = text.replace(old, 'extern "C" SharedFilePtr data_ov002_0210d9f0;')
    anchor = 'int daObjPathLift_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '''
    static const bool ready = [] {
        data_ov002_0210d9f0.Construct(0x9c0c);
        data_ov100_02148a54.Construct(0x9c0d);
        data_ov100_02148a5c.Construct(0x9c0e);
        return true;
    }();
    (void)ready;
''')
    text += '''
static_assert(sizeof(daObjPathLift_c)==0x4b4,"native path lift allocation");
static_assert(offsetof(daObjPathLift_c,mModel)+offsetof(Model,mat4x3)+offsetof(Matrix4x3,t)==0x114,
              "native path lift model translation");
static_assert(offsetof(daObjPathLift_c,unk_478)+offsetof(Matrix4x3,t)==0x49c,
              "native path lift shadow translation");
'''
    return '#include <cstddef>\n' + observe(
        text, 'daObjPathLift_c', 'state,node,nodes,ridden',
        ['int(mState)', 'int(mCurrPathNode)', 'int(mPath.NumNodes())', 'int(mAfterClsnRan)'])
