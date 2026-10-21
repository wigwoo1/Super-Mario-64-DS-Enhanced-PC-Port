from pathlib import Path
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import dsmt_host


class SettingsTypography(unittest.TestCase):
    def test_real_title_font_initializer(self):
        root=Path(__file__).resolve().parents[2]
        source=(root/'src/unnamed/ov007/func_ov007_020b8548.c').read_text()
        result=dsmt_host.transform(source,'func_ov007_020b8548.c')
        hook='pc_settings_capture_title_font((const void *)p1, (unsigned short)p2);'
        self.assertEqual(result.count(hook),1)
        self.assertLess(result.index('data_ov007_0210345c = p1;'),result.index(hook))
        self.assertLess(result.index(hook),result.index('MultiStore32Bytes(v1'))
        with self.assertRaisesRegex(ValueError,'font capture anchor'):
            dsmt_host.transform(source.replace('data_ov007_0210345c = p1;','changed;'),'func_ov007_020b8548.c')


if __name__=='__main__':
    unittest.main()
