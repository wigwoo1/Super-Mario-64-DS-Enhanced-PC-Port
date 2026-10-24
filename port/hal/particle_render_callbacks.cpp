// The original particle renderer's two indirect tables contain ARM function
// addresses, not serialized effect data. Bind them to the recovered native
// workers. Byte-sized texture dimensions use exact native adapter signatures.
extern "C" {
void func_0204c24c(int,int);
void func_0204c194(int,int);
void func_0204c0a8(int,int,void *);
void func_0204c0e8(int,int,void *);
void pc_particle_quad_normal(unsigned char x,unsigned char y){func_0204c24c(x,y);}
void pc_particle_quad_offset(unsigned char x,unsigned char y){func_0204c194(x,y);}
using Quad=void (*)(unsigned char,unsigned char);
using Rotation=void (*)(int,int,void *);
Quad data_02099fb4[2]={pc_particle_quad_normal,pc_particle_quad_offset};
Rotation data_02099fbc[2]={func_0204c0a8,func_0204c0e8};
}
