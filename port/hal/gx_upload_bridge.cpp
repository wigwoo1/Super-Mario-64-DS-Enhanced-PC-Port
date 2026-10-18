// MSVC-method bridges for gate 4b (the cross-linkage seam, gate-3a style).
//
// The Model TUs declare GX's upload entry points as STATIC MEMBERS of a
// struct GX (mangling ?LoadTex@GX@@SAX...), while the definitions are either
// namespace-style C++ (GX::LoadTex from src/) or C-named HAL stubs. Same
// story for the SharedFilePtr and ModelComponents methods whose real
// definitions are C-named .c files. Each bridge here exists because the two
// spellings mangle differently on MSVC; none of them add behavior.
typedef unsigned int u32;
typedef unsigned short u16;
#include "SharedFilePtr.h"
#include "ModelBase.h"
#include "Model.h"

// Three older callers still declare LoadFile as void. MSVC encodes return
// types, unlike Itanium here; the call ABI is otherwise identical and all three
// callers discard the real pointer result.
#pragma comment(linker, "/alternatename:?LoadFile@SharedFilePtr@@QAEXXZ=?LoadFile@SharedFilePtr@@QAEPAXXZ")
#pragma comment(linker, "/alternatename:__ZN2GX11LoadTexPlttEPKvjj=?LoadTexPltt@GX@@YAXPBXII@Z")
#pragma comment(linker, "/alternatename:?data_020a60b0@@3IA=_data_020a60b0")

// cstd::abs's C-spelling forwarder moved to hal/heap_globals.cpp, next to the
// other Itanium-spelling bridges; every target that links this file links that
// one, and the allocator layer needs the same symbol.

extern "C" {
void _ZN2GX12BeginLoadTexEv(void);
void _ZN2GX10EndLoadTexEv(void);
void _ZN2GX16BeginLoadTexPlttEv(void);
void _ZN2GX14EndLoadTexPlttEv(void);
void _ZN2GX7LoadTexEPKvjj(const void *, u32, u32);
void _ZN2GX11LoadTexPlttEPKvjj(const void *, u32, u32);
}

struct GX {
    static void BeginLoadTex();
    static void EndLoadTex();
    static void BeginLoadTexPltt();
    static void EndLoadTexPltt();
    static void LoadTex(const void *, u32, u32);
    static void LoadTexPltt(const void *, u32, u32);
};
void GX::BeginLoadTex() { _ZN2GX12BeginLoadTexEv(); }
void GX::EndLoadTex() { _ZN2GX10EndLoadTexEv(); }
void GX::BeginLoadTexPltt() { _ZN2GX16BeginLoadTexPlttEv(); }
void GX::EndLoadTexPltt() { _ZN2GX14EndLoadTexPlttEv(); }
void GX::LoadTex(const void *s, u32 o, u32 z) { _ZN2GX7LoadTexEPKvjj(s, o, z); }
void GX::LoadTexPltt(const void *s, u32 a, u32 z) { _ZN2GX11LoadTexPlttEPKvjj(s, a, z); }

// Shrinks the file image to its post-parse size on the DS (a heap-space
// optimization). Skipped on host: the image simply stays at load size.
void SharedFilePtr::ReallocateModelFile() {}

// Historical C-name callers -> methods that are now real C++ definitions.
extern "C" {
void _ZN15ModelComponents21UpdateVertsUsingBonesEv(ModelComponents *self)
{ self->ModelComponents::UpdateVertsUsingBones(); }
void _ZN15ModelComponents11UpdateBonesEP8BCA_Filei(
    ModelComponents *self, BCA_File *file, int frame)
{ self->ModelComponents::UpdateBones(file, frame); }
}


// Legacy C-name callers in decompiled translation units forward to the real
// recovered C++ method. Keep only this ABI thunk; the method itself lives in
// src/engine/model/Model.cpp and must not be defined twice on the host.
extern "C" u32 _ZN5Model27LoadCompressedTextureToVramEPcjS0_(char *src, u32 size, char *idx)
{
    return Model::LoadCompressedTextureToVram(src, size, idx);
}