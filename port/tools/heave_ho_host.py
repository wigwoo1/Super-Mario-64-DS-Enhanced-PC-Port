"""Bind the recovered Heave-Ho actor to the native PC runtime."""
import struct

from battlefield_platform_host import lazy_handles


def transform(root, text):
    overlay = (root / 'extracted/overlays/overlay_0077.bin').read_bytes()
    base = 0x02123740
    assert struct.unpack_from('<IHHIiiii', overlay, 0x02127a74 - base) == (
        0x021271d4, 238, 93, 3, 0x64000, 0xc8000, 0x1000000, 0x1000000)
    assert struct.unpack_from('<3i', overlay, 0x02127a5c - base) == (0, 0, 0x2c000)
    text = lazy_handles(text, ('PopoiModelFile', 'PopoiAnimationFilePtr'))
    record = 'struct daPopoi_StateRecord { int (daPopoi_c::*enter)(); int (daPopoi_c::*update)(); };'
    text = text.replace(record, '')
    start = text.index('struct PopoiTurnAwayRec {')
    end = text.index('extern PopoiModelFile', start)
    text = text[:start] + record + '\n\n' + text[end:]
    for name in ('02127cd8', '02127ce8', '02127cf8', '02127d08', '02127d18'):
        text = text.replace(f'PopoiPMF data_ov077_{name}[2]', f'daPopoi_StateRecord data_ov077_{name}')
        text = text.replace(f'(daPopoi_StateRecord *)data_ov077_{name}', f'&data_ov077_{name}')
        text = text.replace(f'(daPopoi_StateRecord *)&data_ov077_{name}', f'&data_ov077_{name}')
    text = text.replace('extern PopoiTurnAwayRec data_ov077_02127d18;',
                        'extern daPopoi_StateRecord data_ov077_02127d18;')
    text = text.replace('PopoiTurnAwayRec data_ov077_02127d18;', '''
daPopoi_StateRecord data_ov077_02127d18 = {
    &daPopoi_c::func_ov077_02126a04, &daPopoi_c::func_ov077_021269a8,
};''')
    text = text.replace('extern Vector3 data_ov077_02127a5c;', 'static const Vector3 data_ov077_02127a5c{0, 0, 0x2c000};')
    text = text.replace('int daPopoi_c::InitResources()\n{', '''int daPopoi_c::InitResources()
{
  static const bool handlesReady = [] {
    // The static records hold overlay handles until the native file table is ready.
    data_ov077_02127c88.Construct(0x40d);
    data_ov077_02127ca0.Construct(0x40e);
    data_ov077_02127c90.Construct(0x40f);
    data_ov077_02127c98.Construct(0x410);
    return true;
  }();
  (void)handlesReady;''')
    text = text.replace('extern int func_ov002_020db674(void *c,', 'extern int func_ov002_020db674(Player *c,')
    text = text.replace('void func_02012694(int a, void *b, int c);',
                        'void func_02012694(unsigned int id, const Vector3 *pos);')
    text = text.replace('func_02012694(daPopoi_SND_THROW_PLAYER, &mCamSpacePosX, 0);', r'''
                                        if (std::getenv("SM64DS_PC_HEAVE_HO_TRACE"))
                                            std::fprintf(stderr, "[native Heave-Ho throw] tick=%llu uid=%u player=%u pos=%d,%d,%d\n",
                                                static_cast<unsigned long long>(ntr::rt_frame()), unsigned(uniqueID),
                                                unsigned(a->uniqueID), a->mPosX, a->mPosY, a->mPosZ);
                                        func_02012694(daPopoi_SND_THROW_PLAYER,
                                            reinterpret_cast<const Vector3 *>(&mCamSpacePosX));''')
    text = text.replace('struct Vector3* src = (struct Vector3*)(((long)&player->mPosX));',
                        'const auto *src = reinterpret_cast<const Vector3 *>(&player->mPosX);')
    text = text.replace('(struct Vector3 *)(void *)(unsigned long long)(unsigned long)&p->mPosX',
                        'reinterpret_cast<Vector3 *>(&p->mPosX)')
    text = text.replace('    ModelAnimDraw *model = (ModelAnimDraw *)((char *)&mModelAnim);\n    model->Render(0);',
                        '    mModelAnim.Render(nullptr);')
    text = text.replace('return (int *)new daPopoi_c;', 'return new daPopoi_c();')
    text = text.replace('extern "C" int *daPopoi_c_classInit(void)', 'extern "C" daPopoi_c *daPopoi_c_classInit()')
    return '#include <cstdio>\n#include <cstdlib>\n#include "ntr/rt.h"\n' + text


def transform_throw(text):
    start = text.index('int func_ov002_020db674(')
    return '''#include "Player.h"
#include "Sound.h"
extern "C" int func_ov002_020d82f0(void *player);
extern "C" Player::State data_ov002_02110094;

extern "C" ''' + text[start:].replace('char* c, int a1, int a2, int a3',
    'Player* c, int a1, int a2, int a3').replace(
    '_ZN5Sound13PlayCharVoiceEjjRK7Vector3(*(unsigned char*)(c + 0x6d9), 0x1d, c + 0x74);',
    'Sound::PlayCharVoice(c->mCharacter, 0x1d, reinterpret_cast<const Vector3 &>(c->mCamSpacePosX));').replace(
    '*(short*)(c + 0x94)', 'c->mPrevAngleY').replace('*(short*)(c + 0x8e)', 'c->mAngleY').replace(
    '*(int*)(c + 0x98)', 'c->mHorzSpeed').replace('*(int*)(c + 0xa8)', 'c->mVertSpeed').replace(
    '*(int*)(c + 0x674)', 'c->mHurtDamage').replace('c[0x6e3]', 'c->mStateStep').replace(
    'c[0x6de]', 'c->mIsAirborne').replace('c[0x6df]', 'c->mLandSoundPlayed').replace(
    '_ZN6Player11ChangeStateERNS_5StateE(c, &data_ov002_02110094);',
    'c->ChangeState(data_ov002_02110094);')
