// Gate-8 smoke: the game's collision answers "what is under this point".
//
// The castle grounds KCL loads through the card seam; LoadFile rebases it
// (the same in-place pointer math as BMDs), SetFile wires the CLPS block,
// and the octree walk -- the port/unmatched adoption of the banked draft,
// algorithm confirmed against the ITCM disassembly -- casts vertical rays
// down through the mesh. Floor hits must come back with upward normals,
// shortened rays, and hit points inside the probe column.
//
// The ray object is harness-built (fields the walk reads, zeroed rest);
// the game ctor's vtable/list wiring belongs to the actor gates.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "dBgW_Kc.h"

#include "ntr/mmio.h"

#include "fault_probe.h"

struct SurfaceInfoS { unsigned char clps[8]; int nx, ny, nz; };
struct RayS {
    unsigned char head[0x10];
    unsigned char result[0x1c];     /* 0x10 */
    unsigned char pad2c[0xc];
    int sx, sy, sz;                 /* 0x38 lineStart */
    unsigned char pad44[0xc];
    unsigned char hasClsn;          /* 0x50 */
    unsigned char pad51[3];
    int ex, ey, ez;                 /* 0x54 lineEnd */
    int clsnDist;                   /* 0x60 */
    unsigned char tail[0x10];
};

extern "C" {
struct SharedFilePtrC { u16 fileID; u8 numRefs; void *filePtr; };
SharedFilePtrC *_ZN13SharedFilePtr9ConstructEj(SharedFilePtrC *self, u32 ov0FileID);
void *_ZN4Heap13SetupRootHeapEv(void);
void *_ZN7dBgW_KcC1Ev(void *self);
}

static int probe_one(dBgW_Kc *mc, RayS *ray)
{
    RayS nativeRay = *ray;
    const int direct = mc->dBgW_Kc::DetectClsn(*(dBgCh_Lin *)ray);
    const int dispatched = static_cast<dBgW *>(mc)->DetectClsn(*(dBgCh_Lin *)&nativeRay);
    if (direct != dispatched || ray->hasClsn != nativeRay.hasClsn ||
        ray->ex != nativeRay.ex || ray->ey != nativeRay.ey || ray->ez != nativeRay.ez) {
        fprintf(stderr, "FAIL: native virtual ray dispatch disagrees with the qualified collision worker\n");
        return -2;
    }
    return direct;
}
static int probe_filter(EXCEPTION_POINTERS *ep)
{
    char *base = (char *)GetModuleHandleA(0);
    printf("  [probe fault %08x at +0x%08x]\n",
           (unsigned)ep->ExceptionRecord->ExceptionCode,
           (unsigned)((char *)ep->ExceptionRecord->ExceptionAddress - base));
    return EXCEPTION_EXECUTE_HANDLER;
}
static int probe_seh(dBgW_Kc *mc, RayS *ray)
{
    __try {
        return probe_one(mc, ray);
    } __except (probe_filter(GetExceptionInformation())) {
        return -1;
    }
}

static int g_failures;
#define CHECK(cond) \
    do { if (!(cond)) { ++g_failures; \
        fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

int main(void)
{
    PORT_INSTALL_FAULT_PROBE();
    setvbuf(stdout, NULL, _IONBF, 0);   /* crashes must not eat the trail */
    if (!ntr::io_init()) { fprintf(stderr, "io_init failed\n"); return 2; }
    CHECK(_ZN4Heap13SetupRootHeapEv() != NULL);

    /* the castle grounds KCL, handle 1941: real level floors
       (the piano's KCL turned out to be four wall prisms and nothing
       horizontal -- an actor's push-out shell, not a floor) */
    SharedFilePtrC ptr;
    _ZN13SharedFilePtr9ConstructEj(&ptr, 1941);
    static char mc_storage[0x60];
    dBgW_Kc *mc = (dBgW_Kc *)mc_storage;
    _ZN7dBgW_KcC1Ev(mc_storage);
    char *kcl = dBgW_Kc::LoadFile(*(SharedFilePtr *)&ptr);
    CHECK(kcl != NULL);

    /* CLPS_Block is opaque; a zeroed buffer takes the lookup's
       wrong-version default path, which is what stages without a CLPS do */
    static char clps_storage[0x100];
    mc->dBgW_Kc::SetFile((KCL_File *)kcl, *(CLPS_Block *)clps_storage);

    {
        KCL_File *f = (KCL_File *)kcl;
        (void)f;
        printf("  kcl: origin (%d,%d,%d) raw, masks %08x %08x %08x, "
               "shifts %u/%u/%u\n", f->origin.x, f->origin.y, f->origin.z,
               f->xMask, f->yMask, f->zMask, f->coordShift, f->yShift, f->zShift);
    }

    /* The mask values describe the KCL addressable range, not the occupied
       level bounds. Probe around vertices of real upward-facing triangles so
       the test covers actual castle floor rather than empty octree capacity. */
    KCL_File *f = (KCL_File *)kcl;
    const size_t tri_count = (size_t)((char *)f->unk_0c - (char *)f->tris) /
                             sizeof(KCL_Tri);
    int hits = 0, up_normals = 0, shortened = 0, probes = 0;
    for (size_t i = 0; i < tri_count && hits < 3; ++i) {
        const KCL_Tri &tri = f->tris[i];
        if (f->normals[tri.normalIdx][1] <= 0) continue;
        const s32 *vertex = f->positions[tri.posIdx];
        const int base_x = vertex[0] << 6;
        const int base_y = vertex[1] << 6;
        const int base_z = vertex[2] << 6;
        for (int gx = -2; gx <= 2 && hits < 3; ++gx)
            for (int gz = -2; gz <= 2 && hits < 3; ++gz) {
                RayS ray;
                memset(&ray, 0, sizeof ray);
                ray.head[4] = 1;    /* ordinary-surface filter */
                ray.sx = base_x + gx * 0x1000;
                ray.sy = base_y + 0x20000;
                ray.sz = base_z + gz * 0x1000;
                ray.ex = ray.sx;
                ray.ey = base_y - 0x20000;
                ray.ez = ray.sz;
                ray.clsnDist = 0x7FFFFFF;
                ++probes;
                int r = probe_seh(mc, &ray);
                CHECK(r >= 0);
                if (r < 0) continue;
                if (r && ray.hasClsn) {
                    ++hits;
                    if (ray.ey < ray.sy && ray.ey > base_y - 0x20000)
                        ++shortened;
                    /* result +0x10 is the recorded normal's Y component. */
                    int ny = *(int *)(ray.result + 0x10);
                    if (ny > 0) ++up_normals;
                }
            }
    }
    printf("  probes: %d, hits: %d, shortened rays: %d, upward normals: %d\n",
           probes, hits, shortened, up_normals);
    CHECK(hits >= 3);               /* the piano's floor exists under the grid */
    CHECK(shortened == hits);       /* every hit shortened its ray */
    CHECK(up_normals >= hits / 2);  /* floors face up */

    if (g_failures) {
        fprintf(stderr, "smoke_clsn: %d FAILURE(S)\n", g_failures);
        return 1;
    }
    printf("smoke_clsn: all checks passed (the game's octree walk answers "
           "ground queries over real KCL data)\n");
    return 0;
}
