// Host side of the Model pipeline (gate 4b).
//
// The game's own code does the loading, rebasing, VRAM upload and render
// walk; this file provides only what hardware/asm provided on the DS:
//
// - the two asm copy/matrix primitives the walk calls,
// - the VRAM window globals GX::LoadTex/LoadTexPltt resolve against
//   (on hardware these come from bank assignment at GX init; here they
//   point into the ntr mapped VRAM so a slot offset IS a host address),
// - the texture-VRAM budget globals Model::GetVramOffset bump-allocates
//   from,
// - storage for the render context globals the walk stores through.
#include <string.h>
#include "Model.h"
#include "ModelAnim2.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include "dExtShadowModel_c.h"

typedef unsigned int u32;

extern "C" {

// ---- asm primitives ------------------------------------------------------
/* 4x3 fx32 scale matrix from (sx,sy,sz): rows {sx,0,0},{0,sy,0},{0,0,sz},0 */
void func_020527e8(int *m, int sx, int sy, int sz)
{
    memset(m, 0, 12 * sizeof(int));
    m[0] = sx;
    m[4] = sy;
    m[8] = sz;
}

/* asm primitive: 4x3 fx32 identity */
void Matrix4x3_LoadIdentity(int *m)
{
    if (std::getenv("SM64DS_PC_TRACE")) {
        std::fprintf(stderr, "[sm64ds] Matrix4x3_LoadIdentity dst=%p\n", m);
    }
    memset(m, 0, 12 * sizeof(int));
    m[0] = m[4] = m[8] = 0x1000;
}

/* asm primitive: expand a 3x3 fx32 matrix (read from a) into a 4x4 (b):
 * fourth column zero, last row {0,0,0,1.0}. The decomp declares the args
 * (dst,src) but r0 is the READ side; register semantics win here. */
void func_02052514(int *a, int *b)
{
    for (int r = 0; r < 3; ++r) {
        b[r * 4 + 0] = a[r * 3 + 0];
        b[r * 4 + 1] = a[r * 3 + 1];
        b[r * 4 + 2] = a[r * 3 + 2];
        b[r * 4 + 3] = 0;
    }
    b[12] = b[13] = b[14] = 0;
    b[15] = 0x1000;
}

/* ITCM fast-path display-list submit: same (channel, src, size) contract as
 * func_0205a358 (func_02044534 calls that one with identical arguments), so
 * the host forwards to the pump that gate 4a already proved byte-exact. */
void func_0205a358(int ch, int src, int size, void (*cb)(int), int arg);
void func_01ffde98(int ch, int src, int size)
{
    func_0205a358(ch, src, size, 0, 0);
}

// ---- GX bank plumbing ----------------------------------------------------
// Begin/End LoadTex unmap the destination banks to LCDC and remap after; the
// host VRAM window is always CPU-visible, so they are no-ops.
void _ZN2GX12BeginLoadTexEv(void) {}
void _ZN2GX10EndLoadTexEv(void) {}
void _ZN2GX16BeginLoadTexPlttEv(void) {}
void _ZN2GX14EndLoadTexPlttEv(void) {}

// GX::LoadTex destination resolution (see src/engine/gx/GX.cpp):
//   data_020a60ac  base window address        -> texture slots at 0x06800000
//   data_020a60bc  secondary bank address     -> 0 = single flat window
//   data_020a60c0  where the window splits    -> unused while _bc is 0
u32 data_020a60ac = 0x06800000u;
u32 data_020a60bc = 0;
u32 data_020a60c0 = 0;
u32 data_020a60b4 = 0;
u32 data_020a60b8 = 0;

// LoadTexPltt destination base (palette slots, same convention)
u32 data_020a60b0 = 0x06880000u;

// ---- Model texture-VRAM budget -------------------------------------------
// Two shared arenas (semantics recovered from LoadCompressedTextureToVram +
// GetVramOffset together):
//   arena A (slot 0): compressed 4x4 BLOCKS rise from _bc8, plain textures
//     fall from _be8 (the ceiling); the loader crashes when they meet.
//   arena B (slot 1): compressed INDEX words rise from _be0 in lockstep
//     (half the block bytes), plain textures fall from _bdc.
// Starting _bc8 at 0 and _be0 at slot 1 (0x20000) keeps the hardware rule
// "index address = slot1 + block_offset/2" true for every compressed
// texture, which is exactly what the GX-side decoder assumes.
u32 data_020a4bc8 = 0;
u32 data_020a4be8 = 0x20000u;
u32 data_020a4be0 = 0x20000u;
u32 data_020a4bdc = 0x40000u;
u32 data_020a4be4 = 0;      /* running total of uploaded texture bytes */
// Palette window: small palettes bump UP from _bcc, large ones DOWN from
// _bd8 (the ceiling); LoadTexAndPal crashes when they meet. 96K = banks
// E+F+G, the full DS texture-palette budget.
u32 data_020a4bcc = 0;
u32 data_020a4bd8 = 0x18000u;

// ---- render context globals ----------------------------------------------
u32 data_020a4bd4;          /* 1 << (scaleShift + 12), set by Render */
void *data_020a4bd0;        /* current model matrix */
unsigned char data_020a4bbc;    /* texture-matrix mode of the bound material */
short data_020a4bc0;
short data_020a4bc4;

/* processing-list heads; the DS image packs these, but keep named host globals
   for the decomp call sites and let the PC actor bridge reconcile pointers. */
int data_020a4b78[4];
int data_020a4b88[4];
int data_020a4b98[4];
int data_02099f24[4];

// ---- common model data array (BSS on the DS) ------------------------------
int data_0209cefc[3 * 64];  /* 64 records x 0xc */
int data_0209cef8[1];       /* record count */
int data_0208e738[1] = { 1 };   /* nonzero: upload textures on registration */
int _ZTV9ModelBase[8];       /* ModelBase vtable storage; never dispatched */
int data_0209b3ec[12];      /* camera Matrix4x3 the render walk composes */

// Model's Itanium vtable storage: installed by the C ctor. The gates call
// every method non-virtually (qualified), so plain zero slots are fine; if
// anything ever dispatches through them it faults immediately and loudly.
void *_ZTV5Model[8];
// Same story for the animated-model hierarchy: primary vtables plus the
// multiple-inheritance thunk table the ModelAnim ctor installs at +0x50.
void *_ZTV15dExtFrameCtrl_c[8];
void *_ZTV9ModelAnim[10];
void *VTable_Animation_ModelAnimThunk[8];
void *_ZTV4dBgW[13];   /* base: never dispatched in the gates */
unsigned char data_020a0c78[8]; /* the default CLPS ENTRY (8-byte storage,
                                   func_02037e9c fills it on first lookup) */

// File-backed constants of the render/animation walk. These addresses follow
// bss_start but their templates are present in extracted/arm9_dec.bin.
// - data_02099f80/f84: 3-byte weight tables for the BCA keyframe samplers.
//   The interpolation is (lo*frac + hi*rem) over 2^shift steps and frac
//   starts at b[shift] minus the step remainder, so b[shift] = 1 << shift.
//   b[0] is unreachable (shift==0 early-returns).
// - data_02099f88: default unit scale for billboard bones (func_02044534).
// - data_02099f94: texture-size table, copied from the same ROM template.
unsigned char data_02099f80[4] = { 1, 2, 4, 0 };
unsigned char data_02099f84[4] = { 1, 2, 4, 0 };
int data_02099f88[3] = { 0x1000, 0x1000, 0x1000 };
unsigned short data_02099f94[8] = { 4, 8, 16, 32, 64, 128, 256, 512 };

} /* extern "C" */

