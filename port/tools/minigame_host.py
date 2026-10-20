"""Host-only minigame callback records, recovered from the supplied ROM."""
import re
import struct

STATE_START, STATE_END = 0x020bc8bc, 0x020bca44


def descriptors(root):
    result = {}
    for ov, base, addresses in (
        (4, 0x020ad660, range(STATE_START, STATE_END, 8)),
        (6, 0x020bfec0, list(range(0x0213cd8c, 0x0213cdec, 8)) +
         list(range(0x0213cdf4, 0x0213ce0c, 8)) + list(range(0x0213ce14, 0x0213ce54, 8))),
    ):
        symbols = (root / f'config/arm9/overlays/ov{ov:03}/symbols.txt').read_text()
        methods = {int(a, 16): name for name, a in re.findall(
            r'^(\S+) kind:function\([^\n]*?addr:0x([\da-fA-F]+)', symbols, re.M)}
        raw = (root / f'extracted/overlays/overlay_{ov:04}.bin').read_bytes()
        for address in addresses:
            target, adjustment = struct.unpack_from('<II', raw, address - base)
            method = methods.get(target, '')
            cls = 'dMgState_c' if ov == 4 else 'dScMgLuigi_c'
            prefix = f'_ZN{len(cls)}{cls}'
            match = re.fullmatch(re.escape(prefix) + r'\d+(\w+)E([vi])', method)
            if adjustment or not match:
                raise ValueError(f'Unsupported minigame callback {address:x}: {method}, {adjustment}')
            result[f'data_ov{ov:03}_{address:08x}'] = (cls, match[1], match[2] == 'i')
    return result


def tables(root):
    source = (root / 'src/unnamed/ov006/__sinit_ov006_02130f64.c').read_text()
    result = {}
    for name, slot, constant in re.findall(r'(data_ov006_\w+)\.p(\d+) = (data_ov006_\w+);', source):
        result.setdefault(name, {})[int(slot)] = constant
    if sorted(map(len, result.values())) != [3, 3, 3, 4, 10]:
        raise ValueError('Wanted callback table roster changed')
    return result


def owned_names(root):
    return set(descriptors(root)) | set(tables(root))


def pause_extents(root):
    """Recover POD width/height initialization without ARM destructor-list nodes."""
    source=(root/'src/unnamed/ov004/__sinit_ov004_020b9b24.c').read_text()
    body=source.split('void __sinit_ov004_020b9b24(void)',1)[1]
    result={}
    for block in re.split(r'func_020731dc\([^;]+;',body)[:-1]:
        address=re.search(r'data_ov004_(020bfe[0-9a-f]{2})',block)
        width=re.search(r'(?:p\[0\]|\)\)\[0\])\s*=\s*(0x[0-9a-f]+)',block)
        height=re.search(r'(?:p\[1\]|\)\)\[1\])\s*=\s*(0x[0-9a-f]+)',block)
        if not width: width=re.search(r'int sz = (0x[0-9a-f]+)',block)
        if not height: height=re.search(r'int h = (0x[0-9a-f]+)',block)
        if not (address and width and height): raise ValueError('Pause extents initializer changed')
        result[int(address[1],16)]=(int(width[1],16),int(height[1],16))
    if sorted(result)!=list(range(0x020bfe74,0x020bfeb8,4)):
        raise ValueError('Pause extent roster changed')
    return result


