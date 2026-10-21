from pathlib import Path
import sys
import unittest
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import bird_host

class BirdHost(unittest.TestCase):
    def test_original_source_and_native_dispatch(self):
        root = Path(__file__).resolve().parents[2]
        source = (root / 'src/game/actors/d_a_s_bird.cpp').read_text()
        with patch.object(bird_host, 'profile', return_value=(1, 2, 3, 4, 5, 6, 7)):
            result = bird_host.transform(root, source, 'd_a_s_bird.cpp')
            with self.assertRaisesRegex(AssertionError, 'dispatch'):
                bird_host.transform(root, source.replace('slots[mState]', 'slots[0]'), 'd_a_s_bird.cpp')
        self.assertNotIn('(this->*data_ov009_02113c48.slots[mState])()', result)
        for state, call in ((0, 'func_ov009_021116ec()'), (1, 'func_ov009_021115d8()'),
                            (2, 'func_ov009_0211145c(reinterpret_cast<char *>(this))'),
                            (3, 'func_ov009_02111234()')):
            self.assertIn(f'case {state}:{call};break;', result)
        for fragment in ('Vec3_HorzLen(&diff)>0x7d0000', 'Spawn(343,0x10,',
                         'func_0201267c(0x6a,&b->mCamSpacePosX)',
                         '5000-RandomIntInternal(&data_0209e650)%4000',
                         'RandomIntInternal(&data_0209e650)>>16',
                         'b->mFlySpeed=0x28000;b->mState=3;b->mFlags&=~0x10000u',
                         'sizeof(daSBird_c)==0x184'):
            self.assertIn(fragment, result)
        # Resource wrapper bodies are already supplied by the generic host transform.
        self.assertNotIn('SBirdModelFilePtr::SBirdModelFilePtr', result)
        self.assertEqual(bird_host.transform(root, source, 'Other.cpp'), source)

if __name__ == '__main__':
    unittest.main()
