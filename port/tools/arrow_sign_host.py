"""Native construction/dispatch for the recovered flat arrow-sign view.

The matching actor TU and header remain untouched. The native derived wrapper
owns exactly the same members/offsets while inheriting complete MSVC callbacks.
"""
import re
import struct


def transform(root, text):
    config=(root/'config/arm9/overlays/ov098/delinks.txt').read_text()
    base=int(re.search(r'\.text\s+start:0x([\da-f]+)',config)[1],16)
    raw=(root/'extracted/overlays/overlay_0098.bin').read_bytes()
    assert struct.unpack_from('<6I',raw,0x0213c380-base)==(
        0x0213c810,0x0213c800,0x0210d914,0x0213c808,0x0213c818,0x0210d734)
    for address,factory,actor,render in ((0x0213c398,0x02138008,299,188),
                                         (0x0213c3b4,0x02137fd0,300,189)):
        assert struct.unpack_from('<IHHIiiii',raw,address-base)==(
            factory,actor,render,2,0xc0000,0xe0000,0x1000000,0x1000000)
    # Replace the two flat `new` expressions: the retail base/shadow constructor
    # chain is otherwise absent in the historical non-inheriting host view.
    assert text.count('return (int *)new daObjYajirusi_c;')==2
    text=text.replace('return (int *)new daObjYajirusi_c;', 'return pc_arrow_create();')
    anchor='// 0x02138008 (0x38)'
    assert text.count(anchor)==1
    text=text.replace(anchor,'static int *pc_arrow_create();\n\n'+anchor,1)
    # Keep the old smoke table storage independent; shipping code uses canonical
    # typed handles and the original six-word row order/12-byte stride.
    for suffix in ('380','384','388'):
        text=text.replace(f'extern ArrowSignFileColumn data_ov098_0213c{suffix}[];', '')
    anchor='// 0x02138008 (0x38)'
    resources=r'''
extern "C" unsigned char data_ov002_0210d914[],data_ov002_0210d734[];
// SharedFilePtr.h deliberately exposes methods only, with no recovered fields.
// Its opaque declaration is NOT allocation storage. Native handles are8 bytes.
struct PcArrowFile {unsigned words[2];SharedFilePtr *get(){return reinterpret_cast<SharedFilePtr *>(this);}};
static_assert(sizeof(PcArrowFile)==8,"native arrow file handle extent");
static PcArrowFile pc_arrow_models[2],pc_arrow_collisions[2];
struct PcArrowResourceRow {SharedFilePtr *model,*collision;void *clps;};
static PcArrowResourceRow pc_arrow_resources[2]={
    {reinterpret_cast<SharedFilePtr *>(&pc_arrow_models[0]),reinterpret_cast<SharedFilePtr *>(&pc_arrow_collisions[0]),data_ov002_0210d914},
    {reinterpret_cast<SharedFilePtr *>(&pc_arrow_models[1]),reinterpret_cast<SharedFilePtr *>(&pc_arrow_collisions[1]),data_ov002_0210d734}
};
static_assert(sizeof(PcArrowResourceRow)==12,"arrow resource row stride");
static void pc_arrow_init_files() {
    static bool ready=false;if(ready)return;ready=true;
    // Original ov098 initializer: model L1175/R1177; collision L1176/R1178.
    pc_arrow_models[0].get()->Construct(1175);pc_arrow_models[1].get()->Construct(1177);
    pc_arrow_collisions[0].get()->Construct(1176);pc_arrow_collisions[1].get()->Construct(1178);
}
static void pc_arrow_observe(const char *event,daObjYajirusi_c *view) {
    const char *path=std::getenv("SM64DS_PC_ARROW_TRACE");if(!path||!*path)return;
    auto *actor=reinterpret_cast<dActor_c *>(view);
    static FILE *file=nullptr;
    if(!file){file=std::fopen(path,"w");if(file)std::fputs("event,tick,uid,actor,variant,x,y,z,angle,collider,model,kcl,alive\n",file);}
    if(file){std::fprintf(file,"%s,%llu,%u,%u,%u,%d,%d,%d,%d,%d,%u,%u,%u\n",event,
        static_cast<unsigned long long>(ntr::rt_frame()),unsigned(actor->uniqueID),unsigned(view->actorID),
        unsigned(view->mVariant),view->mPosX,view->mPosY,view->mPosZ,int(view->mAngleY),
        int(view->mMeshCollider.IsEnabled()),
        unsigned(reinterpret_cast<std::uintptr_t>(view->mModel.data.modelFile)),
        unsigned(reinterpret_cast<std::uintptr_t>(pc_arrow_resources[view->mVariant&1].collision)),
        unsigned(actor->aliveState));std::fflush(file);}
    // Read-only queries through the live production collider registry, once
    // per actor instance after its normal distance test enables collision.
    if(std::strcmp(event,"behavior") || !view->mMeshCollider.IsEnabled())return;
    static std::set<unsigned> tested;
    if(tested.count(actor->uniqueID))return;
    tested.insert(actor->uniqueID);
    char geometryPath[4096];std::snprintf(geometryPath,sizeof(geometryPath),"%s.geometry.csv",path);
    static FILE *geometry=nullptr;
    if(!geometry){geometry=std::fopen(geometryPath,"w");if(geometry)std::fputs("tick,uid,actor,x,z,hit,owner,owner_uid,height,nx,ny,nz\n",geometry);}
    auto *player=static_cast<Player *>(dActor_c::FindWithActorID(191,nullptr));
    if(!geometry || !player)return;
    for(int dx=-150;dx<=150;dx+=25)for(int dz=-150;dz<=150;dz+=25){
        Vector3 point={view->mPosX+dx*4096,view->mPosY+500*4096,view->mPosZ+dz*4096};
        dBgCh_Gnd query;query.SetObjAndPos(point,player);const int hit=query.DetectClsn();
        std::fprintf(geometry,"%llu,%u,%u,%d,%d,%d,%u,%u,%d,%d,%d,%d\n",
            static_cast<unsigned long long>(ntr::rt_frame()),unsigned(actor->uniqueID),unsigned(actor->actorID),
            point.x,point.z,hit,hit&&query.owner?unsigned(query.owner->actorID):0,
            hit&&query.owner?unsigned(query.owner->uniqueID):0,hit?int(query.clsnY):0,
            hit?query.surface.normal.x:0,hit?query.surface.normal.y:0,hit?query.surface.normal.z:0);
    }
    std::fflush(geometry);
}
'''
    text=text.replace(anchor,resources+'\n'+anchor,1)
    text=re.sub(r'data_ov098_0213c380\[([^]]+)\]\.value',r'pc_arrow_resources[\1].model',text)
    text=re.sub(r'data_ov098_0213c384\[([^]]+)\]\.value',r'pc_arrow_resources[\1].collision',text)
    text=re.sub(r'data_ov098_0213c388\[([^]]+)\]\.value',r'pc_arrow_resources[\1].clps',text)
    text=text.replace('int daObjYajirusi_c::InitResources()\n{',
        'int daObjYajirusi_c::InitResources()\n{\n    pc_arrow_init_files();')
    text=text.replace('int daObjYajirusi_c::Render()\n{',
        'int daObjYajirusi_c::Render()\n{\n    pc_render_probe::Scope probe(reinterpret_cast<dActor_c *>(this),0,&mModel.mat4x3);')
    for method,event in (('CleanupResources','cleanup'),('Behavior','behavior')):
        anchor=f'int daObjYajirusi_c::{method}()\n{{'
        assert text.count(anchor)==1
        condition='if(ntr::rt_frame()%5==0)' if method=='Behavior' else ''
        text=text.replace(anchor,anchor+f'\n    {condition}pc_arrow_observe("{event}",this);',1)
    text+=r'''
// Complete compiler-owned native hierarchy; matching flat view remains0x380.
struct PcNativeArrowSign final : dBgActor_c {
    dExtShadowModel_c shadow;
    Matrix4x3 shadowMatrix;
    unsigned char pad378[4],variant,pad37d[3];
    daObjYajirusi_c *view(){return reinterpret_cast<daObjYajirusi_c *>(this);}
    ~PcNativeArrowSign() override {pc_arrow_observe("destroy",view());}
    int InitResources() override {int result=view()->InitResources();pc_arrow_observe("ready",view());return result;}
    int CleanupResources() override {return view()->CleanupResources();}
    int Behavior() override {return view()->Behavior();}
    int Render() override {return view()->Render();}
    int OnAttacked1(dActor_c &other) override {return view()->OnAttacked1(other);}
    void OnHitByMegaChar(Player &player) override {view()->OnHitByMegaChar(player);}
    void Kill() override {view()->Kill();}
};
static_assert(sizeof(PcNativeArrowSign)==0x380,"native arrow allocation");
static_assert(offsetof(PcNativeArrowSign,shadow)==0x320,"native arrow shadow");
static_assert(offsetof(PcNativeArrowSign,shadowMatrix)==0x348,"native arrow shadow matrix");
static_assert(offsetof(PcNativeArrowSign,variant)==0x37c,"native arrow variant");
static int *pc_arrow_create(){return reinterpret_cast<int *>(new PcNativeArrowSign());}
'''
    return '#include <cstddef>\n#include <cstdint>\n#include <cstdio>\n#include <cstdlib>\n#include <cstring>\n#include <set>\n#include "dBgCh_Gnd.h"\n#include "ntr/rt.h"\n#include "hal/actor_render_probe.h"\n'+text
