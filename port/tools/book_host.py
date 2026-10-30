"""Defer recovered haunted-book file handles until native resources exist."""
def transform(root, text):
    for kind in ('BookModelFilePtr', 'BookAnimationFileHandle'):
        old=f'{kind}(unsigned int fileID) {{ pc_sharedfile_handle_construct(this, fileID); }}'
        assert text.count(old)==1
        text=text.replace(old,f'{kind}(u32 fileID) {{words[0]=fileID;words[1]=0;}}')
        old=f'~{kind}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(old)==1
        text=text.replace(old,f'~{kind}() {{}}')
    anchor='// @symbol _ZN8daBook_cD1Ev'
    helper='''static void pc_book_files() {
    static bool ready=false;if(ready)return;
    data_ov020_02114aa0.Construct(0x2c8);
    data_ov020_02114ab8.Construct(0x2cb);
    data_ov020_02114aa8.Construct(0x2c9);
    data_ov020_02114ab0.Construct(0x2ca);ready=true;
}
'''
    assert text.count(anchor)==1
    text=text.replace(anchor,helper+anchor)
    for cls in ('daBook_c', 'daBookGen_c'):
        anchor=f'int {cls}::InitResources()\n{{'
        assert text.count(anchor)==1
        text=text.replace(anchor,anchor+'\n    pc_book_files();')
    trace=r'''
static void pc_book_observe(daBook_c *self) {
    const char *prefix=std::getenv("SM64DS_PC_MERRY_TRACE");if(!prefix||!*prefix)return;
    static FILE *file=nullptr;
    if(!file){char path[2048];std::snprintf(path,sizeof(path),"%s.books.csv",prefix);file=std::fopen(path,"w");if(file)std::fputs("tick,uid,actor,param,parent,state,x,y,z,model,area\n",file);}
    if(file){std::fprintf(file,"%llu,%u,%u,%u,%u,%d,%d,%d,%d,%u,%d\n",static_cast<unsigned long long>(ntr::rt_frame()),unsigned(self->uniqueID),unsigned(self->actorID),unsigned(self->param1),unsigned(self->mLinkedActorID),int(self->mState),self->mPosX,self->mPosY,self->mPosZ,unsigned(reinterpret_cast<std::uintptr_t>(self->mModel.data.modelFile)),int(self->mAreaId));std::fflush(file);}
}
'''
    anchor='// @symbol _ZN8daBook_cD1Ev'
    text=text.replace(anchor,trace+anchor,1)
    anchor='int daBook_c::Behavior()\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    if(ntr::rt_frame()%5==0)pc_book_observe(this);')
    anchor='int daBook_c::Render()\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    pc_render_probe::Scope probe(this,false,&mModel.mat4x3);')
    helper=r'''
static void pc_book_generator_observe(daBookGen_c *self) {
    const char *prefix=std::getenv("SM64DS_PC_MERRY_TRACE");if(!prefix||!*prefix)return;
    static FILE *file=nullptr;
    if(!file){char path[2048];std::snprintf(path,sizeof(path),"%s.book-generators.csv",prefix);file=std::fopen(path,"w");if(file)std::fputs("tick,uid,actor,timer,x,y,z,yaw,area\n",file);}
    if(file){std::fprintf(file,"%llu,%u,%u,%u,%d,%d,%d,%d,%d\n",static_cast<unsigned long long>(ntr::rt_frame()),unsigned(self->uniqueID),unsigned(self->actorID),unsigned(self->mSpawnTimer),self->mPosX,self->mPosY,self->mPosZ,int(self->mAngleY),int(self->mAreaId));std::fflush(file);}
}
'''
    anchor='// @symbol _ZN11daBookGen_c8BehaviorEv'
    assert text.count(anchor)==1
    text=text.replace(anchor,helper+anchor)
    anchor='int daBookGen_c::Behavior()\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    pc_book_generator_observe(this);')
    text+='\nstatic_assert(sizeof(daBookGen_c)==0xd8,"native book generator layout");\n'
    text+='\nstatic_assert(sizeof(daBook_c)==0x454,"native book layout");\nstatic_assert(sizeof(BookModelFilePtr)==8 && sizeof(BookAnimationFileHandle)==8,"native book handles");\n'
    return '#include <cstdio>\n#include <cstdlib>\n#include <cstdint>\n#include "ntr/rt.h"\n#include "hal/actor_render_probe.h"\n'+text

