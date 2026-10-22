"""Check recovered Star Gate callback/resource adaptation against local source."""
import pathlib
import sys
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'port/tools'))
import hostgen
import star_gate_host

class StarGateHostTest(unittest.TestCase):
    def test_original_flow_is_preserved(self):
        source=(ROOT/'src/game/actors/d_a_star_gate.cpp').read_text()
        transformed,_=hostgen.transform(source)
        result=star_gate_host.transform(transformed)
        self.assertEqual(result.count('return s; }();'),5)
        for name in ('St_Wait_Main','St_TalkingToPlayer_Init','St_Unlocking_Main',
                     'St_OpenClose_Main','St_StayClosed_Main'):
            self.assertIn('&daStarGate_c::'+name,result)
        for original in ('if (IsInFrontOfPlayer(player))',
                         'notEnoughStars = (int)NumStars() < entry->numStars;',
                         'isMario = ((unsigned char *)data_0209caa0)[0x41] == 0;',
                         'player->TryEnterStarDoor(*(Vector3 *)&mPosX, a)',
                         'ChangeArea(t);'):
            self.assertIn(original,result)
        self.assertIn('data_ov100_02148934.Construct(0x4a1)',result)
        self.assertIn('sizeof(daStarGate_c)==0x118',result)

    def test_reject_changed_callback_source(self):
        text,_=hostgen.transform((ROOT/'src/game/actors/d_a_star_gate.cpp').read_text())
        with self.assertRaises(AssertionError):
            star_gate_host.transform(text.replace('ST_STAY_CLOSED = {','ST_STAY_CLOSED_CHANGED = {',1).replace('&daStarGate_c::St_StayClosed_Main','0',1))

if __name__=='__main__':
    unittest.main()
