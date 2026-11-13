"""Native resources for the reviewed Dark World sliding-platform variant."""
import re,struct
from battlefield_platform_host import lazy_handles,observe

def observe_dorifu(text):
    """Add opt-in, read-only state/collider sampling for Dark World stairs."""
    start=text.index('s32 daObjDorifu_c::Behavior()')
    end=text.find('// @symbol',start)
    if end<0:end=len(text)
    chunk=text[start:end]
    last=chunk.rfind('    return 1;')
    assert last>=0
    trace='''    if(const char *path=std::getenv("SM64DS_PC_DORIFU_TRACE")) {
        static FILE *file=nullptr;
        if(!file){file=std::fopen(path,"a");if(file)std::fputs("# daObjDorifu_c:tick,uid,actor,x,y,z,active_plank,timer,state,event_bit,collision_mask\\n",file);}
        if(file && ntr::rt_frame()%5==0){
            unsigned mask=0;for(int j=0;j<5;++j)if(mPlankClsn[j].IsEnabled())mask|=1u<<j;
            std::fprintf(file,"%llu,%u,%u,%d,%d,%d,%u,%u,%u,%u,%u\\n",
                static_cast<unsigned long long>(ntr::rt_frame()),unsigned(uniqueID),unsigned(actorID),
                mPosX,mPosY,mPosZ,unsigned(mActivePlank),unsigned(mStepTimer),unsigned(mState),unsigned(mEventBit),mask);
            std::fflush(file);
        }
    }
'''
    text=text[:start]+chunk[:last]+trace+chunk[last:]+text[end:]
    return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n'+text

def transform(root,text):
    cfg=(root/'config/arm9/overlays/ov091/delinks.txt').read_text()
    base=int(re.search(r'\.text\s+start:0x([\da-f]+)',cfg)[1],16)
    raw=(root/'extracted/overlays/overlay_0091.bin').read_bytes()
    assert struct.unpack_from('<IHHIiiii',raw,0x02134f60-base)==(0x02132908,138,179,2,0,0x400000,0x2000000,0)
    assert struct.unpack_from('<3I',raw,0x02135024-base)==(0x02135520,0x02135538,0x02111be0)
    assert struct.unpack_from('<H',raw,0x02134504-base)[0]==69
    assert struct.unpack_from('<H',raw,0x02134514-base)[0]==16384
    assert struct.unpack_from('<IHHIiiii',raw,0x02134fd0-base)==(
        0x02132848,147,218,2,0,0x350000,0x1000000,0)
    assert struct.unpack_from('<3I',raw,0x02135054-base)==(0x02135510,0x02135540,0x02112530)
    assert struct.unpack_from('<H',raw,0x0213450c-base)[0]==60
    assert struct.unpack_from('<h',raw,0x0213451c-base)[0]==0
    firesea=(root/'extracted/overlays/overlay_0045.bin').read_bytes()
    signature,stride,count=struct.unpack_from('<4sHH',firesea,0x02112530-0x021111a0)
    assert signature==b'CLPS' and stride==8 and count>0
    text=lazy_handles(text,('SimpleLiftModelFilePtr','SimpleLiftClsnFileHandle'))
    text,n=re.subn(r'\*\(SharedFilePtr \*\*\)\(data_ov091_02135024 \+ mVariant \* 0xc\)',
                   '(mVariant==4 ? &data_ov091_02135510 : &data_ov091_02135520)',text);assert n==2,n
    text,n=re.subn(r'\*\(SharedFilePtr \*\*\)\(data_ov091_02135028 \+ (?:mVariant \* 0xc|tableOffset)\)',
                   '(mVariant==4 ? &data_ov091_02135540 : &data_ov091_02135538)',text);assert n==3,n
    text=text.replace('*(void **)(data_ov091_0213502c + tableOffset)','surface')
    text=text.replace('extern char data_ov091_0213502c[];','extern unsigned char data_ov043_02111be0[];')
    anchor='int daObjSimpleLift_c::InitResources()\n{'
    text=text.replace(anchor,anchor+'''
    assert(actorID==138 || actorID==147);
    void *surface=data_ov043_02111be0;
    if (actorID==147) {
        surface=pc_level_overlay_address(37, 0x02112530);
        if (!surface) return 0;
        static const bool ready=[] {
            data_ov091_02135510.Construct(0x665);
            data_ov091_02135540.Construct(0x666);
            return true;
        }();
        (void)ready;
    } else {
        static const bool ready=[] {
            data_ov091_02135520.Construct(0x647);
            data_ov091_02135538.Construct(0x648);
            return true;
        }();
        (void)ready;
    }
''',1)
    text='#include "hal/level_overlay.h"\n'+text
    text+='\nstatic_assert(sizeof(SimpleLiftModelFilePtr)==8 && sizeof(SimpleLiftClsnFileHandle)==8,"native simple lift handles");\n'
    return observe(text,'daObjSimpleLift_c','variant,timer,pause,speed,heading',['int(mVariant)','int(mMoveTimer)','int(mPauseTimer)','int(mHorzSpeed)','int(mPrevAngleY)'])


