import pathlib
import unittest
import buddy_host
import hostgen

ROOT=pathlib.Path(__file__).resolve().parents[2]


class BuddyHostTests(unittest.TestCase):
    def test_rom_callbacks(self):
        self.assertEqual(buddy_host.inventory(ROOT),[0x0212c92c,0x0212c8b0,0x0212c89c,0x0212c508,0x0212c4a0,0x0212c1a0])

    def prepare(self,name):
        text=(ROOT/'src/game/actors'/name).read_text()
        text,_=hostgen.transform(text)
        return buddy_host.transform(ROOT,text,name)

    def test_callback_dispatch(self):
        text=self.prepare('d_a_red_bombhei.cpp')
        self.assertIn('sizeof(RedBombheiState)==16',text)
        self.assertIn('.update.call(this)',text)
        self.assertNotIn('(this->*data_ov084_',text)

    def test_canonical_handles(self):
        text=self.prepare('d_a_red_bombhei.cpp')
        self.assertIn('unsigned words[2]',text)
        self.assertIn('data_ov084_02130da4.Construct(0x2b9)',text)
        self.assertIn('data_ov084_02130d9c.Construct(0x2ba)',text)

    def test_deferred_shutter_resources(self):
        text=self.prepare('d_a_obj_cannon_shutter.cpp')
        self.assertNotIn('pc_sharedfile_handle_construct(this,',text)
        self.assertIn('data_ov002_0210e12c.Construct(0x45f)',text)
        self.assertIn('data_ov002_0210e124.Construct(0x460)',text)

    def test_unknown_source_is_unchanged(self):
        self.assertEqual(buddy_host.transform(ROOT,'unchanged','other.cpp'),'unchanged')

    def test_line_registry_dispatch(self):
        for name in ('func_0203842c.cpp',):
            text=(ROOT/'src/unnamed/arm9/0203'/name).read_text()
            native=buddy_host.transform(ROOT,text,name)
            self.assertNotIn('->v7(',native)
            self.assertIn('->DetectClsn(*reinterpret_cast<dBgCh_Lin *>',native)

    def test_static_queries_unchanged(self):
        # Scope this fix to dynamic actors; keep the established static-stage
        # collision path until its separate regression coverage is expanded.
        for name in ('func_0203859c.cpp','func_02038ea4.cpp'):
            original=(ROOT/'src/unnamed/arm9/0203'/name).read_text()
            self.assertEqual(buddy_host.transform(ROOT,original,name),original)

    def test_platform_callback_dispatch(self):
        name='func_02038324.cpp'
        native=buddy_host.transform(ROOT,(ROOT/'src/unnamed/arm9/0203'/name).read_text(),name)
        self.assertNotIn('->m9(',native)
        self.assertIn('->BeforeClsn(',native)


if __name__=='__main__':
    unittest.main()
