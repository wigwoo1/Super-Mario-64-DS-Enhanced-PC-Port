//cpp
// @symbol _ZN15ModelComponents21UpdateVertsUsingBonesEv
/* ModelComponents::UpdateVertsUsingBones() at 0x0204504c -- reset the per-bone
 * transform array to identity, then rebuild vertex positions from the BMD
 * bone list via func_02045074.
 *
 * Language-mode migration: real method so mwccarm mangles the symbol. Layout
 * comes from include/ModelBase.h (modelFile @0x00, transforms @0x0c). The
 * bones pointer passed to the helper is modelFile->bones (BMD_File @0x08), not
 * the ModelComponents::bones field. */
#include "ModelBase.h"
#include <cstdio>
#include <cstdlib>

extern "C" {
void Matrix4x3_LoadIdentity(Matrix4x3* m);
void func_02045074(ModelComponents* self, BMD_Bone* bones);
}

void ModelComponents::UpdateVertsUsingBones()
{
    if (std::getenv("SM64DS_PC_TRACE")) {
        std::fprintf(stderr,
                     "[sm64ds] ModelComponents update self=%p file=%p bones=%p transforms=%p materials=%p",
                     this, modelFile, modelFile ? modelFile->bones : nullptr,
                     transforms, materials);
        if (modelFile)
            std::fprintf(stderr, " boneCount=%u materialCount=%u",
                         modelFile->numBones, modelFile->numMaterials);
        std::fputc('\n', stderr);
    }
    Matrix4x3_LoadIdentity(transforms);
    func_02045074(this, modelFile->bones);
}
