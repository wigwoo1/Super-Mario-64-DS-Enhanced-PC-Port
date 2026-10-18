// Host file system: the card layer under SharedFilePtr.
//
// SEAM LEVEL: SharedFilePtr::Load(). Everything above it -- LoadFile's
// refcounting, Release, func_02017c24, func_02017e0c, and every
// Model/dExtFrameCtrl_c/Collider LoadFile -- is portable src/ and runs verbatim.
// Load itself is card hardware (overlay-file table, card streaming, CP15
// cache flushes), so the HAL reimplements its CONTRACT: resolve fileID to
// bytes, decompress if LZ77-tagged, allocate the result from the game's own
// heap (gate 3a), stash filePtr, return it.
//
// ID NAMESPACES (the trap): handles below 0x8000 resolve through overlay 0's
// handle table to NitroFS IDs. Values at or above 0x8000 are direct archive
// member IDs; the ARM9 table assigns each range to a NARC. Both paths read the
// user's local extraction under extracted/dsd/files/.
//
// Construct is HAL-owned for an ABI reason, not a hardware one: on the DS,
// _ZN13SharedFilePtr9ConstructEj passes ov0FileID to func_02017e48 by
// LEAVING IT IN r1 across a call that only names one argument -- a
// ride-through C cannot express. The host version calls func_02017e0c with
// both arguments spelled out; same result, portable ABI.
//
// COMPRESSION: extracted catalog files begin with ASCII "LZ77" followed by a
// standard type-0x10 stream (u32 header: 0x10 | decompressed_size << 8).
// DecompressLZ16 is an asm hatch in src/, so the HAL carries a byte-wise
// C decode; the DS asm's halfword write dance is VRAM-safety only, the
// output bytes are identical (smoke_fs cross-checks against values computed
// by an independent Python decoder).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../ntr/include/ntr/fs.h"
#include "ntr/texture_assets.h"

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
#include "SharedFilePtr.h"

extern "C" {
void pc_trace_shared_file_handle(const char *label, const void *handle)
{
    if (getenv("SM64DS_PC_TRACE") && label && handle)
        fprintf(stderr, "[sm64ds] handle %s=%04x at %p\n", label,
                *(const unsigned short *)handle, handle);
}

void *_ZN13SharedFilePtr8LoadFileEv(SharedFilePtr *self)
{ return self->SharedFilePtr::LoadFile(); }
void _ZN13SharedFilePtr7ReleaseEv(SharedFilePtr *self)
{ self->SharedFilePtr::Release(); }
}

#ifndef PORT_REPO_ROOT
#define PORT_REPO_ROOT "."
#endif

// ---- the asset catalog ---------------------------------------------------
enum { MAX_FILES = 4096, MAX_HANDLES = 4096, PATH_MAX_ = 260 };

static char g_paths[MAX_FILES][PATH_MAX_]; /* by FAT file id */
static u16 g_handle_to_id[MAX_HANDLES];    /* ov0 handle -> FAT file id */
static int g_catalog_loaded;

struct ArchiveRange { u16 first, end; const char *path; };
static const ArchiveRange g_archives[] = {
    {0x8000, 0x80c6, "ARCHIVE/arc0.narc"},
    {0x8400, 0x8427, "ARCHIVE/en1.narc"},
    {0x8800, 0x8805, "ARCHIVE/vs1.narc"},
    {0x8c00, 0x8c05, "ARCHIVE/vs2.narc"},
    {0x9000, 0x9005, "ARCHIVE/vs3.narc"},
    {0x9400, 0x9405, "ARCHIVE/vs4.narc"},
    {0x9800, 0x9810, "ARCHIVE/c2d.narc"},
    {0x9c00, 0x9c11, "ARCHIVE/ar1.narc"},
    {0xa000, 0xa010, "ARCHIVE/cee.narc"},
    {0xa400, 0xa410, "ARCHIVE/cef.narc"},
    {0xa800, 0xa810, "ARCHIVE/ceg.narc"},
    {0xac00, 0xac10, "ARCHIVE/cei.narc"},
    {0xb000, 0xb010, "ARCHIVE/ces.narc"},
};

static const char *asset_root(void)
{
    const char *env = getenv("SM64DS_ASSET_ROOT");
    return env ? env : PORT_REPO_ROOT;
}

