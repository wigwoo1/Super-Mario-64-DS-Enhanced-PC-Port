//cpp
// func_02043fdc at 0x02043fdc
#if defined(SM64DS_PLATFORM_PC)
#include <cstdio>
#include <cstdlib>
#endif
class Obj { public: int dummy; };
typedef void (Obj::*PMF)();

struct Node {
    int pad;        // 0x0
    Node *next;     // 0x4
    Obj *obj;       // 0x8
};

struct Thing {
    Node *head;     // 0x0
    int pad;        // 0x4
    PMF callback;   // 0x8
};

extern "C" Node *data_020a4b68;
#if defined(SM64DS_PLATFORM_PC)
extern "C" int func_0204322c(void *);
extern "C" int func_02043288(void *);
extern "C" int func_020432e4(void *);
extern "C" int func_0204335c(void *);
extern "C" int func_02043880(void *);
#endif

extern "C" void *func_02043fdc(Thing *thiz)
{
    Node *node;
#if defined(SM64DS_PLATFORM_PC)
    static unsigned trace_count;
    const char *trace = std::getenv("SM64DS_PC_TRACE");
    if (trace && trace[0] && trace[0] != '0' && trace_count < 32) {
        const int *callback_words = reinterpret_cast<const int *>(
            reinterpret_cast<const char *>(thiz) + 8);
        std::fprintf(stderr,
            "[sm64ds] actor scheduler: pass=%u head=%p callback=(%d,%d)\n",
            trace_count, static_cast<void *>(thiz->head),
            callback_words[0], callback_words[1]);
        std::fflush(stderr);
        ++trace_count;
    }
#endif
    if (thiz->callback == 0) return (void *)1;
    node = thiz->head;
    while (node != 0) {
        Node *next;
        data_020a4b68 = node;
        next = node->next;
#if defined(SM64DS_PLATFORM_PC)
        /* This manager stores an ARM direct member pointer in its first word.
           Route known ROM callback addresses to their linked host functions. */
        const int *record = reinterpret_cast<const int *>(
            reinterpret_cast<const char *>(thiz) + 8);
        void *object = node->obj;
        if (record[1] != 0) {
            std::fprintf(stderr, "[sm64ds] invalid actor callback tag: %d\n", record[1]);
            std::abort();
        }
        if (trace && trace[0] && trace[0] != '0') {
            std::fprintf(stderr, "[sm64ds] actor callback: begin addr=%08x object=%p node=%p next=%p state=%u flags=%u\n",
                         static_cast<unsigned>(record[0]), object, static_cast<void *>(node),
                         static_cast<void *>(next),
                         object ? static_cast<unsigned char *>(object)[0x0e] : 0,
                         object ? static_cast<unsigned char *>(object)[0x13] : 0);
            std::fflush(stderr);
        }
        switch (static_cast<unsigned>(record[0])) {
        case 0x0204322c: func_0204322c(object); break;
        case 0x02043288: func_02043288(object); break;
        case 0x020432e4: func_020432e4(object); break;
        case 0x0204335c: func_0204335c(object); break;
        case 0x02043880: func_02043880(object); break;
        default: std::abort();
        }
        if (trace && trace[0] && trace[0] != '0') {
            std::fprintf(stderr, "[sm64ds] actor callback: end addr=%08x node=%p\n",
                         static_cast<unsigned>(record[0]), static_cast<void *>(node));
            std::fflush(stderr);
        }
#else
        (node->obj->*thiz->callback)();
#endif
        node = next;
    }
    data_020a4b68 = node;
    return (void *)1;
}
