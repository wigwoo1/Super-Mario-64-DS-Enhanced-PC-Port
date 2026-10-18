// Gate-9 smoke: a real actor lives its lifecycle on host.
//
// ArrowSignRight_Spawn allocates from the game heap and runs the ctor
// chain (dBgActor_c -> dActor_c -> fBase_c, Model and dBgW_KcMbg
// subobjects, dExtShadowModel_c); then every lifecycle step dispatches THROUGH
// THE VTABLE exactly as the game's processing lists do: InitResources
// loads the sign's model and collider through the full asset pipeline,
// Behavior ticks, Render walks the model into the GX. This is the vtable
// law's first full-class workout -- the rung before Player.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ntr/gx.h"
#include "ntr/mmio.h"
#include "ntr/ppu.h"

#include "fault_probe.h"
#include "Heap.h"

typedef unsigned int u32;

extern "C" {
void func_0203c178(int *m, int sx, int sy, int sz);
int *daObjYajirusi_c_classInit_YAJIRUSI_R(void);
void *_ZN4Heap13SetupRootHeapEv(void);
void _ZN6Memory10DeallocateEPvP4Heap(void *, void *);
extern int data_0209b3ec[12];       /* camera matrix */
/* the spawn context dBase_c::Spawn would have staged */
struct SharedFilePtrC { unsigned short fileID; unsigned char numRefs;
                        unsigned char pad; void *filePtr; };
SharedFilePtrC *_ZN13SharedFilePtr9ConstructEj(SharedFilePtrC *s, u32 id);
void *_ZN5Model8LoadFileER13SharedFilePtr(SharedFilePtrC *s);
extern unsigned short data_020a4b54;    /* pending actor ID */
extern void **data_020a4bb8;            /* actorID -> SpawnInfo* */
extern void *data_ov098_0213c380[6];    /* {model,kcl,?} entry table */
extern char data_ov098_0213c384[0x18];  /* column-b view of the same */
extern void *data_020a0eac_c;           /* actor heap = root heap */
extern void *data_020a0ea0;             /* defaultHeapPtr (gate 3a) */
extern void *data_0209f394[];           /* the player array */
extern unsigned char data_0209f21c;     /* player count */
void hal_fill_model_vtable(void);
void hal_fill_shadow_vtable(void);
void hal_fill_mmc_vtable(void);
extern void *_ZTV14ArrowSignRight[20];
}

/* the fBase_c virtual surface, MSVC view: dispatch helpers */
typedef int (__thiscall *Fn0)(void *);
static int vcall0(void *actor, int slot)
{
    void **vt = *(void ***)actor;
    return ((Fn0)vt[slot])(actor);
}

