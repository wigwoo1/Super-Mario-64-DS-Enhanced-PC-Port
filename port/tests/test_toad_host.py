from pathlib import Path
import sys,unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import toad_host
class NativeToad(unittest.TestCase):
    def test_replay_scope_and_floor_gate(self):
        root=Path(__file__).resolve().parents[2]
        source=(root/'port/main.cpp').read_text()
        replay=source.split('static bool replay_toad_controls() {',1)[1].split('static bool replay_course_route()',1)[0]
        self.assertIn('SM64DS_PC_AUTOMATION_ONLY',replay)
        self.assertIn('SM64DS_PC_TEST_TOAD_LEVEL',replay)
        self.assertIn('SM64DS_PC_TEST_TOAD_STAR',replay)
        self.assertIn('cycle && wanted_level!=2',replay)
        self.assertIn('data_0209f2f8!=wanted_level',replay)
        self.assertIn('mStarID!=wanted_star',replay)
        self.assertIn('std::abs(double(a->mPosY)-p->mPosY)>120*4096.0',replay)
        self.assertNotIn('dActor_c::Spawn(',replay)
        self.assertNotIn('->mPosX=',replay)

    def test_original_source_adapter(self):
        root=Path(__file__).resolve().parents[2]
        source=(root/'src/game/actors/d_a_kinopio.cpp').read_text()
        with patch.object(toad_host,'inventory',return_value=(185,171,3,262144,524288,16777216,16777216)):
            result=toad_host.transform(root,source)
            with self.assertRaisesRegex(AssertionError,'state declaration'):
                toad_host.transform(root,source.replace('extern daKinopio_c::State data_ov085_0212fe88[];',''))
            with self.assertRaisesRegex(AssertionError,'file declaration'):
                toad_host.transform(root,source.replace('extern SharedFilePtr data_ov085_02130480;',''))
        for name in ('St_Idle_Init','St_Idle_Main','St_Talk_Init','St_Talk_Main'):
            self.assertIn('&daKinopio_c::'+name,result)
        self.assertIn('PcToadFile : SharedFilePtr { unsigned words[2]; }',result)
        self.assertIn('data_ov085_02130480.Construct(0x35c)',result)
        self.assertIn('data_ov085_02130488.Construct(0x35d)',result)
        self.assertIn('data_ov085_02130490.Construct(0x35e)',result)
        self.assertIn('pc_toad_init_resources();',result)
        self.assertIn('pc_toad_capture_contact(this);',result)
        self.assertIn('const int pc_started=mTalkPlayer->StartTalk(*this,0);',result)
        self.assertIn('if(!pc_started)return;',result)
        self.assertIn('v.flags&0x08000000u',result)
        self.assertIn('sizeof(daKinopio_c)==0x210',result)
        self.assertIn('dActor_c::Spawn(\n                    0xb2, mStarID | 0x20',result)
        self.assertIn('IsStarCollectedInCurLevel(mStarID)',result)
        self.assertIn('Model::LoadFile(data_ov002_0210da40)',result)
        self.assertEqual(result.count('PcToadProfile g_profile_KINOPIO='),1)
if __name__=='__main__':unittest.main()