def write_sources(root, directory):
    entries = descriptors(root)
    cpp = ['#include <cstddef>', '#include "dScMgLuigi_c.h"',
           '#include "ntr/minigame_callback.h"',
           'static_assert(sizeof(dMgState_c) == 0x28, "state controller layout");',
           'static_assert(offsetof(dMgState_c, mState) == 0x18, "state id offset");',
           'static_assert(sizeof(dScMgBase_c) == 0x4660, "minigame base layout");',
           'static_assert(offsetof(dScMgBase_c, mTouchOptions) == 0xf4, "touch options offset");',
           'static_assert(offsetof(dScMgBase_c, mSceneKind) == 0x465e, "scene kind offset");',
           'static_assert(sizeof(dScMgLuigi_c) == 0x545c, "Wanted layout");',
           'static_assert(offsetof(dScMgLuigi_c, mState) == 0x4f78, "Wanted state offset");',
           'extern "C" {',
           'int pc_minigame_canvas_mode(void *scene) { return static_cast<dScMgBase_c *>(scene)->OnHitByCannonBlastedChar(); }']
    extents=pause_extents(root)
    cpp += [f'extern short data_ov004_{address:08x}[];' for address in extents]
    cpp.append('void pc_minigame_init_pause_extents() {')
    for address,(width,height) in extents.items():
        cpp.append(f'    data_ov004_{address:08x}[0]={width}; data_ov004_{address:08x}[1]={height};')
    cpp.append('}')
    for cls, method, slot in sorted(set(entries.values())):
        arg, call = (', int slot', 'slot') if slot else ('', '')
        cpp.append(f'void pc_mg_{cls}_{method}(void *self{arg}) {{ static_cast<{cls} *>(self)->{method}({call}); }}')
    for name, (cls, method, slot) in entries.items():
        typ = 'int' if slot else ''
        cpp.append(f'PcMinigameCallback<{typ}> {name}{{pc_mg_{cls}_{method}, 0}};')
    for name, items in tables(root).items():
        values = [entries[items[i]] for i in range(len(items))]
        slot = values[0][2]
        if any(v[2] != slot for v in values): raise ValueError('Mixed callback signatures')
        typ = 'int' if slot else ''
        cpp.append(f'PcMinigameCallback<{typ}> {name}[] = {{')
        cpp += [f'    {{pc_mg_{cls}_{method}, 0}},' for cls, method, _ in values]
        cpp.append('};')
    cpp.append('}')
    directory.mkdir(parents=True, exist_ok=True)
    output = directory / 'minigame_native_callbacks.cpp'
    text = '\n'.join(cpp) + '\n'
    if not output.exists() or output.read_text() != text: output.write_text(text)
    return [output.as_posix()]


