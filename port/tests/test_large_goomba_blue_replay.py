import copy
import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'tools'))
from check_large_goomba_blue_replay import verify


def fixture():
    rows, states = [], []
    for tick in range(1500, 1801):
        common = dict(tick=str(tick), x='0', y='0', z='0', param='242',
                      course_coins='5' if tick >= 1551 else '0', death='0', payload='0')
        rows.append(dict(common, id='1', kind='288'))
        if tick <= 1535:
            rows.append(dict(common, id='13', kind='202',
                             death='1' if tick >= 1520 else '0',
                             payload='3' if tick >= 1520 else '1'))
        if 1535 <= tick <= 1550:
            rows.append(dict(common, id='49', kind='290'))
        if tick % 10 == 0:
            states.append(dict(tick=str(tick), level='24', player_x='0',
                               player_y='0', player_z='0', player_disabled='0',
                               player_no_control='0', player_state=(
                                   'St_GroundPound_Main' if tick == 1520 else
                                   'St_Jump_Main' if tick == 1700 else 'St_Wait_Main')))
    return rows, states


class LargeBlueReplayTest(unittest.TestCase):
    def setUp(self):
        self.rows, self.states = fixture()

    def test_complete_pickup(self):
        self.assertEqual(verify(self.rows, self.states, 0)['blue_coins'], [('49', 1551)])

    def test_timeout_is_not_pickup(self):
        for row in self.rows:
            row['course_coins'] = '0'
        with self.assertRaisesRegex(AssertionError, 'exactly five'):
            verify(self.rows, self.states, 0)

    def test_wrong_coin_value(self):
        for row in self.rows:
            if row['course_coins'] == '5':
                row['course_coins'] = '1'
        with self.assertRaisesRegex(AssertionError, 'exactly five'):
            verify(self.rows, self.states, 0)

    def test_unrelated_blue_coin(self):
        for row in self.rows:
            if row['kind'] == '290':
                row['x'] = str(1000 * 4096)
        with self.assertRaisesRegex(AssertionError, 'exactly five'):
            verify(self.rows, self.states, 0)

    def test_wrong_death_reward(self):
        for row in self.rows:
            if row['kind'] == '202':
                row['payload'] = '1'
        with self.assertRaisesRegex(AssertionError, 'no large Goomba'):
            verify(self.rows, self.states, 0)

    def test_ground_pound_required(self):
        for state in self.states:
            if state['player_state'] == 'St_GroundPound_Main':
                state['player_state'] = 'St_Jump_Main'
        with self.assertRaisesRegex(AssertionError, 'no real player ground pound'):
            verify(self.rows, self.states, 0)

    def test_control_and_course_required(self):
        for key, value, message in [('level', '1', 'left the test course'),
                                    ('player_disabled', '1', 'control/idle')]:
            states = copy.deepcopy(self.states)
            for state in states:
                if int(state['tick']) >= 1750:
                    state[key] = value
            with self.assertRaisesRegex(AssertionError, message):
                verify(self.rows, states, 0)

    def test_crash_rejected(self):
        with self.assertRaisesRegex(AssertionError, 'exit cleanly'):
            verify(self.rows, self.states, 1)


if __name__ == '__main__':
    unittest.main()
