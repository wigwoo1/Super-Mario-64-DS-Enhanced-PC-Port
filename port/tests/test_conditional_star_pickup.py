"""Unit checks of evidence validation; gameplay is verified separately by replay."""
import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'tools'))
from check_conditional_star_pickup import verify


def fixture():
    rows, states = [], []
    for tick in range(1500, 2001):
        common = dict(tick=str(tick), x='0', z='0', param='303', kind='200',
                      target='0', payout_bits='2', death='0', last_hit='0',
                      hit='0', other='0', state='0', silver_stars='1' if tick >= 1600 else '0',
                      active_star_index='1' if tick >= 1570 else '0',
                      active_star_id='2' if tick >= 1570 else '0')
        if tick <= 1545:
            rows.append(dict(common, id='1', death='1' if tick >= 1530 else '0',
                             last_hit='32' if tick >= 1530 else '0',
                             payout_bits='3' if tick >= 1540 else '2',
                             param='271' if tick >= 1540 else '303'))
        if tick >= 1540:
            rows.append(dict(common, id='3', kind='180', param='48', target='2' if tick < 1570 else '0'))
            if tick <= 1600:
                rows.append(dict(common, id='2', kind='179', param='48', target='3',
                                 state='5' if tick >= 1570 else '8',
                                 hit='4194304' if tick == 1569 else '0',
                                 other='4' if tick == 1569 else '0'))
        if tick % 10 == 0:
            states.append(dict(tick=str(tick), level='0', player_disabled='0',
                               player_no_control='2' if 1570 <= tick < 1600 else '0',
                               player_state='St_GroundPound_Main' if tick == 1530 else
                               'St_Jump_Main' if tick == 1800 else 'St_Wait_Main'))
    return rows, states


class ConditionalPickupTest(unittest.TestCase):
    def setUp(self):
        self.rows, self.states = fixture()

    def test_complete(self):
        result = verify(self.rows, self.states, 0)
        self.assertEqual((result['pickup'], result['credit'], result['next_star']), (1570, 1600, 2))

    def reject_mutation(self, key, value, message):
        for row in self.rows:
            row[key] = value
        with self.assertRaisesRegex(AssertionError, message):
            verify(self.rows, self.states, 0)

    def test_no_collection(self):
        self.reject_mutation('state', '8', 'collection states')

    def test_no_contact(self):
        self.reject_mutation('hit', '0', 'no player contact')

    def test_no_sequence_advance(self):
        self.reject_mutation('active_star_index', '0', 'advance exactly once')

    def test_sequence_advance_without_next_id(self):
        self.reject_mutation('active_star_id', '0', 'invalid next ID')

    def test_no_credit(self):
        self.reject_mutation('silver_stars', '0', 'count did not increase')

    def test_duplicate_credit(self):
        for row in self.rows:
            if row['silver_stars'] == '1':
                row['silver_stars'] = '2'
        with self.assertRaisesRegex(AssertionError, 'count did not increase'):
            verify(self.rows, self.states, 0)

    def test_no_collection_pose(self):
        for state in self.states:
            state['player_no_control'] = '0'
        with self.assertRaisesRegex(AssertionError, 'collection pose'):
            verify(self.rows, self.states, 0)

    def test_no_later_jump(self):
        for state in self.states:
            if state['player_state'] == 'St_Jump_Main':
                state['player_state'] = 'St_Wait_Main'
        with self.assertRaisesRegex(AssertionError, 'no later jump'):
            verify(self.rows, self.states, 0)

    def test_crash(self):
        with self.assertRaisesRegex(AssertionError, 'exit cleanly'):
            verify(self.rows, self.states, 1)


if __name__ == '__main__':
    unittest.main()
