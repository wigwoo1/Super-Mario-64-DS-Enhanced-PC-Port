"""Native silver-star/cap ABI adaptations; preserve cartridge sources."""
import re


CAP_METHODS = (
    'InitFall', 'InitTimedWait', 'Wait', 'InitWait', 'Dormant', 'TouchWait',
    'Taken', 'Anim', 'InitTimedWait2', 'TimedWait2', 'InitSlide', 'Slide',
    'InitTaken', 'InitDormant', 'TimedWait', 'InitAnim', 'InitTouchWait', 'Fall',
)

# St_NoControl_Init's 20-entry initializer, copied by the original ov002
# constructor. Repeated descriptors intentionally share the same worker.
NO_CONTROL_DESCRIPTORS = (
    0x02109dfc,0x0210a264,0x0210a244,0x0210a23c,0x0210a274,
    0x0210a22c,0x0210a29c,0x0210a1fc,0x0210a1ec,0x02109e2c,
    0x0210a1cc,0x0210a1b4,0x02109f54,0x0210a30c,0x0210a174,
    0x02109f44,0x0210a154,0x0210a144,0x0210a134,0x0210a12c)
NO_CONTROL_TARGETS = (
    0x020c9de4,0x020c9de4,0x020c9de4,0x020c9de4,0x020c9d68,
    0x020c9d1c,0x020c9cc0,0x020c9c4c,0x020c9c00,0x020c9b7c,
    0x020c9b5c,0x020c9ac0,0x020c9b10,0x020c9a04,0x020c9998,
    0x020c990c,0x020c98a4,0x020c9840,0x020c97f8,0x020c97e0)
NO_CONTROL_FREE = {0x020c9de4,0x020c9b7c,0x020c9998,0x020c990c}


def transform(text):
    if any(marker in text for marker in ('native cap callback stride',
            'native silver-star layout','native no-control initialization')):
        return text
    if 'int Player::St_NoControl_Init()' in text:
        start=text.index('  unsigned idx = mNoCtrlKind;',text.index('int Player::St_NoControl_Init()'))
        end=text.index('  return 1;',start)
        text=text[:start]+'''  pc_player_no_control_init(this,
      reinterpret_cast<const int *>(&data_ov002_02110884[mNoCtrlKind]));
'''+text[end:]
        text='extern "C" void pc_player_no_control_init(struct Player *, const int *);\n'+text
        text+='''
// native no-control initialization: dispatch original direct descriptors with
// the native method ABI. These rows are separate from Player::State.
extern "C" void pc_player_no_control_init(Player *self, const int *row) {
    if (self->mNoCtrlKind >= 20 || row[1] != 0) {
        std::fprintf(stderr,"Unsupported Player no-control descriptor\\n");
        std::abort();
    }
    switch (static_cast<unsigned>(row[0])) {
'''
        for target in sorted(set(NO_CONTROL_TARGETS)):
            function=f'func_ov002_{target:08x}'
            call=f'{function}((char *)self)' if target in NO_CONTROL_FREE else f'self->{function}()'
            text+=f'    case 0x{target:08x}: {call}; return;\n'
        text+='''    default:
        std::fprintf(stderr,"Unmapped Player no-control callback %08x\\n",row[0]);
        std::abort();
    }
}
'''
        return '#include <cstdio>\n#include <cstdlib>\n'+text
    elif 'int daObjMarioCap_c::EnterState(void *rec)' in text:
        text = text.replace('typedef int (daObjMarioCap_c::*McapStatePmf)();',
            'struct McapStatePmf { int (*fn)(daObjMarioCap_c *); int adjustment; };\n'
            'static_assert(sizeof(McapStatePmf)==8,"native cap callback stride");')
        start = text.index('    CapEnterSelf *c = (CapEnterSelf *)this;',
                           text.index('int daObjMarioCap_c::EnterState(void *rec)'))
        end = text.index('\n}', start)
        text = text[:start] + '''    mStateEntry = (s32)rec;
    auto *state = static_cast<McapStateRec *>(rec);
    return state->enter.fn ? state->enter.fn(this) : 0;''' + text[end:]
        text = text.replace('CapStateRec *q = *(CapStateRec **)&mStateEntry;',
                            'McapStateRec *q = (McapStateRec *)mStateEntry;')
        text = text.replace('if (q->fn != 0)', 'if (q && q->exec.fn != 0)')
        text = text.replace('(((CapStateSelf *)this)->*(q->fn))();',
                            'q->exec.fn(this);')
        # Static caps have no BCA. Advancing their empty frame controller
        # reaches the recovered modulo-zero path on x86. Only an installed
        # animation has a playback cursor; leave static models at frame zero.
        text = text.replace('mModelAnim.Advance();',
                            'if (mModelAnim.file) mModelAnim.Advance();')
        # Explicit original callback manifest: each ROM descriptor has zero
        # adjustment, and all targets are non-virtual methods of this class.
        defs = []
        for address, method in zip(range(0x0210951c, 0x021095ac, 8), CAP_METHODS):
            name = f'data_ov002_{address:08x}'
            defs += [f'static int pc_cap_{method}(daObjMarioCap_c *p) {{ return p->{method}(); }}',
                     f'extern "C" McapStatePmf {name} = {{pc_cap_{method}, 0}};']
        text += '\n' + '\n'.join(defs) + '\n'
        text += 'static_assert(sizeof(McapStateRec)==16,"native cap state stride");\n'
        text += 'static_assert(sizeof(daObjMarioCap_c)==0x410 && offsetof(daObjMarioCap_c,mStateEntry)==0x3bc,"native cap layout");\n'
    elif 'typedef void (daStar_c::*StateHandler)();' in text:
        # The complete table is defined in this TU using native member pointers.
        # Do not copy the DS PMF bytes over it during overlay initialization.
        text += '\nstatic_assert(sizeof(daStar_c)==0x4c4 && offsetof(daStar_c,mMarkerID)==0x434,"native silver-star layout");\n'
        text += 'static_assert(sizeof(daStarBase_c)==0x1dc && offsetof(daStarBase_c,mLinkedStarID)==0x1cc,"native star-marker layout");\n'
        # The recovered no-argument veneer relied on ARM retaining this in r0.
        text = text.replace('_ZN12daStarBase_c7CollectEv();',
                            'static_cast<daStarBase_c *>(dActor_c::FindWithID(mMarkerID))->Collect();')
    else:
        return text
    return '#include <stddef.h>\n' + text
