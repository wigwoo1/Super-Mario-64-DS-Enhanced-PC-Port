//cpp
#include "dScStage_c.h"
#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#endif
extern "C" {
extern int data_0209f340;
extern int data_0209f320;
void _ZN5Model14LoadAndSetFileEtii(char* model, unsigned short id, int b, int c);
}
void dScStage_c::LoadModel() {
    char *self = (char *)this;
    char *sourceLevelData = (char *)data_0209f340;
#if defined(SM64DS_PLATFORM_PC)
    if (getenv("SM64DS_PC_TRACE")) {
        fprintf(stderr, "[sm64ds] stage LoadModel: stageData=%p\n", sourceLevelData);
        fflush(stderr);
    }
    if (!sourceLevelData) return;
#endif
    char *levelData = sourceLevelData;
    unsigned short modelId = *(unsigned short *)(levelData + 8);
#if defined(SM64DS_PLATFORM_PC)
    if (getenv("SM64DS_PC_TRACE")) {
        fprintf(stderr, "[sm64ds] stage LoadModel: source=%p data=%p id=%04x\n",
                sourceLevelData, levelData, modelId);
        fflush(stderr);
    }
#endif
    _ZN5Model14LoadAndSetFileEtii(self + 0x86c, modelId, 1, -1);
    char* list;
    unsigned int count;
    char* node;
    unsigned int i;
    char* p;

    p = (char*)(self + 0x874);
    list = ((char**)p)[0];
    node = ((char**)p)[1];
#if defined(SM64DS_PLATFORM_PC)
    if (getenv("SM64DS_PC_TRACE")) {
        fprintf(stderr, "[sm64ds] stage LoadModel: model=%p list=%p node=%p\n",
                *(void **)(self + 0x870), list, node);
        fflush(stderr);
    }
    if (!list) return;
#endif
    count = *(unsigned int*)(list + 0x24);
    i = 0;
    while (i < count) {
        unsigned int v = *(unsigned int*)(node + 0x24);
        i++;
        if ((v & 0x1f0000) != 0x1f0000) {
            *(unsigned int*)(node + 0x24) = (v & ~0x3f000000) | 0x13000000;
        }
        node += 0x30;
    }
    data_0209f320 = (int)p;
}
