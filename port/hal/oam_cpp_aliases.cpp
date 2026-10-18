// OAM.cpp contains a few legacy declarations without extern "C". On MSVC
// those identifier spellings are decorated as normal C++ free functions; this
// TU supplies those exact functions and forwards to the recovered OAM methods.
#include "OAM.h"
#include "OamAttr.h"

unsigned char _ZN3OAM11GetObjWidthEii(int shape, int size)
{ return OAM::GetObjWidth(shape, size); }

unsigned char _ZN3OAM12GetObjHeightEii(int shape, int size)
{ return OAM::GetObjHeight(shape, size); }

int _ZN3OAM16LoadAffineParamsEP7OamAttrPiP9Matrix2x2(
    OamAttr *oam, int *count, Matrix2x2 *matrix)
{ return OAM::LoadAffineParams(oam, count, matrix); }
