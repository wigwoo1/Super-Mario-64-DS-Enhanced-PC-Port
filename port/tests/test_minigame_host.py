import pathlib
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'port/tools'))
import minigame_host
import minigame_data


class MinigameHostTests(unittest.TestCase):
    def test_original_callback_inventory(self):
        records = minigame_host.descriptors(ROOT)
        self.assertEqual(len(records), 72)
        self.assertEqual(records['data_ov004_020bc96c'], ('dMgState_c', 'func_ov004_020b853c', False))
        self.assertEqual(records['data_ov006_0213cdd4'], ('dScMgLuigi_c', 'StateSetup', False))
        self.assertEqual(records['data_ov006_0213cd8c'], ('dScMgLuigi_c', 'MovePictureSway', True))
        self.assertEqual(sum(map(len, minigame_host.tables(ROOT).values())), 23)

    def test_real_dispatchers_and_idempotence(self):
        for source in ('src/actors/dMgState_c.cpp', 'src/actors/dScMgLuigi_c.cpp', 'src/minigames/d_s_mg_base.cpp'):
            native = minigame_host.transform((ROOT / source).read_text())
            self.assertEqual(native, minigame_host.transform(native))
            self.assertNotIn('this->*', native)
            self.assertNotIn('data_0209f5bc->m_', native)
            self.assertNotIn('data_ov004_020beb68->f26()', native)
            self.assertNotIn('sl->f68()', native)
            self.assertNotIn('g->m()', native)
            self.assertNotIn('((Obj*)c)->v28()', native)
            self.assertNotIn('extern int func_ov004_020b8f78(', native)
        # A call to the base method in another scene must not trigger its transform.
        other = 'int Other::BeforeBehavior() { return dScMgBase_c::BeforeBehavior(); }'
        self.assertEqual(minigame_host.transform(other), other)
        entry = (ROOT/'src/_ZN10dScEntry_c6RenderEv.cpp').read_text()
        self.assertEqual(minigame_host.transform(entry), entry)
        effects = minigame_host.transform((ROOT/'src/actors/unit020b2c84.cpp').read_text())
        self.assertNotIn('->*', effects)
        self.assertNotIn('static EntryTable data_ov004_020bc6e8', effects)

    def test_pause_extents(self):
        extents=minigame_host.pause_extents(ROOT)
        self.assertEqual(len(extents),17)
        self.assertEqual(extents[0x020bfe9c],(60,16))
        self.assertEqual(extents[0x020bfeb4],(32,16))
        original=(ROOT/'src/actors/dMgPsOpt_c.cpp').read_text()
        native=minigame_host.transform(original)
        self.assertIn('pc_minigame_init_pause_extents();',native)
        self.assertEqual(native,minigame_host.transform(native))

    def test_wanted_settings_consumes_input_before_retail_resume(self):
        native=minigame_host.transform((ROOT/'src/minigames/d_s_mg_base.cpp').read_text())
        behavior=native.split('int dScMgBase_c::BeforeBehavior()\n{',1)[1]
        self.assertLess(behavior.index('pc_wanted_settings_update(this)'),
                        behavior.index('flags = data_020a0e5a'))
        self.assertIn('if (!pc_wanted_settings_open(c)) func_ov004_020b9220(c + 0xf4);',native)
        self.assertIn('pc_wanted_settings_reset();',native)
        self.assertEqual(native,minigame_host.transform(native))

    def test_native_tables_generated(self):
        with tempfile.TemporaryDirectory(dir=ROOT / 'build') as directory:
            path = pathlib.Path(minigame_host.write_sources(ROOT, pathlib.Path(directory))[0])
            text = path.read_text()
            self.assertIn('PcMinigameCallback<> data_ov006_02142234[]', text)
            self.assertIn('{pc_mg_dScMgLuigi_c_StateSetup, 0}', text)
            self.assertNotIn('0x020f', text)

    def test_frame_dispatch_and_file_loader(self):
        for name,slot in [('02019390',0),('02019404',1),('02019144',2),('02019100',3)]:
            original=(ROOT/f'src/unnamed/arm9/0201/func_{name}.c').read_text()
            native=minigame_host.transform(original)
            self.assertIn('pc_minigame_graph_dispatch(',native)
            self.assertIn(','+str(slot)+',&',native)
            self.assertEqual(native,minigame_host.transform(native))
        loader=minigame_host.transform((ROOT/'src/unnamed/arm9/0201/func_020182bc.c').read_text())
        self.assertIn('return pc_fs_load_path(path);',loader)

    def test_resource_storage_and_relocations(self):
        labels, relocations = minigame_data.inventory(ROOT)
        self.assertEqual(len(labels), 1522)
        self.assertEqual(len(relocations), 1157)
        self.assertEqual(len(minigame_data.function_targets(ROOT)), 38)
        self.assertEqual(labels['data_ov006_0213ce70'], (6, 0x0213ce70))
        self.assertNotIn('data_ov006_0213cf10', labels)  # ROM vtable is never hosted.
        self.assertNotIn('data_ov004_020beb74', labels)  # Real C++ callback object.
        self.assertFalse(set(labels) & minigame_host.owned_names(ROOT))


if __name__ == '__main__': unittest.main()
