"""Finish a precompiled native build using data from your own Europe Rev 0 ROM.

Requires Python 3 + ndspy and an x86 Visual Studio developer command prompt.
All extracted and generated data remains local under build/ and extracted/.
"""
import argparse
import csv
import hashlib
import json
import os
import pathlib
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
ROM_SHA256 = 'ce5829aaa79f06c67140ed1f45cbca738b0a9d0180538c8c1498472d859db032'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(args, cwd=ROOT):
    subprocess.run([str(x) for x in args], cwd=cwd, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=pathlib.Path)
    args = parser.parse_args()
    rom = args.rom.resolve()
    if sha(rom) != ROM_SHA256:
        sys.exit('This build requires an unmodified European Rev 0 ROM. No files changed.')
    if not shutil.which('cl') or not shutil.which('ml') or not shutil.which('link'):
        sys.exit('Run ASSEMBLE-PC.cmd with Visual Studio C++ Build Tools installed.')
    import ndspy.rom
    import ndspy.codeCompression
    metadata = json.loads((ROOT / '.native-kit/manifest.json').read_text())
    sys.path.insert(0,str(ROOT/'port/tools'))
    from native_kit_data import validate_manifest
    validate_manifest(metadata)
    for item in metadata['prebuilt']:
        if sha(ROOT / item['path']) != item['sha256']:
            sys.exit('Build kit integrity check failed: ' + item['path'])
    source_files = json.loads((ROOT / 'SOURCE-MANIFEST.json').read_text())['files']
    for item in source_files:
        content = (ROOT / item['path']).read_bytes().replace(b'\r\n', b'\n')
        if hashlib.sha256(content).hexdigest() != item['sha256']:
            sys.exit('Source integrity check failed: ' + item['path'])
    build = ROOT / 'build/port'
    host = build / 'host-src'
    host.mkdir(parents=True, exist_ok=True)
    for dependency in metadata.get('runtime_dependencies', []):
        if dependency['id'] != 'sdl3':
            sys.exit('Unsupported runtime dependency: ' + dependency['id'])
        sys.path.insert(0, str(ROOT / 'port/tools'))
        import sdl3_dependency
        if dependency['version'] != sdl3_dependency.VERSION or dependency['sdk_sha256'] != sdl3_dependency.SHA256:
            sys.exit('SDL dependency recipe does not match the verified kit')
        sdk = sdl3_dependency.prepare(build / 'controller-deps')
        shutil.copyfile(sdk / 'lib/x86/SDL3.dll', build / 'SDL3.dll')
        shutil.copyfile(sdk / 'LICENSE.txt', build / 'SDL3-LICENSE.txt')
    (ROOT / 'extracted/overlays').mkdir(parents=True, exist_ok=True)
    cartridge = ndspy.rom.NintendoDSRom.fromFile(str(rom))
    (ROOT / 'extracted/arm9_dec.bin').write_bytes(ndspy.codeCompression.decompress(cartridge.arm9))
    for index, overlay in cartridge.loadArm9Overlays().items():
        (ROOT / f'extracted/overlays/overlay_{index:04d}.bin').write_bytes(overlay.data)
    run([sys.executable, ROOT / 'tools/asset_catalog.py', 'generate', rom])
    filesystem = ROOT / 'extracted/dsd/files'
    filesystem.mkdir(parents=True, exist_ok=True)
    with (ROOT / 'build/assets/files.tsv').open(newline='', encoding='utf-8') as catalog:
        for row in csv.DictReader(catalog, delimiter='\t'):
            destination = (filesystem / row['path']).resolve()
            if not destination.is_relative_to(filesystem.resolve()):
                sys.exit('Invalid ROM filesystem path')
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(cartridge.files[int(row['file_id'])])
    shutil.copyfile(ROOT / '.native-kit/link-names.json', host / 'pc_link_data.json')
    shutil.copyfile(ROOT / '.native-kit/link-requests.txt', ROOT / 'build/pc-game-link-history.log')
    run([sys.executable, ROOT / 'port/tools/host_headers.py', ROOT,
         build / 'pc-host-include', build / 'pc-host-include.stamp'])
    for recipe in metadata['generators']:
        run([sys.executable] + [a.replace('{root}', ROOT.as_posix()) for a in recipe])
    generated_objects = []
    for item in metadata['local_objects']:
        source = ROOT / item['source']
        if sha(source) != item['source_sha256']:
            sys.exit('Generated data differs from the tested build: ' + item['source'])
        output = build / item['object']
        if source.suffix == '.asm':
            command = ['ml', '/nologo', '/c', '/coff', '/safeseh', '/Fo' + str(output), str(source)]
        else:
            command = ['cl', '/nologo', '/c', '/TC' if source.suffix == '.c' else '/TP',
                       '/MD', '/O2', '/Ob2', '/DNDEBUG', '/Gy', '/EHsc', '/std:c++17',
                       '/DSM64DS_PLATFORM_PC', '/DSM64DS_GAME_TARGET',
                       '/I' + str(build / 'pc-host-include'), '/I' + str(ROOT / 'port'),
                       '/I' + str(ROOT / 'include'), '/I' + str(ROOT / 'port/ntr/include'),
                       '/Fo' + str(output), str(source)]
        run(command)
        generated_objects.append(str(output))
    objects = [str(ROOT / x['path']) for x in metadata['prebuilt'] if x['path'].endswith('.obj')]
    response = build / 'kit-link.rsp'
    link_args = objects + generated_objects + [str(ROOT / '.native-kit/ntr.lib')] + metadata['system_libraries']
    response.write_text('\n'.join(subprocess.list2cmdline([arg]) for arg in link_args) + '\n')
    run(['link', '/nologo', '@' + str(response), '/out:' + str(build / 'sm64ds_pc.exe'),
         '/subsystem:console', '/OPT:REF', '/version:0.0', '/MAP:' + str(build / 'sm64ds_pc.map')])
    (ROOT / 'Play.cmd').write_text('@echo off\nsetlocal\ncd /d "%~dp0"\nset "SM64DS_ASSET_ROOT=%~dp0"\n"%~dp0build\\port\\sm64ds_pc.exe" "' + str(rom) + '"\n')
    print('Native game assembled locally. Double-click Play.cmd. This is an unfinished development preview.')


if __name__ == '__main__':
    main()
