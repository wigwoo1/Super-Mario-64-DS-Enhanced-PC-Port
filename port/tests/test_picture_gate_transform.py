"""Guard the native painting source adaptation against upstream layout drift."""
from pathlib import Path
import sys
import unittest
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'port/tools'))
from picture_gate_host import transform

class PaintingTransformTests(unittest.TestCase):
    def test_native_factory_and_handles(self):
        original=(ROOT/'src/actors/daPicGate_c.cpp').read_text()
        result=transform(original)
        self.assertIn('auto *p = new daPicGate_c;', result)
        self.assertNotIn('p[0] = (int)_ZTV11daPicGate_c;', result)
        self.assertEqual(result.count('.Construct('),19)
        self.assertIn('SharedFilePtr *pc_picgate_handles[19]',result)
        self.assertIn('if(!pc_picgate_live) { std::memset',result)
        self.assertIn('[native painting] texture cache reset',result)
        self.assertIn('SM64DS_PC_PICGATE_TRACE',result)
        self.assertIn('sizeof(daPicGate_c)==0x1bc',result)

    def test_texture_flags_share_record_storage(self):
        result=transform((ROOT/'src/game/actors/d_a_pic_gate.cpp').read_text())
        self.assertIn('#define data_ov080_02128688 pc_picgate_textures',result)
        self.assertIn('#define data_ov080_0212869c (pc_picgate_textures+20)',result)
        self.assertIn('if(pc_picgate_live) --pc_picgate_live;',result)

    def test_unrelated_source_unchanged(self):
        source='int unrelated() { return 1; }'
        self.assertEqual(transform(source),source)

if __name__=='__main__':unittest.main()
