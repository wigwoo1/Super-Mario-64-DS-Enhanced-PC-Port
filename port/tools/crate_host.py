"""Native resources and typed state dispatch for the original wooden crate."""

STATE_METHODS = (
    ('func_ov098_02138b18', 'func_ov098_021389f8'),
    ('func_ov098_021389cc', 'func_ov098_021388bc'),
    ('func_ov098_02138818', 'func_ov098_02138734'),
    ('func_ov098_021385e0', 'func_ov098_021384fc'),
    ('func_ov098_02138484', 'func_ov098_02138344'),
    ('func_ov098_02138318', 'func_ov098_02138238'),
    ('func_ov098_021381e8', 'func_ov098_0213814c'),
)


def transform(text):
    import re
    for kind in ('BlockSModelFilePtr', 'BlockSCollisionFilePtr'):
        pattern = rf'{kind}\(unsigned int fileID\) \{{ pc_sharedfile_handle_construct\(this, fileID\); \}}'
        text, count = re.subn(pattern, f'{kind}(unsigned int fileID) {{words[0]=fileID;words[1]=0;}}', text)
        assert count == 1, kind
        old = f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old) == 1, kind
        text = text.replace(old, f'~{kind}() {{}}', 1)

    old = 'extern int data_ov098_0213c4c8[];'
    assert text.count(old) == 1
    text = text.replace(old, 'extern "C" void *pc_crate_files[3];\nextern "C" unsigned char data_ov002_0210d934[];')
    text = text.replace('f = data_ov098_0213c4c8;', 'f = (int *)pc_crate_files;')
    text = text.replace('f = (char *)data_ov098_0213c4c8;', 'f = (char *)pc_crate_files;')

    old = '''struct BlockSStateHost;
typedef void (BlockSStateHost::*BlockSStateFn)();
struct BlockSStateEntry { BlockSStateFn fn[2]; };
extern BlockSStateEntry data_ov098_0213c878[];
struct BlockSStateHost { char pad[0x560]; int idx; };'''
    new = '''using BlockSStateHost = daObjBlockS_c;
typedef void (*BlockSStateFn)(BlockSStateHost *);
struct BlockSStateEntry { BlockSStateFn fn[2]; };
extern BlockSStateEntry data_ov098_0213c878[];'''
    assert text.count(old) == 1
    text = text.replace(old, new)
    text = text.replace('c->idx', 'c->mState')
    for index in (0, 1):
        old = f'(c->*data_ov098_0213c878[j].fn[{index}])();'
        assert text.count(old) == 1
        text = text.replace(old, f'data_ov098_0213c878[j].fn[{index}](c);')
    anchor = '    data_ov098_0213c878[j].fn[0](c);'
    text = text.replace(anchor, anchor + '\n    pc_crate_probe::event("state_enter", self);', 1)

    # Bind scalar/recovered C spellings to the actual native thiscall methods.
    replacements = (
        ('_ZN6Player14IsFrontSlidingEv((char *)mHoldingPlayer)', 'mHoldingPlayer->IsFrontSliding()'),
        ('_ZN6Player17LostGrabbedObjectEv((char *)mHoldingPlayer)', 'mHoldingPlayer->LostGrabbedObject()'),
        ('_ZN8dActor_c11UpdateCarryER6PlayerRK7Vector3((char *)this, (char *)mHoldingPlayer, (struct Vector3 *)&mCarryOffsetX)',
         '(char *)UpdateCarry(*mHoldingPlayer, *(Vector3 *)&mCarryOffsetX)'),
        ('_ZN8dActor_c19DisappearPoofDustAtERK7Vector3((char *)this, &vec2)', 'DisappearPoofDustAt(vec2)'),
        ('_ZN8dActor_c19DisappearPoofDustAtERK7Vector3(((char *)this), &vec2)', 'DisappearPoofDustAt(vec2)'),
        ('_ZN10dBgW_KcMbg9TransformERK9Matrix4x3s(&mMeshCollider, mClsnMat, mAngleY)',
         'mMeshCollider.Transform(mClsnMat, mAngleY)'),
    )
    for old, new in replacements:
        assert old in text, old
        text = text.replace(old, new)
    # The shared native Matrix4x3 header names its translation Vector3.
    # These are the same original words at offsets36/40/44.
    for matrix in ('mShadowMtx', 'mClsnMat', 'mModel.mat4x3'):
        for index, axis in enumerate(('x', 'y', 'z'), 9):
            old = f'{matrix}.m[{index}]'
            assert text.count(old) == 1, old
            text = text.replace(old, f'{matrix}.t.{axis}')

    signature = 'int daObjBlockS_c::InitResources()\n{'
    assert text.count(signature) == 1
    text = text.replace(signature, signature + '''
    static bool ready=false;
    if(!ready) {
        data_ov098_0213c850.Construct(0x458);
        data_ov098_0213c858.Construct(0x459);
        ready=true;
    }
''', 1)
    for signature, event in (
        ('int daObjBlockS_c::Behavior()', 'behavior'),
        ('void daObjBlockS_c::Kill()', 'kill'),
        ('void daObjBlockS_c::OnGroundPounded(dActor_c &other)', 'ground_pound'),
        ('void daObjBlockS_c::OnTurnIntoEgg(Player &player)', 'egg'),
        ('int daObjBlockS_c::OnYoshiTryEat()', 'try_eat'),
    ):
        anchor = signature + '\n{'
        assert text.count(anchor) == 1, signature
        text = text.replace(anchor, anchor + f'\n    pc_crate_probe::event("{event}", this);', 1)

    start = text.index('/* The fourteen state member-pointer records live in .data')
    methods = [name for pair in STATE_METHODS for name in pair]
    wrappers = ''.join(f'static void pc_crate_dispatch_{i}(daObjBlockS_c *self) {{ self->{name}(); }}\n'
                       for i, name in enumerate(methods))
    table = 'BlockSStateEntry data_ov098_0213c878[7] = {\n' + ''.join(
        f'    {{ &pc_crate_dispatch_{i*2}, &pc_crate_dispatch_{i*2+1} }},\n' for i in range(7)) + '};\n'
    text = text[:start] + wrappers + table + '''
extern "C" void *pc_crate_files[3] = {
    &data_ov098_0213c850, &data_ov098_0213c858, data_ov002_0210d934
};
'''
    return '#include "hal/crate_probe.h"\n' + text
