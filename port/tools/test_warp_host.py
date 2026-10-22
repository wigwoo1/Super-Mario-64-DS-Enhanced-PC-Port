import pathlib
import unittest
import hostgen
import warp_host

ROOT=pathlib.Path(__file__).resolve().parents[2]


class WarpHostTests(unittest.TestCase):
    def test_original_cylinder_and_warp_gates_preserved(self):
        original,_=hostgen.transform((ROOT/'src/game/actors/d_a_warpkun.cpp').read_text())
        native=warp_host.transform(original)
        for line in original.splitlines():
            self.assertIn(line,native)
        for clause in ('player->CanWarp()', 'IsPlayerWarping(player->mPlayerNo)',
                       'mdCcAc_c.Clear();', 'mdCcAc_c.Update();',
                       'mAngleY = 0;', 'WarpPlayer(player->mPlayerNo, warpParam & 0xff);'):
            self.assertEqual(original.count(clause),native.count(clause))

    def test_changed_source_rejected(self):
        with self.assertRaises(AssertionError):warp_host.transform('')


if __name__=='__main__':unittest.main()
