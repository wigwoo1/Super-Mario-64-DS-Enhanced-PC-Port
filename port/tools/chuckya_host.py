"""Bind the original Chuckya state records and shared file handles on PC."""
import re
import struct


STATE_TEMPLATES = {
    0x0211de70: (0x0211d920, 0x0211d908),
    0x0211de80: (0x0211d968, 0x0211d928),
    0x0211de90: (0x0211d900, 0x0211d918),
    0x0211dea0: (0x0211d980, 0x0211d990),
    0x0211deb0: (0x0211d948, 0x0211d970),
    0x0211dec0: (0x0211d988, 0x0211d930),
    0x0211ded0: (0x0211d940, 0x0211d938),
    0x0211dee0: (0x0211d998, 0x0211d950),
    0x0211def0: (0x0211d978, 0x0211d910),
    0x0211df00: (0x0211d958, 0x0211d960),
}
HANDLES = {
    0x0211ddf0: 0x327, 0x0211dde8: 0x328, 0x0211dde0: 0x329,
    0x0211de00: 0x32a, 0x0211de08: 0x32b, 0x0211ddf8: 0x32c,
}


def transform(root, text):
    image = (root / 'extracted/overlays/overlay_0062.bin').read_bytes()
    base = 0x02115ee0
    assert struct.unpack_from('<IHHIiiii', image, 0x0211d9bc - base) == (
        0x02117478, 190, 100, 0x10000003, 0xc8000, 0x320000, 0x1000000, 0x1000000)
    members = {int(value, 16) for value in re.findall(r'int daHolhei_c::func_ov062_([0-9a-f]+)\(', text)}
    helpers = {
        int(address, 16): argument.strip().replace(' ', '')
        for address, argument in re.findall(
            r'extern "C" (?:int|s16) func_ov062_([0-9a-f]+)\(([^)]*)\)', text)
    }

    start = text.index('struct C;')
    end = text.index('int ApproachLinear', start)
    text = text[:start] + '''using HolheiCallback = void (*)(daHolhei_c *);
using HolheiState = HolheiCallback[2];

''' + text[end:]
    text = text.replace('extern int daHolhei_c_ChangeState(void *c, void *p);',
                        'extern void daHolhei_c_ChangeState(void *c, HolheiState &state);')
    definitions = []
    prepare = []
    for address, file_id in HANDLES.items():
        name = f'data_ov062_{address:08x}'
        text, count = re.subn(rf'^extern [^;\n]+\b{name}(?:\[\])?;[^\n]*', f'HolheiFile {name}{{0x{file_id:x}, nullptr}};', text, flags=re.M)
        assert count == 1, name
        text = text.replace(f'*){name}', f'*)&{name}').replace(f'*)& &{name}', f'*)&{name}')
        prepare.append(f'        reinterpret_cast<SharedFilePtr *>(&{name})->Construct(0x{file_id:x});')

    for state, templates in STATE_TEMPLATES.items():
        name = f'data_ov062_{state:08x}'
        text, count = re.subn(rf'^extern (?:int|char) {name}\[\];', f'extern HolheiState {name};', text, flags=re.M)
        assert count == 1, name
        workers = []
        for slot in templates:
            address, adjustment = struct.unpack_from('<II', image, slot - base)
            assert adjustment == 0
            name_of_worker = f'func_ov062_{address:08x}'
            if address in members:
                call = f'self->{name_of_worker}()'
            else:
                arg = helpers[address]
                if arg == 'void':
                    call = f'{name_of_worker}()'
                else:
                    pointer_type = arg.rsplit('*', 1)[0]
                    if pointer_type == 'unsignedchar':
                        pointer_type = 'unsigned char'
                    call = f'{name_of_worker}(reinterpret_cast<{pointer_type} *>(self))'
            parameter = 'daHolhei_c *' if call.endswith('5e0()') else 'daHolhei_c *self'
            workers.append(f'    +[]({parameter}) {{ {call}; }},')
        definitions.append(f'extern "C" HolheiState {name} = {{\n' + '\n'.join(workers) + '\n};')

    text, count = re.subn(
        r'extern "C" int daHolhei_c_ChangeState\(C \*c, PMF \*p\) \{[^\n]+\}',
        '''extern "C" void daHolhei_c_ChangeState(void *actor, HolheiState &state)
{
    auto *self = static_cast<daHolhei_c *>(actor);
    self->mState = state;
    if (state[0]) state[0](self);
}''', text)
    assert count == 1
    text = text.replace('M* m = (M*)mState;\n        if (m->pmf != 0)\n            (((Klass*)c)->*(m->pmf))();',
                        'const auto *callbacks = static_cast<const HolheiCallback *>(mState);\n'
                        '        if (callbacks[1]) callbacks[1](this);')
    text = re.sub(r'extern int data_ov062_0211df(?:10|14|18)\[\];\n', '', text)
    text = text.replace('int k = idx * 0xc;',
                        'static const Vector3 offsets[] = {{0x2c000, -0x14000, -0x78000},\n'
                        '                                      {0x44000, -0x14000, -0x78000}};')
    for axis, address in zip('xyz', ('10', '14', '18')):
        text = text.replace(f'*(int*)((char*)data_ov062_0211df{address} + k)', f'offsets[idx].{axis}')
    text = text.replace('((dExtFrameCtrl_c *)((char *)c + 0x350))->',
                        'static_cast<dExtFrameCtrl_c &>(static_cast<daHolhei_c *>(static_cast<void *>(c))->mModel).')
    text = text.replace('char *o = *(char **)((char *)this + 0x3f8);', 'auto *o = mHeld;')
    text = re.sub(r'    o = \*\(char \*\*\)\(\(char \*\)this \+ 0x3f8\);\n', '', text)
    for field, offset in zip(('mPosX', 'mPosY', 'mPosZ'), ('5c', '60', '64')):
        text = text.replace(f'*(int *)(o + 0x{offset})', f'o->{field}')
    text = text.replace('extern int func_ov002_020db54c(int p,', 'extern void func_ov002_020db54c(void *p,')
    text = text.replace('func_ov002_020db54c((int)self->mHeld,', 'func_ov002_020db54c(self->mHeld,')
    text = text.replace('extern short func_02010844', 'extern int func_02010844')
    text = text.replace('extern void func_02012694(int a, void *p);',
                        'extern void func_02012694(unsigned int id, const Vector3 *pos);')
    text = re.sub(r'func_02012694\((0x[0-9a-f]+), &(self->)?mCamSpacePosX\)',
                  r'func_02012694(\1, reinterpret_cast<const Vector3 *>(&\2mCamSpacePosX))', text)
    text = text.replace('extern int func_ov062_02115f84(char* c);', 'extern int func_ov062_02115f84(char* c);')
    text = text.replace('void func_ov062_02116010(void* self);', 'void func_ov062_02116010(char* self);')
    anchor = 'int daHolhei_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '\n    static const bool handlesReady = [] {\n' +
                        '\n'.join(prepare) + '\n        return true;\n    }();\n    (void)handlesReady;')
    render = 'int daHolhei_c::Render()\n{'
    assert text.count(render) == 1
    text = text.replace(render, render + '\n    pc_render_probe::Scope probe(this, 0, &mModel.mat4x3);')
    return '#include "hal/actor_render_probe.h"\n' + text + '\n' + '\n\n'.join(definitions) + '\n'
