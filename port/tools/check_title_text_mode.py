"""Check isolated native title text-mode replays, persistence and save integrity."""
import argparse
import csv
import hashlib
import json
import struct
import zlib
from pathlib import Path


def setting(path):
    values = dict(line.split('=', 1) for line in path.read_text(encoding='utf-8-sig').splitlines() if '=' in line)
    return int(values.get('brain_rot_text', '0'))


def check(case):
    config = json.loads((case/'acceptance-config.json').read_text(encoding='utf-8-sig'))
    assert config['muted'] and config['rom_unchanged']
    assert hashlib.sha256((case/'sm64ds_pc.exe').read_bytes()).hexdigest() == config['exe_sha256'].lower()
    expected = {
        'controller': (0, 1, {1650:0, 1900:0, 2050:1, 3250:1}),
        'reload-on': (1, 0, {1650:1, 1950:0, 2200:0}),
        'reload-off': (0, 0, {1650:0, 1800:1, 2000:0, 2400:0}),
    }
    for name, (before, after, states) in expected.items():
        run = case/name
        assert (run/'exit-code.txt').read_text(encoding='utf-8-sig').strip() == '0', name
        assert setting(run/'before.ini') == before and setting(run/'after.ini') == after, name
        rows = list(csv.DictReader((run/'menu.csv').open()))
        by_tick = {int(row['tick']):row for row in rows}
        for tick, enabled in states.items():
            assert int(by_tick[tick]['brain_rot_text']) == enabled, (name,tick,by_tick[tick])
        assert all(row['backlight']=='1' for row in rows), 'text choice changed legacy backlight state'
        assert rows[-1]['root']=='0' and rows[-1]['sub']=='0', 'Back failed to return to title'
        replay = json.loads((run/'test-config.json').read_text(encoding='utf-8-sig'))
        assert replay['muted'] and replay['exe_sha256']==config['exe_sha256']
        from PIL import Image
        for tick in replay['captures'].split(','):
            with Image.open(run/f'menu-{tick}.bmp') as image:
                assert image.size==(1280,720)
                image.save(run/f'menu-{tick}.png')
        assert 'unhandled exception' not in (run/'stderr.txt').read_text()
    controller = {int(row['tick']):row for row in csv.DictReader((case/'controller/menu.csv').open())}
    for tick, sound, focus in [(2250,2,10),(2350,1,10),(2450,0,10),(2650,1,11),(2750,2,11),(2850,0,11)]:
        assert (int(controller[tick]['sound']),int(controller[tick]['focus']))==(sound,focus)
        assert controller[tick]['brain_rot_text']=='1'
    seeds = json.loads((case/'source-hashes.json').read_text(encoding='utf-8-sig'))
    for name, digest in seeds.items():
        assert hashlib.sha256((case/'seed'/name).read_bytes()).hexdigest()==digest
    original = (case/'seed/slot1.sav').read_bytes()
    for name, slot in [('slot1.sav',0),('slot2.sav',1)]:
        raw = bytearray((case/'saves'/name).read_bytes())
        crc = struct.unpack_from('<I',raw,16)[0]
        struct.pack_into('<I',raw,16,0)
        assert zlib.crc32(raw)==crc and struct.unpack_from('<I',raw,8)[0]==slot
        assert raw[24:]==original[24:], 'adventure progress changed'
    assert (case/'saves/minigames.sav').read_bytes()==(case/'seed/minigames.sav').read_bytes()
    assert not (case/'saves/slot3.sav').exists()
    print('PASS: controller and pointer text toggle; On/Off reload persistence; sound cycling; Back; isolated save integrity; muted ROM-preserving runs')


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('case',type=Path)
    check(parser.parse_args().case)
