"""Guard the read-only mission-selection observation used by PC replay tests."""
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'port/tools'))
from star_select_host import transform


class StarSelectionTransformTests(unittest.TestCase):
    def test_snapshot_lifetime(self):
        factory = transform((ROOT / 'src/d_s_star_sel.cpp').read_text())
        self.assertIn('ntr::rt_frame()-pc_observed_tick>1', factory)
        self.assertIn('if(!pc_observed ||', factory)
        self.assertIn('pc_observed=false;', factory)
        cleanup = transform((ROOT / 'src/_ZN12dScStarSel_c16CleanupResourcesEv.cpp').read_text())
        self.assertIn('pc_star_select_forget();', cleanup)

    def test_original_selection_remains_authoritative(self):
        behavior = transform((ROOT / 'src/_ZN12dScStarSel_c8BehaviorEv.cpp').read_text())
        self.assertIn('pc_star_select_observe(FB(this,0x115),FB(this,0x135)', behavior)
        self.assertEqual(behavior.count('data_0209f1f0 = FB(this, 0x115) + 1;'), 1)
        self.assertIn('data_020a0e58[0] & 0x20', behavior)
        self.assertIn('if (cur != 0)', behavior)

    def test_unrelated_source(self):
        source = 'int unrelated() { return 7; }'
        self.assertEqual(transform(source), source)


if __name__ == '__main__':
    unittest.main()