def transform_firesea_path_lift(root, text):
    """Bind the Fire Sea variant of the original two-profile path lift."""
    rom = (root / 'extracted/overlays/overlay_0091.bin').read_bytes()
    base = 0x02130f00
    assert struct.unpack_from('<IHHIiiii', rom, 0x02134e9c - base) == (
        0x02132394, 144, 215, 2, 0x100000, 0x300000, 0x1000000, 0
    )
    for address, target in ((0x02134e54, 0x02132000), (0x02134e44, 0x02131f9c),
                            (0x02134e4c, 0x02131ef0)):
        assert struct.unpack_from('<II', rom, address - base) == (target, 0)
    for address, target in ((0x02134500, 0x021354a8), (0x021344f8, 0x02135490),
                            (0x02134e60, 0x02112610)):
        assert struct.unpack_from('<I', rom, address - base) == (target,)
    level = (root / 'extracted/overlays/overlay_0045.bin').read_bytes()
    clps = 0x02112610 - 0x021111a0
    magic, stride, count = struct.unpack_from('<4sHH', level, clps)
    assert magic == b'CLPS' and stride == 8 and count and clps + 8 + stride*count <= len(level)

    text = lazy_handles(text, ('Linelift2ModelFilePtr', 'Linelift2CollisionFilePtr'))
    text = text.replace('void (dBgActor_c::*StateFunc)()', 'void (daLinelift2_c::*StateFunc)()')
    for symbol, method in (('02134e54', '02132000'), ('02134e44', '02131f9c'), ('02134e4c', '02131ef0')):
        text = text.replace(f'extern StateFunc data_ov091_{symbol};\n', '')
        text = text.replace(f'data_ov091_{symbol},', f'&daLinelift2_c::func_ov091_{method},')
    text = re.sub(r'extern (?:SharedFilePtr|CLPS_Block) \*data_ov091_(?:021344fc|021344f4|02134e5c)\[\];[^\n]*\n', '', text)
    text = text.replace('*data_ov091_021344fc[mVariant]', 'data_ov091_021354a8')
    text = text.replace('*data_ov091_021344f4[mVariant]', 'data_ov091_02135490')
    text = text.replace('data_ov091_02134e5c[mVariant]', 'surface')
    text = text.replace('data_ov091_021344fc[mVariant]->Release()', 'data_ov091_021354a8.Release()')
    text = text.replace('data_ov091_021344f4[mVariant]->Release()', 'data_ov091_02135490.Release()')
    anchor = '    mVariant = param1 >> 8;'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '''
    if (mVariant != 1) return 0;
    auto *surface = static_cast<CLPS_Block *>(pc_level_overlay_address(37, 0x02112610));
    if (!surface) return 0;
    static bool files_ready = false;
    if (!files_ready) {
        data_ov091_021354a8.Construct(0x669);
        data_ov091_02135490.Construct(0x66a);
        files_ready = true;
    }
''', 1)
    text = ('#include "hal/level_overlay.h"\n'
            'struct Linelift2ModelFilePtr;\nstruct Linelift2CollisionFilePtr;\n'
            'extern Linelift2ModelFilePtr data_ov091_021354a8;\n'
            'extern Linelift2CollisionFilePtr data_ov091_02135490;\n' + text)
    return observe(text, 'daLinelift2_c', 'variant,state,node,timer,speed,target_x,target_y,target_z',
                   ['int(mVariant)', 'mState', 'mNodeIndex', 'int(mStateTimer)', 'mHorzSpeed',
                    'mTargetPosX', 'mTargetPosY', 'mTargetPosZ'])


