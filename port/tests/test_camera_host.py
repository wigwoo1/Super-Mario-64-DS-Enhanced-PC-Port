import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'port/tools'))
import camera_host


class CameraHostTests(unittest.TestCase):
    def setUp(self):
        self.original = (ROOT / 'src/engine/view/dCamera_c.cpp').read_text()

    def test_render_hook_and_original_tracking(self):
        adapted = camera_host.transform(self.original)
        self.assertEqual(adapted.count('pc_camera_zoom(this, sp);'), 1)
        behavior = self.original.split('int dCamera_c::Behavior()')[1].split('// @symbol')[0]
        self.assertIn(behavior, adapted)
        self.assertLess(adapted.index('pc_camera_zoom(this, sp);'),
                        adapted.index('s32 s2 = (s32)(sp[2] + 4) >> 3;'))
        self.assertIn('_ZN9dBgCh_Lin10DetectClsnEv(&line)!=0', adapted)
        self.assertIn('!player[0x6f6] && !player[0x709]', adapted)
        self.assertIn('inspection-exit tick=', adapted)
        self.assertIn('player+0x6ce) &= ~4u', adapted)
        self.assertIn('recenter_from_no_control',adapted)
        self.assertIn('recenter-exit tick=',adapted)

    def test_changed_anchor_is_rejected(self):
        with self.assertRaises(ValueError):
            camera_host.transform(self.original.replace('s32 s2 =', 's32 changed ='))


if __name__ == '__main__':
    unittest.main()
