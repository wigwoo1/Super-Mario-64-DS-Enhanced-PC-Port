//cpp
// @symbol _ZN5Model14LoadAndSetFileEtii
#include "Model.h"
extern "C" {
void *LoadFile(u16 ov0ID);
int func_02016ff4(Model *self, BMD_File *file, int a, int b);
int func_02017060(BMD_File *file);
}

void Model::LoadAndSetFile(u16 ov0ID, int a, int b)
{
    BMD_File *file = (BMD_File *)::LoadFile(ov0ID);
    modelFile = file;
#if defined(SM64DS_PLATFORM_PC)
    /* The PC bootstrap can reach a Stage before the ROM's overlay vtable
       setup has completed. Preserve func_02016ff4's operation order while
       making the recovered virtual call explicit for this concrete class. */
    if (!file) return;
    UpdateFileOffsets(*file);
    if (this->Model::DoSetFile((char *)file, a, b))
        func_02017060(file);
#else
    func_02016ff4(this, file, a, b);
#endif
}
