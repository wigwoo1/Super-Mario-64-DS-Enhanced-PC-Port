#!/usr/bin/env python3
"""Report the static source closure below the ARM9 boot and scene-loop roots.

This is a dependency audit, not a linker: indirect actor/profile tables and
ROM-owned data are reported as known limitations rather than guessed away.
"""

import argparse
import hashlib
import json
import pathlib
import re
import sys


DEFAULT_ROOTS = ("func_0201a054", "func_020197b8",
                 "_ZN10dScStage_c14GraphCallback1Ev",
                 "func_0204c194", "func_0204c0a8", "func_0204c0e8",
                 "daKinopio_c_classInit", "_ZN11daKinopio_cD1Ev",
                 "daSBird_c_classInit", "_ZN9daSBird_cD1Ev",
                 "daCnn_c_classInit", "daWbm_c_classInit", "daTree_c_classInit",
                 "daWarpkun_c_classInit", "_ZN11daWarpkun_cD1Ev",
                 "daObjMc_Metalnet_c_classInit",
                 "daRedBombhei_c_classInit", "daObjCannonShutter_c_classInit",
                 "daWanwan_c_classInit", "daObjPile_c_classInit", "daObjWanwanShutter_c_classInit",
                 "daObjFl_London_c_classInit",
                 "daObjFl_Seesaw_c_classInit",
                 "daObjSeesaw_c_classInit_BOMB_SEESAW",
                 "daObjRotateUpdownLift_c_classInit_UPDOWN_LIFT",
                 "daObjBSwdoor_c_classInit",
                 "daRNk_c_classInit", "daRFlag_c_classInit",
                 "daObjBlockL_c_classInit_BLOCK_L",
                 "daObjBlockS_c_classInit",
                 "da1up_c_classInit_ONEUPKINOKO",
                 "daObj1UpLogo_c_classInit",
                 "daObjTatefuda_c_classInit",
                 "daObjHatenaBlock_c_classInit_ITEM_BLOCK",
                 "daObjHatenaBlock_c_classInit_HATENA_BLOCK",
                 "daObjPowerUpItem_c_classInit",
                 "daFeather_c_classInit",
                 "daObjHatenaSwitch_c_classInit",
                 "daObjC1Hikari_c_classInit",
                 "daSetSE_c_classInit",
                 "daObjHeart_c_classInit",
                 "daObjYajirusi_c_classInit_YAJIRUSI_L", "daObjYajirusi_c_classInit_YAJIRUSI_R",
                 "daObjTh_Fall_Block_c_classInit",
                 "daTrsTrap_c_classInit_MERRYGOROUND",
                 "daTrsTrap_c_classInit_TERESAPIT",
                 "daTrsTrap_c_classInit_BOOKSHELF", "daBook_c_classInit_BOOK_SWITCH",
                 "daBook_c_classInit_SHOOT_BOOK",
                 "daBook_c_classInit_KILLER_BOOK", "daBookGen_c_classInit",
                 "daChair_c_classInit", "daPiano_c_classInit",
                 "daUdlift_c_classInit_UDLIFT_TERESA",
                 "daTrs_c_classInit_TERESA", "daTrs_c_classInit_BOSS_TERESA",
                 "daTrsIcon_c_classInit", "daTBasket_c_classInit",
                 "func_ov055_02111264",
                 "_ZN11daTrsTrap_c13InitResourcesEv",
                 "daObjSwitch_c_classInit_HANSWITCH", "daObjSwitch_c_classInit_STAR_SWITCH",
                 "_ZN13daObjSwitch_c13InitResourcesEv",
                 "daObjC0_Switch_c_classInit", "daObjC0Water_c_classInit",
                 "daMip_c_classInit", "daObj_Mip_Key_c_classInit",
                 "daChScene_c_classInit", "daDoor_c_classInit", "daStarGate_c_classInit", "dMap_c_classInit",
                 "daPicGate_c_classInit", "_ZN11daPicGate_cD1Ev", "_ZN11daPicGate_c19func_ov080_021261f4Ev",
                 "daDemo_c_classInit", "daC_Jugem_c_classInit", "__sinit_02073e6c",
                 "func_ov002_020e1e70", "func_ov002_020e200c",
                 "func_ov002_020e1c20", "_ZN9dCamera_cD1Ev",
                 "_ZN10dScStage_c14GraphCallback2Ev", "__sinit_ov002_0210804c",
                 "dScDSMT_c_classInit","func_ov007_020c3e4c","func_ov007_020c3e64",
                 "dScMiniGm_c_classInit", "dScMgLuigi_c_classInit",
                 "dScStarSel_c_classInit", "_ZN12dScStarSel_c13InitResourcesEv",
                 "_ZN12dScStarSel_c16CleanupResourcesEv", "_ZN12dScStarSel_c8BehaviorEv",
                 "_ZN12dScStarSel_c6RenderEv")
SYMBOL_ROW = re.compile(r"^(\S+) kind:(function\([^)]*\)) addr:0x([0-9a-fA-F]+)")
IDENTIFIER = re.compile(r"\b[A-Za-z_][A-Za-z0-9_]*\b")
ASM = re.compile(r"^\s*(?:extern\s+\"C\"\s+)?(?:__)?asm\s", re.MULTILINE)
SYMBOL_ANNOTATION = re.compile(
    r"//\s*@symbol\s+(_Z(?:N|\d)\S+|func_(?:ov\d+_)?[0-9a-fA-F]{8})")
COMMENT_OR_STRING = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
QUALIFIED_DEFINITION = re.compile(
    r"(?m)^[ \t]*[^\n;{}]*?"
    r"((?:[A-Za-z_]\w*::)+~?[A-Za-z_]\w*)[ \t]*"
    r"\([^;{}]*\)[ \t\r\n]*(?:const[ \t\r\n]*)?\{")


