import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parents[1]/'tools'))
import hostgen

class ConstructorArgument(unittest.TestCase):
    def test_native_touch_sample_and_idempotence(self):
        original=(Path(__file__).parents[2]/'src/unnamed/arm9/0203/func_0203bb60.c').read_text()
        result,count=hostgen.transform(original)
        self.assertEqual(count,1)
        self.assertIn('if (i == 0) { ntr_input_touch_sample(&local.field_0); r = &local; }',result)
        self.assertIn('else r = func_0203dabc(i);',result)
        self.assertEqual(hostgen.transform(result),(result,0))
        declaration='void func_0203bb60(void);\nvoid f(void) {func_0203bb60();}\n'
        self.assertEqual(hostgen.transform(declaration),(declaration,0))
    def test_rec_room_lifetime_hooks_and_idempotence(self):
        original=(Path(__file__).parents[2]/'src/actors/dScMiniGm_c.cpp').read_text()
        result,count=hostgen.transform(original)
        self.assertGreaterEqual(count,2)
        self.assertEqual(result.count('ntr::video_set_minigame_active(true);'),1)
        self.assertEqual(result.count('ntr::video_set_minigame_active(false);'),1)
        self.assertEqual(result.count('data_0209f5bc->IsAtStart()'),6)
        self.assertNotIn('if (vt[5]',result)
        self.assertNotIn('data_0209f5bc->IsActive()',result)
        self.assertEqual(hostgen.transform(result),(result,0))
    def test_minigame_lifetime_hooks_and_idempotence(self):
        original=(Path(__file__).parents[2]/'src/minigames/d_s_mg_base.cpp').read_text()
        result,count=hostgen.transform(original)
        self.assertGreaterEqual(count,2)
        self.assertEqual(result.count('ntr::video_set_minigame_active(true);'),1)
        self.assertEqual(result.count('ntr::video_set_minigame_active(false);'),1)
        self.assertIn('data_ov004_020beb68 = this;\n    ntr::video_set_minigame_active(true);',result)
        self.assertIn('data_ov004_020beb68 = 0;\n    ntr::video_set_minigame_active(false);',result)
        self.assertEqual(hostgen.transform(result),(result,0))
        with self.assertRaises(ValueError):
            hostgen.transform(original.replace('data_ov004_020beb68 = this;','data_ov004_020beb68 = nullptr;'))

    def test_exact_original_call_and_idempotence(self):
        original=(Path(__file__).parents[2]/'src/unnamed/ov007/func_ov007_020c798c.c').read_text()
        result,count=hostgen.transform(original)
        self.assertEqual(count,2)
        expected=original.replace('func_ov007_020c78b0(void);','func_ov007_020c78b0(void *object);').replace('func_ov007_020c78b0();','func_ov007_020c78b0(t);')
        self.assertEqual(result,expected)
        self.assertEqual(hostgen.transform(result),(result,0))

    def test_unrelated_no_argument_calls_unchanged(self):
        original='extern void other(void);\nvoid f(void) {other();}\n'
        self.assertEqual(hostgen.transform(original),(original,0))

if __name__=='__main__':unittest.main()
