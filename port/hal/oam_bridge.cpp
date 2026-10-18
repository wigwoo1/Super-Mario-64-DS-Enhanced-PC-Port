// Historical raw-name calls in the OAM render TU -> real migrated static
// methods. Static methods are cdecl, but typed wrappers keep the signatures
// reviewable and avoid hard-coding MSVC decorations.
#include "OAM.h"
#include "OamAttr.h"

extern "C" {
void _ZN3OAM4LoadEv() { OAM::Load(); }
// The title scene's local legacy declaration passes its temporary object in
// r0, while the retail static helper ignores arguments and only clears this
// flag. Keep that exact observable effect behind the ROM-name bridge.
extern unsigned char data_0209e660;
u32 _ZN3OAM12EnableSubOAMEv(void *) { data_0209e660 = 0; return 0; }
u8 _ZN3OAM11GetObjWidthEii(int shape, int size)
{ return OAM::GetObjWidth(shape, size); }
u8 _ZN3OAM12GetObjHeightEii(int shape, int size)
{ return OAM::GetObjHeight(shape, size); }
void _ZN3OAM9RenderSubEP7OamAttrii(OamAttr *data, int x, int y)
{ OAM::RenderSub(data, x, y); }
int _ZN3OAM16LoadAffineParamsEP7OamAttrPiP9Matrix2x2(
    OamAttr *oam, int *count, Matrix2x2 *matrix)
{ return OAM::LoadAffineParams(oam, count, matrix); }
void _ZN3OAM9RenderSubEP7OamAttriiii(
    OamAttr *data, int x, int y, int palette, int priority)
{ OAM::RenderSub(data, x, y, palette, priority); }
}

// Several historic declarations inside the promoted C++ TU have ordinary
// C++ linkage even though their identifiers carry an Itanium spelling. MSVC
// decorates those identifiers again; bind those call sites to the C ABI
// forwarders above.
#pragma comment(linker, "/alternatename:?_ZN3OAM11GetObjWidthEii@@YAEHH@Z=__ZN3OAM11GetObjWidthEii")
#pragma comment(linker, "/alternatename:?_ZN3OAM12GetObjHeightEii@@YAEHH@Z=__ZN3OAM12GetObjHeightEii")
#pragma comment(linker, "/alternatename:?_ZN3OAM16LoadAffineParamsEP7OamAttrPiP9Matrix2x2@@YAHPAUOamAttr@@PAHPAUMatrix2x2@@Z=__ZN3OAM16LoadAffineParamsEP7OamAttrPiP9Matrix2x2")
#pragma comment(linker, "/alternatename:__ZN4cstd4fdivEii=_ZN4cstd4fdivEii")