def transform_firesea_gura(root, text):
    rom = (root / 'extracted/overlays/overlay_0045.bin').read_bytes()
    base = 0x021111a0
    assert struct.unpack_from('<IHHIiiii', rom, 0x02112ffc - base) == (
        0x02111d0c, 141, 212, 2, 0x120000, 0x200000, 0x2000000, 0
    )
    assert struct.unpack_from('<3I', rom, 0x02112fdc - base) == (
        0x02113220, 0x02113228, 0x021124f0
    )
    assert struct.unpack_from('<4sHH', rom, 0x021124f0 - base) == (b'CLPS', 8, 2)
    text = lazy_handles(text, ('Km2GuraModelFilePtr', 'Km2GuraClsnFilePtr'))
    assert text.count('&data_ov045_021124f0') == 1
    text = text.replace('extern CLPS_Block data_ov045_021124f0;\n', '')
    text = text.replace('&data_ov045_021124f0', 'nullptr')
    anchor = 's32 daObjKm2_Gura_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '''
    if (!data_ov045_02112fdc.clps) {
        auto *surface = static_cast<CLPS_Block *>(pc_level_overlay_address(37, 0x021124f0));
        if (!surface) return 0;
        data_ov045_02113220.Construct(1631);
        data_ov045_02113228.Construct(1632);
        data_ov045_02112fdc.clps = surface;
    }
''', 1)
    return '#include "hal/level_overlay.h"\n' + text


def transform_firesea_ukishima(root, text):
    rom = (root / 'extracted/overlays/overlay_0045.bin').read_bytes()
    base = 0x021111a0
    assert struct.unpack_from('<IHHIiiii', rom, 0x02112f2c - base) == (
        0x02111bf4, 145, 216, 2, 0x60000, 0x1000000, 0x2000000, 0
    )
    assert struct.unpack_from('<3I', rom, 0x02112f08 - base) == (
        0x02113200, 0x021131f8, 0x02112550
    )
    assert struct.unpack_from('<4sHH', rom, 0x02112550 - base) == (b'CLPS', 8, 2)
    text = lazy_handles(text, ('UkishimaModelFilePtr', 'UkishimaCollisionFilePtr'))
    text = text.replace('Fix12i bobAmplitude);', 'int bobAmplitude);')
    anchor = 'extern ResourceDescriptor data_ov045_02112f08;'
    assert text.count(anchor) == 1
    text = text.replace(anchor, '')
    anchor = 's32 daObjKm2_Ukishima_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, '''
extern "C" ResourceDescriptor data_ov045_02112f08 = {
    &data_ov045_02113200, &data_ov045_021131f8, nullptr
};

''' + anchor + '''
    if (!data_ov045_02112f08.clps) {
        auto *surface = static_cast<CLPS_Block *>(pc_level_overlay_address(37, 0x02112550));
        if (!surface) return 0;
        data_ov045_02113200.Construct(1643);
        data_ov045_021131f8.Construct(1644);
        data_ov045_02112f08.clps = surface;
    }
