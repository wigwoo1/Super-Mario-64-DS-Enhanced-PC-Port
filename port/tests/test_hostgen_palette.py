import sys, unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parents[1]/'tools'))
import hostgen

class MenuPalette(unittest.TestCase):
    def test_curve_constructor_return(self):
        source=(Path(__file__).parents[2]/'src/unnamed/ov007/func_ov007_020c2b9c.c').read_text()
        result,count=hostgen.transform(source)
        self.assertEqual(count,2)
        self.assertIn('void *func_ov007_020c2b9c',result)
        self.assertIn('return r;',result)
        self.assertEqual(hostgen.transform(result),(result,0))
    def test_grid_register_macros(self):
        source=(Path(__file__).parents[2]/'src/unnamed/ov007/func_ov007_020caeac.c').read_text()
        result,count=hostgen.transform(source)
        self.assertEqual(count,5)
        self.assertEqual(result.count('NTR_MMIO(u32,'),5)
        self.assertNotIn('(*(vu32 *)',result)
        self.assertEqual(hostgen.transform(result),(result,0))
    def test_original_and_idempotence(self):
        source=(Path(__file__).parents[2]/'src/unnamed/ov007/func_ov007_020c1404.c').read_text()
        result,count=hostgen.transform(source)
        self.assertEqual(count,3)
        self.assertEqual(result.count('NTR_MMIO(unsigned int, 0x040004ac) ='),2)
        self.assertNotIn('*reg',result)
        self.assertEqual(hostgen.transform(result),(result,0))
    def test_other_pointer_is_unchanged(self):
        source='void f(int *reg) {*reg = 7;}\n'
        self.assertEqual(hostgen.transform(source),(source,0))
    def test_quad_corner_alias(self):
        source=(Path(__file__).parents[2]/'src/unnamed/ov007/func_ov007_020c4684.c').read_text()
        result,count=hostgen.transform(source)
        self.assertIn('NTR_MMIO(int, 0x04000488) = x1 | (y1 << 16);',result)
        self.assertNotIn('*new_var = x1',result)
        self.assertEqual(hostgen.transform(result),(result,0))

if __name__=='__main__':unittest.main()