static int g_failures;
#define CHECK(cond) \
    do { if (!(cond)) { ++g_failures; \
        fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

static void ident_fx(void *m)
{
    memset(m, 0, 48);
    ((int *)m)[0] = ((int *)m)[4] = ((int *)m)[8] = 0x1000;
}

static void reset_scene()
{
    ntr::gx_reset();
    NTR_MMIO(uint32_t, 0x04000440) = 0;
    NTR_MMIO(uint32_t, 0x04000454) = 0;
    NTR_MMIO(uint32_t, 0x04000440) = 1;
    NTR_MMIO(uint32_t, 0x04000454) = 0;
    NTR_MMIO(uint32_t, 0x04000580) = 0u | (0u << 8) | (255u << 16) | (191u << 24);
    ntr::gx_set_light(0, -0.4f, -0.6f, -0.7f, 0x7FFF);
    ntr::gx_enable_lights(0x1);
}

int main(void)
{
    PORT_INSTALL_FAULT_PROBE();
    setvbuf(stdout, NULL, _IONBF, 0);
    /* Exercise the production ARM-veneer replacement with unequal scales
       and sentinels: the four arguments must survive the host call. */
    int scale_storage[14];
    for (int i = 0; i < 14; ++i) scale_storage[i] = 0x13572468;
    func_0203c178(scale_storage + 1, -4096, 8192, 12288);
    const int expected_scale[12] = {-4096, 0, 0, 0, 8192, 0, 0, 0, 12288, 0, 0, 0};
    CHECK(memcmp(scale_storage + 1, expected_scale, sizeof(expected_scale)) == 0);
    CHECK(scale_storage[0] == 0x13572468 && scale_storage[13] == 0x13572468);
    if (!ntr::io_init()) { fprintf(stderr, "io_init failed\n"); return 2; }
    CHECK(_ZN4Heap13SetupRootHeapEv() != NULL);
    /* Actor deletion must return storage to the supplied child heap, while
       the default stays the root. The former bridge silently discarded it. */
    Heap *root = static_cast<Heap *>(data_020a0ea0);
    Heap *child = Heap::CreateExpandingHeap(4096, root, 4);
    CHECK(child != NULL);
    if (!child) return 1;
    const u32 rootWithChild = root->VMemoryLeft();
    const u32 childBefore = child->VMemoryLeft();
    void *allocation = child->Allocate(128);
    CHECK(allocation != NULL);
    _ZN6Memory10DeallocateEPvP4Heap(allocation, child);
    CHECK(child->VMemoryLeft() == childBefore);
    CHECK(root->VMemoryLeft() == rootWithChild);
    if (g_failures) return 1;
    allocation = root->Allocate(128);
    _ZN6Memory10DeallocateEPvP4Heap(allocation, NULL);
    CHECK(root->VMemoryLeft() == rootWithChild);
    if (g_failures) return 1;
    ident_fx(data_0209b3ec);

    /* stage the spawn context: actor 0x12b = YAJIRUSI_L, its SpawnInfo
       priorities, the actor heap, and the ov098 file entries (handle 1177 =
       yajirusi_l.bmd, 1178 = its kcl) in both column views */
    hal_fill_model_vtable();
    hal_fill_shadow_vtable();
    hal_fill_mmc_vtable();
    data_020a4b54 = 0x12b;
    static unsigned short spawn_info[4] = { 0, 0, 100, 100 };
    data_020a4bb8[0x12b] = spawn_info;
    data_020a0eac_c = data_020a0ea0;
    static SharedFilePtrC sign_model, sign_kcl;
    _ZN13SharedFilePtr9ConstructEj(&sign_model, 1175);
    _ZN13SharedFilePtr9ConstructEj(&sign_kcl, 1176);
    _ZN5Model8LoadFileER13SharedFilePtr(&sign_model);
    data_ov098_0213c380[0] = &sign_model;
    data_ov098_0213c380[1] = &sign_kcl;
    *(void **)(data_ov098_0213c384 + 0) = &sign_kcl;

    /* a stand-in player: the actor world assumes at least one exists
       (ClosestPlayer callers deref without a null check -- on the DS
       Mario always exists). Position at +0x5c, near the sign. */
    static char fake_player[0x800];
    data_0209f394[0] = fake_player;
    *(unsigned char *)&data_0209f21c = 1;

    int *actor = daObjYajirusi_c_classInit_YAJIRUSI_R();
    CHECK(actor != NULL);
    /* The host flattens the actor layout and supplies the ROM-order vtable
       manually; the base constructor's temporary fBase_c table is not the
       derived lifecycle table used by the DS factory. */
    *(void **)actor = _ZTV14ArrowSignRight;
    /* The PC base ctor doesn't consume the same global spawn context as the
       original registry path; stage the factory's actor ID in its pinned ROM
       field before InitResources selects its resource column. */
    *(unsigned short *)((char *)actor + 0x0c) = 0x12b;
    printf("  spawned at %p, vtable %p\n", (void *)actor, *(void **)actor);

    /* the lifecycle, dispatched as the processing lists would */
    int init_ok = vcall0(actor, 0);         /* InitResources */
    printf("  InitResources -> %d\n", init_ok);
    CHECK(init_ok == 1);

    reset_scene();
    for (int f = 0; f < 4; ++f) {
        printf("  tick %d\n", f);
        vcall0(actor, 6);                   /* Behavior */
    }
    printf("  render\n");
    int render_ok = vcall0(actor, 9);       /* Render */
    printf("  Render -> %d\n", render_ok);

    size_t tris = 0;
    ntr::gx_polygons(tris);
    printf("  triangles from the actor's render: %zu\n", tris);
    CHECK(tris > 0);

    if (g_failures) {
        fprintf(stderr, "smoke_actor: %d FAILURE(S)\n", g_failures);
        return 1;
    }
    printf("smoke_actor: all checks passed (an actor spawned, initialized, "
           "behaved and rendered through vtable dispatch)\n");
    return 0;
}