''', 1)
    return '#include "hal/level_overlay.h"\n' + text


def transform_ukiyuka(text):
    # Keep original arithmetic, use named fields with the compiler's host layout.
    text = text.replace('    char *c = (char *)this;\n', '')
    text = text.replace('IsClsnInRangeE5Fix12IiES1_(c,', 'IsClsnInRangeE5Fix12IiES1_(this,')
    text = text.replace('*(u16 *)((int)c + 0x328) += 0x100;', 'mBobPhase = s16(u16(mBobPhase) + 0x100);')
    text = text.replace('*(int *)((int)c + 0x60) -=', 'mPosY -=')
    return observe(text, 'daObjUkiyuka_c', 'rest_y,amplitude,bob_phase,rest_timer',
                   ['mRestY', 'int(mBobAmplitude)', 'int(u16(mBobPhase))', 'int(mRestTimer)'],
                   return_type='s32')


def transform_firesea_net_pole(root, text):
    rom = (root / 'extracted/overlays/overlay_0045.bin').read_bytes()
    base = 0x021111a0
    assert struct.unpack_from('<IHHIiiii', rom, 0x02112d98 - base) == (
        0x02111808, 142, 213, 2, 0x100000, 0x300000, 0x1000000, 0
    )
    assert struct.unpack_from('<4sHH', rom, 0x02112510 - base) == (b'CLPS', 8, 1)
    text = lazy_handles(text, ('AmiBouModelFilePtr', 'AmiBouCollisionFilePtr'))
    assert text.count('&data_ov045_02112510') == 1
    text = text.replace('extern CLPS_Block data_ov045_02112510;\n', '')
    text = text.replace('&data_ov045_02112510', 'surface')
    anchor = 's32 daObjKm2_Ami_Bou_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '''
    auto *surface = static_cast<CLPS_Block *>(pc_level_overlay_address(37, 0x02112510));
    if (!surface) return 0;
    static bool files_ready = false;
    if (!files_ready) {
        data_ov045_021131b0.Construct(1629);
        data_ov045_021131a8.Construct(1630);
        files_ready = true;
    }
''', 1)
    return '#include "hal/level_overlay.h"\n' + observe(
        text, 'daObjKm2_Ami_Bou_c', 'param,phase,radius,height,flags',
        ['int(param1)', 'int(mHeightAng)', 'int(mdCcAc_c.radius)', 'int(mdCcAc_c.height)',
         'int(mdCcAc_c.flags)'], return_type='s32')


def transform_firesea_rising_lift(root, text):
    rom = (root / 'extracted/overlays/overlay_0045.bin').read_bytes()
    base = 0x021111a0
    profile = struct.unpack_from('<IHHIiiii', rom, 0x02112cd0 - base)
    assert profile == (0x021114dc, 140, 211, 2, 0xfa000, 0xfa000, 0xfa0000, 0xfa0000)
    assert struct.unpack_from('<4sHH', rom, 0x021125d0 - base) == (b'CLPS', 8, 2)
    text = lazy_handles(text, ('AgaruModelFilePtr', 'AgaruCollisionFilePtr'))
    assert text.count('&data_ov045_021125d0') == 1
    text = text.replace('extern CLPS_Block data_ov045_021125d0;\n', '')
    text = text.replace('&data_ov045_021125d0', 'surface')
    anchor = 's32 daObjKm2_Agaru_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '''
    auto *surface = static_cast<CLPS_Block *>(pc_level_overlay_address(37, 0x021125d0));
    if (!surface) return 0;
    static const bool files_ready = [] {
        data_ov045_02113188.Construct(1627);
        data_ov045_02113180.Construct(1628);
        return true;
    }();
    (void)files_ready;
''', 1)
    values = ', '.join(str(value) for value in profile[1:])
    text += f'''
