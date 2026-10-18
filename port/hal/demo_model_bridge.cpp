// Host ABI adapters for the cutscene's cartridge RTTI names. The decomp's
// `dExtSimpleModel_c` and `dExtAnmModel_c` have the same field/vtable layout
// as the real project Model and ModelAnim implementations; route their host
// virtual calls to those implementations rather than duplicating model logic.
#include <new>

#include "daDemo_c.h"
#include "Model.h"
#include "ModelAnim.h"
#include "dFdColor_c.h"

extern "C" {
void _ZN10dFdColor_cD1Ev(dFdColor_c *self) { self->~dFdColor_c(); }
extern unsigned char data_0209b294[];
void __sinit_02073e6c(void);
void pc_rebase_cutscene_curves(void);
void pc_init_cutscene_data() {
    static bool initialized;
    if (initialized) return;
    __sinit_02073e6c();
    pc_rebase_cutscene_curves();
    // Restore the native vptr after the cartridge initializer's DS stores.
    ::new ((void *)data_0209b294) dFdColor_c(0x1000);
    initialized = true;
}
void func_0203cbc0(void *ptr);
// Canonical eight-byte resource handles shared by DEMO and native Lakitu.
__declspec(align(4)) unsigned char data_ov085_0213074c[8] = {};
__declspec(align(4)) unsigned char data_ov085_02130744[8] = {};
__declspec(align(4)) unsigned char data_ov085_0213073c[8] = {};
void *func_02017acc(void *, unsigned int);
void pc_demo_init_jugem_resources() {
    static bool initialized;
    if (initialized) return;
    func_02017acc(data_ov085_0213074c, 0x2d7);
    func_02017acc(data_ov085_02130744, 0x2d8);
    func_02017acc(data_ov085_0213073c, 0x2d9);
    initialized = true;
}
void _ZN5ModelC2Ev(Model *self)
{ ::new (self) Model(); }

void _ZN9ModelAnimC2Ev(ModelAnim *self)
{ ::new (self) ModelAnim(); }
}

#ifndef SM64DS_PLATFORM_PC
// These RTTI-named model types duplicate ModelBase/Model layouts. Keep each
// destructor layer's ownership work distinct so the compiler-generated base
// teardown does not free the model file or transforms buffer twice.
dExtModel_c::~dExtModel_c()
{ reinterpret_cast<ModelBase *>(this)->~ModelBase(); }

dExtSimpleModel_c::~dExtSimpleModel_c()
{
    if (transformsBuf)
        func_0203cbc0(transformsBuf);
}

int dExtSimpleModel_c::DoSetFile(char *file, int a, int b)
{ return reinterpret_cast<Model *>(this)->Model::DoSetFile(file, a, b); }

void dExtSimpleModel_c::UpdateVerts()
{ reinterpret_cast<Model *>(this)->Model::UpdateVerts(); }

void dExtSimpleModel_c::Virtual10(Matrix4x3 &mat)
{ reinterpret_cast<Model *>(this)->Model::Virtual10(mat); }

void dExtSimpleModel_c::Render(const Vector3 *scale)
{ reinterpret_cast<Model *>(this)->Model::Render(scale); }

dExtAnmModel_c::~dExtAnmModel_c()
{}

void dExtAnmModel_c::UpdateVerts()
{ reinterpret_cast<ModelAnim *>(this)->ModelAnim::UpdateVerts(); }

void dExtAnmModel_c::Virtual10(Matrix4x3 &mat)
{ reinterpret_cast<ModelAnim *>(this)->ModelAnim::Virtual10(mat); }

void dExtAnmModel_c::Render(const Vector3 *scale)
{ reinterpret_cast<ModelAnim *>(this)->ModelAnim::Render(scale); }

void dExtAnmModel_c::Virtual18(u32 mat, const Vector3 *scale)
{ reinterpret_cast<ModelAnim *>(this)->ModelAnim::Virtual18(mat, scale); }
#else
static_assert(sizeof(daDemo_c) == 0x104, "DEMO actor layout");
static_assert(offsetof(daDemo_c::anmModel_c, mScale) == 0x64, "DEMO animation parameter base");
static_assert(offsetof(daDemo_c::simpleModel_c, mScale) == 0x50, "DEMO model parameter base");
void daDemo_c::anmModel_c::Destructor1() { this->~anmModel_c(); }
void daDemo_c::anmModel_c::Destructor0() { this->~anmModel_c(); _ZN6Memory16operator_delete2EPv(this); }
void daDemo_c::simpleModel_c::Destructor1() { this->~simpleModel_c(); }
void daDemo_c::simpleModel_c::Destructor0() { this->~simpleModel_c(); _ZN6Memory16operator_delete2EPv(this); }

template<class T> static void __fastcall demo_d1(T *self, void *) { self->T::Destructor1(); }
template<class T> static void __fastcall demo_d0(T *self, void *) { self->T::Destructor0(); }
template<class T> static int __fastcall demo_file(T *self, void *, char *file, int a, int b) { return self->Model::DoSetFile(file, a, b); }
template<class T> static void __fastcall demo_update(T *self, void *) { self->T::UpdateVerts(); }
template<class T> static void __fastcall demo_matrix(T *self, void *, Matrix4x3 &m) { self->T::Virtual10(m); }
template<class T> static void __fastcall demo_render(T *self, void *, const Vector3 *v) { self->T::Render(v); }
static void __fastcall demo_anim_matrix(daDemo_c::anmModel_c *self, void *, u32 m, const Vector3 *v) { self->ModelAnim::Virtual18(m, v); }
extern "C" void pc_demo_install_model_table(void *self, int animated) {
    using S = daDemo_c::simpleModel_c;
    using A = daDemo_c::anmModel_c;
    static void *simple[] = {(void*)&demo_d1<S>, (void*)&demo_d0<S>, (void*)&demo_file<S>, (void*)&demo_update<S>, (void*)&demo_matrix<S>, (void*)&demo_render<S>};
    static void *anim[] = {(void*)&demo_d1<A>, (void*)&demo_d0<A>, (void*)&demo_file<A>, (void*)&demo_update<A>, (void*)&demo_matrix<A>, (void*)&demo_render<A>, (void*)&demo_anim_matrix};
    *(void ***)self = animated ? anim : simple;
    // ModelAnim's constructor already installs its native frame-controller table.
}
#endif
