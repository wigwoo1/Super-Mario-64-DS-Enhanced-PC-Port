"""The PC menu caller must match the full-width native rotation primitive ABI."""
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parents[1] / 'tools'))
import hostgen

class RotationABI(unittest.TestCase):
    def test_original_declarations_and_idempotence(self):
        source = Path(__file__).parents[2] / 'src/unnamed/ov007/func_ov007_020c39f8.c'
        original = source.read_text()
        transformed, count = hostgen.transform(original)
        self.assertEqual(count, 3)
        self.assertEqual(transformed, original.replace('short a, short b', 'int a, int b'))
        self.assertEqual(hostgen.transform(transformed), (transformed, 0))

    def test_unrelated_short_interface_is_preserved(self):
        source = 'extern void other(struct Matrix4x3 *out, short a, short b);'
        self.assertEqual(hostgen.transform(source), (source, 0))

if __name__ == '__main__':
    unittest.main()
