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
        'Selected title/file/course, Toad/Star Gate reward/save/reload/repeat, cannon, '
        'original gold cannon aiming sight at DS/720p/4K (including map-hidden), repeat rides '
        'and Battlefield re-entry, Chain Chomp release, Battlefield flower-warp return/re-entry, '
        'Lethal Lava Land painting entry and moving bridge reload/collision, and Wanted flows '
        'have isolated checks. Koopa the Quick spawns, talks, accepts and completes its native '
        'path, but a Mario win, Star reward and save/reload lifecycle are unverified. '
        'October22 camera '
        'lens/menu-input fixes and original-menu audio/stage-bank routing have selected '
        'muted checks; physical Nintendo Switch Pro and listening fidelity are unverified. '
        'Some mixed audio still clips. Lakitu cloud timbre, mountain/all-course warp coverage, minimap '
        'broader gameplay remain open. Star/coin/health/life counters now use original art and scale with '
        'the minimap; selected English/Yoshi coin/damage/scaling/map-off checks pass. Other character/language '
        'coverage remains incomplete. Native audio remains opt-in/diagnostic. '
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
