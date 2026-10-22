from pathlib import Path
import sys,unittest
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'port/tools'))
from toad_reward_test_source import worker


class ToadRewardWorker(unittest.TestCase):
    def test_exact_original_worker(self):
        original=(ROOT/'src/game/actors/d_a_kinopio.cpp').read_text()
        extracted=worker(original)
        self.assertIn(extracted.rstrip(),original)
        self.assertTrue(extracted.startswith('void daKinopio_c::St_Talk_Main()\n{'))
        self.assertTrue(extracted.rstrip().endswith('}'))
        self.assertIn('NumStars() >= data_ov085_0212f27c[mStarReqIndex]',extracted)
        self.assertIn('IsStarCollectedInCurLevel(mStarID) == 0',extracted)
        self.assertIn('0xb2, mStarID | 0x20, starPos, 0, mAreaId, -1',extracted)
        self.assertEqual(extracted.count('mTalkFinished = 1;'),1)

    def test_changed_anchor_rejected(self):
        with self.assertRaises(ValueError):worker('void renamed() {}')


if __name__=='__main__':unittest.main()
