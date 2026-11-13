"""Reviewed local-only native data recipes. No cartridge bytes are stored here."""
import argparse
from pathlib import Path
import re

HOST_DATA = frozenset({
    'minigame_menu_data.asm', 'dsmt_data.asm', 'pc_link_data.cpp', 'romdata.c',
    'ov002_data.cpp', 'ov012_data.cpp', 'ov015_data.cpp', 'ov001_image.cpp', 'ov009_image.cpp',
    'level_overlay_tables.cpp', 'title_overlay_data.cpp', 'level_overlay_data.cpp',
})
BOOT_DATA = frozenset({
    'camera/camera_native_data.cpp', 'camera/camera_native_data.asm',
    'sound/stage_sound_data.asm', 'minigame/minigame_native_callbacks.cpp',
    'minigame/minigame_resources.asm',
})
DATA_SOURCES = frozenset({'build/port/host-src/'+p for p in HOST_DATA} |
                         {'build/port/pc-boot-src/'+p for p in BOOT_DATA})
SOURCE_ADAPTERS = frozenset({'pc_boot_dsmt.cpp', 'pc_title_session.cpp', 'player_render.cpp'})
ROM_ACTOR_SOURCES = (
    ('src/actors/daBrq_c.cpp', 'amp_host', 'transform'),
    ('src/actors/daOts_c.cpp', 'bully_host', 'transform_base'),
    ('src/game/actors/d_a_obj_km2_agaru.cpp', 'simple_lift_host', 'transform_firesea_rising_lift'),
)
DATA_SOURCES |= frozenset('build/port/pc-boot-src/' + rel
                          for rel, _, _ in ROM_ACTOR_SOURCES)


def local_source(original):
    """Classify an object before it is permitted into a downloadable kit."""
    original = original.replace('\\', '/')
    if not original.endswith('.obj'):
        raise ValueError('Expected a native object: '+original)
    for folder in ('host-src', 'pc-boot-src'):
        marker='/'+folder+'/'
        if marker not in original:
            continue
        relative=original.split(marker,1)[1][:-4]
        path='build/port/'+folder+'/'+relative
        if path in DATA_SOURCES:
            return path
        if relative.startswith('src/') and relative.endswith(('.c','.cpp')):
            return None
        if folder=='host-src' and relative in SOURCE_ADAPTERS:
            return None
        if folder=='pc-boot-src' and relative.startswith('link-seeds/') and relative.endswith('.cpp'):
            return None
        raise ValueError('Review new generated object and local reconstruction recipe: '+original)
    return None


def validate_generated_inputs(inputs):
    for original in inputs:
        local_source(original)


def validate_manifest(metadata):
    if metadata.get('format')!=2:
        raise ValueError('Use a corrected format-2 kit with all reviewed local data recipes')
    local=metadata['local_objects']
    if len(local)!=len(DATA_SOURCES) or {x['source'] for x in local}!=DATA_SOURCES:
        raise ValueError('Local reconstruction data roster differs from the reviewed inputs')
    outputs=set()
    for item in local:
        if not re.fullmatch(r'local-\d{4}\.obj',item['object']) or item['object'] in outputs:
            raise ValueError('Invalid or duplicate local object name')
        if not re.fullmatch(r'[0-9a-f]{64}',item['source_sha256']):
            raise ValueError('Missing local source digest')
        outputs.add(item['object'])
    for item in metadata['prebuilt']:
        origin=item.get('origin_object','')
        path=item['path']
        if path=='.native-kit/ntr.lib' and origin=='ntr.lib':
            continue
        if not re.fullmatch(r'\.native-kit/objects/\d{4}\.obj',path):
            raise ValueError('Unreviewed prebuilt path: '+path)
        if not origin or local_source(origin):
            raise ValueError('Local ROM-derived data object cannot be prebuilt: '+origin)


def generate(root):
    from importlib import import_module
    import boot_closure, hostgen
    import camera_host,minigame_host,minigame_data,sound_host
    directory=root/'build/port/pc-boot-src'
    paths=camera_host.write_sources(root,directory/'camera')
    paths+=minigame_host.write_sources(root,directory/'minigame')
    paths+=minigame_data.write_sources(root,directory/'minigame')
    paths+=sound_host.write_sources(root,directory/'sound')
    # These host adapters embed original tables in their native translation
    # units. Compile the complete units locally and keep their objects private.
    for relative, module, function in ROM_ACTOR_SOURCES:
        text = (root / relative).read_text(encoding='utf-8')
        text = re.sub(r'\A//cpp[^\n]*\n', '', text)
        transformed, _ = hostgen.transform(text)
        transform = getattr(import_module(module), function)
        transformed, _ = boot_closure.add_free_function_aliases(transform(root, transformed))
        path = directory / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(f'// Generated from {relative}; do not edit.\n'
                        '#include "ntr/mmio.h"\n' + transformed, encoding='utf-8')
        paths.append(path)
    relative={Path(p).relative_to(root).as_posix() for p in paths}
    expected=DATA_SOURCES - {'build/port/host-src/'+p for p in HOST_DATA}
    if relative!=expected:
        raise ValueError('Reviewed native data output roster changed')
    return sorted(relative)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root',type=Path)
    print('\n'.join(generate(parser.parse_args().root.resolve())))
