"""Native registration for the recovered shared-list tree actor."""
import re
import struct


def transform_walljump(text):
    start = text.index('      int v = row[1];')
    end = text.index('      f(p);', start) + len('      f(p);')
    # This is the same original character jump table as St_Jump_Main.
    # Its existing native resolver checks adjustment and known target IDs.
    text = text[:start] + '      pc_player_jump_callback(this, row);' + text[end:]
    return 'extern "C" void pc_player_jump_callback(struct Player *, const int *);\n' + text


def transform_player(text):
    """Climbing contacts are dCc_c objects; use their native virtual slots."""
    old = 'Ret *r = arg->m2();'
    assert text.count(old) == 1, 'Tree contact position call changed'
    text = text.replace(old, 'Ret *r = reinterpret_cast<Ret *>(&reinterpret_cast<dCc_c *>(arg)->GetPos());')
    old = '(*(GroundPosFn*)(*(char**)gr + 8))(gr)'
    assert text.count(old) == 4, 'Climb position calls changed'
    text = text.replace(old, 'reinterpret_cast<s32 *>(&reinterpret_cast<dCc_c *>(gr)->GetPos())')
    old = '''void* vtable = *(void**)base;
    void* (*func)(void*) = *(void* (**)(void*))((char*)vtable + 8);
    void* res = func(base);'''
    assert text.count(old) == 1, 'Climb entry position call changed'
    text = text.replace(old, 'void* res = &reinterpret_cast<dCc_c *>(base)->GetPos();')
    old = 'ObjHS* r = obj->v2();'
    assert text.count(old) == 1, 'Tree handstand position call changed'
    text = text.replace(old, 'ObjHS* r = reinterpret_cast<ObjHS *>(&reinterpret_cast<dCc_c *>(obj)->GetPos());')
    return '#include "dCc_c.h"\n' + text


def transform(root, text):
    config = (root / 'config/arm9/overlays/ov002/delinks.txt').read_text()
    base = int(re.search(r'\.text\s+start:0x([\da-f]+)', config)[1], 16)
    raw = (root / 'extracted/overlays/overlay_0002.bin').read_bytes()
    profile = struct.unpack_from('<IHHIiiii', raw, 0x0210abdc - base)
    symbols = (root / 'config/arm9/overlays/ov002/symbols.txt').read_text()
    factory = int(re.search(r'^daTree_c_classInit .*addr:0x([\da-f]+)', symbols, re.M)[1], 16)
    assert profile[0] == factory, 'Tree profile factory changed'
    files = struct.unpack_from('<5H', raw, 0x0210abb8 - base)
    # The fifth ID is an archive member (ar1.narc), not a missing catalog ID.
    assert files[4] & 0x8000, 'Tree archive resource changed'
    # Construct the real native cylinder vtable on its already allocated node.
    text = text.replace('_ZN8dCcPos_cC1Ev(&node->clsn);', 'new (&node->clsn) dCcPos_c();')
    text = text.replace('    *slot = node;', '''    *slot = node;
    if (getenv("SM64DS_PC_TREE_TRACE")) fprintf(stderr,"[tree] add variant=%d pos=%d,%d,%d first=%d\\n",variant,mPosX,mPosY,mPosZ,node->next==0);''')
    text = text.replace('                TreeNode *node = *slot;', '''                TreeNode *node = *slot;
                if (getenv("SM64DS_PC_TREE_TRACE")) fprintf(stderr,"[tree] cleanup variant=%d\\n",i);''')
    text = text.replace('dist < kClipFar', 'dist < pc_render_distance::limit(ntr::video_get_settings().render_distance,kClipFar,false)')
    call = '''_ZN8dClipper13Func_02015560ER9Matrix4x3R7Vector35Fix12IiES3_(
                    data_0209f43c, &data_0209b3ec, &node->pos, kCanopyLift,
                    &out)'''
    assert text.count(call) == 1, 'Tree clipping call changed'
    text = text.replace(call, 'pc_tree_clip(&node->pos, &out)')
    helper = '''
static int pc_tree_clip(Vector3 *position, Vector3 *out) {
    auto *clipper=reinterpret_cast<dClipper *>(data_0209f43c);
    const int oldFar=clipper->mFarZ;
    if(ntr::video_get_settings().render_distance!=ntr::RenderDistance::Minimum)
        clipper->mFarZ=pc_render_distance::kMaximumDepth;
    const int depth=_ZN8dClipper13Func_02015560ER9Matrix4x3R7Vector35Fix12IiES3_(
        clipper,&data_0209b3ec,position,kCanopyLift,out);
    clipper->mFarZ=oldFar;
    return depth;
}
'''
    text = text.replace('namespace Memory {', helper + '\nnamespace Memory {', 1)
    return '#include <stddef.h>\n#include <new>\n#include <stdio.h>\n#include <stdlib.h>\n#include "dClipper.h"\n#include "hal/render_distance_policy.h"\n' + text + '''
extern "C" {
TreeNode *data_ov002_02110a48[5] = {};
u16 data_ov002_0210abb8[5] = {
''' + ','.join(str(v) for v in files) + '''};
}
static_assert(sizeof(daTree_c)==0x264 && offsetof(daTree_c,mModel)==0xd4,"native tree model layout");
static_assert(sizeof(TreeNode)==0x4c && offsetof(TreeNode,next)==0x48,"native tree node layout");
struct PcTreeProfile { void *factory; unsigned short behaviorPriority, renderPriority; unsigned flags; int clipOffsetY, clipRadius, clipDistance, farDistance; };
extern "C" PcTreeProfile g_profile_TREE = {(void *)&daTree_c_classInit,
''' + ','.join(str(v) for v in profile[1:]) + '};\n'
