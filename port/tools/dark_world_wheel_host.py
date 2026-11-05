"""Native resources and typed shared initialization for the Dark World wheel."""
from battlefield_platform_host import lazy_handles, observe
import re

def transform(root, text, name):
    if name == 'func_ov002_020b6c54.c':
        return '''#include "daObjKurumajiku_c.h"
#include "dBgW.h"
extern "C" int func_ov002_020b6a80(char *);
extern "C" void _ZN10dBgW_KcMbg7SetFileEP8KCL_FileRK9Matrix4x35Fix12IiEsR10CLPS_Block(
    dBgW_KcMbg *, KCL_File *, const Matrix4x3 *, int, s16, CLPS_Block *);
extern "C" int func_ov002_020b6c54(daObjKurumajiku_c *self,
    daObjKurumajiku_c::Resources *resources, unsigned actorID) {
    self->mModel.SetFile((BMD_File *)Model::LoadFile(*resources->model),1,-1);
    func_ov002_020b6a80(reinterpret_cast<char *>(self));
    self->UpdateClsnPosAndRot();
    _ZN10dBgW_KcMbg7SetFileEP8KCL_FileRK9Matrix4x35Fix12IiEsR10CLPS_Block(
        &self->mMeshCollider,(KCL_File *)dBgW_Kc::LoadFile(*resources->collision),
        &self->mClsnMat,0x199,self->mAngleY,resources->clps);
    for(int i=0;i<4;++i) {
        self->mMountedActorIds[i]=0;
        auto *cart=dActor_c::Spawn(actorID,0,*reinterpret_cast<Vector3 *>(&self->mPosX),nullptr,self->mAreaId,-1);
        if(cart)self->mMountedActorIds[i]=cart->uniqueID;
    }
    return 1;
}
'''
    bases={'daObjKurumajiku_c.cpp':'daObjKurumajiku_c','daObjKuruma_c.cpp':'daObjKuruma_c'}
    if name in bases:
        cls=bases[name]
        text=text.replace('s32 '+cls+'::','int '+cls+'::')
        if cls=='daObjKurumajiku_c':
            # Retail initializes four adjoining Vector3 BSS labels. The generic
            # linker provides only the first 12-byte label, not a 48-byte array.
            # Reuse the recovered initializer on one native array; Vector3's
            # destructor registrations carry no gameplay work and are omitted.
            declaration='extern Vector3 data_ov002_0210ddd0[4];'
            assert text.count(declaration)==1
            text=text.replace(declaration,'static Vector3 pc_wheel_mounts[4];')
            text=text.replace('data_ov002_0210ddd0','pc_wheel_mounts')
            init=(root/'src/unnamed/ov002/__sinit_ov002_02100f84.c').read_text()
            init=init[init.index('void __sinit_ov002_02100f84(void) {'):]
            body=init[init.index('{')+1:init.rindex('}')]
            body=re.sub(r'func_020731dc\([^;]+;', '', body)
            for i,address in enumerate(('0210ddd0','0210dddc','0210dde8','0210ddf4')):
                body=body.replace('data_ov002_'+address,
                    'reinterpret_cast<int *>(&pc_wheel_mounts[%d])'%i)
            anchor='int daObjKurumajiku_c::Behavior()\n{'
            assert text.count(anchor)==1
            setup='static void pc_init_wheel_mounts() {\n static bool ready=false;if(ready)return;\n'+body+'\nready=true;\n}\n'
            text=text.replace(anchor,setup+anchor+'\n    pc_init_wheel_mounts();',1)
            return observe(text,cls,'angle_z,cart0,cart1,cart2,cart3',
                ['int(mAngleZ)']+['unsigned(mMountedActorIds[%d])'%i for i in range(4)])
        return observe(text,cls,'angle_z', ['int(mAngleZ)'])
    specs={
        'd_a_obj_km1_kurumajiku.cpp':('daObjKm1_Kurumajiku_c',
            'Km1KurumajikuModelFilePtr','Km1KurumajikuCollisionFilePtr',
            '02112610','02112608','02112344','02111bc0',1621,1622),
        'd_a_obj_km1_kuruma.cpp':('daObjKm1_Kuruma_c',
            'Km1KurumaModelFile','Km1KurumaCollisionFilePtr',
            '02112638','02112630','02112418','02111b80',1619,1620)}
    cls,mkind,ckind,m,c,table,clps,mfile,cfile=specs[name]
    text=lazy_handles(text,(mkind,ckind))
    original='data_ov043_'+table
    assert text.count('extern ResourceDescriptor '+original+';')==1
    text=text.replace('extern ResourceDescriptor '+original+';',
                      'static ResourceDescriptor pc_'+original+';')
    text=text.replace('&'+original,'&pc_'+original)
    fn='pc_'+cls+'_resources'
    anchor='// @symbol '+cls+'_classInit'
    assert text.count(anchor)==1
    text=text.replace(anchor,'static void '+fn+'();\n'+anchor,1)
    anchor='s32 '+cls+'::InitResources()\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    '+fn+'();',1)
    text+=f'''
extern "C" unsigned char data_ov043_{clps}[];
static void {fn}() {{
    static bool ready=false;if(ready)return;
    data_ov043_{m}.Construct({mfile});
    data_ov043_{c}.Construct({cfile});
    pc_{original}={{&data_ov043_{m},&data_ov043_{c},reinterpret_cast<CLPS_Block *>(data_ov043_{clps})}};
    ready=true;
}}
'''
    return text
