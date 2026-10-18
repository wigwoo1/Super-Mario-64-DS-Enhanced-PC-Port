"""Keep the menu's native input correction tied to the reviewed source block."""
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parents[1]/'tools'))
import dsmt_host

class MenuInputTransform(unittest.TestCase):
    def setUp(self):
        self.source=(Path(__file__).parents[2]/'src/actors/dScDSMT_c.cpp').read_text()

    def test_runtime_input_is_adapted_and_initialization_preserved(self):
        result=dsmt_host.transform(self.source,'dScDSMT_c.cpp')
        self.assertEqual(result.count('result = pc_dsmt_input_update();'),1)
        self.assertNotIn('u16 *p = func_0203dabc();',result)
        self.assertIn('func_ov007_020b7090(0, 0, 0, 0, 0);',result)

    def test_changed_source_requires_review(self):
        changed=self.source.replace('u16 *p = func_0203dabc();','u16 *p = changed();')
        with self.assertRaisesRegex(ValueError,'input ABI anchor'):
            dsmt_host.transform(changed,'dScDSMT_c.cpp')

if __name__=='__main__':unittest.main()