struct AgaruProfile {{
    daObjKm2_Agaru_c *(*factory)();
    u16 behaviorPriority, renderPriority;
    u32 flags;
    s32 clipOffsetY, clipRadius, clipDistance, farDistance;
}};
static_assert(sizeof(AgaruProfile) == 28);
extern "C" AgaruProfile g_profile_KM2_AGARU = {{daObjKm2_Agaru_c_classInit, {values}}};
'''
    return '#include "hal/level_overlay.h"\n' + observe(
        text, 'daObjKm2_Agaru_c', 'rest_y,timer,state',
        ['mRestY', 'int(mWaitTimer)', 'int(mState)'], return_type='s32')


def transform_firesea_stretch_lift(root, text):
    rom = (root / 'extracted/overlays/overlay_0045.bin').read_bytes()
    base = 0x021111a0
    assert struct.unpack_from('<IHHIiiii', rom, 0x02112e5c - base) == (
        0x02111ad4, 148, 219, 7, 0x3e8000, 0x7d0000, 0x3e80000, 0x50000)
    magic, stride, count = struct.unpack_from('<4sHH', rom, 0x021125b0 - base)
    assert magic == b'CLPS' and stride == 8 and count > 0
    text = lazy_handles(text, ('NobiruModelFileHandle', 'NobiruClsnFileHandle'))
    anchor = 'int daObjKm2_Nobiru_c::InitResources()\n{'
    assert text.count(anchor) == 1 and text.count('*(CLPS_Block *)data_ov045_021125b0') == 1
    text = text.replace(anchor, anchor + '''
    auto *surface = static_cast<CLPS_Block *>(pc_level_overlay_address(37, 0x021125b0));
    if (!surface) return 0;
    static const bool ready = [] {
        PoleLift_ModelFile.Construct(0x663);
        PoleLift_ClsnFile.Construct(0x664);
        return true;
    }();
    (void)ready;
''', 1).replace('*(CLPS_Block *)data_ov045_021125b0', '*surface')
    text = observe(text, 'daObjKm2_Nobiru_c', 'scale_y,growing',
                   ['func_0203aad0(&mCollider)', 'int(mGrowing)'])
    text = text.replace('unsigned(mMeshCollider.IsEnabled())', 'unsigned(mCollider.IsEnabled())')
    return '#include "hal/level_overlay.h"\n' + text

def transform_wdw_cage_lift(root, text):
    rom = (root / 'extracted/overlays/overlay_0029.bin').read_bytes()
    base = 0x021111a0
    assert struct.unpack_from('<IHHIiiii', rom, 0x02113e50 - base) == (
        0x02111e74, 99, 275, 2, 0, 0x250000, 0x2000000, 0x2000000)
    assert struct.unpack_from('<4sHH', rom, 0x0211306c - base) == (b'CLPS', 8, 2)
    text = lazy_handles(text, ('WcObj05ModelFilePtr', 'WcObj05CollisionFilePtr'))
    anchor = 's32 daObjWc_Obj05_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + """
    auto *surface = static_cast<CLPS_Block *>(pc_level_overlay_address(21, 0x0211306c));
    if (!surface) return 0;
    static const bool files_ready = [] {
        data_ov029_0211428c.Construct(0x6d3);
        data_ov029_02114284.Construct(0x6d4);
        return true;
    }();
    (void)files_ready;
""", 1).replace('mAngleY, data_ov029_0211306c);', 'mAngleY, *surface);')
    start = text.index('namespace tail_e40')
    end = text.index('/* -------------------------------------------------------------------------- */', start)
    text = text[:start] + """static void pc_wdw_cage_rider(void *, daObjWc_Obj05_c *lift, dActor_c *actor)
{
    if (actor && actor->actorID == 191)
        lift->unk_32a = 1;
}

""" + text[end:]
    start = text.index('/* BeforeClsn helper:')
    end = text.index('/* -------------------------------------------------------------------------- */', start)
    text = text[:start] + text[end:]
    text = text.replace('(void *)&func_ov029_02111e60', '(void *)&pc_wdw_cage_rider')
    text += '\nstatic_assert(sizeof(daObjWc_Obj05_c) == 0x330);\n'
    return '#include "hal/level_overlay.h"\n' + observe(
        text, 'daObjWc_Obj05_c', 'rest_y,timer,state',
        ['unk_320', 'int(unk_328)', 'int(unk_32c)'], return_type='s32')


def transform_wdw_arrow_lift(root, text):
    rom = (root / 'extracted/overlays/overlay_0029.bin').read_bytes()
    base = 0x021111a0
    assert struct.unpack_from('<IHHIiiii', rom, 0x02113ccc - base) == (
        0x021116f8, 96, 272, 2, 0, 0x100000, 0x1000000, 0x1000000)
    assert struct.unpack_from('<4sHH', rom, 0x0211302c - base) == (b'CLPS', 8, 1)
    text = lazy_handles(text, ('WcObj02ModelFilePtr', 'WcObj02CollisionFilePtr'))
    start = text.index('namespace tail_6c4')
    end = text.index('// @symbol _ZN15daObjWc_Obj02_c13InitResourcesEv', start)
    text = text[:start] + '''static void pc_arrow_lift_on_contact(dBgW *, daObjWc_Obj02_c *lift,
                                     dActor_c *actor)
{
    if (actor->actorID == 191) lift->mTriggered = 1;
}