def transform(text):
    marker = '// Native minigame ABI adapted.\n'
    if marker in text: return text
    original = text
    # Adapt only the actual C dispatcher definitions, not unrelated callers.
    dispatchers = {
        'void func_02019404(void)': ('  ((void(*)(void*))vt[1])(o);',
            '  int result;\n  if(!pc_minigame_graph_dispatch(o,1,&result)) ((void(*)(void*))vt[1])(o);'),
        'void func_02019390(void)': ('if (o->vt[0](o) == 0)',
            'int result;\n        if ((pc_minigame_graph_dispatch(o,0,&result) ? result : o->vt[0](o)) == 0)'),
        'void func_02019144(void)': ('graph_result = p->vt->func8(p);',
            'if(!pc_minigame_graph_dispatch(p,2,&graph_result)) graph_result = p->vt->func8(p);'),
        'void func_02019100(void)': ('if (o) o->vt[3](o);',
            'if (o) { int result; if(!pc_minigame_graph_dispatch(o,3,&result)) o->vt[3](o); }'),
    }
    for signature,(old,new) in dispatchers.items():
        if re.search(re.escape(signature)+r'\s*\{',text):
            if text.count(old)!=1: raise ValueError('Graph dispatcher changed: '+signature)
            text='extern int pc_minigame_graph_dispatch(void *, int, int *);\n'+text.replace(old,new)
    if 'void *func_020182bc(const char *path)\n{' in text:
        text = ('extern void *pc_fs_load_path(const char *);\n' + re.sub(
            r'void \*func_020182bc\(const char \*path\)\s*\{[^}]+\}',
            'void *func_020182bc(const char *path) { return pc_fs_load_path(path); }', text, count=1))
    if 'void dMgState_c::SetState(s32 state)' in text:
        text = text.replace('extern Base* data_ov004_020beb68;', '')
        text = re.sub(r'\bdata_ov004_020beb68\b', 'static_cast<Base *>(data_ov004_020beb68)', text)
        text = '#include "dScMgBase_c.h"\n' + text
        text = re.sub(r'struct Obj \{.*?\n\};', 'using Obj = dFader_c;', text, count=1, flags=re.S)
        # Declarations use several POD spellings for the same two-word data.
        text = re.sub(r'(?m)^extern ((?:W2|Pair|dMgState_c::Callback) data_ov004_\w+;)', r'extern "C" \1', text)
        for field in ('mEnter', 'mBehavior', 'mRender'):
            text = text.replace(f'(this->*{field})();', f'{field}(this);')
        text = text.replace(': mState(-1)', ': mEnter(), mBehavior(), mRender(), mState(-1)')
        text = text.replace('mRender = 0;', 'mRender = {};')
        # A native dFader owns these virtuals. DS slot numbers cannot be used.
        text = text.replace('extern Obj* data_0209f5bc;', 'extern "C" dFader_c *data_0209f5bc;')
        for old, new in [('m_18()', 'IsAtEnd()'), ('m_1c()', 'IsBetweenStartAndEnd()')]:
            text = text.replace('->' + old, '->' + new)
        text = re.sub(r'(\w+)->m_(c|10)\(([^,]+), 0\)',
                      lambda m: m[1] + '->' + ('SetBackwardTime' if m[2] == 'c' else 'SetForwardTime') + '(' + m[3] + ')', text)
        text = re.sub(r'(\w+)->m_4c\(\(void \*\)mState\)',
                      lambda m: 'reinterpret_cast<dScMgBase_c *>(' + m[1] + ')->OnTurnIntoEgg(mState)', text)
        text = re.sub(r'(\w+)->m_50\(\)', lambda m: 'reinterpret_cast<dScMgBase_c *>(' + m[1] + ')->Virtual50()', text)
    if 'typedef void (dScMgLuigi_c::*LuigiSlotHandler)(int);' in text:
        text = '#include "ntr/minigame_callback.h"\n' + text
        text = text.replace('typedef void (dScMgLuigi_c::*LuigiSlotHandler)(int);', 'using LuigiSlotHandler = PcMinigameCallback<int>;')
        text = text.replace('typedef void (dScMgLuigi_c::*LuigiStateHandler)();', 'using LuigiStateHandler = PcMinigameCallback<>;')
        declarations = sorted(set(re.findall(r'extern (Luigi(?:Slot|State)Handler data_ov006_\w+\[\];)', text)))
        text = re.sub(r'extern Luigi(?:Slot|State)Handler data_ov006_\w+\[\];', '', text)
        text = text.replace('using LuigiStateHandler = PcMinigameCallback<>;',
            'using LuigiStateHandler = PcMinigameCallback<>;\n' + '\n'.join('extern "C" ' + d for d in declarations))
        text, count = re.subn(r'\(this->\*\(?(data_ov006_\w+\[[^;]+?\])\)?\)\(([^;]*)\);',
                             lambda m: m[1] + '(this' + (', ' + m[2] if m[2] else '') + ');', text)
        if count != 6: raise ValueError(f'Wanted callback call roster changed: {count}')
    if 'dMgPsOpt_c::dMgPsOpt_c()\n{' in text:
        text='extern "C" void pc_minigame_init_pause_extents();\n'+text.replace(
            'dMgPsOpt_c::dMgPsOpt_c()\n{',
            'dMgPsOpt_c::dMgPsOpt_c()\n{\n    pc_minigame_init_pause_extents();')
    if 'int dScMgBase_c::BeforeBehavior()\n{' in text:
        hooks = {
            '    if (data_0209f5bc->IsActive()) {':
                '    if (mMenuOpen && pc_wanted_settings_update(this)) return 0;\n\n    if (data_0209f5bc->IsActive()) {',
            '    if (func_ov004_020b8f18((void *)(self + 0xf4)) != 0)':
                '    if (pc_wanted_settings_render()) return;\n    if (func_ov004_020b8f18((void *)(self + 0xf4)) != 0)',
            '        func_ov004_020b9220(c + 0xf4);':
                '        if (!pc_wanted_settings_open(c)) func_ov004_020b9220(c + 0xf4);',
            'dScMgBase_c::~dScMgBase_c()\n{':
                'dScMgBase_c::~dScMgBase_c()\n{\n    pc_wanted_settings_reset();',
        }
        for old,new in hooks.items():
            if text.count(old)!=1: raise ValueError('Wanted settings hook changed: '+old)
            text=text.replace(old,new)
        text='#include "hal/touch_menu_navigation.h"\n'+text
        anchor='    if (mMenuOpen && pc_wanted_settings_update(this)) return 0;'
        text=text.replace(anchor,'    if (mMenuOpen && pc_wanted_settings_update(this)) return 0;\n    pc_touch_menu::Frame pc_controller_frame(this);')
        anchor='flags = data_020a0e5a[mode * 2];'
        if text.count(anchor)!=1: raise ValueError('Shared menu button gate changed')
        text=text.replace(anchor,'flags = data_020a0e5a[mode * 2] & ~pc_controller_frame.consumed_keys();')
        text=text.replace('    pc_wanted_settings_reset();','    pc_wanted_settings_reset();\n    pc_touch_menu::reset();')
        text='#include "hal/wanted_pause_settings.h"\n'+text
        # Verified ARM LDR ip/BX ip veneer to LoadFile: preserve its ID and
        # returned allocation explicitly under the x86 stack calling convention.
        text = text.replace('extern void LoadFile(void);\nvoid func_ov004_020adc68(void) { LoadFile(); }',
            'extern void *LoadFile(int);\nvoid *func_ov004_020adc68(int id) { return LoadFile(id); }')
        text = text.replace('extern SceneVCall6 *data_0209f5bc;', 'extern dFader_c *data_0209f5bc;')
        text = text.replace('data_0209f5bc->IsActive()', 'data_0209f5bc->IsAtStart()')
        for offset, method in [('64', 'OnPushed'), ('58', 'OnAttacked1'), ('5c', 'OnAttacked2')]:
            pattern = r'void \(\*fn\)\(dScMgBase_c \*\) = [^;]+\+ 0x' + offset + r'[^;]*;\s*fn\(p\);'
            text, count = re.subn(pattern, 'p->' + method + '();', text)
            if count != 1: raise ValueError('Minigame graph callback changed: ' + offset)
        old = 'return (*(int (**)(dScMgBase_c *))((char *)*(void **)p + 0x60))(p);'
        if text.count(old) != 1: raise ValueError('Minigame graph callback2 changed')
        text = text.replace(old, 'return p->OnKicked();')
        text = text.replace('extern int func_ov004_020b8f78(char* p);',
                            'extern unsigned char func_ov004_020b8f78(char* p);')
        text = text.replace('((Obj*)c)->v28();', 'self->OnHitFromUnderneath();')
        text = text.replace('self->method78();', 'OnAimedAtWithEggReturnVec();')
        text = text.replace('self->method74();', 'OnAimedAtWithEgg();')
        text = text.replace('((Obj*)c)->v28();', 'static_cast<dScMgBase_c *>(static_cast<void *>(c))->OnHitFromUnderneath();')
        text = text.replace('c->v18(arg);', 'reinterpret_cast<dScMgBase_c *>(c)->OnYoshiTryEat((int)arg);')
        call = 'reinterpret_cast<dScMgBase_c *>(g)->OnHitByCannonBlastedChar()'
        text = text.replace('int (*vf)(char*) = *(int(**)(char*))(*(char**)g + 0x68);', 'const int pcMode = ' + call + ';')
        text = text.replace('vf(g)', 'pcMode')
        text = text.replace('void** vt = *(void***)g;\n    GetFn f = (GetFn)vt[0x1a];', 'const int pcMode = ' + call + ';')
        text = text.replace('f(g)', 'pcMode')
        text = text.replace('(*(int(**)(char*))(*(char**)g + 0x68))(g)', call)
        text = text.replace('int (**vt)(void*) = *(int(***)(void*))data_ov004_020beb68;', '')
        text = text.replace('vt[0x68/4]((void*)data_ov004_020beb68)',
            'reinterpret_cast<dScMgBase_c *>(data_ov004_020beb68)->OnHitByCannonBlastedChar()')
        for old, obj in [('data_ov004_020beb68->f26()', 'data_ov004_020beb68'),
                         ('g->m()', 'g'), ('sl->f68()', 'sl')]:
            text = text.replace(old, 'reinterpret_cast<dScMgBase_c *>(' + obj + ')->OnHitByCannonBlastedChar()')
    if 's32 dScMiniGm_c::Behavior()\n{' in text:
        text='#include "hal/touch_menu_navigation.h"\n'+text
        text=text.replace('s32 dScMiniGm_c::Behavior()\n{','s32 dScMiniGm_c::Behavior()\n{\n    pc_touch_menu::Frame pc_controller_frame(this);')
        text=text.replace('s32 dScMiniGm_c::CleanupResources()\n{','s32 dScMiniGm_c::CleanupResources()\n{\n    pc_touch_menu::reset();')
        anchor='if ((data_020a0e5a[gActivePlayerSlot << 1] & 0xfff) != 0)'
        if text.count(anchor)!=1: raise ValueError('Rec Room unhandled-button sound gate changed')
        text=text.replace(anchor,'if ((data_020a0e5a[gActivePlayerSlot << 1] & (0xfff & ~0x3f3)) != 0)')
    if 'typedef void (C31b4::*PMF31b4)();' in text or 'void func_ov004_020b52fc(C *c)' in text:
        text = '#include "ntr/minigame_callback.h"\n' + text
        text = re.sub(r'typedef void \((?:C31b4|C321c|Base|C)::\*(PMF\w*)\)\(\);',
                      r'using \1 = PcMinigameCallback<>;', text)
        text = text.replace('(c->*(c->pmf))();', 'c->pmf(c);')
        text = text.replace('(((Base *)self)->*((PMF *)data_ov004_020bf490)[st])();',
                            '((PMF *)data_ov004_020bf490)[st](self);')
        text = re.sub(r'static EntryTable data_ov004_020bc6e8 = [^;]+;',
                      'extern "C" EntryTable data_ov004_020bc6e8;', text)
        if 'data_ov004_020beb68->m26()' in text:
            # Keep the TU's scalar shadow view, call through a typed helper.
            text = 'extern "C" int pc_minigame_canvas_mode(void *);\n' + text.replace(
                'data_ov004_020beb68->m26()', 'pc_minigame_canvas_mode(data_ov004_020beb68)')
    return marker + text if text != original else text