def source_text(path: pathlib.Path) -> str:
    text = path.read_text(encoding="utf-8", errors="replace")
    return COMMENT_OR_STRING.sub(" ", text)


def add_free_function_aliases(text: str) -> tuple[str, int]:
    """Alias annotated free/namespace functions and static OAM methods.

    Decompiled callers often spell the CodeWarrior Itanium name with
    ``extern \"C\"`` while the recovered definition is a normal C++ free
    function. MSVC gives that definition a different decorated name. A linker
    alternate-name directive placed inside the function can use
    ``__FUNCDNAME__`` for the exact MSVC spelling and keep one implementation.
    Non-static member definitions are skipped: their MSVC ``thiscall`` ABI
    needs a typed forwarding thunk instead of a raw alias. Known SDK namespaces
    and OAM's measured all-static utility methods can safely use the raw alias.
    """
    masked = COMMENT_OR_STRING.sub(
        lambda match: re.sub(r"[^\n]", " ", match.group(0)), text)

    def function_name(symbol: str) -> str | None:
        if symbol.startswith("func_"):
            return symbol
        body = symbol[3:] if symbol.startswith("_ZN") else symbol[2:]
        parts = []
        while body:
            match = re.match(r"(\d+)", body)
            if not match:
                break
            size = int(match.group(1))
            start = match.end()
            parts.append(body[start:start + size])
            body = body[start + size:]
        return parts[-1] if parts else None

    def definition_brace(annotation, name):
        if not name:
            return None
        pattern = re.compile(r"\b" + re.escape(name) + r"\s*\(")
        next_annotation = SYMBOL_ANNOTATION.search(text, annotation.end())
        section_end = next_annotation.start() if next_annotation else len(masked)
        for occurrence in pattern.finditer(masked, annotation.end(), section_end):
            opening = masked.find("(", occurrence.start(), occurrence.end())
            depth = 0
            closing = None
            for index in range(opening, len(masked)):
                if masked[index] == "(":
                    depth += 1
                elif masked[index] == ")":
                    depth -= 1
                    if depth == 0:
                        closing = index
                        break
            if closing is None:
                continue
            # A call inside an if condition is not a definition. Searching for
            # any later brace could attach its alias to the caller instead.
            body = re.match(r"\s*(?:const\s*)?\{", masked[closing + 1:section_end])
            if not body:
                continue
            brace = closing + 1 + body.end() - 1
            prior_delimiters = [masked.rfind(";", 0, occurrence.start()),
                                masked.rfind("{", 0, occurrence.start()),
                                masked.rfind("}", 0, occurrence.start())]
            prefix = masked[max(prior_delimiters) + 1:occurrence.start()]
            qualified_oam = re.search(
                r"\bOAM::\s*" + re.escape(name) + r"\s*$", prefix)
            qualified_scope = re.search(r"([A-Za-z_]\w*)::\s*$", prefix)
            # A closed SDK namespace plus a later extern-C block can make
            # aggregate brace counts appear balanced. Explicit class members
            # still require a thiscall bridge; never infer them as free code.
            sdk_scopes = {"G2", "G2S", "G2x", "GX", "GXS", "Sound", "CP15", "Event", "IRQ"}
            if qualified_scope and not qualified_oam and qualified_scope.group(1) not in sdk_scopes:
                return None
            namespace_alias = name.startswith("func_") and not re.search(
                r"[A-Za-z_][A-Za-z0-9_]*::\s*$", prefix)
            for namespace in ("G2", "G2S", "G2x", "GX", "GXS", "Sound", "CP15", "Event", "IRQ"):
                for scope in re.finditer(
                        r"\bnamespace\s+" + namespace + r"\s*\{", masked[:occurrence.start()]):
                    between = masked[scope.end():occurrence.start()]
                    if between.count("{") == between.count("}"):
                        namespace_alias = True
                        break
                if namespace_alias:
                    break
            if not (qualified_oam or namespace_alias):
                continue
            return brace
        return None

    insertions: dict[int, list[str]] = {}
    for annotation in SYMBOL_ANNOTATION.finditer(text):
        symbol = annotation.group(1)
        brace = definition_brace(annotation, function_name(symbol))
        if brace is None:
            continue
        coff_symbol = "_" + symbol
        insertions.setdefault(brace + 1, []).append(
            '#pragma comment(linker, "/alternatename:' + coff_symbol +
            '=" __FUNCDNAME__)')
    if not insertions:
        return text, 0
    out = text
    for position in sorted(insertions, reverse=True):
        directives = "\n" + "\n".join(insertions[position]) + "\n"
        out = out[:position] + directives + out[position:]
    return out, sum(len(items) for items in insertions.values())


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--decomp", type=pathlib.Path,
                        default=pathlib.Path(__file__).resolve().parents[2])
    parser.add_argument("--json", dest="json_out", type=pathlib.Path,
                        help="write full source/symbol audit JSON")
    parser.add_argument("--cmake", dest="cmake_out", type=pathlib.Path,
                        help="write a CMake source list")
    parser.add_argument("--generated-dir", type=pathlib.Path,
                        help="emit host-transformed sources beneath this directory")
    parser.add_argument("--host-overrides", nargs="*", default=[],
                        help="function symbols implemented by the PC HAL; their ROM TUs are omitted")
    parser.add_argument("--link-log", type=pathlib.Path,
                        help="use unresolved function symbols to discover omitted C++ method TUs")
    parser.add_argument("roots", nargs="*", default=list(DEFAULT_ROOTS))
    args = parser.parse_args()
    if args.cmake_out is not None and args.generated_dir is None:
        parser.error("--cmake requires --generated-dir so MMIO can be rewritten")
    root = args.decomp.resolve()
    sys.path.insert(0, str(root / "tools"))
    import srcpath  # pylint: disable=import-outside-toplevel
    srcpath.set_root(root)

    function_names: set[str] = set()
    for symbol_file in (root / "config").rglob("symbols.txt"):
        for line in symbol_file.read_text(encoding="utf-8", errors="replace").splitlines():
            match = SYMBOL_ROW.match(line)
            if match:
                function_names.add(match.group(1))

    # Index free Itanium functions by their unqualified function name. MSVC
    # link errors expose these as `?Name@@...`, which cannot be passed to
    # srcpath directly. This lets the iterative closure pull the recovered
    # implementation TU instead of silently omitting global helpers such as
    # the level object-overlay dispatcher.
    free_function_sources: dict[str, set[pathlib.Path]] = {}
    for symbol in function_names:
        if not symbol.startswith("_Z") or symbol.startswith("_ZN"):
            continue
        name_match = re.match(r"^_Z(\d+)(.*)$", symbol)
        if not name_match:
            continue
        name_length = int(name_match.group(1))
        name = name_match.group(2)[:name_length]
        if len(name) != name_length:
            continue
        source = srcpath.path_for(symbol)
        if source is not None:
            free_function_sources.setdefault(name, set()).add(source)

    root_paths = {name: srcpath.path_for(name) for name in args.roots}
    missing_roots = [name for name, path in root_paths.items() if path is None]
    if missing_roots:
        sys.exit("root symbol has no source: " + ", ".join(missing_roots))

    override_paths = {srcpath.path_for(name) for name in args.host_overrides}
    missing_overrides = [name for name in args.host_overrides
                         if srcpath.path_for(name) is None]
    if missing_overrides:
        sys.exit("host override symbol has no source: " + ", ".join(missing_overrides))
    override_paths.discard(None)

    link_seed_paths: set[pathlib.Path] = set()
    for name in ('daKrb_c_classInit_KURIBO', 'daKrb_c_classInit_KURIBO_S', 'daKrb_c_classInit_KURIBO_L', 'daYegg_c_classInit', 'daObjMarioCap_c_classInit'):
        source = srcpath.path_for(name)
        if source is None: sys.exit('Goomba factory has no source: ' + name)
        link_seed_paths.add(source)
    # Camera startup descriptors contain indirect workers, invisible to the
    # normal source-reference walk. Resolve them from the local ROM metadata.
    sys.path.insert(0, str(root / "port/tools"))
    import camera_host
    import minigame_data
    import link_data
    # Message inline control codes reach these workers only through ROM data.
    for name in link_data.message_text_callbacks(root):
        source = srcpath.path_for(name)
        if source is None: sys.exit('message formatter has no source: ' + name)
        link_seed_paths.add(source)
    for name in minigame_data.function_targets(root):
        source = srcpath.path_for(name)
        if source is None: sys.exit('minigame resource callback has no source: ' + name)
        link_seed_paths.add(source)
    for slots in camera_host.bindings(root).values():
        for name in slots.values():
            source = srcpath.path_for(name)
            if source is None:
                sys.exit("camera callback has no source: " + name)
            link_seed_paths.add(source)
    # This shared fader TU contains the real wipe destructor and fade virtual.
    # Seed it explicitly: the ROM roots reach only the class's host constructor
    # bridge, so those virtual methods are otherwise invisible to source scans.
    fader_source = root / "src/engine/fader/dFader_c.cpp"
    if fader_source.is_file():
        link_seed_paths.add(fader_source.resolve())
    if args.link_log is not None and args.link_log.is_file():
        source_index = sorted((root / "src").rglob("*.cpp"))
        method_index = {}
        for source in source_index:
            content = source.read_text(encoding="utf-8", errors="replace")
            for definition in QUALIFIED_DEFINITION.finditer(content):
                method_index.setdefault(definition.group(1), set()).add(source)

        def method_sources(linker_name: str):
            # MSVC method names are not listed in the decomp's Itanium symbol
            # closure. Recover their source TUs from the unresolved name.
            method = None
            owner = None
            if linker_name.startswith("??0") or linker_name.startswith("??1"):
                ctor = linker_name.startswith("??0")
                body = linker_name[3:].split("@@", 1)[0]
                pieces = [piece for piece in body.split("@") if piece]
                if pieces:
                    owner = "::".join(reversed(pieces))
                    method = pieces[0] if ctor else "~" + pieces[0]
            else:
                match = re.match(r"\?([^@]+)@(.+?)@@", linker_name)
                if match:
                    method, owner_raw = match.groups()
                    owner = "::".join(reversed([x for x in owner_raw.split("@") if x]))
            if not method or not owner:
                return []
            needle = owner + "::" + method
            # Calls to a missing member are common throughout actor code.
            # Seed only TUs indexed with a line-start qualified definition.
            matches = method_index.get(needle, ())
            if not matches and "::" in owner:
                # Namespaces are omitted from member definitions written
                # inside a namespace block, e.g. `namespace Particle {
                # SysTracker::~SysTracker()`. The exact MSVC owner is still
                # known from the unresolved symbol; use the leaf definition
                # only when its source also declares the matching namespace.
                leaf = owner.rsplit("::", 1)[-1] + "::" + method
                namespace = owner.rsplit("::", 1)[0]
                candidates = method_index.get(leaf, ())
                namespace_pattern = re.compile(
                    r"\bnamespace\s+" + re.escape(namespace) + r"\s*\{")
                matches = [source for source in candidates
                           if namespace_pattern.search(source.read_text(
                               encoding="utf-8", errors="replace"))]
            return sorted(matches)

        seen_linker_names = set()
        for line in args.link_log.read_text(encoding="utf-8", errors="replace").splitlines():
            if "unresolved external symbol" not in line:
                continue
            match = re.search(r"unresolved external symbol (.+?)(?: referenced in function|$)", line)
            if not match:
                continue
            raw = match.group(1).strip()
            decorated = re.search(r"\((\?[^()]*)\)$", raw)
            linker_name = decorated.group(1) if decorated else raw
            if linker_name in seen_linker_names:
                continue
            seen_linker_names.add(linker_name)
            # These are data symbols, so match only at the linker-name start
            # (or after its C++/COFF prefix). An unanchored `gFoo` alternative
            # accidentally filtered class methods containing `g` + uppercase,
            # e.g. dBgCh constructors and destructors.
            if re.search(r"(?:^|[?_])(?:data_(?:ov\d+_)?[0-9a-fA-F]{8}|g[A-Z][A-Za-z0-9_]*)", linker_name):
                continue
            if linker_name.startswith("__ZN"):
                # The first underscore is COFF's cdecl decoration; the second
                # begins the Itanium symbol spelling used by srcpath.
                decomp_name = linker_name[1:]
                candidate = srcpath.path_for(decomp_name)
                if candidate is not None:
                    link_seed_paths.add(candidate)
            elif linker_name.startswith("__Z"):
                # COFF adds one leading underscore to the decomp's raw `_Z...`
                # free-function spelling, just as it does for `_ZN...`.
                candidate = srcpath.path_for(linker_name[1:])
                if candidate is not None:
                    link_seed_paths.add(candidate)
            elif linker_name.startswith("_") and not linker_name.startswith("__"):
                candidate = srcpath.path_for(linker_name[1:])
                if candidate is not None:
                    link_seed_paths.add(candidate)
            elif linker_name.startswith("?"):
                if linker_name.startswith("?_Z"):
                    # MSVC decorates the recovered `_Z...` free-function
                    # identifier with a leading question mark. Preserve the
                    # exact call spelling via a generated alternate name, but
                    # use the undecorated `_Z...` source filename as its seed.
                    candidate = srcpath.path_for(linker_name[1:])
                    if candidate is not None:
                        link_seed_paths.add(candidate)
                    continue

                free_match = re.match(r"^\?([^@]+)@@", linker_name)
                if free_match:
                    link_seed_paths.update(
                        free_function_sources.get(free_match.group(1), ()))
                    if free_match.group(1) in free_function_sources:
                        continue

                link_seed_paths.update(method_sources(linker_name))


    pending = list(root_paths.values()) + list(link_seed_paths)
    visited: set[pathlib.Path] = set()
    unresolved: set[str] = set()
    while pending:
        path = pending.pop()
        if path in visited:
            continue
        if path in override_paths:
            continue
        visited.add(path)
        identifiers = set(IDENTIFIER.findall(source_text(path)))
        for name in identifiers & function_names:
            dependency = srcpath.path_for(name)
            if dependency is None:
                unresolved.add(name)
            elif dependency not in visited:
                pending.append(dependency)

    import dsmt_host
    dsmt_veneers=dsmt_host.tail_veneers(root)
    excluded_host_tus = set()

    source_files = []
    asm_files = []
    generated_files = []
    for path in sorted(visited, key=lambda item: item.as_posix().lower()):
        rel = path.relative_to(root).as_posix()
        if rel in excluded_host_tus:
            continue
        if ASM.search(path.read_text(encoding="utf-8", errors="replace")):
            asm_files.append(rel)
        else:
            text = path.read_text(encoding="utf-8", errors="replace")
            cpp_marker = text.startswith("//cpp")
            markerless = re.sub(r"\A//cpp[^\n]*\n", "", text)
            if args.generated_dir is not None:
                sys.path.insert(0, str(root / "port/tools"))
                import hostgen  # pylint: disable=import-outside-toplevel
                transformed, rewrites = hostgen.transform(markerless)
                if path.name in ('daRNk_c.cpp', 'daRFlag_c.cpp'):
                    import koopa_host
                    transformed=koopa_host.transform(transformed,path.name)
                    rewrites+=1
                if path.name in ('_ZN8Particle7Manager9AddSystemEiR7Vector3.cpp',
                                 '_ZN8Particle10SysTracker6UpdateEv.cpp','func_0204a5c8.c',
                                 '_ZN10dScStage_c13InitResourcesEv.cpp','_ZN10dScStage_c14GraphCallback1Ev.cpp'):
                    import particle_host
                    transformed=particle_host.transform(transformed,path.name)
                    cpp_marker=True
                    rewrites+=1
                if path.name in ('daObjSeesaw_c.cpp','daObjRotateUpdownLift_c.cpp',
                                 'd_a_obj_b_swdoor.cpp','daObjSwdoor_c.cpp'):
                    import battlefield_platform_host
                    transformed=battlefield_platform_host.transform(transformed,path.name)
                    rewrites+=1
                if path.name=='d_a_warpkun.cpp':
                    import warp_host
                    transformed=warp_host.transform(transformed)
                    rewrites+=1
                if path.name in ('d_a_obj_c0_switch.cpp','d_a_obj_c0_water.cpp'):
                    import basement_host
                    transformed=basement_host.transform(transformed,path.name=='d_a_obj_c0_water.cpp')
                    rewrites+=1
                if path.name == 'daObjBlockL_c.cpp':
                    import block_host
                    transformed=block_host.transform(transformed)
                    rewrites+=1
                if path.name == 'daObjBlockS_c.cpp':
                    import crate_host
                    transformed=crate_host.transform(transformed)
                    rewrites+=1
                if path.name == 'da1up_c.cpp':
                    import oneup_host
                    transformed=oneup_host.transform(transformed)
                    rewrites+=1
                if path.name == 'daObj1UpLogo_c.cpp':
                    import oneup_host
                    transformed=oneup_host.transform_logo(transformed)
                    rewrites+=1
                if path.name == 'daObjTatefuda_c.cpp':
                    import signpost_host
                    transformed=signpost_host.transform(transformed)
                    rewrites+=1
                if path.name == 'daObjHatenaBlock_c.cpp':
                    import item_block_host
                    transformed=item_block_host.transform(transformed)
                    rewrites+=1
                if path.name == 'dBgActor_c.cpp':
                    import item_block_host
                    transformed=item_block_host.trace_ceiling(transformed)
                    rewrites+=1
                if path.name == 'daObjPowerUpItem_c.cpp':
                    import power_flower_host
                    transformed=power_flower_host.transform(transformed)
                    rewrites+=1
                if path.name == 'daFeather_c.cpp':
                    import feather_host
                    transformed=feather_host.transform(transformed)
                    original='extern SharedFilePtr data_ov002_0210da58;'
                    assert transformed.count(original)==1
                    transformed=transformed.replace(original,'extern "C" SharedFilePtr data_ov002_0210da58;')
                    transformed='#include <cstddef>\n'+transformed+'''
static_assert(sizeof(daFeather_c)==0x388,"native Feather layout");
static_assert(offsetof(daFeather_c,mModel)==0xd4,"native Feather model");
'''
                    rewrites+=1
                if path.name == 'd_a_obj_hatena_switch.cpp':
                    import hatena_switch_host
                    transformed=hatena_switch_host.transform(transformed)
                    rewrites+=1
                if path.name == 'd_a_obj_c1_hikari.cpp':
                    import light_beam_host
                    transformed=light_beam_host.transform(transformed)
                    rewrites+=1
                if path.name == 'd_a_set_se.cpp':
                    import ambient_sound_host
                    transformed=ambient_sound_host.transform(transformed)
                    rewrites+=1
                if path.name == 'daObjHeart_c.cpp':
                    import healing_heart_host
                    transformed=healing_heart_host.transform(transformed)
                    rewrites+=1
                if path.name == 'daObjYajirusi_c.cpp':
                    import arrow_sign_host
                    transformed=arrow_sign_host.transform(root,transformed)
                    rewrites+=1
                if path.name in ('daObjFallBlock_c.cpp', 'd_a_obj_th_fall_block.cpp'):
                    import fall_block_host
                    transformed=fall_block_host.transform(root,transformed,path.name=='d_a_obj_th_fall_block.cpp')
                    rewrites+=1
                if path.name == 'd_a_trs_trap.cpp':
                    import merry_go_round_host
                    transformed=merry_go_round_host.transform(root,transformed)
                    rewrites+=1
                if path.name == 'daBook_c.cpp':
                    import book_host
                    transformed=book_host.transform(root,transformed)
                    rewrites+=1
                if path.name == 'daUdlift_c.cpp':
                    import udlift_host
                    transformed=udlift_host.transform(root,transformed)
                    rewrites+=1
                if path.name in ('daChair_c.cpp','d_a_piano.cpp'):
                    import haunted_furniture_host
                    transformed=haunted_furniture_host.transform(root,transformed,path.name=='d_a_piano.cpp')
                    rewrites+=1
                if path.name == 'daTrs_c.cpp':
                    import boo_host
                    transformed=boo_host.transform(root,transformed)
                    rewrites+=1
                if path.name in ('daObjSwitch_c.cpp', 'd_a_obj_switch_hanswitch.c',
                                 'd_a_obj_switch_star_switch.c', 'func_ov002_020baa2c.c'):
                    import floor_switch_host
                    transformed=floor_switch_host.transform(transformed,path.name)
                    cpp_marker=True
                    rewrites+=1
                if path.name=='d_a_star_gate.cpp':
                    import star_gate_host
                    transformed=star_gate_host.transform(transformed)
                    rewrites+=1
                if path.name=='d_a_kinopio.cpp':
                    import toad_host
                    transformed=toad_host.transform(root,transformed)
                    rewrites+=1
                if path.name in ('d_a_s_bird.cpp','d_a_s_bird_head.cpp'):
                    import bird_host
                    transformed=bird_host.transform(root,transformed,path.name)
                    rewrites+=1
                if path.name in ('Sound.cpp','SoundEngine.cpp','func_ov007_020bd9e4.c',
                                 'func_ov007_020bdbcc.c','func_ov007_020bde2c.c',
                                 'func_ov007_020bde70.c','func_ov007_020bdebc.c','func_ov007_020bdf60.c',
                                 'func_ov007_020be098.c'):
                    import sound_host
                    transformed=sound_host.transform(transformed,path.name)
                    rewrites+=1
                if path.stem in dsmt_veneers and 'ov007' in path.parts:
                    transformed=dsmt_host.transform_veneer(path.stem,dsmt_veneers[path.stem])
                    rewrites+=1
                if path.name in ('dScBoot_c.cpp','dScDSMT_c.cpp','func_ov007_020aed98.cpp','func_ov007_020b7eec.c','func_ov007_020b8548.c'):
                    import dsmt_host
                    transformed=dsmt_host.transform(transformed,path.name)
                    rewrites+=1
                # Ownership follows the BMD model being rendered, rather than
                # shared VRAM slots. Keep provenance in the PC adaptation only.
                texture_hooks = {
                    '_ZN15ModelComponents6RenderEP9Matrix4x3P7Vector3.cpp':
                        ('void ModelComponents::Render(Matrix4x3* mat, Vector3* vec)',
                         'ntr::TextureModelScope texture_model_scope(info);'),
                    '_ZN4Heap10DeallocateEPv.cpp':
                        ('void Heap::Deallocate(void* ptr)', 'ntr::texture_model_forget(ptr);'),
                    '_ZN9SolidHeap14VDeallocateAllEv.cpp':
                        ('void SolidHeap::VDeallocateAll()',
                         'ntr::texture_model_forget_range(allocator->mStart,allocator->mEnd);'),
                    '_ZN13ExpandingHeap14VDeallocateAllEv.cpp':
                        ('void ExpandingHeap::VDeallocateAll()',
                         'ntr::texture_model_forget_range(allocator->mStart,allocator->mEnd);'),
                    'ExpandingHeapAllocator.cpp':
                        ('void HeapAllocator::Destroy()',
                         'ntr::texture_model_forget_range(mStart,mEnd);'),
                }
                import texture_consumer_hooks
                transformed,consumer_hooks=texture_consumer_hooks.transform(transformed,path.name)
                rewrites+=consumer_hooks
                if path.name in ('func_0201f138.cpp', 'func_0201ef38.c', 'func_0201eaac.c'):
                    import intro_host
                    transformed = intro_host.transform(transformed, path.name)
                    rewrites += 1
                if path.name in texture_hooks:
                    signature,statement=texture_hooks[path.name]
                    transformed='#include "ntr/texture_assets.h"\n'+transformed
                    pattern=re.escape(signature)+r'\s*\{'
                    transformed,count=re.subn(pattern,lambda m:m[0]+'\n    '+statement,transformed,count=1)
                    if count!=1:raise ValueError('Texture ownership hook changed: '+signature)
                    rewrites+=1
                if path.name == 'dScStage_c.cpp':
                    import native_save_hooks
                    transformed=native_save_hooks.transform(transformed)
                    rewrites+=2
                    transformed = ('#include "hal/course_clear_input.h"\n'
                        '#include "hal/stage_options_input.h"\n'
                        'extern "C" int pc_pause_settings_update();\n'
                        'extern "C" int pc_pause_settings_render();\n'
                        'extern "C" void pc_pause_settings_reset();\n' + transformed)
                    for signature, statement in (
                        ('void dScStage_c::LC_Update()', 'pc_course_clear::Frame pc_controller_frame;'),
                        ('void dScStage_c::PS_Update()', 'if (pc_pause_settings_update()) return; pc_stage_options::Frame pc_stage_options_frame;'),
                        ('void dScStage_c::PS_Render()', 'if (pc_pause_settings_render()) return;'),
                        ('void dScStage_c::PS_Cleanup()', 'pc_pause_settings_reset();')):
                        pattern = re.escape(signature) + r'\s*\{'
                        transformed, count = re.subn(pattern, lambda m: m[0]+'\n    '+statement,
                                                     transformed,count=1)
                        if count != 1: raise ValueError('Native pause hook changed: '+signature)
                    rewrites += 4
                if path.name == 'IsButtonInputValid.c':
                    old = 'if (val & 0xc)'
                    if transformed.count(old) != 1: raise ValueError('Pause confirm hook changed')
                    transformed = transformed.replace(old,
                        'if ((val & 0xc) || (data_0209f2c4 && (val & 1)))')
                    rewrites += 1
                if path.name == 'daC_Jugem_c.cpp':
                    import lakitu_host
                    transformed = lakitu_host.transform(root, transformed)
                    rewrites += 1
                if path.name in ('daMip_c.cpp','daObj_Mip_Key_c.cpp'):
                    import rabbit_host
                    transformed = rabbit_host.transform(root, transformed, path.name)
                    rewrites += 1
                if path.name in ('d_a_cnn.cpp','daWbm_c.cpp','func_0201ffcc.c'):
                    import cannon_host
                    transformed = cannon_host.transform(root, transformed, path.name)
                    rewrites += 1
                if path.name in ('d_a_red_bombhei.cpp','d_a_obj_cannon_shutter.cpp','dBgW_KcMbg.cpp',
                                 'func_02038324.cpp','func_0203842c.cpp','func_0203859c.cpp','func_02038ea4.cpp'):
                    import buddy_host
                    transformed = buddy_host.transform(root, transformed, path.name)
                    rewrites += 1
                if path.name in ('dActor_c.cpp','d_a_wanwan.cpp','daObjPile_c.cpp','d_a_obj_wanwan_shutter.cpp'):
                    import chomp_host
                    transformed = chomp_host.transform(root, transformed, path.name)
                    rewrites += 1
                if path.name == 'daObjFl_London_c.cpp':
                    import lava_bridge_host
                    transformed = lava_bridge_host.transform(transformed)
                    rewrites += 1
                if path.name == 'daObjFl_Seesaw_c.cpp':
                    import seesaw_host
                    transformed = seesaw_host.transform(transformed)
                    rewrites += 1
                if path.name == '_ZN6Player16St_WallJump_MainEv.cpp':
                    import tree_host
                    transformed = tree_host.transform_walljump(transformed)
                    rewrites += 1
                if path.name == 'Player.cpp':
                    import player_music_host
                    transformed=player_music_host.transform(transformed)
                    import item_block_host
                    transformed=item_block_host.trace_player_collision(transformed)
                    import tree_host
                    transformed = tree_host.transform_player(transformed)
                    import jump_input_host
                    transformed = jump_input_host.transform(transformed)
                    rewrites += 1
                if path.name == 'd_a_tree.cpp':
                    import tree_host
                    transformed = tree_host.transform(root, transformed)
                    rewrites += 1
                if path.name == 'IsCannonOpenInCurLevel.c':
                    transformed = transformed.replace('void IsCannonOpenInCurLevel(void)', 'int IsCannonOpenInCurLevel(void)').replace('(void)IsCannonOpenInLevel(data_0209f2f8);', 'return IsCannonOpenInLevel(data_0209f2f8);')
                    rewrites += 1
                if path.name == 'dCamera_c.cpp':
                    transformed = camera_host.transform(transformed)
                    rewrites += 1
                if path.name == 'dMap_c.cpp':
                    # Display orientation applies to the BG and every icon.
                    # Keep the original internal intro-angle state advancing
                    # independently, so locking the display cannot stall it.
                    render_start = transformed.index('int dMap_c::Render()')
                    behavior_start = transformed.index('s32 dMap_c::Behavior()')
                    behavior_end = transformed.index('/* What InitResources loads')
                    render = transformed[render_start:behavior_start]
                    anchor = 'Player *player = data_0209f394[idx];'
                    if render.count(anchor) != 1:
                        raise ValueError('Minimap icon orientation hook changed')
                    render = render.replace(anchor, anchor +
                        '\n        const u16 pcDisplayAngle = ntr::video_minimap_angle(\n'
                        '            data_0209f318 ? (unsigned short)data_0209f318->f17c : 0);')
                    render = render.replace('this->mAngle', 'pcDisplayAngle')
                    behavior = transformed[behavior_start:behavior_end]
                    anchor = 'player = (Obj *)data_0209f394[data_0209f250];'
                    if behavior.count(anchor) != 1 or behavior.count('L4d8:\n') != 1:
                        raise ValueError('Minimap projection orientation hook changed')
                    behavior = behavior.replace(anchor, anchor +
                        '\n    const u16 pcDisplayAngle = ntr::video_minimap_angle(\n'
                        '        data_0209f318 ? (unsigned short)data_0209f318->f17c : 0);')
                    internal, projection = behavior.split('L4d8:\n')
                    projection = projection.replace('this->mAngle', 'pcDisplayAngle')
                    projection = projection.replace(', mAngle);', ', pcDisplayAngle);')
                    behavior = internal + 'L4d8:\n' + projection
                    transformed = ('#include "ntr/video.h"\n' + transformed[:render_start] +
                                   render + behavior + transformed[behavior_end:])
                    rewrites += 1
                if rel == "src/engine/fader/dFader_c.cpp":
                    guard = "#ifndef _MSC_VER"
                    first = transformed.find(guard)
                    second = transformed.find(guard, first + len(guard))
                    if second >= 0:
                        transformed = (transformed[:second] + "#if 1" +
                                       transformed[second + len(guard):])
                    # Native boot preserves fade progression without writing
                    # Nintendo DS blend registers through absolute MMIO.
                    transformed = re.sub(
                        r"void dFdColor_c::AdvanceFade\(\)\s*\{.*?\n\}",
                        "void dFdColor_c::AdvanceFade() { AdvanceInterp(); }",
                        transformed, count=1, flags=re.S)
                if path.name == "_Z26LoadOrUnloadObjectOverlaysPFviEi.cpp":
                    transformed = re.sub(
                        r"#if !defined\(SM64DS_PLATFORM_PC\)\s*"
                        r"extern \"C\" int overlay_60;\s*"
                        r"extern \"C\" int overlay_98;\s*"
                        r"extern \"C\" int overlay_102;\s*#endif",
                        "", transformed)
                    transformed = transformed.replace(
                        "fn((int)&overlay_60);", "fn(60);")
                    transformed = transformed.replace(
                        "fn((int)&overlay_98);", "fn(98);")
                    transformed = transformed.replace(
                        "fn((int)&overlay_102);", "fn(102);")
                if path.name == "_ZN10dScStage_c9LoadModelEv.cpp":
                    # This TU is emitted as a host-generated copy, so its
                    # source-level SM64DS_PLATFORM_PC branches are not present
                    # in the generated file. Resolve level data through the
                    # host overlay map before reading it, and guard the model
                    # list while diagnosing early native boot failures.
                    transformed = transformed.replace(
                        '#include "dScStage_c.h"',
                        '#include "dScStage_c.h"\n'
                        '#include <stdio.h>\n'
                        '#include <stdlib.h>\n'
                        '// Stage initialization supplies the canonical host record.', 1)
                    transformed = re.sub(
                        r"void dScStage_c::LoadModel\(\)\s*\{.*?\n\}",
                        r'''void dScStage_c::LoadModel() {
    char *self = (char *)this;
    char *source = (char *)data_0209f340;
    if (getenv("SM64DS_PC_TRACE")) {
        fprintf(stderr, "[sm64ds] stage LoadModel: stageData=%p\\n", source);
        fflush(stderr);
    }
    if (!source) return;
    char *level = source;
    unsigned short modelId = *(unsigned short *)(level + 8);
    if (getenv("SM64DS_PC_TRACE")) {
        fprintf(stderr, "[sm64ds] stage LoadModel: source=%p data=%p id=%04x\\n",
                source, level, modelId);
        fflush(stderr);
    }
    _ZN5Model14LoadAndSetFileEtii(self + 0x86c, modelId, 1, -1);
    char *p = self + 0x874;
    char *list = ((char **)p)[0];
    char *node = ((char **)p)[1];
    if (getenv("SM64DS_PC_TRACE")) {
        fprintf(stderr, "[sm64ds] stage LoadModel: model=%p list=%p node=%p\\n",
                *(void **)(self + 0x870), list, node);
        fflush(stderr);
    }
    if (!list) return;
    unsigned int count = *(unsigned int *)(list + 0x24);
    for (unsigned int i = 0; i < count; ++i, node += 0x30) {
        unsigned int v = *(unsigned int *)(node + 0x24);
        if ((v & 0x1f0000) != 0x1f0000)
            *(unsigned int *)(node + 0x24) = (v & ~0x3f000000) | 0x13000000;
    }
    data_0209f320 = (int)p;
}''',
                        transformed, count=1, flags=re.S)
                if path.name in ("func_02058308.c", "func_02058df4.c"):
                    # These are absolute ARM linker boundary labels used only
                    # to size firmware stacks. The PC scheduler uses native
                    # Windows fibers, so preserve their literal address values
                    # instead of trying to link functions at guest addresses.
                    transformed = transformed.replace(
                        "(s32)func_00000000", "0")
                    transformed = transformed.replace(
                        "(s32)func_00000600", "0x600")
                transformed, aliases = add_free_function_aliases(transformed)
                rewrites += aliases
                # A number of recovered callers declare a raw Itanium symbol
                # as an ordinary C++ identifier (for example
                # `_ZN3OAM11GetObjWidthEii`). MSVC decorates that identifier as
                # `?_ZN...@@...`, while the callee TU exports the raw symbol
                # and its correctly typed host implementation. Alias only
                # references found in this TU to that exact raw spelling.
            else:
                transformed, rewrites = markerless, 0
            needs_short_copy = path.suffix == ".cpp" and len(path.name) > 70
            platform_override = path.name in (
                "_Z26LoadOrUnloadObjectOverlaysPFviEi.cpp",
                "_ZN10dScStage_c9LoadModelEv.cpp",
                "func_02058308.c", "func_02058df4.c")
            if args.generated_dir is not None and (rewrites or needs_short_copy or platform_override):
                if needs_short_copy and not rewrites:
                    digest = hashlib.sha1(rel.encode("utf-8")).hexdigest()[:16]
                    output = args.generated_dir / "link-seeds" / (digest + ".cpp")
                else:
                    output = args.generated_dir / pathlib.PurePosixPath(rel).with_suffix(".cpp")
                output.parent.mkdir(parents=True, exist_ok=True)
                linkage = "" if path.suffix == ".cpp" or cpp_marker else 'extern "C" {\n'
                ending = "" if not linkage else "\n}  // extern C generated from a C TU\n"
                generated = ("// Generated from " + rel + "; do not edit.\n"
                             '#include "ntr/mmio.h"\n' + linkage + transformed + ending)
                # Keep unchanged generated sources' timestamps, so one actor
                # repair does not recompile the entire native source closure.
                if not output.exists() or output.read_text(encoding="utf-8") != generated:
                    output.write_text(generated, encoding="utf-8")
                generated_files.append(output.as_posix())
                source_files.append(output.as_posix())
            else:
                source_files.append(rel)

    if args.generated_dir is not None:
        source_files.extend(camera_host.write_sources(root, args.generated_dir / 'camera'))
        import sound_host
        source_files.extend(sound_host.write_sources(root, args.generated_dir / 'sound'))
        import minigame_host
        source_files.extend(minigame_host.write_sources(root, args.generated_dir / 'minigame'))
        source_files.extend(minigame_data.write_sources(root, args.generated_dir / 'minigame'))

    report = {
        "roots": args.roots,
        "translation_units": len(visited),
        "host_compilable_candidates": len(source_files),
        "assembly_translation_units": len(asm_files),
        "source_files": source_files,
        "assembly_files": asm_files,
        "unresolved_function_symbols": sorted(unresolved),
        "host_overrides": sorted(args.host_overrides),
        "link_seed_translation_units": len(link_seed_paths),
        "link_seed_sources": sorted(path.relative_to(root).as_posix()
                                    for path in link_seed_paths),
        "limitations": [
            "Dynamic actor/profile dispatch tables are not followed.",
            "ROM-backed data, relocations, and overlay selection are not modeled.",
            "Source reachability does not prove host compilation or link closure.",
        ],
    }
    if args.json_out:
        args.json_out.parent.mkdir(parents=True, exist_ok=True)
        args.json_out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"roots: {', '.join(args.roots)}")
    print(f"reachable translation units: {len(visited)}")
    print(f"source candidates: {len(source_files)}; assembly: {len(asm_files)}")
    print(f"function symbols without source: {len(unresolved)}")
    if args.json_out:
        print(f"report: {args.json_out}")
    if args.cmake_out:
        args.cmake_out.parent.mkdir(parents=True, exist_ok=True)
        lines = ["# Generated by port/tools/boot_closure.py"]
        lines.append("set(BOOT_CLOSURE_SOURCES")
        lines.extend(f'    "{(pathlib.Path(item) if pathlib.Path(item).is_absolute() else root / pathlib.PurePosixPath(item)).as_posix()}"'
                     for item in source_files)
        lines.append(")")
        lines.append("set(BOOT_CLOSURE_CPP_MARKED")
        lines.extend(
            f'    "{(root / pathlib.PurePosixPath(rel)).as_posix()}"'
            for rel in source_files
            if rel.endswith(".c") and (root / rel).is_file()
            and (root / rel).read_text(encoding="utf-8", errors="replace").startswith("//cpp")
        )
        lines.append(")")
        args.cmake_out.write_text("\n".join(lines) + "\n", encoding="utf-8")
        print(f"CMake source list: {args.cmake_out}")
    if asm_files:
        print("assembly files:")
        for name in asm_files:
            print(f"  {name}")
    if unresolved:
        print("function symbols without source:")
        for name in sorted(unresolved):
            print(f"  {name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())





