#include "types.h"
#if defined(SM64DS_PLATFORM_PC)
extern u32 pc_fs_load_compressed_file_at(u32 fileID, void *target);
#else
extern void *func_0201817c(u16 fileID);
extern u32 func_02018ac4(void *buffer);
extern void DecompressLZ16(void *source, void *dest);
extern void Deallocate(void *ptr);
#endif

u32 LoadCompressedFileAt(u16 fileID, void *target)
{
#if defined(SM64DS_PLATFORM_PC)
    return pc_fs_load_compressed_file_at(fileID, target);
#else
    void *src;
    u32 size;

    src = func_0201817c(fileID);
    if (!src)
        return 0;

    size = func_02018ac4(src);
    DecompressLZ16(src, target);
    Deallocate(src);
    return size;
#endif
}