static void catalog_load(void)
{
    char line[512], path[PATH_MAX_];
    FILE *f;
    if (g_catalog_loaded)
        return;
    g_catalog_loaded = 1;

    snprintf(path, sizeof path, "%s/build/assets/files.tsv", asset_root());
    f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "FATAL: fs catalog missing: %s\n"
                "       run tools/asset_catalog.py generate <rom>\n", path);
        abort();
    }
    fgets(line, sizeof line, f); /* header row */
    while (fgets(line, sizeof line, f)) {
        unsigned id;
        char p[PATH_MAX_];
        if (sscanf(line, "%u\t%*s\t%259[^\t]", &id, p) == 2 && id < MAX_FILES)
            strcpy(g_paths[id], p);
    }
    fclose(f);

    snprintf(path, sizeof path, "%s/build/assets/handles.tsv", asset_root());
    f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "FATAL: fs catalog missing: %s\n", path);
        abort();
    }
    fgets(line, sizeof line, f);
    while (fgets(line, sizeof line, f)) {
        unsigned h, id;
        if (sscanf(line, "%u\t%*s\t%u", &h, &id) == 2 && h < MAX_HANDLES)
            g_handle_to_id[h] = (u16)id;
    }
    fclose(f);
}

/* Expose catalog initialization to the game bootstrap. Overlay static
   constructors translate ordinary file handles before player resource setup. */
extern "C" void pc_fs_init_handle_table(void)
{
    catalog_load();
}

extern "C" unsigned int pc_fs_handle_to_file_id(unsigned int handle)
{
    catalog_load();
    return handle < MAX_HANDLES ? g_handle_to_id[handle] : 0;
}

static int catalog_path_id(const char *requested_path)
{
    char wanted[PATH_MAX_];
    size_t i, j = 0;
    if (!requested_path) return -1;
    while (*requested_path == '/' || *requested_path == '\\') ++requested_path;
    for (; requested_path[j] && j + 1 < sizeof(wanted); ++j) {
        char c = requested_path[j] == '\\' ? '/' : requested_path[j];
        wanted[j] = (char)tolower((unsigned char)c);
    }
    if (requested_path[j]) return -1;
    wanted[j] = 0;
    catalog_load();
    for (i = 0; i < MAX_FILES; ++i) {
        char candidate[PATH_MAX_];
        size_t k;
        if (!g_paths[i][0]) continue;
        for (k = 0; g_paths[i][k] && k + 1 < sizeof(candidate); ++k) {
            char c = g_paths[i][k] == '\\' ? '/' : g_paths[i][k];
            candidate[k] = (char)tolower((unsigned char)c);
        }
        candidate[k] = 0;
        if (strcmp(wanted, candidate) == 0) return (int)i;
    }
    return -1;
}

static u16 read16(const u8 *p)
{ return (u16)(p[0] | (p[1] << 8)); }

static u32 read32(const u8 *p)
{ return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24); }

