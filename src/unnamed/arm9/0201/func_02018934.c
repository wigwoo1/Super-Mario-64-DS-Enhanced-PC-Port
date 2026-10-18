#include "types.h"
#if defined(SM64DS_PLATFORM_PC)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct PcArchiveAllocation { struct PcArchiveAllocation *next; void *ptr; } PcArchiveAllocation;
static PcArchiveAllocation *pc_archive_allocations;
typedef struct PcArchiveObject { struct PcArchiveObject *next; void *self; void *fatb; void *fimg; } PcArchiveObject;
static PcArchiveObject *pc_archive_objects;
static void pc_archive_register_parsed_archive(void *self, void *fatb, void *fimg)
{
    PcArchiveObject *node = (PcArchiveObject *)malloc(sizeof(*node));
    if (node) { node->self = self; node->fatb = fatb; node->fimg = fimg; node->next = pc_archive_objects; pc_archive_objects = node; }
}
void pc_archive_register_host_pointers(void *self, void *base, void *fatb, void *fimg)
{
    (void)base;
    pc_archive_register_parsed_archive(self, fatb, fimg);
}
void pc_archive_release_host(void *self)
{
    PcArchiveObject **link = &pc_archive_objects;
    while (*link) {
        PcArchiveObject *node = *link;
        if (node->self == self) {
            *link = node->next;
            pc_archive_free(node->fatb);
            pc_archive_free(node->fimg);
            free(node);
            return;
        }
        link = &node->next;
    }
}
static void *pc_archive_alloc(u32 size)
{
    SYSTEM_INFO info;
    PcArchiveAllocation *node = (PcArchiveAllocation *)malloc(sizeof(*node));
    void *ptr;
    if (!node) return 0;
    GetSystemInfo(&info);
    ptr = VirtualAlloc(info.lpMinimumApplicationAddress, size,
                       MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE);
    if (!ptr) { free(node); return 0; }
    node->ptr = ptr;
    node->next = pc_archive_allocations;
    pc_archive_allocations = node;
    return ptr;
}
static int pc_archive_free(void *ptr)
{
    PcArchiveAllocation **link = &pc_archive_allocations;
    while (*link) {
        PcArchiveAllocation *node = *link;
        if (node->ptr == ptr) {
            *link = node->next;
            { PcArchiveObject **obj = &pc_archive_objects; while (*obj) { if ((*obj)->self == ptr) { PcArchiveObject *old = *obj; *obj = old->next; free(old); break; } obj = &(*obj)->next; } }
            VirtualFree(node->ptr, 0, MEM_RELEASE);
            free(node);
            return 1;
        }
        link = &node->next;
    }
    return 0;
}
typedef struct PcNarcObject { u8 pad[0x50]; u32 base; u32 fatb; u32 fimg; } PcNarcObject;
static u16 pc_rd16(const u8 *p) { return (u16)(p[0] | (p[1] << 8)); }
static u32 pc_rd32(const u8 *p) { return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24); }
static int pc_parse_narc(PcNarcObject *obj, const u8 *data, int size)
{
    int pos, fatb = -1, fimg = -1, chunks, i;
    if (size < 16 || memcmp(data, "NARC", 4) != 0) return 0;
    chunks = pc_rd16(data + 0xe);
    pos = pc_rd16(data + 0xc);
    for (i = 0; i < chunks && pos + 8 <= size; ++i) {
        u32 len = pc_rd32(data + pos + 4);
        if (len < 8 || len > (u32)(size - pos)) return 0;
        if (memcmp(data + pos, "BTAF", 4) == 0) fatb = pos;
        if (memcmp(data + pos, "GMIF", 4) == 0) fimg = pos;
        pos += (int)len;
    }
    if (fatb < 0 || fimg < 0) return 0;
    {
    u32 file_count = pc_rd16(data + fatb + 8);
    u32 image_size = pc_rd32(data + fimg + 4) - 8;
    u8 *fat_table;
    u8 *image;
    if (file_count > 16384 || 12u + file_count * 8u > pc_rd32(data + fatb + 4)) return 0;
    fat_table = (u8 *)pc_archive_alloc(12 + file_count * 8);
    image = (u8 *)pc_archive_alloc(image_size);
    if (!fat_table || !image) return 0;
    memcpy(fat_table, data + fatb, 12 + file_count * 8);
    memcpy(image, data + fimg + 8, image_size);
    for (i = 0; i < (int)file_count; ++i) {
        u32 start = pc_rd32(data + fatb + 12 + i * 8);
        u32 end = pc_rd32(data + fatb + 16 + i * 8);
        if (end < start || end > image_size) return 0;
    }
    obj->fatb = (u32)(uintptr_t)fat_table;
    obj->fimg = (u32)(uintptr_t)image;
    pc_archive_register_parsed_archive(obj, fat_table, image);
    }
    return 1;
}
int pc_archive_member_pointer(const void *archive, unsigned index)
{
    const PcNarcObject *obj = (const PcNarcObject *)archive;
    const u8 *fatb = (const u8 *)(uintptr_t)obj->fatb;
    const u8 *image = (const u8 *)(uintptr_t)obj->fimg;
    u32 count = pc_rd16(fatb + 8);
    u32 offset, end;
    if (index >= count) return 0;
    offset = pc_rd32(fatb + 12 + index * 8);
    end = pc_rd32(fatb + 16 + index * 8);
    return end < offset ? 0 : (int)(uintptr_t)(image + offset);
}
#include <stdio.h>
#include <stdlib.h>
#define PC_ARC_TRACE(label) do { const char *t = getenv("SM64DS_PC_TRACE"); if (t && t[0] && t[0] != '0') { fprintf(stderr, "[sm64ds] archive file: %s\n", label); fflush(stderr); } } while (0)
#else
#define PC_ARC_TRACE(label) ((void)0)
#endif
/* func_02018934 at 0x02018934
 *
 * Matched byte-for-byte with mwccarm 1.2/sp2p3 (arm9 main).
 */
