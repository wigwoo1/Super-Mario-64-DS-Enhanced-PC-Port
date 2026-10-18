//cpp
// func_020441cc at 0x020441cc
// Matched byte-for-byte with mwccarm 1.2/sp2p3 (arm9 main).
#if defined(SM64DS_PLATFORM_PC)
#include <cstdio>
#include <cstdlib>
#endif
class Obj { public: int dummy; };
typedef void (Obj::*PMF)();

struct Node {
    char pad[0x10];
    Obj *obj;     // 0x10
};

struct Thing {
    Node *head;   // 0
    PMF callback; // 4
};

extern "C" void *func_0203b394(Node *n);
#if defined(SM64DS_PLATFORM_PC)
extern "C" int func_02043880(void *);
#endif

extern "C" void *func_020441cc(Thing *thiz)
{
    Node *node;
    if (thiz->callback == 0) return (void *)1;
    node = thiz->head;
    while (node) {
        Node *next = (Node *)func_0203b394(node);
#if defined(SM64DS_PLATFORM_PC)
        const int *record = reinterpret_cast<const int *>(
            reinterpret_cast<const char *>(thiz) + 4);
        const char *trace = std::getenv("SM64DS_PC_TRACE");
        if (record[0] != 0x02043880 || record[1] != 0) {
            std::fprintf(stderr, "[sm64ds] invalid scene callback record (%08x,%d)\n",
                         static_cast<unsigned>(record[0]), record[1]);
            std::abort();
        }
        if (trace && trace[0] && trace[0] != '0') {
            std::fprintf(stderr, "[sm64ds] scene graph callback: begin object=%p\n", node->obj);
            std::fflush(stderr);
        }
        func_02043880(node->obj);
        if (trace && trace[0] && trace[0] != '0') {
            std::fprintf(stderr, "[sm64ds] scene graph callback: end\n");
            std::fflush(stderr);
        }
#else
        (node->obj->*thiz->callback)();
#endif
        node = next;
    }
    return (void *)1;
}