static int load_archive_member(u16 file_id, u8 **bytes_out, long *size_out)
{
    const ArchiveRange *range = 0;
    char path[PATH_MAX_ * 2];
    FILE *f;
    long archive_size;
    u8 *archive;
    size_t fat_entries = 0, fat_count = 0, data_start = 0;
    size_t pos = 0x10, index;
    u16 block_count;
    size_t i;

    for (i = 0; i < sizeof(g_archives) / sizeof(g_archives[0]); ++i) {
        if (file_id >= g_archives[i].first && file_id < g_archives[i].end) {
            range = &g_archives[i];
            break;
        }
    }
    if (!range) return 0;
    snprintf(path, sizeof path, "%s/extracted/dsd/files/%s", asset_root(), range->path);
    f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    archive_size = ftell(f);
    if (archive_size < 0x10 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return 0; }
    archive = (u8 *)malloc((size_t)archive_size);
    if (!archive) { fclose(f); return 0; }
    if (fread(archive, 1, (size_t)archive_size, f) != (size_t)archive_size) {
        fclose(f); free(archive); return 0;
    }
    fclose(f);
    if (memcmp(archive, "NARC", 4) != 0) { free(archive); return 0; }

    block_count = read16(archive + 0x0e);
    for (i = 0; i < block_count; ++i) {
        u32 block_size;
        if (pos + 8 > (size_t)archive_size) { free(archive); return 0; }
        block_size = read32(archive + pos + 4);
        if (block_size < 8 || block_size > (size_t)archive_size - pos) {
            free(archive); return 0;
        }
        if (memcmp(archive + pos, "BTAF", 4) == 0) {
            if (block_size < 12) { free(archive); return 0; }
            fat_count = read16(archive + pos + 8);
            fat_entries = pos + 12;
        } else if (memcmp(archive + pos, "GMIF", 4) == 0) {
            data_start = pos + 8;
        }
        pos += block_size;
    }
    index = (size_t)(file_id - range->first);
    if (!fat_count || !data_start || index >= fat_count ||
        fat_entries + (index + 1) * 8 > (size_t)archive_size) {
        free(archive); return 0;
    }
    {
        u32 start = read32(archive + fat_entries + index * 8);
        u32 end = read32(archive + fat_entries + index * 8 + 4);
        if (end < start || data_start + end > (size_t)archive_size) {
            free(archive); return 0;
        }
        *size_out = (long)(end - start);
        *bytes_out = (u8 *)malloc(*size_out ? (size_t)*size_out : 1);
        if (!*bytes_out) { free(archive); return 0; }
        memcpy(*bytes_out, archive + data_start + start, (size_t)*size_out);
    }
    free(archive);
    return 1;
}

// ---- the game-facing surface ---------------------------------------------
// The Memory:: layer is real C++ on host (gate-3a slice), so the HAL calls
// the namespace; the C-linkage name below serves the C TUs (func_02017c24).
namespace Memory {
void *Allocate(u32 size);
void *Allocate(u32 size, int align);
void Deallocate(void *p);
}
extern "C" void _ZN6Memory10DeallocateEPv(void *p) { Memory::Deallocate(p); }

