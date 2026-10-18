"""Verify an isolated original-menu course exit/re-entry replay."""
import argparse
import csv
import json
from pathlib import Path
import re


def verify(prefix: Path, seed: Path, saved: Path):
    log = prefix.with_suffix('.log').read_text()
    assert int(prefix.with_suffix('.exit-code.txt').read_text()) == 0, 'game did not exit cleanly'
    rows_path = prefix.with_name(prefix.name + '-state.csv')
    rows = list(csv.DictReader(rows_path.open()))
    assert rows and rows[0]['player_state'] == 'none', 'replay did not start in original menus'
    assert log.count('[native course route] original transition 2 -> 6') >= 2, 'did not enter Battlefield twice'
    assert len(re.findall(r'\[native mission selection\] confirmed mission=\d+ level=6', log)) >= 2, 'both mission selections did not confirm'
    assert log.count('[native painting] texture cache reset') >= 2, 'painting cache did not reset on castle reload'
    assert log.count('[native painting] actor created live=6') >= 2, 'six painting actors were not recreated twice'
    assert log.count('[native painting] actor destroyed live=0') >= 2, 'painting actors did not clean up on both course entries'
    second_reset = log.find('[native painting] texture cache reset', log.find('[native painting] texture cache reset') + 1)
    second_create = log.find('[native painting] actor created live=6', log.find('[native painting] actor created live=6') + 1)
    painting_capture = log.find('[native capture] tick=7100')
    second_cleanup = log.find('[native painting] actor destroyed live=0', log.find('[native painting] actor destroyed live=0') + 1)
    assert second_reset < second_create < painting_capture < second_cleanup, 'painting screenshot was not captured between reload and course cleanup'
    assert 'tick=7100 raster=1280x720 triangles=908' in log, 'painting frame did not render scene geometry'
    course_rows = [r for r in rows if r['level'] == '6']
    assert len(course_rows) >= 2, 'state capture lacks both course visits'
    first_tick = int(course_rows[0]['tick'])
    second_course_rows = [r for r in course_rows if int(r['tick']) > first_tick + 2000]
    assert second_course_rows, 'second course visit missing'
    second_tick = int(second_course_rows[0]['tick'])
    assert any(first_tick < int(r['tick']) < second_tick and r['level'] == '2' for r in rows), 'Exit Course did not return to the castle between course visits'
    for frame in (7100, 9800, 10400):
        screenshot = prefix.parent / f'{prefix.name}-{frame}.bmp'
        assert screenshot.is_file(), f'missing requested screenshot at frame {frame}'
    final = rows[-1]
    assert final['level'] == '6' and final['player_character'] == '3', 'final scene is not Yoshi in Battlefield'
    assert final['player_no_control'] == final['player_disabled'] == '0', 'Yoshi did not regain normal control'
    assert seed.read_bytes() == saved.read_bytes(), 'isolated QA save changed'
    return {
        'status': 'PASS',
        'course_visits': 2,
        'painting_cache_resets': log.count('[native painting] texture cache reset'),
        'painting_actors_created': log.count('[native painting] actor created live=6'),
        'painting_actors_destroyed': log.count('[native painting] actor destroyed live=0'),
        'final_frame': int(final['tick']),
        'final_character': 'Yoshi',
        'screenshots': ['7100.bmp (reloaded painting)', '9800.bmp', '10400.bmp'],
        'qa_save_unchanged': True,
    }


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('prefix', type=Path)
    parser.add_argument('seed', type=Path)
    parser.add_argument('saved', type=Path)
    args = parser.parse_args()
    print(json.dumps(verify(args.prefix, args.seed, args.saved), indent=2))

