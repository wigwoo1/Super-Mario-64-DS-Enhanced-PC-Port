import pathlib
import unittest
import cannon_host
import hostgen

ROOT=pathlib.Path(__file__).resolve().parents[2]


class CannonHostTests(unittest.TestCase):
    def test_offline_input_publication(self):
        source=ROOT/'src/unnamed/arm9/0201/func_0201ffcc.c'
        text=cannon_host.transform(ROOT,source.read_text(),source.name)
        self.assertIn('if (data_0209e64c == 0) { func_0203e0ac(); return; }',text)
        self.assertIn('func_0203df40();',text)
    def test_retail_callback_order(self):
        self.assertEqual(cannon_host.inventory(ROOT),[0x213b0a4,0x213ade8,0x213ad08,0x213aa28])
        self.assertEqual(cannon_host.inventory(ROOT,True),[0x213b9d8,0x213bb1c,0x213b7e8])

    def test_native_tables_and_deferred_resources(self):
        for path in ('src/game/actors/d_a_cnn.cpp','src/actors/daWbm_c.cpp'):
            source=ROOT/path
            text,_=hostgen.transform(source.read_text())
            native=cannon_host.transform(ROOT,text,source.name)
            self.assertNotIn('(this->*data_ov098_',native)
            self.assertNotIn('pc_sharedfile_handle_construct(this,',native)
            self.assertIn('unk0=id;file=0;',native)
            self.assertIn('.call(this)',native)
            self.assertIn('sizeof(State)==8',native)
            self.assertNotIn('extern State data_ov098_',native)

    def test_callback_adaptation_keeps_gameplay(self):
        source=ROOT/'src/game/actors/d_a_cnn.cpp'
        text,_=hostgen.transform(source.read_text())
        native=cannon_host.transform(ROOT,text,source.name)
        for signature in ('void daCnn_c::func_ov098_0213aa28()',
                          'void daCnn_c::func_ov098_0213ad08()'):
            # Native table/resource changes must preserve both original bodies.
            self.assertEqual(text[text.index(signature):],native[native.index(signature):native.index('\nstatic_assert(sizeof(State)')])


if __name__=='__main__':unittest.main()