extern "C" {
u32 data_0209d3bc; /* last fileID touched; the game's FS breadcrumb */
u32 func_02018a24(u32 handle);
void DecompressLZ16(void *src, void *dst);

/* Native LoadFile adapter for PC callers that need a whole archive or
   catalog asset without entering the DS card-thread / DMA path. */
void *pc_fs_load_file(u32 handle)
{
    const u16 file_id = func_02018a24(handle);
    u8 *raw = nullptr;
    long size = 0;
    if (file_id >= 0x8000) {
        if (!load_archive_member(file_id, &raw, &size)) return nullptr;
    } else {
        char path[PATH_MAX_ * 2];
        FILE *f;
        catalog_load();
        if (file_id >= MAX_FILES || !g_paths[file_id][0]) return nullptr;
        snprintf(path, sizeof path, "%s/extracted/dsd/files/%s",
                 asset_root(), g_paths[file_id]);
        f = fopen(path, "rb");
        if (!f) return nullptr;
        if (fseek(f, 0, SEEK_END) != 0 || (size = ftell(f)) < 0 ||
            fseek(f, 0, SEEK_SET) != 0) {
            fclose(f);
            return nullptr;
        }
        raw = static_cast<u8 *>(malloc(size ? static_cast<size_t>(size) : 1));
        if (!raw || fread(raw, 1, static_cast<size_t>(size), f) !=
                        static_cast<size_t>(size)) {
            free(raw);
            fclose(f);
            return nullptr;
        }
        fclose(f);
    }
    if (size > 8 && memcmp(raw, "LZ77", 4) == 0) {
        const u32 decoded = (raw[4] | raw[5] << 8 | raw[6] << 16 |
                             static_cast<u32>(raw[7]) << 24) >> 8;
        void *result = Memory::Allocate(decoded);
        if (result) DecompressLZ16(raw + 4, result);
        ntr::texture_model_register(result,decoded,file_id<MAX_FILES?g_paths[file_id]:nullptr);
        free(raw);
        return result;
    }
    void *result = Memory::Allocate(static_cast<u32>(size), 0x20);
    if (result) memcpy(result, raw, static_cast<size_t>(size));
    ntr::texture_model_register(result,static_cast<size_t>(size),file_id<MAX_FILES?g_paths[file_id]:nullptr);
    free(raw);
    return result;
}

/* Native LoadCompressedFileAt adapter. Unlike LoadFile, this caller supplies
   its destination (often a graphics buffer), so keep the compressed bytes in
   a temporary host buffer and decode directly into that destination. */
u32 pc_fs_load_compressed_file_at(u32 handle, void *target)
{
    const u16 file_id = (u16)func_02018a24(handle);
    u8 *raw = nullptr;
    long size = 0;

    if (!target) return 0;
    if (file_id >= 0x8000) {
        if (!load_archive_member(file_id, &raw, &size)) return 0;
    } else {
        char path[PATH_MAX_ * 2];
        FILE *f;
        catalog_load();
        if (file_id >= MAX_FILES || !g_paths[file_id][0]) return 0;
        snprintf(path, sizeof path, "%s/extracted/dsd/files/%s",
                 asset_root(), g_paths[file_id]);
        f = fopen(path, "rb");
        if (!f) return 0;
        if (fseek(f, 0, SEEK_END) != 0 || (size = ftell(f)) < 0 ||
            fseek(f, 0, SEEK_SET) != 0) {
            fclose(f);
            return 0;
        }
        raw = static_cast<u8 *>(malloc(size ? static_cast<size_t>(size) : 1));
        if (!raw || fread(raw, 1, static_cast<size_t>(size), f) !=
                        static_cast<size_t>(size)) {
            free(raw);
            fclose(f);
            return 0;
        }
        fclose(f);
    }

    u8 *header = nullptr;
    if (size >= 8 && memcmp(raw, "LZ77", 4) == 0)
        header = raw + 4;
    else if (size >= 4 && raw[0] == 0x10)
        header = raw;
    if (!header) {
        fprintf(stderr, "fs: compressed file handle %04x has unknown header %02x %02x %02x %02x\n",
                handle, raw[0], raw[1], raw[2], raw[3]);
        free(raw);
        return 0;
    }
    const u32 decoded = (header[0] | header[1] << 8 | header[2] << 16 |
                         static_cast<u32>(header[3]) << 24) >> 8;
    DecompressLZ16(header, target);
    ntr::texture_model_register(target,decoded,file_id<MAX_FILES?g_paths[file_id]:nullptr);
    free(raw);
    return decoded;
}

/* Native LoadFileAt adapter for raw palette and screen-map assets. The caller
   passes a DS address; ntr::io_init maps the corresponding hardware region in
   the PC process, so the original destination remains valid. */
u32 pc_fs_load_file_at(u32 handle, void *target)
{
    const u16 file_id = (u16)func_02018a24(handle);
    u8 *raw = nullptr;
    long size = 0;

    if (!target) return 0;
    if (file_id >= 0x8000) {
        if (!load_archive_member(file_id, &raw, &size)) {
            fprintf(stderr, "fs: cannot read raw archive member %04x\n", file_id);
            return 0;
        }
    } else {
        char path[PATH_MAX_ * 2];
        FILE *f;
        catalog_load();
        if (file_id >= MAX_FILES || !g_paths[file_id][0]) return 0;
        snprintf(path, sizeof path, "%s/extracted/dsd/files/%s",
                 asset_root(), g_paths[file_id]);
        f = fopen(path, "rb");
        if (!f) {
            fprintf(stderr, "fs: missing raw file %04x: %s\n", file_id, path);
            return 0;
        }
        if (fseek(f, 0, SEEK_END) != 0 || (size = ftell(f)) < 0 ||
            fseek(f, 0, SEEK_SET) != 0) {
            fclose(f);
            return 0;
        }
        raw = static_cast<u8 *>(malloc(size ? static_cast<size_t>(size) : 1));
        if (!raw || fread(raw, 1, static_cast<size_t>(size), f) !=
                        static_cast<size_t>(size)) {
            free(raw);
            fclose(f);
            return 0;
        }
        fclose(f);
    }
    if (size > 0) memcpy(target, raw, static_cast<size_t>(size));
    free(raw);
    return static_cast<u32>(size);
}

/* Native equivalent of the NitroFS path walker for the few callers that use
   a pathname instead of a catalog handle. */
int pc_fs_find_path(void *out_file_id, const char *requested_path)
{
    int id;
    if (!out_file_id || (id = catalog_path_id(requested_path)) < 0) return 0;
    *(u16 *)out_file_id = (u16)id;
    return 1;
}

int pc_fs_read_path(const char *requested_path, void *dst, unsigned int capacity)
{
    char full_path[PATH_MAX_ * 2];
    FILE *f;
    long size;
    size_t read_size;
    int id = catalog_path_id(requested_path);
    if (!dst || !capacity || id < 0) return 0;
    if (ntr::fs_mounted()) {
        const int32_t rom_size = ntr::fs_size(id);
        if (rom_size >= 0)
            return ntr::fs_read(id, dst, (int32_t)capacity);
    }
    snprintf(full_path, sizeof full_path, "%s/extracted/dsd/files/%s",
             asset_root(), g_paths[id]);
    f = fopen(full_path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0 || (size = ftell(f)) < 0 ||
        fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return 0;
    }
    read_size = (size_t)size < capacity ? (size_t)size : capacity;
    if (fread(dst, 1, read_size, f) != read_size) {
        fclose(f);
        return 0;
    }
    fclose(f);
    return (int)read_size;
}

int pc_fs_path_size(const char *requested_path)
{
    char full_path[PATH_MAX_ * 2];
    FILE *f;
    long size;
    int id = catalog_path_id(requested_path);
    if (id < 0) return -1;
    if (ntr::fs_mounted()) {
        const int32_t rom_size = ntr::fs_size(id);
        if (rom_size >= 0) return rom_size;
    }
    snprintf(full_path, sizeof full_path, "%s/extracted/dsd/files/%s",
             asset_root(), g_paths[id]);
    f = fopen(full_path, "rb");
    if (!f) return -1;
    if (fseek(f, 0, SEEK_END) != 0 || (size = ftell(f)) < 0) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return (size > 0 && size <= 64 * 1024 * 1024) ? (int)size : -1;
}

// func_020182bc's mode-1 contract: return an owned, raw file allocation.
// Its callers may subsequently decode a type-0x10 stream into VRAM themselves.
void *pc_fs_load_path(const char *requested_path)
{
    const int size = pc_fs_path_size(requested_path);
    if (size <= 0) return nullptr;
    void *result = Memory::Allocate(static_cast<u32>(size), 0x20);
    if (!result) return nullptr;
    if (pc_fs_read_path(requested_path,result,static_cast<u32>(size)) != size) {
        Memory::Deallocate(result);
        return nullptr;
    }
    return result;
}

/* Archive paths are ROM asset names, not NitroFS paths in the host catalog.
   Resolve these through the mounted ROM path index to avoid interpreting a
   path string as an ARM pointer. */
int pc_fs_archive_path_size(const char *requested_path)
{
    int32_t id = ntr::fs_id_by_path(requested_path);
    return id < 0 ? -1 : ntr::fs_size(id);
}

int pc_fs_archive_read_path(const char *requested_path, void *dst, unsigned int capacity)
{
    int32_t id = ntr::fs_id_by_path(requested_path);
    return id < 0 ? 0 : ntr::fs_read(id, dst, static_cast<int32_t>(capacity));
}

/* ov0 handle -> FAT file id (data_ov000_020bd4b8 on the DS) */
u32 func_02018a24(u32 handle)
{
    if (handle >= 0x8000)
        return handle;
    catalog_load();
    if (handle >= MAX_HANDLES || g_handle_to_id[handle] >= MAX_FILES ||
        g_paths[g_handle_to_id[handle]][0] == 0) {
        fprintf(stderr, "FATAL: fs handle %u not in catalog\n", handle);
        abort();
    }
    return (u32)g_handle_to_id[handle];
}

/* LZ77 type-0x10 decode; src points at the u32 header */
void DecompressLZ16(void *src_, void *dst_)
{
    const u8 *s = (const u8 *)src_;
    u8 *d = (u8 *)dst_;
    u32 size = (s[0] | s[1] << 8 | s[2] << 16 | (u32)s[3] << 24) >> 8;
    u32 out = 0;
    s += 4;
    while (out < size) {
        u8 flags = *s++;
        for (int bit = 0; bit < 8 && out < size; ++bit) {
            if (flags & (0x80 >> bit)) {
                u32 len = (s[0] >> 4) + 3;
                u32 disp = ((s[0] & 0xF) << 8 | s[1]) + 1;
                s += 2;
                while (len-- && out < size) {
                    d[out] = d[out - disp];
                    ++out;
                }
            } else {
                d[out++] = *s++;
            }
        }
    }
}

/* SharedFilePtr::Load -- the seam. Same contract as 0x02017c54. */
struct SharedFilePtrC { u16 fileID; u8 numRefs; void *filePtr; };

void *_ZN13SharedFilePtr4LoadEv(struct SharedFilePtrC *self)
{
    char path[PATH_MAX_ * 2];
    FILE *f;
    long fsize;
    u8 *raw;
    void *dst;
    size_t model_size;

    data_0209d3bc = self->fileID;
    if (self->fileID == 0 && getenv("SM64DS_PC_TRACE")) {
        void *frames[6] = {};
        USHORT count = CaptureStackBackTrace(1, 6, frames, 0);
        fprintf(stderr, "[sm64ds] SharedFilePtr load zero handle at %p module=%p stack=", (void *)self, (void *)GetModuleHandleA(nullptr));
        for (USHORT i = 0; i < count; ++i)
            fprintf(stderr, "%s%p", i ? "," : "", frames[i]);
        fputc('\n', stderr);
    }
    if (self->fileID >= 0x8000) {
        if (!load_archive_member(self->fileID, &raw, &fsize)) {
            fprintf(stderr, "fs: cannot read archive member %04x\n", self->fileID);
            return 0;
        }
    } else {
        catalog_load();
        if (self->fileID >= MAX_FILES || g_paths[self->fileID][0] == 0) {
            fprintf(stderr, "FATAL: fs fileID %u not in catalog\n", self->fileID);
            abort();
        }
        snprintf(path, sizeof path, "%s/extracted/dsd/files/%s",
                 asset_root(), g_paths[self->fileID]);
        f = fopen(path, "rb");
        if (!f) {
            /* Regional variants may omit otherwise cataloged files. */
            fprintf(stderr, "fs: missing on disk: %s\n", path);
            return 0;
        }
        fseek(f, 0, SEEK_END);
        fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (fsize < 0) { fclose(f); return 0; }
        raw = (u8 *)malloc(fsize ? fsize : 1);
        if (!raw) { fclose(f); return 0; }
        if ((long)fread(raw, 1, fsize, f) != fsize) {
            fclose(f);
            free(raw);
            return 0; /* real Load returns 0 on a short read */
        }
        fclose(f);
    }

    model_size=static_cast<size_t>(fsize);
    if (fsize > 8 && memcmp(raw, "LZ77", 4) == 0) {
        u32 dec = (raw[4] | raw[5] << 8 | raw[6] << 16 | (u32)raw[7] << 24) >> 8;
        dst = Memory::Allocate(dec);
        model_size=dec;
        DecompressLZ16(raw + 4, dst);
    } else {
        dst = Memory::Allocate((u32)fsize, 0x20);
        memcpy(dst, raw, fsize);
    }
    free(raw);
    self->filePtr = dst;
    ntr::texture_model_register(dst,model_size,self->fileID<MAX_FILES?g_paths[self->fileID]:nullptr);
    return dst;
}

/* Construct: host ABI spells out both args (see header comment) */
void func_02017e0c(void *self, u32 arg); /* portable src/ */
struct SharedFilePtrC *_ZN13SharedFilePtr9ConstructEj(
        struct SharedFilePtrC *self, u32 ov0FileID)
{
    func_02017e0c(self, ov0FileID);
    return self;
}
} /* extern "C" */

/* The seam above is spelled as a C function because that is how src/ reached it
   before the callers were converted. include/SharedFilePtr.h now declares Load as
   the real member, and src/_ZN13SharedFilePtr8LoadFileEv.cpp calls it as one -- so
   this target, which deliberately does NOT compile the card-hardware
   src/_ZN13SharedFilePtr4LoadEv.cpp, has to export the member too. Same seam, one
   spelling forwarding to the other. */
#include "SharedFilePtr.h"

void *SharedFilePtr::Load()
{
    return _ZN13SharedFilePtr4LoadEv((struct SharedFilePtrC *)this);
}
