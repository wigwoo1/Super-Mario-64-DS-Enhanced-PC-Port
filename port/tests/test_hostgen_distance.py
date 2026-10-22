"""Distance MMIO hook must affect its definition, never matching declarations."""
import pathlib,sys,unittest
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'port/tools'))
import hostgen

class DistanceHostTest(unittest.TestCase):
    def test_real_definition_uses_hardware_hook(self):
        text,_=hostgen.transform((ROOT/'src/unnamed/arm9/0205/func_02053274.c').read_text())
        self.assertIn('NTR_MMIO(unsigned long long, 0x040002b8)',text)
        self.assertNotIn('param[0] =',text)

    def test_declaration_in_another_worker_is_untouched(self):
        declaration='extern int func_02053274(int *a, int *b);\n'
        self.assertEqual(hostgen.transform(declaration)[0],declaration)

if __name__=='__main__':unittest.main()
