import pathlib,sys,unittest
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'port/tools'))
import hostgen
class BacklightHost(unittest.TestCase):
    def test_sync_command_does_not_wait_for_arm7(self):
        text=(ROOT/'src/unnamed/arm9/0205/func_0205f9ac.c').read_text()
        native,count=hostgen.transform(text)
        self.assertGreater(count,0)
        body=native.split('int func_0205f9ac(int a, int b) {',1)[1]
        self.assertNotIn('func_0205ff08()',body)
        self.assertNotIn('func_0205f9e8(',body)
        self.assertIn('(a >= 0 && a <= 2 && (b == 0 || b == 1)) ? 0 : 0xffff',body)
        self.assertEqual(hostgen.transform(native)[0],native)
    def test_ui_state_and_unrelated_sdk_functions_remain_original(self):
        for relative in ['src/TurnBacklightOn.c','src/TurnBacklightOff.c','src/unnamed/arm9/0205/func_0205ff08.c']:
            text=(ROOT/relative).read_text()
            self.assertEqual(hostgen.transform(text)[0],text)
if __name__=='__main__':unittest.main()
