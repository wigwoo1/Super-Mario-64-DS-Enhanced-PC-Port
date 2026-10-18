"""File-local decompiler helpers must not merge across native C linkages."""
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parents[1]/'tools'))
import hostgen

class InlineIsolation(unittest.TestCase):
    def test_incompatible_original_helpers_are_local(self):
        root=Path(__file__).parents[2]
        paths=['src/Color_Interp.c','src/actors/daDemo_c.cpp',
            'src/unnamed/ov007/func_ov007_020b58e4.c',
            'src/unnamed/ov007/func_ov007_020c31e0.c',
            'src/unnamed/ov007/func_ov007_020c8b34.c']
        for path in paths:
            with self.subTest(path=path):
                original=(root/path).read_text()
                transformed,count=hostgen.transform(original)
                self.assertGreater(count,0)
                self.assertIn('static inline ',transformed)
                self.assertNotRegex(transformed,r'(?m)^inline[^\n]*\binline_fn\s*\(')
                second,_=hostgen.transform(transformed)
                self.assertEqual(second,transformed)

    def test_other_functions_and_constructors_are_unchanged(self):
        original='inline A::A(){}\nstatic inline int other(int a){return a;}\n'
        self.assertEqual(hostgen.transform(original),(original,0))

if __name__=='__main__':unittest.main()
