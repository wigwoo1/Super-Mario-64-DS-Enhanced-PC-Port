"""Prepare an allowlisted, text-only public source snapshot. Never pushes files.

Run from the development checkout, with a separate export directory. Review its
manifest and git diff before committing. ROM-derived build outputs stay private.
"""
import argparse
import hashlib
import json
import os
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]
TREES = ('src', 'include', 'config', 'symbols', 'tools', 'port')
ROOT_FILES = ('.gitignore', '.gitattributes', 'LICENSE', 'CREDITS.md',
              'CONTRIBUTING.md', 'attribution.json', 'contributions.json',
              'function-authors.json', 'langmode-baseline.json')
EXTENSIONS = {'.c', '.cpp', '.h', '.hpp', '.py', '.md', '.txt', '.json',
              '.yaml', '.yml', '.toml', '.cmd', '.ps1', '.sh', '.asm', '.s',
              '.inc', '.cmake', '.def', '.ini'}
SKIP_DIRS = {'build', '__pycache__', '.git', '.venv', 'venv', 'bin',
             'mwccarm', 'dataset', 'ghidra_out', 'frames', 'playlog'}
SKIP_NAMES = {'license.dat', 'claims_key.txt', 'claims_handle.txt'}
SECRET = re.compile(r'(?:github_pat_[A-Za-z0-9_]{30,}|gh[pousr]_[A-Za-z0-9]{30,}|-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----|AKIA[0-9A-Z]{16})')
PRIVATE_PATH = re.compile(r'[A-Za-z]:[\\/]Users[\\/][^\\/\s"\']+')


def stage(destination):
    destination = destination.resolve()
    if destination == ROOT or ROOT in destination.parents:
        raise ValueError('Export must be outside the source checkout')
    previous_manifest = destination / 'SOURCE-MANIFEST.json'
    previous_paths = set()
    if previous_manifest.is_file():
        previous_paths = {x['path'] for x in json.loads(previous_manifest.read_text())['files']}
    files = [ROOT / name for name in ROOT_FILES if (ROOT / name).is_file()]
    for tree in TREES:
        for folder, folders, names in os.walk(ROOT / tree):
            folders[:] = [name for name in folders if name not in SKIP_DIRS and name != 'launchers']
            for name in names:
                path = pathlib.Path(folder) / name
                rel = path.relative_to(ROOT)
                if (not path.is_file() or path.is_symlink() or
                set(rel.parts) & SKIP_DIRS or path.name in SKIP_NAMES or
                path.suffix.lower() not in EXTENSIONS or
                path.name.startswith(('pc-game-', 'claims_')) or
                path.name == 'BUILD-STATUS.txt'):
                    continue
                files.append(path)
    # The upstream README remains available with its attribution and context.
    files.append(ROOT / 'README.md')
    records = []
    for path in sorted(set(files)):
        relative = path.relative_to(ROOT).as_posix()
        if relative == 'README.md':
            relative = 'README.upstream.md'
        data = path.read_bytes()
        if b'\0' in data:
            raise ValueError('Binary data in text allowlist: ' + relative)
        text = data.decode('utf-8-sig')
        if SECRET.search(text):
            raise ValueError('Credential pattern found: ' + relative)
        # Documentation is exported without the developer's Windows username.
        # Code remains byte-for-byte unchanged; machine-specific build wrappers
        # must be made portable in the actual source before publication.
        if path.suffix == '.md' or relative.startswith('port/docs/'):
            text = PRIVATE_PATH.sub('<USER_HOME>', text)
            data = text.encode('utf-8')
        if relative == '.gitignore':
            data += b'\n# Local reconstruction and release staging\n.native-kit/\nPlay.cmd\n*.obj\n*.lib\n*.exe\n*.pdb\n*.log\n'
        data = data.replace(b'\r\n', b'\n')
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        records.append({'path': relative, 'sha256': hashlib.sha256(data).hexdigest(),
                        'bytes': len(data)})
    destination.mkdir(parents=True, exist_ok=True)
    manifest = {'format': 1, 'upstream': 'https://github.com/tangosdev/sm64ds-decomp',
                'upstream_revision': 'Original archive revision not recorded; no revision is asserted.',
                'files': records}
    (destination / 'SOURCE-MANIFEST.json').write_text(json.dumps(manifest, indent=2) + '\n')
    (destination / 'README.md').write_bytes((ROOT / 'port/docs/public-readme.md').read_bytes())
    for relative in previous_paths - {x['path'] for x in records}:
        stale = (destination / relative).resolve()
        if destination not in stale.parents:
            raise ValueError('Invalid previous manifest path')
        stale.unlink(missing_ok=True)
    print(f'Prepared {len(records)} text files, {sum(r["bytes"] for r in records):,} bytes')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('destination', type=pathlib.Path)
    stage(parser.parse_args().destination)
