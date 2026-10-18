"""Verify native Power Star contact, course clear, save and return to control.

The labelled QA placement creates only a normal actor178/param1 in course6.
Collection, progression, camera, save menu and course exit use original code.
"""
import argparse
import csv
import pathlib
import struct
import zlib


def save_payload(path):
    raw = bytearray(path.read_bytes())
    assert len(raw) == 92 and raw[:8] == b'SM64PC01', 'invalid native save header'
    slot, size, checksum, reserved = struct.unpack_from('<4I', raw, 8)
    assert (slot, size, reserved) == (0, 68, 0), 'wrong save slot or size'
    struct.pack_into('<I', raw, 16, 0)
    assert zlib.crc32(raw) == checksum, 'save checksum failed'
    payload = bytes(raw[24:])
    assert payload[:4] == b'8000', 'invalid game-save payload'
    return payload


def verify(rows, states, exit_code, before, after, log):
    assert exit_code == 0, 'native game crashed or failed to exit normally'
    assert rows and states, 'missing replay observations'
    # The actual course object, not the QA placement, proves actor178 also
    # comes through the normal original map object loader.
    assert any(r['kind'] == '178' and r['param'] == '7' and r['state'] == '4'
               for r in rows), 'original course Power Star did not initialize'
    actors = {r['id'] for r in rows if r['kind'] == '178' and r['param'] == '1'}
    assert len(actors) == 1, 'expected exactly one labelled Power Star fixture'
    star = [r for r in rows if r['id'] in actors]
    assert star[0]['star_kind'] == '0' and star[0]['star_home_state'] == '4', 'wrong star kind/state'
    assert star[0]['save_star_mask'] == '0', 'fixture star was already collected'
    collected = [r for r in star if r['state'] == '5']
    assert collected, 'no actual Power Star pickup'
    pickup = int(collected[0]['tick'])
    assert any(int(r['tick']) < pickup and int(r['hit']) & 0x400000 and int(r['other']) != 0
               for r in star), 'no player collision before collection'
    assert {'5', '6', '7'} <= {r['state'] for r in star}, 'collection pose did not progress'
    for r in star:
        taken = int(r['tick']) >= pickup
        assert int(r['save_star_mask']) == (2 if taken else 0), 'wrong course star bit or timing'
        assert int(r['power_star_total']) == int(star[0]['power_star_total']) + int(taken), 'Power Star total changed incorrectly'
    assert before[0x14] == 0 and after[0x14] == 2, 'course star was not persisted to disk'
    assert before[0x15:0x32] == after[0x15:0x32], 'other course star progress changed'
    assert before[0x41] == after[0x41], 'saved character changed unexpectedly'
    assert '[native save] slot=0 write=committed bytes=68' in log, 'save was never committed'
    assert '[native save] Save failed' not in log, 'save failure was reported'
    exits = [s for s in states if int(s['tick']) > pickup and s['level'] == '1']
    assert exits and all(s['level'] == '6' for s in states if pickup-20 <= int(s['tick']) <= pickup), 'course exit unrelated to pickup'
    assert any(s['player_no_control'] != '0' and int(s['tick']) > pickup
               for s in states), 'collection/return never entered the original control state'
    last_star = int(star[-1]['tick'])
    assert last_star < int(states[-1]['tick']) - 100, 'star collection never finished'
    final = states[-1]
    assert final['level'] == '1' and final['player_state'] == 'St_Wait_Main' and \
        final['player_no_control'] == final['player_disabled'] == '0', 'castle control/idle did not return'
    assert any(s['level'] == '1' and s['player_state'] == 'St_Jump_Main'
               for s in states if int(s['tick']) > pickup + 150), 'no later jump in the castle'
    return dict(star=next(iter(actors)), pickup=pickup, course_exit=int(exits[0]['tick']),
                final_frame=int(final['tick']), persisted_course_mask=after[0x14])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('trace', type=pathlib.Path)
    parser.add_argument('exit_code', type=pathlib.Path)
    parser.add_argument('seed_save', type=pathlib.Path)
    parser.add_argument('saved_file', type=pathlib.Path)
    parser.add_argument('log', type=pathlib.Path)
    args = parser.parse_args()
    state = args.trace.with_name(args.trace.name.replace('-rewards.csv', '-state.csv'))
    with args.trace.open(newline='') as f:
        rows = list(csv.DictReader(f))
    with state.open(newline='') as f:
        states = list(csv.DictReader(f))
    print('PASS:', verify(rows, states, int(args.exit_code.read_text(encoding='utf-8-sig').strip()),
                          save_payload(args.seed_save), save_payload(args.saved_file), args.log.read_text()))


if __name__ == '__main__':
    main()