''' + text[end:]
    anchor = 'int daObjWc_Obj02_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '''
    auto *surface = static_cast<CLPS_Block *>(pc_level_overlay_address(21, 0x0211302c));
    if (!surface) return 0;
    static const bool ready = [] {
        data_ov029_02114250.Construct(1742);
        data_ov029_02114248.Construct(1743);
        return true;
    }();
    (void)ready;
''', 1)
    text = text.replace('extern CLPS_Block data_ov029_0211302c;\n', '')
    text = text.replace('mAngleY, data_ov029_0211302c);', 'mAngleY, *surface);')
    text = text.replace('(short *)((unsigned long long)((int)this + 0x94))', '&mPrevAngleY')
    text = text.replace('(int)&func_ov029_021116e4', '(int)&pc_arrow_lift_on_contact')
    return '#include "hal/level_overlay.h"\n' + observe(
        text, 'daObjWc_Obj02_c', 'state,timer,travel,speed,heading',
        ['int(mState)', 'int(mStateTimer)', 'mTravelDist', 'mHorzSpeed', 'int(mPrevAngleY)'])


def transform_wdw_water(text, name):
    if name == 'd_a_obj_wc_obj03.cpp':
        text = lazy_handles(text, ('WcObj03ModelFilePtr',))
        anchor = 'int daObjWc_Obj03_c::InitResources()\n{'
        assert text.count(anchor) == 1
        return text.replace(anchor, anchor + '''
    static const bool ready = [] {
        data_ov029_02114270.Construct(1744);
        return true;
    }();
    (void)ready;
''', 1)

    text = lazy_handles(text, ('WcMizuModelFilePtr', 'WcMizuCollisionFilePtr'))
    anchor = 'int daObjWc_Mizu_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '''
    auto *levels = static_cast<const int *>(pc_level_overlay_address(21, 0x02112b2c));
    auto *animation = static_cast<BTA_File *>(pc_level_overlay_address(21, 0x02112fb8));
    auto *surface = static_cast<CLPS_Block *>(pc_level_overlay_address(21, 0x02112fec));
    if (!levels || !animation || !surface) return 0;
    static const bool ready = [] {
        daObjWc_Obj07_c_ModelFile.Construct(1738);
        daObjWc_Obj07_c_ClsnFile.Construct(1739);
        return true;
    }();
    (void)ready;