extern void FS_InitFile(int *s);
extern int func_02018d98(int *s, int a);
extern void *_ZN4Heap9_AllocateEji(void *heap, u32 size, int align);
extern void _ZN4CP1527FlushAndInvalidateDataCacheEjj(u32 addr, u32 size);
extern int func_02018d48(int *s, void *buf, int n);
extern int func_0204ee40(void *a, void *b, void *c);
extern void _ZN4Heap11_DeallocateEPv(void *heap, void *ptr);
extern void FS_CloseFile(int *s);

void *func_02018934(void *arg0, int arg1, void *heap)
{
    int buf[0x11];
    void *result = 0;
    void *ptr;
    void *p60;
    int diff, aligned;

#if defined(SM64DS_PLATFORM_PC)
    extern int pc_fs_path_size(const char *path);
    extern int pc_fs_read_path(const char *path, void *dst, unsigned int capacity);
    extern int pc_fs_archive_path_size(const char *path);
    extern int pc_fs_archive_read_path(const char *path, void *dst, unsigned int capacity);
    static const char *const pc_archive_paths[] = {
        "ARCHIVE/arc0.narc", "ARCHIVE/ar1.narc", "ARCHIVE/vs1.narc",
        "ARCHIVE/vs2.narc", "ARCHIVE/vs3.narc", "ARCHIVE/vs4.narc",
        "ARCHIVE/c2d.narc", "ARCHIVE/en1.narc", "ARCHIVE/cee.narc",
        "ARCHIVE/cef.narc", "ARCHIVE/ceg.narc", "ARCHIVE/cei.narc",
        "ARCHIVE/ces.narc"
    };
    const char *path = arg1 > 0 && arg1 <= 13 ? pc_archive_paths[arg1 - 1] : 0;
    PC_ARC_TRACE("host archive read: catalog size lookup");
    diff = pc_fs_archive_path_size(path);
    if (diff <= 0) {
        PC_ARC_TRACE("host archive read: path missing or empty");
        return 0;
    }
    aligned = (diff + 0xf) & ~0xf;
    ptr = _ZN4Heap9_AllocateEji(heap, aligned + 0x60, -0x10);
    if (!ptr) return 0;
    p60 = (char *)ptr + 0x60;
    PC_ARC_TRACE("host archive read: reading NARC bytes");
    if (pc_fs_archive_read_path(path, p60, (unsigned int)diff) != diff) {
        PC_ARC_TRACE("host archive read: file read failed");
        _ZN4Heap11_DeallocateEPv(heap, ptr);
        return 0;
    }
    PC_ARC_TRACE("host archive read: parsing NARC");
    if (pc_parse_narc((PcNarcObject *)ptr, (const u8 *)p60, diff)) return ptr;
    PC_ARC_TRACE("host archive read: NARC parse failed");
    _ZN4Heap11_DeallocateEPv(heap, ptr);
    return 0;
#else
    PC_ARC_TRACE("FS_InitFile begin");
    FS_InitFile(buf);
    PC_ARC_TRACE("FS_InitFile complete");
    #if defined(SM64DS_PLATFORM_PC)
    { const char *t = getenv("SM64DS_PC_TRACE"); if (t && t[0] && t[0] != '0') { fprintf(stderr, "[sm64ds] archive file: path pointer=%p\n", (void *)(uintptr_t)(unsigned)arg1); fflush(stderr); } }
    #endif
    func_02018d98(buf, arg1);
    PC_ARC_TRACE("path open complete");
    diff = buf[9] - buf[8];
    aligned = (diff + 0xf) & ~0xf;
    PC_ARC_TRACE("archive buffer allocate begin");
    ptr = _ZN4Heap9_AllocateEji(heap, aligned + 0x60, -0x10);
    PC_ARC_TRACE("archive buffer allocate complete");
    p60 = (char *)ptr + 0x60;
    _ZN4CP1527FlushAndInvalidateDataCacheEjj((u32)p60, aligned);
    PC_ARC_TRACE("archive read begin");
    if (func_02018d48(buf, p60, diff) == diff &&
        func_0204ee40(ptr, arg0, p60)) {
        result = ptr;
    } else {
        _ZN4Heap11_DeallocateEPv(heap, ptr);
    }
    FS_CloseFile(buf);
    return result;
#endif
}