// The Windows class layout reserves the ROM's separate D1/D0 virtual slots
// with these names. The real destructor bodies below them have distinct
// Itanium symbols and own resource release; these slot fillers are never
// dispatched by the smoke drivers.
void ModelBase::Destructor1() {}
void ModelBase::Destructor0() {}
void Model::Destructor1() {}
void Model::Destructor0() {}
void dExtShadowModel_c::Destructor1() {}
void dExtShadowModel_c::Destructor0() {}

// ModelAnim2 declares the ROM's two destructor vtable slots as plain virtual
// methods under MSVC, while its real C++ destructor remains a separate,
// non-virtual method. Preserve the DS slot behavior for host vtable dispatch.
void ModelAnim2::Destructor1()
{
    this->~ModelAnim2();
}

void ModelAnim2::Destructor0()
{
    this->~ModelAnim2();
    ModelAnim2::operator delete(this);
}

// The host exposes DS texture VRAM as one fixed, contiguous address window.
// These selectors choose the standard A bank (textures) and E bank
// (palettes); the game's SetBank calls need no remapping in that window.
extern "C" unsigned short func_02054168(void) { return 0x10; }
extern "C" unsigned short func_0205417c(void) { return 1; }
namespace GX {
void SetBankForTex(unsigned short);
void SetBankForTexPltt(unsigned short);
}
extern "C" void _ZN2GX13SetBankForTexEt(unsigned int bank)
{
    GX::SetBankForTex(static_cast<unsigned short>(bank));
}
extern "C" void _ZN2GX17SetBankForTexPlttEt(unsigned int bank)
{
    GX::SetBankForTexPltt(static_cast<unsigned short>(bank));
}

// The LoadTex TU declares its globals at C++ linkage; alias them to the
// C-named storage above (same mechanism as gate 3a's data aliases).
#pragma comment(linker, "/alternatename:?data_020a60ac@@3IA=_data_020a60ac")
#pragma comment(linker, "/alternatename:?data_020a60bc@@3IA=_data_020a60bc")
#pragma comment(linker, "/alternatename:?data_020a60c0@@3IA=_data_020a60c0")
#pragma comment(linker, "/alternatename:?data_020a60b4@@3IA=_data_020a60b4")
#pragma comment(linker, "/alternatename:?data_020a60b8@@3IA=_data_020a60b8")
#pragma comment(linker, "/alternatename:?data_02099fd0@@3IA=_data_02099fd0")

// ---- upload-chain bridges -------------------------------------------------
// C name -> the real namespace-style GX::LoadTex from src/.
namespace GX {
void LoadTex(const void *src, unsigned offset, unsigned size);
}
extern "C" void _ZN2GX7LoadTexEPKvjj(const void *s, unsigned o, unsigned z)
{
    GX::LoadTex(s, o, z);
}

// operator_new2 on the DS is a register-passthrough tail-call veneer to
// func_0203cc0c; that shape breaks on cdecl (the callee would read the
// wrong stack slot), so the host bridge passes the argument explicitly.
extern "C" void *func_0203cc0c(unsigned size);
extern "C" void *_ZN6Memory13operator_new2Ej(unsigned size)
{
    return func_0203cc0c(size);
}

