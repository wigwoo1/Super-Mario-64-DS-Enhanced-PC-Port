#include "G3i.h"
#include "math/Matrix.h"
#include "ntr/mmio.h"
#include "ntr/gx.h"
#include <cmath>
#include <cstdio>

extern "C" void _ZN3G3i13PerspectiveW_E5Fix12IiES1_S1_S1_S1_S1_bP9Matrix4x3(
    int, int, int, int, int, int, int, int *);
namespace cstd { int fdiv_result(); long long ldiv_result(); }
// Production provides these C entry points through HAL namespace bridges.
extern "C" int _ZN4cstd11fdiv_resultEv() { return cstd::fdiv_result(); }
extern "C" long long _ZN4cstd11ldiv_resultEv() { return cstd::ldiv_result(); }

int main() {
    if (!ntr::io_init()) return 2;
    ntr::gx_reset();
    // NormalizeVec3 leaves mode 2 behind during normal game rendering. The
    // production divide helper must select mode 1 again before projection.
    ntr::io_write(0x04000280u, 2, 2);
    int projection[16] = {};
    _ZN3G3i13PerspectiveW_E5Fix12IiES1_S1_S1_S1_S1_bP9Matrix4x3(
        2896, 2896, 4096, 4096, 40960, 4096, 1, projection);
    Vector3 eye{0, 0, 20480}, up{0, 4096, 0}, target{0, 0, 0};
    Matrix4x3 view;
    // The recovered routine's first argument is the camera position; its
    // third argument is the look target, as used by dCamera_c::Render.
    G3i::LookAt_(&eye, &up, &target, true, &view);
    ntr::gx_write_port(0x04000500, 0);
    ntr::gx_write_port(0x0400048c, 0);
    ntr::gx_write_port(0x0400048c, 0);
    ntr::gx_write_port(0x0400048c, 1024u | (1024u << 16));
    ntr::gx_write_port(0x0400048c, 0);
    ntr::gx_write_port(0x0400048c, 2048u | (static_cast<uint32_t>(uint16_t(-1024)) << 16));
    ntr::gx_write_port(0x0400048c, 0);
    ntr::gx_write_port(0x04000504, 0);
    size_t count = 0;
    const auto *triangles = ntr::gx_polygons(count);
    if (count != 1) return 3;
    const auto &vertex = triangles[0].v[0];
    if (projection[11] != -4096 || std::abs(projection[0] - 4096) > 2 ||
        view.t.z != -20480 || std::fabs(vertex.w - 5.0f) > 0.01f ||
        std::fabs(vertex.x - 128.0f) > 0.01f ||
        std::fabs(vertex.y - 96.0f) > 0.01f ||
        vertex.z < 0.88f || vertex.z > 0.90f) {
        std::fprintf(stderr,"FAIL: camera stream p=%d,%d viewZ=%d vertex=%g,%g,%g,w=%g\n",
            projection[0], projection[11], (int)view.t.z,
            vertex.x, vertex.y, vertex.z, vertex.w);
        return 1;
    }
    std::puts("PASS: recovered projection/view writes reach native geometry engine");
}
