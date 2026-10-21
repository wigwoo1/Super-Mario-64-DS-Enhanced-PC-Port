"""Fetch the pinned, unmodified SDL Windows SDK used for native gamepads."""
import hashlib
import shutil
import sys
import urllib.request
import zipfile
from pathlib import Path

VERSION='3.2.28'
URL=f'https://www.libsdl.org/release/SDL3-devel-{VERSION}-VC.zip'
SHA256='dc8328a923643a19a53e54ca418e8e2ac49f3cf6c95e90bbebf758cd3a00c2fb'

def prepare(cache):
    cache=Path(cache);cache.mkdir(parents=True,exist_ok=True)
    archive=cache/f'SDL3-devel-{VERSION}-VC.zip'
    if not archive.exists():
        temporary=archive.with_suffix('.download')
        with urllib.request.urlopen(URL,timeout=120) as response,temporary.open('wb') as out:
            shutil.copyfileobj(response,out)
        if hashlib.sha256(temporary.read_bytes()).hexdigest()!=SHA256:
            temporary.unlink();raise ValueError('SDL SDK download checksum mismatch')
        temporary.replace(archive)
    if hashlib.sha256(archive.read_bytes()).hexdigest()!=SHA256:
        raise ValueError('Cached SDL SDK checksum mismatch')
    root=cache/f'SDL3-{VERSION}'
    if not (root/'lib/x86/SDL3.dll').exists():
        with zipfile.ZipFile(archive) as z:
            for name in z.namelist():
                relative=Path(name)
                if relative.is_absolute() or '..' in relative.parts:
                    raise ValueError('Unsafe SDK archive member')
            z.extractall(cache)
    return root

if __name__=='__main__':print(prepare(sys.argv[1]).resolve())