''', 1)
    for declaration, original, native in (
        ('extern int data_ov029_02112b2c[];', 'data_ov029_02112b2c[idx]', 'levels[idx]'),
        ('extern BTA_File data_ov029_02112fb8;', 'data_ov029_02112fb8', '*animation'),
        ('extern CLPS_Block data_ov029_02112fec;', '&data_ov029_02112fec', 'surface'),
    ):
        assert text.count(declaration) == 1
        text = text.replace(declaration + '\n', '').replace(original, native)
    text = text.replace('(s8 *)((u8 *)(((int)((u8 *)this) + 0x300)) + 0x40)', '(s8 *)&mTrueAreaID')
    text = text.replace('(s8 *)((u8 *)(((unsigned)((u8 *)this) + 0x300)) + 0x40)', '(s8 *)&mTrueAreaID')
    text = text.replace('((dBgW *)(((u8 *)this) + 0x124))->Enable((dActor_c *)(((u8 *)this)))',
                        'mMeshCollider.Enable(this)')
    text = text.replace('(int *)(((int)((u8 *)this) + 0x60))', '&mPosY')
    text = text.replace('(((u8 *)this) + 0x74)', '(&mCamSpacePosX)')
    text = text.replace('(s16 *)(((int)((u8 *)this) + 0x342))', '&mPhaseAngle')
    text = text.replace('*(u16 *)((u8 *)(((unsigned)((u8 *)this) + 0x300)) + 0x42)',
                        'static_cast<u16>(mPhaseAngle)')
    return '#include "hal/level_overlay.h"\n' + observe(
        text, 'daObjWc_Mizu_c', 'target,surface,true_area,spawn_y',
        ['mTargetPosY', 'mWaterHeight', 'int(mTrueAreaID)', 'int(mUseSpawnPosY)'])


def transform_wdw_float_board(text, name):
    if name == 'daObjFloatBoard_c.cpp':
        text = text.replace('            char *st = (char *)this + 0x300;\n', '')
        text = text.replace('*(unsigned short *)(st + 0x38)', 'static_cast<u16>(mBobPhase)')
        text = text.replace('s32 daObjFloatBoard_c::Render()', 'int daObjFloatBoard_c::Render()')
        return observe(text, 'daObjFloatBoard_c', 'water,floor,sink,bob,rider',
                       ['mWaterY', 'mFallbackWaterY', 'mSinkOffset', 'mBobOffset',
                        'mRider ? int(mRider->uniqueID) : 0'])

    if name == 'd_a_obj_wc_obj01.cpp':
        leaf, model, collision, file_id, descriptor, clps = (
            '01', '02114220', '02114228', 1740, '02113be8', '0211300c')
    else:
        leaf, model, collision, file_id, descriptor, clps = (
            '06', '021142ac', '021142b4', 1749, '02113f00', '0211308c')
    text = lazy_handles(text, (f'WcObj{leaf}ModelFilePtr', f'WcObj{leaf}CollisionFilePtr'))
    anchor = f'int daObjWcObj{leaf}_c::InitResources()\n{{'
    declaration = f'extern "C" daObjFloatBoard_c_Resources data_ov029_{descriptor};\n'
    assert text.count(anchor) == 1 and text.count(declaration) == 1
    text = text.replace(declaration, '').replace(anchor, anchor + f'''
    auto *surface = static_cast<CLPS_Block *>(pc_level_overlay_address(21, 0x{clps}));
    if (!surface) return 0;
    static daObjFloatBoard_c_Resources resources = [surface] {{
        data_ov029_{model}.Construct({file_id});
        data_ov029_{collision}.Construct({file_id + 1});
        return daObjFloatBoard_c_Resources{{&data_ov029_{model}, &data_ov029_{collision}, surface}};
    }}();
''', 1)
    text = text.replace(f'&data_ov029_{descriptor}', '&resources')
    text = text.replace('*(int *)((char *)this + 0x320)', 'mRestPosX')
    text = text.replace('*(int *)((char *)this + 0x328)', 'mRestPosZ')
    # The lazy globals are defined after InitResources in the recovered TU.
    declarations = (f'extern WcObj{leaf}ModelFilePtr data_ov029_{model};\n'
                    f'extern WcObj{leaf}CollisionFilePtr data_ov029_{collision};\n')
    return '#include "hal/level_overlay.h"\n' + text.replace(anchor, declarations + anchor, 1)
