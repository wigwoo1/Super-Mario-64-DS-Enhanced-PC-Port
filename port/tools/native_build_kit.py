"""Package native object code while omitting every ROM-derived data object.

The consumer regenerates and compiles those objects using their own ROM. A
fresh-folder reconstruction and runtime check are required before publication.
"""
import argparse
import hashlib
import json
import pathlib
import re
import shutil
from native_kit_data import DATA_SOURCES, HOST_DATA, local_source, validate_generated_inputs, validate_manifest

ROOT = pathlib.Path(__file__).resolve().parents[2]
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('public_source', type=pathlib.Path)
    parser.add_argument('output', type=pathlib.Path)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists():
        raise ValueError('Use a fresh build-kit output directory')
    ninja = (ROOT / 'build/port/build.ninja').read_text()
    line = next(x for x in ninja.splitlines() if x.startswith('build sm64ds_pc.exe:'))
    inputs = line.split(': ', 1)[1].split(' | ', 1)[0].split()[1:]
    assert all(x.endswith('.obj') for x in inputs)
    validate_generated_inputs(inputs)
    if {local_source(x) for x in inputs if local_source(x)} != DATA_SOURCES:
        raise ValueError('Reviewed local-data object roster differs from this build')
    shutil.copytree(args.public_source, output, ignore=shutil.ignore_patterns('.git'))
    kit = output / '.native-kit'
    (kit / 'objects').mkdir(parents=True)
    prebuilt, local = [], []
    for index, original in enumerate(inputs):
        original = original.replace('\\', '/')
        source = local_source(original)
        if source:
            local.append({'source': source, 'object': f'local-{index:04d}.obj',
                          'source_sha256': hashlib.sha256((ROOT / source).read_bytes()).hexdigest()})
            continue
        data = (ROOT / 'build/port' / original).read_bytes()
        # Replace the compiled-in local checkout fallback with ".". The launcher
        # supplies SM64DS_ASSET_ROOT explicitly. Preserve COFF offsets and sizes.
        for prefix in (str(ROOT).encode(), ROOT.as_posix().encode()):
            data = data.replace(prefix, b'.' + bytes(len(prefix) - 1))
        relative = f'.native-kit/objects/{index:04d}.obj'
        (output / relative).write_bytes(data)
        prebuilt.append({'path': relative, 'sha256': hashlib.sha256(data).hexdigest(), 'origin_object': original})
    assert {x['source'] for x in local} == DATA_SOURCES
    library = (ROOT / 'build/port/ntr.lib').read_bytes()
    for prefix in (str(ROOT).encode(), ROOT.as_posix().encode()):
        library = library.replace(prefix, b'.' + bytes(len(prefix) - 1))
    (kit / 'ntr.lib').write_bytes(library)
    prebuilt.append({'path': '.native-kit/ntr.lib', 'sha256': hashlib.sha256(library).hexdigest(), 'origin_object': 'ntr.lib'})
    commands = (ROOT / 'build/native-kit-commands.txt').read_text(encoding='utf-8-sig').splitlines()
    generators = []
    for command in commands:
        if ' && ' not in command or '.py ' not in command:
            continue
        match = re.search(r'(' + re.escape(ROOT.as_posix()) + r'/port/tools/\S+\.py) (.*)"$', command)
        if not match:
            continue
        argv = [match[1]] + match[2].split()
        if any(pathlib.Path(a).name in HOST_DATA for a in argv):
            generators.append([a.replace(ROOT.as_posix(), '{root}') for a in argv])
    assert len(generators) == len(HOST_DATA), (len(generators), generators)
    generators.append(['{root}/port/tools/native_kit_data.py', '{root}'])
    shutil.copyfile(ROOT / 'build/port/host-src/pc_link_data.json', kit / 'link-names.json')
    requests = []
    for line in (ROOT / 'build/pc-game-link-history.log').read_text(errors='replace').splitlines():
        if 'unresolved external symbol ' in line:
            requests.append('unresolved external symbol ' + line.split('unresolved external symbol ', 1)[1])
    (kit / 'link-requests.txt').write_text('\n'.join(dict.fromkeys(requests)) + '\n')
    link_block = ninja.split('build sm64ds_pc.exe:', 1)[1].split('\n\n', 1)[0]
    libraries = re.search(r'  LINK_LIBRARIES = (.+)', link_block)[1].split()
    libraries = [x for x in libraries if x.lower() != 'ntr.lib']
    assert all(re.fullmatch(r'[A-Za-z0-9_]+\.lib', x) for x in libraries)
    metadata = {'format': 2, 'source_commit': 'PENDING',
                'runtime_dependencies': [{'id': 'sdl3', 'version': '3.2.28',
                    'sdk_sha256': 'dc8328a923643a19a53e54ca418e8e2ac49f3cf6c95e90bbebf758cd3a00c2fb'}],
                'prebuilt': prebuilt, 'local_objects': local, 'generators': generators,
                'system_libraries': libraries, 'root_fallback': 'Compiled local checkout paths replaced with dot; launcher sets SM64DS_ASSET_ROOT.'}
    metadata['status']=(
        'Experimental incomplete native port. Format2 omits all17 reviewed ROM-derived '
        'data objects, including camera/minigame/stage-sound data, for local regeneration. '
        'Secret Slide original castle-to-slide entry through Peach\'s window, ITEM_BLOCK21 '
        'hit by Mario, Star pickup, Save & Continue, reload and repeat without duplicate credit '
        'pass in muted isolated tests. Yoshi box hits, collection, saving and repeat '
        'pickup without duplicate credit also have isolated checks. The timer '
        'starts/stops through original course surfaces and its colorful original-art PC HUD '
        'is top-centered. Pause, high-score and timed-Star flows remain unverified. The original '
        'course uses a floor-triggered finish and timed Star, with no separate finish-line actor; '
        'its original checkerboard finish stripe is visible in a selected current replay. '
        'Selected title/file/course, Toad/Star Gate reward/save/reload/repeat, cannon, '
        'original gold cannon aiming sight at DS/720p/4K (including map-hidden), repeat rides '
        'and Battlefield re-entry, Chain Chomp release, Battlefield flower-warp return/re-entry, '
        'Lethal Lava Land painting entry and moving bridge reload/collision, and Wanted flows '
        'have isolated checks. Koopa the Quick spawns, talks, accepts and completes its native '
        'path. Selected original Mario wins, winning dialogue, Star spawn, actual '
        'collection, original Save & Continue, and a fresh-process repeat pickup '
        'without duplicate Star credit pass with CRC-valid disk saves. '
        'The October24 native OS clock correction passes a real elapsed-time test '
        'through the original HUD conversion; complete timer/pause/high-score flows '
        'remain unverified. Battlefield lifts, tilting bridge and switch gates are '
        'covered by selected checks. Yoshi rides and dismounts both original orange '
        'lifts; one also passes original Exit Course/painting re-entry/repeat ride. '
        'Mission2 Mario reaches the original switch, opens both shortcut gates and '
        'walks through. Original Exit Course/painting re-entry resets the switch and '
        'gates; Mario presses and passes through again. Broader variants remain unverified. '
        'The native seven-tile falling-block row in level46 has selected muted, ' +
        'two-way crossings and original reset-door scene reload checks for Yoshi, ' +
        'Mario, Luigi and Wario. All seven tiles receive grounded contact and the ' +
        'fresh row is crossed again after reload. Prepared diagnostic saves/course ' +
        'picker only; earned campaign entry and other placements/courses remain ' +
        'unverified. Wario also completes a native profile160 trapdoor long-jump ' +
        'landing, tilt, natural settling and safe recovery. This is a diagnostic ' +
        'level46 route. KAIDAN65 passes authored three-step spawn and master ' +
        'visibility checks in level12 missions1/2; all three render, child links ' +
        'are correct, and Mario traverses all three steps and the upper landing in a muted test. ' +
        'Boss-triggered rise and lifecycle remain open. ' +
        'Bookshelf158 and switch213 now pass the original three-book puzzle, ' +
        'shelf opening and original course exit/re-entry with a fresh closed shelf. ' +
        'Native piano249 and chairs326 spawn/render; selected checks cover piano ' +
        'attack/damage, linked-chair tipping, safe escape, fresh course instances ' +
        'and repeated piano activation. An independent chair leaps and breaks ' +
        'with original wood/dust; the stationary replay later dies. Safe independent ' +
        'chair escape and destroyed-chair re-entry remain unverified. ' +
        'Native attacking books327 and hidden generators328 use their original ' +
        'profiles. All four corridor generators fire visible projectiles in a ' +
        'selected muted test. Mario defeats one attacking book, collects its ' +
        'original five-coin blue reward, exits and re-enters through the original ' +
        'game, then defeats and collects it again. All three books and four ' +
        'generators receive fresh instances. Other characters/placements and ' +
        'repeated corridor firing remain unverified. ' +
        'Native Boo/Big Boo resources load; ' +
        'three distinct ordinary Boo defeats, separate five-coin payouts and the main-hall boss counter ' +
        'advancing to three are verified. A separate Mission1 replay verifies the original rear entrance, ' +
        'the two additional area6 Boo defeats and their five-coin payouts. A continuous Mission1 run ' +
        'now verifies all five defeats and collected payouts, the same boss counter reaching five, ' +
        'and Big Boo activation with safe player control. Selected later checks cover ' +
        'Big Boo defeat and Star spawn; full campaign acceptance remains open. ' +
        'Selected original brick debris and the opening sleep-bubble pop render; '
        'Mario collects the brick coins and the original Yoshi restriction is preserved. '
        'Wider effects remain open. '
        'October22 camera '
        'lens/menu-input fixes and original-menu audio/stage-bank routing have selected '
        'muted checks; physical Nintendo Switch Pro and listening fidelity are unverified. '
        'Some mixed audio still clips. Lakitu cloud timbre, mountain/all-course warp coverage, minimap '
        'broader gameplay remain open. Star/coin/health/life counters now use original art and scale with '
        'the minimap; selected English/Yoshi coin/damage/scaling/map-off checks pass. Other character/language '
        'coverage remains incomplete. Native audio remains opt-in/diagnostic. '
        'A selected authored Hazy Maze Cave wooden crate receives Yoshi ground-pound, '
        'renders original debris and releases three coins that are collected. Entry used '
        'the diagnostic picker; same-session respawn/repeat pound yields no duplicate payout. '
        'Yoshi capture/swallow creates the original cuboid egg carrying three coins. '
        'A nearby-wall egg flight/burst and three-coin pickup are verified. '
        'Mario collects all three coins after an ordinary crate throw; Luigi and Wario '
        'have selected carry/throw/break checks. Original HMC entry and Yoshi crate '
        'course re-entry/repeat payout have isolated acceptance. Long-distance egg '
        'targeting, other placements and broader character coverage remain unverified. '
        'Selected Battlefield question-block rewards and course re-entry pass for '
        'Yoshi fire, Mario wings, Luigi vanish and Wario metal. Mario active-wing '
        'refresh passes after a native music-call signature repair. The original '
        'question-block activation switch presses, animates, saves and reloads; '
        'an actually earned activation save enables a real Battlefield Feather pickup. '
        'Original castle inspection entry with a prepared14-Star Mario save and '
        '13-Star denial pass selected checks. The lobby light-beam model/hint and '
        '13-Star/activated-save visibility have native checks. An original front-door '
        'round trip reloads the model and repeats its hint. Earned '
        'prerequisite progression and other block contents/placements remain unverified. '
        'Native ambient emitters have selected castle playback/range/room reload and '
        'camera-underwater suppression/resumption checks. Their original level8 '
        'mission/collected-Star gate passes isolated comparisons. Native sound-range '
        'defaults use the original preset initializer. Nearby level8 playback, other '
        'gates, listening fidelity and spatial mixing remain unverified. '
        'Native recovery-heart model/spin/contact/actual health restoration and '
        'original Battlefield Exit Course/castle/painting re-entry/repeat healing '
        'pass a selected Mario test with prepared prerequisites. The native '
        'common-model cache now reserves the original100 twelve-byte entries '
        'instead of64; a muted course cycle exercises68, clears and rebuilds '
        'the cache without the earlier stage-cleanup crash. Other heart '
        'placements/characters and full campaign coverage remain unverified. '
        'Both native arrow directions have selected Mario mesh/player collision '
        'and original Battlefield course-reentry checks, with five old actors '
        'unloaded and five fresh placements restored. The original Star selector '
        'uses one canvas with live character controls; controller-style Mission1 '
        'and composed face-click Mission2 entry pass selected4:3/16:9 checks. '
        'Other character strips/courses, destruction attacks, physical controller '
        'hardware and alternate renderer runtime remain unverified. '
        'Five original castle signposts spawn; Yoshi\'s selected original sign read, two '
        'ground pounds, dialogue close and normal control restoration pass muted runtime checks. '
        'Black sign text receives a light parchment dialogue card while white Lakitu text keeps '
        'the dark card; contrast was visually checked on D3D11. Software-renderer appearance and '
        'other dialogue coverage remain unverified. '
        'The first native Rainbow Ride flying carpet has a selected Yoshi route check, '
        'including obstacle climb, long-jump reboarding, safe endpoint dismount and '
        'reset. The other three paths and Rainbow Ride course re-entry remain unverified. '
        'High-refresh presentation repeats original-rate game frames; interpolation, '
        'HD texture packs, online/browser VS and other roadmap features remain unfinished.')
    validate_manifest(metadata)
    (kit / 'manifest.json').write_text(json.dumps(metadata, indent=2) + '\n')
    (output / 'ASSEMBLE-PC.cmd').write_text('''@echo off
setlocal
cd /d "%~dp0"
set "VSWHERE=%ProgramFiles(x86)%\\Microsoft Visual Studio\\Installer\\vswhere.exe"
if not exist "%VSWHERE%" (echo Install Visual Studio C++ Build Tools first. & pause & exit /b 1)
for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
if not defined VSINSTALL (echo Install the Desktop development with C++ workload first. & pause & exit /b 1)
call "%VSINSTALL%\\VC\\Auxiliary\\Build\\vcvars32.bat"
py -3 tools\\assemble_pc_build.py "%~1"
pause
''')
    print(f'Kit: {len(prebuilt)-1} precompiled objects, one native library, {len(local)} private-data objects omitted')


if __name__ == '__main__':
    main()


