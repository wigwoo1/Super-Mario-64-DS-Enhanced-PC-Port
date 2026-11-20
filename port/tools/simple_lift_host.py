"""Native resources for the supported moving-platform variants."""
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


def transform_sky_dorifu(root, text):
    """Bind native handles and resolve each surface in its owning course."""
    rom = (root / 'extracted/overlays/overlay_0047.bin').read_bytes()
    base = 0x021111a0
    assert struct.unpack_from('<IHHIiiii', rom, 0x021124ec - base) == (
        0x0211164c, 153, 224, 0, 0x500000, 0x1000000, 0x2000000, 0)
    text = lazy_handles(text, ('DorifuModelFilePtr', 'DorifuClsnFileHandle'))
    declaration = 'extern "C" {\nextern daObjDorifuResources data_ov047_02112508[5];\n}\n'
    assert text.count(declaration) == 1
    text = text.replace(declaration, '')
    rows = []
    surfaces = []
    for i in range(5):
        model, collision, surface = struct.unpack_from('<3I', rom, 0x02112508 - base + i*12)
        assert struct.unpack_from('<4sHH', rom, surface - base) == (b'CLPS', 8, 2)
        model_name = f'data_ov047_{model:08x}'
        collision_name = f'data_ov047_{collision:08x}'
        assert f'{model_name}(0x{0x66f+i*2:x})' in text
        assert f'{collision_name}(0x{0x670+i*2:x})' in text
        rows.append(f'    {{&{model_name}, &{collision_name}, nullptr}}')
        surfaces.append(f'0x{surface:08x}')
    marker = '// @symbol _ZN17daObjKm3_Dorifu_c13InitResourcesEv'
    handles = re.findall(r'^(Dorifu\w+) (data_ov047_\w+)\((0x\w+)\);', text, re.M)
    assert len(handles) == 10 and text.count(marker) == 1
    declarations = [f'extern {handle} {name};' for handle, name, _ in handles]
    bindings = '\n'.join(declarations) + '\nextern "C" daObjDorifuResources pc_sky_dorifu_resources[5] = {\n'
    bindings += ',\n'.join(rows) + '\n};\n\n'
    text = text.replace(marker, bindings + marker)
    anchor = 's32 daObjKm3_Dorifu_c::InitResources()\n{'
    assert text.count(anchor) == 1
    setup = """
    constexpr unsigned int surfaceAddresses[] = {SURFACES};
    for (unsigned int row = 0; row < 5; ++row) {
        auto *surface = static_cast<CLPS_Block *>(pc_level_overlay_address(39, surfaceAddresses[row]));
        if (!surface) return 0;
        pc_sky_dorifu_resources[row].clps = surface;
    }
"""
    setup += "    static const bool filesReady = [] {\n"
    setup += ''.join(f'        {name}.Construct({file_id});\n' for _, name, file_id in handles)
    setup += "        return true;\n    }();\n    (void)filesReady;\n"
    text = text.replace(anchor, anchor + setup.replace('SURFACES', ', '.join(surfaces)))
    return '#include "hal/level_overlay.h"\n' + text.replace('data_ov047_02112508', 'pc_sky_dorifu_resources')


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
    assert struct.unpack_from('<IHHIiiii',raw,0x02134f7c-base)==(
        0x021328d8,154,225,2,0,0x350000,0x2000000,0)
    assert struct.unpack_from('<3I',raw,0x02135030-base)==(0x02135560,0x021354f8,0x02111b34)
    assert struct.unpack_from('<H',raw,0x02134506-base)[0]==80
    assert struct.unpack_from('<h',raw,0x02134516-base)[0]==16384
    assert struct.unpack_from('<IHHIiiii',raw,0x02134f98-base)==(
        0x021328a8,155,226,2,0,0x350000,0x2000000,0)
    assert struct.unpack_from('<3I',raw,0x0213503c-base)==(0x02135528,0x02135518,0x02111b74)
    assert struct.unpack_from('<H',raw,0x02134508-base)[0]==78
    assert struct.unpack_from('<h',raw,0x02134518-base)[0]==16384
    sky=(root/'extracted/overlays/overlay_0047.bin').read_bytes()
    assert struct.unpack_from('<4sHH',sky,0x02111b34-0x021111a0)==(b'CLPS',8,1)
    assert struct.unpack_from('<4sHH',sky,0x02111b74-0x021111a0)==(b'CLPS',8,1)
    assert struct.unpack_from('<IHHIiiii',raw,0x02135008-base)==(
        0x021327e8,55,208,2,0,0x350000,0x1000000,0)
    assert struct.unpack_from('<3I',raw,0x0213506c-base)==(0x02135508,0x02135530,0x02113554)
    assert struct.unpack_from('<H',raw,0x02134510-base)[0]==50
    assert struct.unpack_from('<h',raw,0x02134520-base)[0]==-16384
    fortress=(root/'extracted/overlays/overlay_0015.bin').read_bytes()
    assert struct.unpack_from('<4sHH',fortress,0x02113554-0x021111a0)==(b'CLPS',8,1)
    text=lazy_handles(text,('SimpleLiftModelFilePtr','SimpleLiftClsnFileHandle'))
    text,n=re.subn(r'\*\(SharedFilePtr \*\*\)\(data_ov091_02135024 \+ mVariant \* 0xc\)',
                   '(mVariant==1 ? &data_ov091_02135560 : '
                   'mVariant==2 ? &data_ov091_02135528 : '
                   'mVariant==4 ? &data_ov091_02135510 : '
                   'mVariant==6 ? &data_ov091_02135508 : &data_ov091_02135520)',text);assert n==2,n
    text,n=re.subn(r'\*\(SharedFilePtr \*\*\)\(data_ov091_02135028 \+ (?:mVariant \* 0xc|tableOffset)\)',
                   '(mVariant==1 ? &data_ov091_021354f8 : '
                   'mVariant==2 ? &data_ov091_02135518 : '
                   'mVariant==4 ? &data_ov091_02135540 : '
                   'mVariant==6 ? &data_ov091_02135530 : &data_ov091_02135538)',text);assert n==3,n
    text=text.replace('*(void **)(data_ov091_0213502c + tableOffset)','surface')
    text=text.replace('extern char data_ov091_0213502c[];','extern unsigned char data_ov043_02111be0[];')
    anchor='int daObjSimpleLift_c::InitResources()\n{'
    text=text.replace(anchor,anchor+'''
    assert(actorID==55 || actorID==138 || actorID==147 || actorID==154 || actorID==155);
    void *surface=data_ov043_02111be0;
    if (actorID==55) {
        surface=pc_level_overlay_address(7, 0x02113554);
        if (!surface) return 0;
        static const bool ready=[] {
            data_ov091_02135508.Construct(0x59f);
            data_ov091_02135530.Construct(0x5a0);
            return true;
        }();
        (void)ready;
    } else if (actorID==154) {
        surface=pc_level_overlay_address(39, 0x02111b34);
        if (!surface) return 0;
        static const bool ready=[] {
            data_ov091_02135560.Construct(0x679);
            data_ov091_021354f8.Construct(0x67a);
            return true;
        }();
        (void)ready;
    } else if (actorID==155) {
        surface=pc_level_overlay_address(39, 0x02111b74);
        if (!surface) return 0;
        static const bool ready=[] {
            data_ov091_02135528.Construct(0x67b);
            data_ov091_02135518.Construct(0x67c);
            return true;
        }();
        (void)ready;
    } else if (actorID==147) {
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


def transform_fortress_rotating_bar(root, text):
    """Bind the rotating bar's original handles and active course surface."""
    rom = (root / 'extracted/overlays/overlay_0015.bin').read_bytes()
    base = 0x021111a0
    assert struct.unpack_from('<IHHIiiii', rom, 0x021146f0 - base) == (
        0x02112ba0, 51, 204, 2, 0, 0x350000, 0x1000000, 0)
    offset = 0x02113654 - base
    magic, stride, count = struct.unpack_from('<4sHH', rom, offset)
    assert magic == b'CLPS' and stride == 8 and count > 0
    assert offset + 8 + stride * count <= len(rom)
    text = lazy_handles(text, ('BkRotebarModelFilePtr', 'BkRotebarCollisionFilePtr'))
    anchor = 's32 daObjBk_Rotebar_c::InitResources()\n{'
    assert text.count(anchor) == 1 and text.count('&data_ov015_02113654);') == 1
    text = text.replace(anchor, anchor + '''
    auto *surface = static_cast<CLPS_Block *>(pc_level_overlay_address(7, 0x02113654));
    if (!surface) return 0;
    static const bool ready = [] {
        data_ov015_02114a8c.Construct(1435);
        data_ov015_02114a84.Construct(1436);
        return true;
    }();
    (void)ready;
''')
    text = text.replace('&data_ov015_02113654);', 'surface);')
    text = text.replace('unsigned PlayLong(', 'int PlayLong(')
    return '#include "hal/level_overlay.h"\n' + observe(
        text, 'daObjBk_Rotebar_c', 'pause,heading',
        ['int(mPauseTimer)', 'int(mPrevAngleY)'], return_type='s32')


def transform_fortress_tower_step(root, text):
    """Load the tower step's native handles and active course surface table."""
    rom = (root / 'extracted/overlays/overlay_0015.bin').read_bytes()
    base = 0x021111a0
    assert struct.unpack_from('<IHHIiiii', rom, 0x0211462c - base) == (
        0x0211290c, 52, 205, 2, 0x30000, 0x180000, 0x1000000, 0x1000000)
    magic, stride, count = struct.unpack_from('<4sHH', rom, 0x02113594 - base)
    assert magic == b'CLPS' and stride == 8 and count > 0
    assert 0x02113594 - base + 8 + stride * count <= len(rom)
    text = lazy_handles(text, ('BkLiftModelFilePtr', 'BkLiftCollisionFilePtr'))
    anchor = 'int daObjBk_Lift_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '''
    auto *surface = static_cast<CLPS_Block *>(pc_level_overlay_address(7, 0x02113594));
    if (!surface) return 0;
    static const bool ready = [] {
        data_ov015_02114a64.Construct(1433);
        data_ov015_02114a5c.Construct(1434);
        return true;
    }();
    (void)ready;
''', 1)
    text = text.replace('&data_ov015_02113594);', 'surface);')
    text = text.replace('    mJustSteppedOn = 1;\n    mMove = 1;\n}',
                        '    mJustSteppedOn = 1;\n    mMove = 1;\n    return 0;\n}')
    return '#include "hal/level_overlay.h"\n' + observe(
        text, 'daObjBk_Lift_c', 'kind,timer,move,heading',
        ['int(param1 & 0xff)', 'int(mMoveTimer)', 'int(mMove)', 'int(mPrevAngleY)'])


def transform_path_lift(root, text):
    """Bind the original Fire Sea and Sky path-lift variants."""
    rom = (root / 'extracted/overlays/overlay_0091.bin').read_bytes()
    base = 0x02130f00
    assert struct.unpack_from('<IHHIiiii', rom, 0x02134e9c - base) == (
        0x02132394, 144, 215, 2, 0x100000, 0x300000, 0x1000000, 0
    )
    assert struct.unpack_from('<IHHIiiii', rom, 0x02134e80 - base) == (
        0x021323cc, 157, 302, 2, 0x100000, 0x300000, 0x1000000, 0
    )
    for address, target in ((0x02134e54, 0x02132000), (0x02134e44, 0x02131f9c),
                            (0x02134e4c, 0x02131ef0)):
        assert struct.unpack_from('<II', rom, address - base) == (target, 0)
    for address, targets in ((0x021344fc, (0x02135498, 0x021354a8)),
                             (0x021344f4, (0x021354a0, 0x02135490)),
                             (0x02134e5c, (0x02111af4, 0x02112610))):
        assert struct.unpack_from('<2I', rom, address - base) == targets
    for overlay, address in ((47, 0x02111af4), (45, 0x02112610)):
        level = (root / f'extracted/overlays/overlay_{overlay:04}.bin').read_bytes()
        clps = address - 0x021111a0
        magic, stride, count = struct.unpack_from('<4sHH', level, clps)
        assert magic == b'CLPS' and stride == 8 and count and clps + 8 + stride*count <= len(level)

    text = lazy_handles(text, ('Linelift2ModelFilePtr', 'Linelift2CollisionFilePtr'))
    text = text.replace('void (dBgActor_c::*StateFunc)()', 'void (daLinelift2_c::*StateFunc)()')
    for symbol, method in (('02134e54', '02132000'), ('02134e44', '02131f9c'), ('02134e4c', '02131ef0')):
        text = text.replace(f'extern StateFunc data_ov091_{symbol};\n', '')
        text = text.replace(f'data_ov091_{symbol},', f'&daLinelift2_c::func_ov091_{method},')
    text = re.sub(r'extern (?:SharedFilePtr|CLPS_Block) \*data_ov091_(?:021344fc|021344f4|02134e5c)\[\];[^\n]*\n', '', text)
    model = '(mVariant == 0 ? data_ov091_02135498 : data_ov091_021354a8)'
    collision = '(mVariant == 0 ? data_ov091_021354a0 : data_ov091_02135490)'
    text = text.replace('*data_ov091_021344fc[mVariant]', model)
    text = text.replace('*data_ov091_021344f4[mVariant]', collision)
    text = text.replace('data_ov091_02134e5c[mVariant]', 'surface')
    text = text.replace('data_ov091_021344fc[mVariant]->Release()', model + '.Release()')
    text = text.replace('data_ov091_021344f4[mVariant]->Release()', collision + '.Release()')
    anchor = '    mVariant = param1 >> 8;'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '''
    if (!((actorID == 157 && mVariant == 0) || (actorID == 144 && mVariant == 1))) return 0;
    const bool sky = mVariant == 0;
    auto *surface = static_cast<CLPS_Block *>(pc_level_overlay_address(
        sky ? 39 : 37, sky ? 0x02111af4 : 0x02112610));
    if (!surface) return 0;
    static bool files_ready = false;
    if (!files_ready) {
        data_ov091_02135498.Construct(0x683);
        data_ov091_021354a0.Construct(0x684);
        data_ov091_021354a8.Construct(0x669);
        data_ov091_02135490.Construct(0x66a);
        files_ready = true;
    }
''', 1)
    text = ('#include "hal/level_overlay.h"\n'
            'struct Linelift2ModelFilePtr;\nstruct Linelift2CollisionFilePtr;\n'
            'extern Linelift2ModelFilePtr data_ov091_02135498;\n'
            'extern Linelift2CollisionFilePtr data_ov091_021354a0;\n'
            'extern Linelift2ModelFilePtr data_ov091_021354a8;\n'
            'extern Linelift2CollisionFilePtr data_ov091_02135490;\n' + text)
    return observe(text, 'daLinelift2_c', 'variant,state,node,timer,speed,target_x,target_y,target_z',
                   ['int(mVariant)', 'mState', 'mNodeIndex', 'int(mStateTimer)', 'mHorzSpeed',
                    'mTargetPosX', 'mTargetPosY', 'mTargetPosZ'])


def transform_emm_yuka(text):
    """Bind the original path lift to host-owned handles and its ov064 surface."""
    text = lazy_handles(text, ('EmmYukaModelFilePtr', 'EmmYukaCollisionFilePtr'))
    declaration = 'extern EmmYukaFiles data_ov052_021125a0;'
    assert text.count(declaration) == 1
    text = text.replace(declaration, 'extern CLPS_Block data_ov064_0211ba0c;')
    anchor = 'int daObjEmmYuka_c::InitResources()\n{'
    assert text.count(anchor) == 1
    binding = '''extern EmmYukaModelFilePtr data_ov052_021126a8;
extern EmmYukaCollisionFilePtr data_ov052_021126b0;
extern "C" EmmYukaFiles data_ov052_021125a0 = {
    &data_ov052_021126a8, &data_ov052_021126b0, &data_ov064_0211ba0c
};

'''
    setup = '''
    static const bool filesReady = [] {
        data_ov052_021126a8.Construct(1509);
        data_ov052_021126b0.Construct(1510);
        return true;
    }();
    (void)filesReady;
'''
    # Cleanup and Behavior, which precede InitResources, also use the bundle.
    marker = '// @symbol _ZN14daObjEmmYuka_cD1Ev'
    assert text.count(marker) == 1
    text = text.replace(marker, binding + marker).replace(anchor, anchor + setup)
    return observe(text, 'daObjEmmYuka_c', 'node,path_direction,speed',
                   ['mNodeIndex', 'mPathDir', 'mHorzSpeed'])


def transform_lava_float_factory(text):
    factory, = re.findall(r'\b(daObjFl_Ukiyuka_c_classInit_FL_UKIYUKA(?:_L)?)\(void\)', text)
    return ('#include "daObjFl_Ukiyuka_c.h"\n'
            f'extern "C" void *{factory}() {{ return new daObjFl_Ukiyuka_c(); }}\n')


def transform_lava_float(root, text):
    level = (root / 'extracted/overlays/overlay_0022.bin').read_bytes()
    assert struct.unpack_from('<6I', level, 0x021140d4 - 0x021111a0) == (
        0x021145e0, 0x021145d0, 0x0211baec, 0x021145d8, 0x021145c8, 0x0211bacc
    )
    overlay = (root / 'extracted/overlays/overlay_0064.bin').read_bytes()
    for address, count in ((0x0211baec, 1), (0x0211bacc, 2)):
        assert struct.unpack_from('<4sHH', overlay, address - 0x02115ee0) == (b'CLPS', 8, count)
    text = lazy_handles(text, ('UkiyukaModelFilePtr', 'UkiyukaCollisionFilePtr'))
    for kind, address in (('Model', '021145e0'), ('Collision', '021145d0'),
                          ('Model', '021145d8'), ('Collision', '021145c8')):
        anchor = f'Ukiyuka{kind}FilePtr data_ov022_{address}('
        assert text.count(anchor) == 1
        text = text.replace(anchor, 'extern "C" ' + anchor, 1)
    anchor = 'int daObjFl_Ukiyuka_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, 'static void init_lava_float_handles();\n' + anchor +
                        '\n    init_lava_float_handles();', 1)
    text = text.replace('    func_ov002_020b6584(this,', '    return func_ov002_020b6584(this,', 1)
    return text + """
extern "C" CLPS_Block data_ov064_0211baec;
extern "C" CLPS_Block data_ov064_0211bacc;
extern "C" ResourceDescriptor data_ov022_021140d4[] = {
    {&data_ov022_021145e0, &data_ov022_021145d0, &data_ov064_0211baec},
    {&data_ov022_021145d8, &data_ov022_021145c8, &data_ov064_0211bacc}
};

static void init_lava_float_handles() {
    static const bool ready = [] {
        data_ov022_021145e0.Construct(0x614);
        data_ov022_021145d0.Construct(0x615);
        data_ov022_021145d8.Construct(0x610);
        data_ov022_021145c8.Construct(0x611);
        return true;
    }();
    (void)ready;
}
"""


def transform_lava_mesh_lift(root, text):
    """Bind the original two-path lift to native resources and collision."""
    rom = (root / 'extracted/overlays/overlay_0064.bin').read_bytes()
    base = 0x02115ee0
    assert struct.unpack_from('<IHHIiiii', rom, 0x0211bc44 - base) == (
        0x02117fe8, 69, 303, 2, 0x80000, 0x1d0000, 0x2000000, 0x2000000
    )
    assert struct.unpack_from('<4sHH', rom, 0x0211bb6c - base) == (b'CLPS', 8, 2)
    assert struct.unpack_from('<6I', rom, 0x0211bc0c - base) == (
        0x02117c24, 0, 0x02117bdc, 0, 0x02117b8c, 0
    )

    def replace(old, new):
        nonlocal text
        assert text.count(old) == 1, old
        text = text.replace(old, new, 1)

    assert text.count('    u32 id; void *ptr;') == 2
    text = text.replace('    u32 id; void *ptr;', '    u32 words[2];')
    text = lazy_handles(text, ('AmiliftModelFilePtr', 'AmiliftClsnFilePtr'))
    declaration = 'extern LiftState data_ov064_0211c750[];\n'
    replace(declaration, '')
    start = text.index('/* Lets Behavior call the state table')
    end = text.index('struct BMD_File;', start)
    text = text[:start] + text[end:]
    start = text.index('/* The three-entry state dispatch table:')
    end = text.index('// @symbol func_ov064_02117fb4', start)
    text = text[:start] + text[end:]
    replace('    (this->*data_ov064_0211c750[idx].pmf)();', '''    switch (idx) {
    case STATE_WAIT: func_ov064_02117c24(); break;
    case STATE_FORWARD: func_ov064_02117bdc(); break;
    case STATE_BACKWARD: func_ov064_02117b8c(); break;
    }''')

    replace('    data_ov064_0211c730.Release();\n    data_ov064_0211c728.Release();', '''    if (mHasLiftResources) {
        data_ov064_0211c730.Release();
        data_ov064_0211c728.Release();
        mHasLiftResources = false;
    }''')
    replace('s32 daObjFl_Amilift_c::InitResources() {', '''extern PathDef *PATH_DEFS;
extern int NUM_PATHS;
s32 daObjFl_Amilift_c::InitResources() {
    mHasLiftResources = false;
    const unsigned pathId = param1 & 0xf;
    if (!PATH_DEFS || !PathPtr::GetNodeTable() || int(pathId) >= NUM_PATHS)
        return 0;
    mPathPtr.FromID(pathId);
    mNodeCount = mPathPtr.NumNodes();
    if (mNodeCount < 2)
        return 0;
    static const bool handlesReady = [] {
        data_ov064_0211c730.Construct(1519);
        data_ov064_0211c728.Construct(1520);
        return true;
    }();
    (void)handlesReady;''')
    replace('    mModel.SetFile(bmd, 1, -1);', '''    if (!bmd || !mModel.SetFile(bmd, 1, -1)) {
        if (bmd) data_ov064_0211c730.Release();
        return 0;
    }''')
    replace('    kcl = _ZN7dBgW_Kc8LoadFileER13SharedFilePtr(data_ov064_0211c728);',
            '''    kcl = _ZN7dBgW_Kc8LoadFileER13SharedFilePtr(data_ov064_0211c728);
    if (!kcl) {
        data_ov064_0211c730.Release();
        return 0;
    }''')
    callbacks = '''    func_020393d4(&mMeshCollider, (void *)&_ZN4dBgW22UpdatePosWithTransformERS_P8dActor_cR5dBgPiR7Vector3P10Vector3_16S8_);
    func_020393c4(&mMeshCollider, (void *)&func_ov064_02117fd4);

    mPathPtr.FromID(param1 & 0xf);
    mNodeCount = mPathPtr.NumNodes();'''
    replace(callbacks, '''    mMeshCollider.beforeClsnCallback = [](dBgW *collider, dActor_c *actor,
            dBgPi *result, Vector3 *position, Vector3_16 *motion, Vector3_16 *angles) {
        dBgW::UpdatePosWithTransform(*collider, actor, *result, *position, motion, angles);
    };
    // The floor-result commit reports riders even before the lift moves.
    func_020393c4(&mMeshCollider, reinterpret_cast<void *>(&func_ov064_02117fd4));
    mHasLiftResources = true;''')
    replace('extern "C" daObjFl_Amilift_c *daObjFl_Amilift_c_classInit(void)\n{\n    return new daObjFl_Amilift_c;\n}',
            '''extern "C" void *daObjFl_Amilift_c_classInit()
{
    void *storage = _ZN7fBase_cnwEj(sizeof(daObjFl_Amilift_c));
    return storage ? ::new (storage) daObjFl_Amilift_c{} : nullptr;
}''')
    replace('    mPosY = saved;\n    mRiderOn = 0;', '''    mPosY = saved;
    const bool hadRider = mRiderOn != 0;
    mRiderOn = 0;''')
    replace('s32 daObjFl_Amilift_c::Render() {', 's32 daObjFl_Amilift_c::Render()\n{')
    text = observe(text, 'daObjFl_Amilift_c', 'state,node,auto_run,rider,sink',
                   ('mState', 'mNodeIndex', 'mAutoRun', 'hadRider', 'mSinkOffset'), return_type='s32')
    return '#include <new>\nextern "C" void *_ZN7fBase_cnwEj(unsigned int);\n' + text


def transform_lava_ring(root, text):
    level = (root / 'extracted/overlays/overlay_0022.bin').read_bytes()
    base = 0x021111a0
    assert struct.unpack_from('<IHHIiiii', level, 0x02113cf4 - base) == (
        0x02111578, 71, 279, 2, 0, 0x780000, 0x2000000, 0
    )
    assert struct.unpack_from('<3I', level, 0x02113cc8 - base) == (
        0x02114500, 0x02114508, 0x0211bbac
    )
    overlay = (root / 'extracted/overlays/overlay_0064.bin').read_bytes()
    assert struct.unpack_from('<4sHH', overlay, 0x0211bbac - 0x02115ee0) == (b'CLPS', 8, 2)
    text = lazy_handles(text, ('RingModelFilePtr', 'RingCollisionFilePtr'))
    anchor = 's32 daObjFl_Ring_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, """
extern "C" unsigned char data_ov064_0211bbac[];
extern "C" daObjFl_Ring_c_Files data_ov022_02113cc8 = {
    &data_ov022_02114500, &data_ov022_02114508, data_ov064_0211bbac
};

""" + anchor + """
    static const bool ready = [] {
        data_ov022_02114500.Construct(1550);
        data_ov022_02114508.Construct(1551);
        return true;
    }();
    (void)ready;
""", 1)
    text = text.replace('    mModel.SetFile((BMD_File *)f, 1, -1);',
                        '    if (!f || !mModel.SetFile((BMD_File *)f, 1, -1)) return 0;', 1)
    text = text.replace('    void *k = dBgW_Kc::LoadFile(*data_ov022_02113cc8.mClsnFile);',
                        '    void *k = dBgW_Kc::LoadFile(*data_ov022_02113cc8.mClsnFile);\n'
                        '    if (!k) return 0;', 1)
    anchor = '            s->mSpawner = this;'
    assert text.count(anchor) == 1
    text = text.replace(anchor, '            if (!s) break;\n            s->mSpawnerId = uniqueID;', 1)
    callbacks = '''    func_020393d4(&mMeshCollider, (void *)dBgW::UpdatePosAndAngs);
    func_020393c4(&mMeshCollider, (void *)func_ov022_02111564);'''
    assert text.count(callbacks) == 1
    return text.replace(callbacks, '''    mMeshCollider.beforeClsnCallback = [](dBgW *collider, dActor_c *actor,
            dBgPi *result, Vector3 *position, Vector3_16 *motion, Vector3_16 *angles) {
        dBgW::UpdatePosAndAngs(*collider, actor, *result, *position, motion, angles);
        if (collider->owner && actor)
            func_ov022_02111564(collider, static_cast<daObjFl_Ring_c *>(collider->owner), actor);
    };''', 1)


def transform_lava_gura(root, text):
    rom = (root / 'extracted/overlays/overlay_0064.bin').read_bytes()
    base = 0x02115ee0
    assert struct.unpack_from('<IHHIiiii', rom, 0x0211bd08 - base) == (
        0x021180fc, 72, 280, 2, 0x80000, 0x1a0000, 0x2000000, 0
    )
    assert struct.unpack_from('<3I', rom, 0x0211adb0 - base) == (
        0x0211c768, 0x0211c770, 0x0211bb8c
    )
    assert struct.unpack_from('<4sHH', rom, 0x0211bb8c - base) == (b'CLPS', 8, 2)
    text = lazy_handles(text, ('FlGuraModelFilePtr', 'FlGuraCollisionFilePtr'))
    for kind, address in (('FlGuraModelFilePtr', '0211c768'),
                          ('FlGuraCollisionFilePtr', '0211c770')):
        anchor = f'{kind} data_ov064_{address}('
        assert text.count(anchor) == 1
        text = text.replace(anchor, 'extern "C" ' + anchor, 1)
    anchor = 's32 daObjFl_Gura_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, 'extern "C" void pc_init_lava_gura_handles();\n' + anchor +
                        '\n    pc_init_lava_gura_handles();', 1)
    return text + """
extern "C" CLPS_Block data_ov064_0211bb8c;
extern "C" ResourceDescriptor data_ov064_0211adb0 = {
    &data_ov064_0211c768, &data_ov064_0211c770, &data_ov064_0211bb8c
};
extern "C" void pc_init_lava_gura_handles() {
    static bool ready = false;
    if (!ready) {
        data_ov064_0211c768.Construct(1523);
        data_ov064_0211c770.Construct(1524);
        ready = true;
    }
}
"""


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


def transform_wdw_switch_plank(root, text):
    rom = (root / 'extracted/overlays/overlay_0029.bin').read_bytes()
    base = 0x021111a0
    assert struct.unpack_from('<IHHIiiii', rom, 0x0211417c - base) == (
        0x02112964, 98, 274, 2, 0x60000, 0x100000, 0x1000000, 0x1000000)
    assert struct.unpack_from('<4sHH', rom, 0x0211304c - base) == (b'CLPS', 8, 2)
    for address, backing, file_id in (
        (0x02112e48, 0x0211432c, 0x6d1), (0x02112e58, 0x02114324, 0x6d2),
    ):
        assert struct.unpack_from('<II', rom, address - base) == (backing, file_id)

    text = lazy_handles(text, ('WcObj04ModelFilePtr', 'WcObj04CollisionFilePtr'))
    anchor = 'int daObjWc_Obj04_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '''
    auto *surface = static_cast<CLPS_Block *>(pc_level_overlay_address(21, 0x0211304c));
    if (!surface) return 0;
    static const bool ready = [] {
        data_ov029_0211432c.Construct(0x6d1);
        data_ov029_02114324.Construct(0x6d2);
        return true;
    }();
    (void)ready;
''', 1)
    text = text.replace('extern CLPS_Block data_ov029_0211304c;\n', '')
    text = text.replace('mAngleY, data_ov029_0211304c);', 'mAngleY, *surface);')
    raw_timer = '*(unsigned short *)(((char *)this) + 0x300 + 0xa0)'
    assert text.count(raw_timer) == 2
    text = text.replace(raw_timer, 'mStateTimer')
    text += '\nstatic_assert(sizeof(daObjWc_Obj04_c) == 0x3a8);\n'
    return '#include "hal/level_overlay.h"\n' + observe(
        text, 'daObjWc_Obj04_c', 'state,timer,visible,event_bit',
        ['int(mState)', 'int(mStateTimer)', 'int(mVisible)', 'int(mEventID)'],
        model='mModel2')


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
