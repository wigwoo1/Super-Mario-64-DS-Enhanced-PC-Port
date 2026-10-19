"""Audit an original mission1 water-bomb encounter, damage and safe recovery."""
import collections
import csv
import math
from pathlib import Path
import re
import sys

from check_power_star_replay import save_payload


def check(case, slot_seed, minigame_seed):
    assert (case / 'exit-code.txt').read_text().strip() == '0', 'unclean game exit'
    log = (case / 'run.log').read_text(errors='replace')
    assert 'confirmed mission=1 level=6' in log, 'original Battlefield mission1 not selected'
    pattern = r'\[native waterbomb replay\] event=([\w-]+) tick=(\d+)'
    events = dict((name, int(tick)) for name, tick in re.findall(pattern, log))
    with (case / 'waterbomb.csv').open(newline='') as stream:
        rows = [{k: v if k == 'player_state' else int(v) for k, v in row.items()}
                for row in csv.DictReader(stream)]
    players = {r['tick']: r for r in rows if r['kind'] == 191 and r['level'] == 6}
    assert players, 'original player not observed in Battlefield'
    grouped = collections.defaultdict(list)
    for r in rows:
        if r['kind'] == 208 and r['level'] == 6:
            grouped[r['id']].append(r)
    droppers = [r for r in grouped.values() if r[0]['variant'] == 0]
    falling = [r for r in grouped.values() if r[0]['variant'] == 1]
    fragments = [r for r in grouped.values() if r[0]['variant'] == 2]
    launched = [r for r in grouped.values() if r[0]['variant'] == 3]
    assert len(droppers) == 2, 'both original mission1 droppers not present'
    assert any(r['kind'] == 207 and r['variant'] == 0 and r['state'] == 1 for r in rows), 'native enemy cannon did not reach firing state'
    assert launched and any(len(r) > 10 and r[0]['vertical_speed'] > 0 and
                            r[-1]['y'] - r[0]['y'] > 1500 * 4096 for r in launched), 'native cannon projectile did not fly upward'
    assert len(falling) >= 2, 'repeated dropper spawns missing'
    births = sorted(r[0]['tick'] for r in falling)
    assert any(b - a == 150 for a, b in zip(births, births[1:])), 'original150-tick drop cooldown not observed'
    assert all(1000 * 4096 < r[0]['y'] - r[0]['py'] < 1250 * 4096 for r in falling), 'bomb did not spawn above the real player'
    assert any(len(r) > 10 and any(x['vertical_speed'] < 0 for x in r) and
               r[0]['y'] - min(x['y'] for x in r) > 500 * 4096 for r in falling), 'gravity-driven fall not observed'
    bouncing = [r for r in falling if {0, 1, 2} <= {x['phase'] for x in r if x['state'] == 2}]
    assert bouncing, 'squash/stretch/hop phases incomplete'
    aimed_hop = False
    for r in bouncing:
        hop = [x for x in r if x['state'] == 2 and x['phase'] == 2 and x['vertical_speed'] > 0]
        if len(hop) < 3:
            continue
        a, b = hop[0], hop[min(5, len(hop) - 1)]
        dx, dz = b['x'] - a['x'], b['z'] - a['z']
        if dx * (a['px'] - a['x']) + dz * (a['pz'] - a['z']) > 0:
            aimed_hop = True
    assert aimed_hop, 'bounce did not move toward the player'

    # Contact is captured before the next Behavior consumes otherOwner. Require
    # the same player to lose exactly one health bar immediately afterward.
    accepted = []
    for bomb in falling:
        for contact in bomb:
            if contact['other'] != contact['player_id'] or contact['other'] == 0:
                continue
            for tick in range(contact['tick'], contact['tick'] + 4):
                before, after = players.get(tick - 1), players.get(tick)
                if before and after and before['health'] - after['health'] == 1 and after['health'] > 0:
                    accepted.append((bomb, contact, tick))
    assert accepted, 'no water-bomb player contact followed by nonfatal health loss'
    burst = None
    for bomb, contact, hurt_tick in accepted:
        end = bomb[-1]
        children = [r for r in fragments if 0 <= r[0]['tick'] - hurt_tick <= 3
                    and math.hypot(r[0]['x'] - end['x'], r[0]['z'] - end['z']) < 160 * 4096
                    and abs(r[0]['y'] - end['y']) < 160 * 4096]
        if len(children) == 5:
            burst = bomb, contact, hurt_tick, children
            break
    assert burst, 'player-hit bomb did not produce exactly five native fragments'
    bomb, contact, hurt_tick, children = burst
    assert any(r['player_state'] == 'St_Hurt_Main' for t, r in players.items()
               if hurt_tick <= t <= hurt_tick + 5), 'original hurt state missing'
    assert all(len(r) > 2 and any(x['vertical_speed'] > 0 for x in r) for r in children), 'fragments did not launch'
    assert players[hurt_tick + 20]['health'] == players[hurt_tick]['health'], 'one splash caused repeated immediate health loss'

    natural_bursts = []
    for parent in falling:
        end = parent[-1]
        if end['bounces'] < 3 or end['state'] != 2 or end['phase'] != 0 or any(x['other'] for x in parent):
            continue
        children3 = [r for r in fragments if r[0]['tick'] == end['tick'] + 1
                     and math.hypot(r[0]['x'] - end['x'], r[0]['z'] - end['z']) < 160 * 4096]
        if len(children3) == 5:
            natural_bursts.append((parent, children3))
    assert natural_bursts, 'third-landing burst without actor contact not observed'
    fragment_hits = []
    for fragment in fragments:
        for hit in fragment:
            if hit['tick'] < events.get('observe', 0) or hit['other'] != hit['player_id'] or hit['other'] == 0:
                continue
            for tick in range(hit['tick'], hit['tick'] + 4):
                before, after = players.get(tick - 1), players.get(tick)
                if before and after and before['health'] - after['health'] == 1 and after['health'] > 0:
                    fragment_hits.append(tick)
    assert fragment_hits, 'fragment contact did not cause measured health loss during observation'

    names = ('observe', 'damage-and-fragments', 'retreat-corner', 'outside-dropper-range', 'complete')
    assert all(name in events for name in names), 'observation/retreat/cleanup replay incomplete'
    assert [events[n] for n in names] == sorted(events[n] for n in names), 'replay event order'
    done = events['complete']
    assert all(r['variant'] == 0 for r in rows if r['kind'] == 208 and r['tick'] >= done), 'moving water bombs or fragments survived cleanup'
    final = players[max(players)]
    assert max(players) == max(r['tick'] for r in rows), 'player left Battlefield before the final sample'
    assert final['player_state'] == 'St_Wait_Main' and final['airborne'] == final['disabled'] == final['no_control'] == 0, 'normal grounded control not restored'
    assert 0 < final['health'] <= 8, 'player did not survive the encounter'
    assert all(math.hypot(final['px'] - r[-1]['x'], final['pz'] - r[-1]['z']) > 1500 * 4096
               for r in droppers), 'final position still triggers an original dropper'
    assert save_payload(case / 'saves/slot1.sav') == save_payload(slot_seed), 'saved progression changed'
    assert (case / 'saves/slot1.sav').read_bytes() == slot_seed.read_bytes(), 'slot container changed'
    assert (case / 'saves/minigames.sav').read_bytes() == minigame_seed.read_bytes(), 'minigame save changed'
    print('PASS: original mission1 cannon launch, repeated drops, gravity, bounce and homing')
    print(f'PASS: water-bomb contact tick{contact["tick"]}, health loss tick{hurt_tick}, five fragments and splash immunity')
    print(f'PASS: natural third-landing burst and fragment damage tick{fragment_hits[0]}')
    print(f'PASS: retreat/cleanup tick{done}, grounded recovery, positive health and unchanged isolated saves')


if __name__ == '__main__':
    check(*(Path(p) for p in sys.argv[1:]))
