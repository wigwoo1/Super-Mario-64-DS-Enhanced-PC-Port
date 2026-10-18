import pathlib
import re
import struct
import sys
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'port/tools'))
import reward_host


class RewardHostTest(unittest.TestCase):
    def test_cap_callbacks_match_original(self):
        symbols=(ROOT/'config/arm9/overlays/ov002/symbols.txt').read_text()
        targets={int(a,16):n for n,a in re.findall(
            r'^(\S+) kind:function.*?addr:0x([0-9a-f]+)',symbols,re.M)}
        data=(ROOT/'extracted/overlays/overlay_0002.bin').read_bytes()
        for address,method in zip(range(0x0210951c,0x021095ac,8),reward_host.CAP_METHODS):
            target,adjustment=struct.unpack_from('<II',data,address-0x020ad660)
            self.assertEqual(adjustment,0)
            self.assertEqual(targets[target],f'_ZN15daObjMarioCap_c{len(method)}{method}Ev')

    def test_cap_native_dispatch_and_idempotence(self):
        source=(ROOT/'src/actors/daObjMarioCap_c.cpp').read_text()
        result=reward_host.transform(source)
        self.assertIn('q->exec.fn(this);',result)
        self.assertIn('state->enter.fn(this)',result)
        self.assertIn('if (mModelAnim.file) mModelAnim.Advance();',result)
        self.assertNotIn('(((CapStateSelf *)this)->*',result)
        self.assertEqual(result.count('extern "C" McapStatePmf data_'),18)
        self.assertEqual(reward_host.transform(result),result)

    def test_star_marker_receiver_and_idempotence(self):
        source=(ROOT/'src/actors/daStar_c.cpp').read_text()
        result=reward_host.transform(source)
        self.assertEqual(result.count('static_cast<daStarBase_c *>(dActor_c::FindWithID(mMarkerID))->Collect();'),2)
        self.assertEqual(reward_host.transform(result),result)
        self.assertIn('(this->*data_ov002_021109d8[mState])();',result)

    def test_unrelated_source(self):
        self.assertEqual(reward_host.transform('int f(){return 1;}'),'int f(){return 1;}')

    def test_no_control_callbacks_match_original(self):
        data=(ROOT/'extracted/overlays/overlay_0002.bin').read_bytes()
        for address,target in zip(reward_host.NO_CONTROL_DESCRIPTORS,reward_host.NO_CONTROL_TARGETS):
            self.assertEqual(struct.unpack_from('<II',data,address-0x020ad660),(target,0))
        text=reward_host.transform((ROOT/'src/actors/Player.cpp').read_text())
        self.assertIn('pc_player_no_control_init(this,',text)
        self.assertIn('case 0x020c9c00: self->func_ov002_020c9c00(); return;',text)
        self.assertIn('case 0x020c9de4: func_ov002_020c9de4((char *)self); return;',text)
        self.assertNotIn('f(obj);',text[text.index('int Player::St_NoControl_Init()'):text.index('/* ROM ordinal 206')])
        self.assertEqual(reward_host.transform(text),text)


if __name__=='__main__':unittest.main()
