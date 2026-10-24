import pathlib
import unittest
import floor_switch_host
import hostgen

ROOT = pathlib.Path(__file__).resolve().parents[2]


class FloorSwitchHostTests(unittest.TestCase):
    def native(self):
        original = (ROOT/'src/actors/daObjSwitch_c.cpp').read_text()
        host, _ = hostgen.transform(original)
        return original, floor_switch_host.transform(host, 'daObjSwitch_c.cpp')

    def test_original_state_methods_and_event_rules_preserved(self):
        original, native = self.native()
        for method in ('func_ov002_020ba520', 'func_ov002_020ba4d8',
                       'func_ov002_020ba3fc', 'func_ov002_020ba3a8',
                       'func_ov002_020ba2d0', 'func_ov002_020ba2ac',
                       'func_ov002_020ba1ac', 'func_ov002_020ba01c',
                       'OnGroundPounded'):
            start = original.index('daObjSwitch_c::'+method+'(')
            end = original.find('// @symbol', start)
            self.assertIn(original[start:end].rstrip(), native)
        for clause in ('mTimeLimit *= 0xa;', 'Event::SetBit(mEventBit);',
                       'Event::ClearBit(mEventBit);', 'mPlayerNearby = 0;'):
            self.assertEqual(original.count(clause), native.count(clause))

    def test_lazy_handles_and_native_rows(self):
        _, native = self.native()
        self.assertNotIn('pc_sharedfile_handle_construct(this,', native)
        self.assertIn('pc_floor_switch_init_resources();', native)
        for file_id in ('0x45c', '0x45d', '0x48f', '0x490'):
            self.assertIn('.Construct('+file_id+')', native)
        self.assertNotIn('data_ov002_021098e8[idx]', native)
        self.assertIn('static_cast<daStar_c *>(p)->mPlayer == 0', native)

    def test_factories_and_callback_use_native_types(self):
        for name in ('hanswitch', 'star_switch'):
            path = ROOT/f'src/d_a_obj_switch_{name}.c'
            native = floor_switch_host.transform(path.read_text(), path.name)
            self.assertIn('new daObjSwitch_c()', native)
            self.assertNotIn('_ZN7fBase_cnwEj(852)', native)
        path = ROOT/'src/unnamed/ov002/func_ov002_020baa2c.c'
        native = floor_switch_host.transform(path.read_text(), path.name)
        self.assertIn('mPlayerNearby=1', native)
        self.assertIn('player->mIsUnderwater && !player->mIsMetal && !player->mIsMega', native)

    def test_source_drift_is_rejected(self):
        with self.assertRaises(AssertionError):
            floor_switch_host.transform('', 'daObjSwitch_c.cpp')


if __name__ == '__main__':
    unittest.main()
