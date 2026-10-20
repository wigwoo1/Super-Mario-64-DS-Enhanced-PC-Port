// OpenGL rasterization through an explicitly loaded Mesa llvmpipe runtime.
// Geometry decoding/clipping and texture identities stay shared with D3D11.
#include "ntr/gl_software.h"
#include "ntr/gx.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <tuple>
#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>

namespace ntr {
namespace {
constexpr GLenum BGRA=0x80e1,RGBA8=0x8058,CLAMP_EDGE=0x812f,MIRROR=0x8370;
constexpr GLenum RGBA16F=0x881a;
constexpr GLenum FBO=0x8d40,RBO=0x8d41,COLOR_ATTACHMENT=0x8ce0;
constexpr GLenum DEPTH_STENCIL_ATTACHMENT=0x821a,DEPTH24_STENCIL8=0x88f0,FBO_COMPLETE=0x8cd5;
constexpr GLenum VERTEX_SHADER=0x8b31,FRAGMENT_SHADER=0x8b30,COMPILE_STATUS=0x8b81,LINK_STATUS=0x8b82;
constexpr GLenum MAX_ANISO=0x84ff,TEXTURE_ANISO=0x84fe;
#define GL_BASE(X) X(Begin) X(End) X(Color4f) X(TexCoord2f) X(Vertex4f) X(Enable) X(Disable) \
 X(Clear) X(ClearColor) X(ClearStencil) X(ClearDepth) X(DepthMask) X(DepthFunc) X(ColorMask) \
 X(StencilMask) X(StencilFunc) X(StencilOp) X(BlendFunc) X(PolygonMode) X(Viewport) \
 X(GenTextures) X(DeleteTextures) X(BindTexture) X(TexParameteri) X(TexParameterf) X(TexImage2D) \
 X(TexSubImage2D) X(CopyTexSubImage2D) X(GetString) X(GetError) X(GetFloatv) X(ReadPixels) X(Finish)
#define GL_EXT(X) \
 X(GLuint,CreateShader,(GLenum)) X(void,ShaderSource,(GLuint,GLsizei,const char *const *,const GLint *)) \
 X(void,CompileShader,(GLuint)) X(void,GetShaderiv,(GLuint,GLenum,GLint *)) \
 X(void,GetShaderInfoLog,(GLuint,GLsizei,GLsizei *,char *)) X(GLuint,CreateProgram,()) \
 X(void,AttachShader,(GLuint,GLuint)) X(void,LinkProgram,(GLuint)) \
 X(void,GetProgramiv,(GLuint,GLenum,GLint *)) X(void,GetProgramInfoLog,(GLuint,GLsizei,GLsizei *,char *)) \
 X(void,DeleteShader,(GLuint)) X(void,DeleteProgram,(GLuint)) X(void,UseProgram,(GLuint)) \
 X(GLint,GetUniformLocation,(GLuint,const char *)) X(void,Uniform1i,(GLint,GLint)) \
 X(void,Uniform3fv,(GLint,GLsizei,const GLfloat *)) \
 X(void,GenFramebuffers,(GLsizei,GLuint *)) X(void,BindFramebuffer,(GLenum,GLuint)) \
 X(void,FramebufferTexture2D,(GLenum,GLenum,GLenum,GLuint,GLint)) \
 X(GLenum,CheckFramebufferStatus,(GLenum)) X(void,DeleteFramebuffers,(GLsizei,const GLuint *)) \
 X(void,GenRenderbuffers,(GLsizei,GLuint *)) X(void,BindRenderbuffer,(GLenum,GLuint)) \
 X(void,RenderbufferStorage,(GLenum,GLenum,GLsizei,GLsizei)) \
 X(void,FramebufferRenderbuffer,(GLenum,GLenum,GLenum,GLuint)) \
 X(void,DeleteRenderbuffers,(GLsizei,const GLuint *)) \
 X(void,BlendFuncSeparate,(GLenum,GLenum,GLenum,GLenum))
struct Api {
#define DECL_BASE(name) decltype(&gl##name) name=nullptr;
    GL_BASE(DECL_BASE)
#undef DECL_BASE
#define DECL_EXT(ret,name,args) ret (APIENTRY *name) args=nullptr;
    GL_EXT(DECL_EXT)
#undef DECL_EXT
    int(WINAPI *ChoosePixelFormat)(HDC,const PIXELFORMATDESCRIPTOR *)=nullptr;
    BOOL(WINAPI *SetPixelFormat)(HDC,int,const PIXELFORMATDESCRIPTOR *)=nullptr;
    HGLRC(WINAPI *CreateContext)(HDC)=nullptr;
    BOOL(WINAPI *MakeCurrent)(HDC,HGLRC)=nullptr;
    BOOL(WINAPI *DeleteContext)(HGLRC)=nullptr;
    PROC(WINAPI *GetProcAddress)(LPCSTR)=nullptr;
} gl;
HMODULE library=nullptr;
HWND context_window=nullptr,game_window=nullptr;
HDC context_dc=nullptr;HGLRC context=nullptr;
GLuint framebuffer=0,color_target=0,depth_target=0,program=0,base_image=0,map_image=0,white_image=0;
GLuint gameplay_image=0,gameplay_framebuffer=0;
unsigned gameplay_width=0,gameplay_height=0;
GLint u_polygon=-1,u_alpha=-1,u_pass=-1,u_mode=-1,u_toon=-1,u_highlight=-1;
unsigned target_width=0,target_height=0,triangle_count=0;
bool menu_scene=false,force_scan=false,readback_ready=false;
float max_aniso=1;uint8_t anisotropic=0;
char adapter_name[200]={};
std::vector<uint32_t> readback_pixels,raw_pixels;
GpuFrameStats stats;
struct Texture {GLuint id=0;bool partial=false;uint64_t bytes=0,used=0;};
using Key=std::tuple<bool,uint64_t,uint32_t>;
std::map<Key,Texture> cache;
std::set<uint64_t> missing;
uint64_t cache_bytes=0,cache_budget=256ull*1024*1024,use_counter=0;
uint32_t epoch=0;
bool ok(const char *operation) {
    const auto error=gl.GetError();
    if(error==GL_NO_ERROR)return true;
    std::fprintf(stderr,"[OpenGL CPU] %s: GL error %04x\n",operation,error);return false;
}
void parameters(unsigned alpha=31,unsigned pass=0,bool polygon=false,unsigned mode=0) {
    gl.Uniform1i(u_polygon,polygon?1:0);gl.Uniform1i(u_alpha,alpha);
    gl.Uniform1i(u_pass,pass);gl.Uniform1i(u_mode,mode);
}
void sample(GLuint image,bool smooth=false,uint8_t address=0,bool filter_geometry=false) {
    gl.BindTexture(GL_TEXTURE_2D,image);
    const bool aniso=filter_geometry&&anisotropic>0;
    gl.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,smooth||aniso?GL_LINEAR:GL_NEAREST);
    gl.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,smooth||aniso?GL_LINEAR:GL_NEAREST);
    gl.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,!(address&1)?CLAMP_EDGE:address&4?MIRROR:GL_REPEAT);
    gl.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,!(address&2)?CLAMP_EDGE:address&8?MIRROR:GL_REPEAT);
    if(max_aniso>1)gl.TexParameterf(GL_TEXTURE_2D,TEXTURE_ANISO,aniso?std::min<float>(anisotropic,max_aniso):1);
}
GLuint image(const uint32_t *pixels,int width,int height) {
    GLuint id=0;gl.GenTextures(1,&id);gl.BindTexture(GL_TEXTURE_2D,id);
    gl.TexImage2D(GL_TEXTURE_2D,0,RGBA8,width,height,0,BGRA,GL_UNSIGNED_BYTE,pixels);
    if(!ok("texture upload")){gl.DeleteTextures(1,&id);return 0;}
    return id;
}
void overlay_state(bool blend) {
    gl.Disable(GL_DEPTH_TEST);gl.Disable(GL_STENCIL_TEST);gl.DepthMask(GL_FALSE);
    gl.ColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    if(blend)gl.Enable(GL_BLEND);else gl.Disable(GL_BLEND);
    parameters();
}
void quad(GLuint id,bool smooth=false,float left=-1,float top=1,float right=1,float bottom=-1) {
    sample(id,smooth);gl.Begin(GL_TRIANGLE_STRIP);gl.Color4f(1,1,1,1);
    gl.TexCoord2f(0,0);gl.Vertex4f(left,top,0,1);
    gl.TexCoord2f(0,1);gl.Vertex4f(left,bottom,0,1);
    gl.TexCoord2f(1,0);gl.Vertex4f(right,top,0,1);
    gl.TexCoord2f(1,1);gl.Vertex4f(right,bottom,0,1);
    gl.End();
}
void viewport(const ScreenRect &r){gl.Viewport(r.x,int(target_height)-r.y-r.height,r.width,r.height);}
void evict(uint64_t incoming) {
    if(!cache.empty()&&(cache_bytes+incoming>cache_budget||cache.size()>=1024))gl.Finish();
    while(!cache.empty()&&(cache_bytes+incoming>cache_budget||cache.size()>=1024)) {
        auto old=std::min_element(cache.begin(),cache.end(),[](const auto&a,const auto&b){return a.second.used<b.second.used;});
        gl.DeleteTextures(1,&old->second.id);cache_bytes-=old->second.bytes;cache.erase(old);++stats.evictions;
    }
}
Texture *upload(const Key &key,const uint32_t *pixels,int width,int height,bool partial) {
    uint64_t bytes=uint64_t(width)*height*4;
    if(width<1||height<1||bytes>cache_budget)return nullptr;
    evict(bytes);
    auto id=image(pixels,width,height);if(!id)return nullptr;
    Texture t{id,partial,bytes,++use_counter};cache_bytes+=bytes;++stats.uploads;stats.upload_bytes+=bytes;
    return &cache.emplace(key,t).first->second;
}
struct Material {const uint32_t *pixels;int width,height;TexturePixelsInfo info;TextureReplacementRef replacement;bool partial_alpha=false;};
Texture *resolve(const Material &m) {
    if(!m.pixels){static Texture white;white={white_image,false,4,0};return &white;}
    const auto &r=m.replacement;
    if(texture_upscaled()&&r.eligible&&(r.role==TextureRole::Level||r.role==TextureRole::Object)) {
        Key k{true,r.key,epoch};auto it=cache.find(k);
        if(it!=cache.end()){it->second.used=++use_counter;++stats.cache_hits;return &it->second;}
        if(!missing.count(r.key)) {
            ++stats.replacement_attempts;std::vector<uint32_t> p;int w=0,h=0;
            if(texture_load_replacement_pixels(r.key,p,w,h)) {
                stats.staging_peak_bytes=std::max<uint64_t>(stats.staging_peak_bytes,p.size()*4);
                bool partial=false;for(auto color:p)partial|=(color>>24)>0&&(color>>24)<255;
                if(auto *t=upload(k,p.data(),w,h,partial)){++stats.replacement_uploads;return t;}
            }
            missing.insert(r.key);
        }
    }
    Key k{false,m.info.key,0};auto it=cache.find(k);
    if(it!=cache.end()){it->second.used=++use_counter;++stats.cache_hits;return &it->second;}
    return upload(k,m.pixels,m.width,m.height,m.info.partial_alpha);
}
bool geometry(bool dual) {
    using FrameKey=std::tuple<const uint32_t *,int,int,uint64_t,TextureRole,bool>;
    std::map<FrameKey,Material> materials;
    struct Draw {GxVertex v[3];Material *material;uint32_t attributes;int x,y,w,h;uint8_t address;};
    std::vector<Draw> draws;
    size_t count=0;auto *source=gx_display_polygons(count);
    uint32_t toon[32]={};bool highlight=false;gx_read_toon_table(toon,highlight);
    float colors[96];for(unsigned i=0;i<32;++i)for(unsigned j=0;j<3;++j)colors[i*3+j]=float((toon[i]>>(16-j*8))&255)/255;
    gl.Uniform3fv(u_toon,32,colors);gl.Uniform1i(u_highlight,highlight?1:0);
    for(size_t i=0;i<count;++i) {
        auto t=source[i];GxVertex clipped[12];
        if(t.perspective&&!menu_scene&&!dual) {
            const float aspect=float(SCREEN_W)*target_height/(float(SCREEN_H)*target_width);
            for(auto &v:t.v)v.clip_x*=aspect;
        }
        unsigned n=gx_clip_polygon(t,clipped);if(n<3)continue;
        FrameKey key{t.tex,t.tw,t.th,t.replacement.key,t.replacement.role,t.replacement.eligible};
        auto found=materials.find(key);
        if(found==materials.end()) {
            Material m{t.tw>0&&t.th>0?t.tex:nullptr,t.tw,t.th,t.pixels_info,t.replacement};
            if(m.pixels) {
                if(!m.info.immutable||force_scan){m.info=texture_pixels_info(m.pixels,m.width,m.height);stats.hash_texels+=uint64_t(m.width)*m.height;}
                else ++stats.identity_uses;
            }
            m.partial_alpha=m.info.partial_alpha;
            found=materials.emplace(key,m).first;
        }
        for(unsigned j=1;j+1<n;++j) {
            const GxVertex v[3]={clipped[0],clipped[j],clipped[j+1]};
            const float area=(v[1].x-v[0].x)*(v[2].y-v[0].y)-(v[1].y-v[0].y)*(v[2].x-v[0].x);
            if(std::fabs(area)<1e-6f||!(t.attributes&(area<0?(1u<<7):(1u<<6))))continue;
            Draw d{{v[0],v[1],v[2]},&found->second,t.attributes,t.viewport_x,t.viewport_y,t.viewport_w,t.viewport_h,t.texture_address};
            draws.push_back(d);++triangle_count;
        }
    }
    bool rendered_shadow=false;
    for(unsigned pass=0;pass<2;++pass)for(const auto &d:draws) {
        unsigned mode=(d.attributes>>4)&3,id=(d.attributes>>24)&63,alpha=(d.attributes>>16)&31;
        bool shadow=mode==3,translucent=alpha>0&&alpha<31;
        if(!pass&&(shadow||translucent))continue;
        if(pass&&!shadow&&!translucent&&!(mode!=1&&d.material->partial_alpha))continue;
        auto *tex=resolve(*d.material);if(!tex)return false;
        d.material->partial_alpha=tex->partial;
        bool partial=mode!=1&&tex->partial;
        parameters(alpha?alpha:31,shadow?0:partial&&!translucent?(pass?2:1):0,true,mode);
        sample(tex->id,false,d.address,true);
        gl.PolygonMode(GL_FRONT_AND_BACK,alpha?GL_FILL:GL_LINE);
        auto emit=[&] {
            gl.Begin(GL_TRIANGLES);
            for(const auto &a:d.v) {
                float red=mode==2?a.toon_intensity/511:float((a.color>>16)&255)/255;
                gl.Color4f(red,float((a.color>>8)&255)/255,float(a.color&255)/255,float(a.color>>24)/255);
                gl.TexCoord2f(d.material->width>0?a.u/d.material->width:0,d.material->height>0?a.v/d.material->height:0);
                float x=a.clip_x*d.w/SCREEN_W+a.w*(2.0f*d.x/SCREEN_W+float(d.w)/SCREEN_W-1);
                float y=a.clip_y*d.h/SCREEN_H+a.w*(1-2.0f*d.y/SCREEN_H-float(d.h)/SCREEN_H);
                gl.Vertex4f(x,y,a.clip_z,a.w);
            }
            gl.End();
        };
        gl.Enable(GL_STENCIL_TEST);gl.Enable(GL_DEPTH_TEST);gl.DepthFunc(d.attributes&(1u<<14)?GL_EQUAL:GL_LESS);
        gl.DepthMask(GL_FALSE);gl.ColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
        if(shadow) {
            if(!id) {
                if(rendered_shadow){gl.StencilMask(0x40);gl.ClearStencil(0);gl.Clear(GL_STENCIL_BUFFER_BIT);rendered_shadow=false;}
                gl.ColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);gl.Disable(GL_BLEND);
                gl.StencilMask(0x40);gl.StencilFunc(GL_ALWAYS,0x40,0x40);gl.StencilOp(GL_KEEP,GL_REPLACE,GL_KEEP);emit();
            } else {
                gl.Disable(GL_DEPTH_TEST);gl.ColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
                gl.StencilMask(0x40);gl.StencilFunc(GL_EQUAL,id,0x3f);gl.StencilOp(GL_KEEP,GL_KEEP,GL_ZERO);emit();
                gl.Enable(GL_DEPTH_TEST);gl.ColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);gl.Enable(GL_BLEND);
                gl.StencilMask(0x7f);gl.StencilFunc(GL_NOTEQUAL,id,0x40);gl.StencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);emit();
                rendered_shadow=true;
            }
        } else {
            gl.DepthMask(!pass||(d.attributes&(1u<<11))?GL_TRUE:GL_FALSE);
            if(pass)gl.Enable(GL_BLEND);else gl.Disable(GL_BLEND);
            gl.StencilMask(0x3f);gl.StencilFunc(GL_ALWAYS,id,0x3f);gl.StencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);emit();
        }
    }
    gl.PolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    return ok("geometry");
}
bool resize(unsigned width,unsigned height) {
    if(width==target_width&&height==target_height&&framebuffer)return true;
    gl.BindTexture(GL_TEXTURE_2D,color_target);
    // llvmpipe's byte framebuffer quantizes source alpha before blending.
    // Keep DS (alpha+1)/32 blending in float, then quantize once on readback.
    gl.TexImage2D(GL_TEXTURE_2D,0,RGBA16F,width,height,0,BGRA,GL_UNSIGNED_BYTE,nullptr);
    gl.BindRenderbuffer(RBO,depth_target);gl.RenderbufferStorage(RBO,DEPTH24_STENCIL8,width,height);
    gl.BindFramebuffer(FBO,framebuffer);
    gl.FramebufferTexture2D(FBO,COLOR_ATTACHMENT,GL_TEXTURE_2D,color_target,0);
    gl.FramebufferRenderbuffer(FBO,DEPTH_STENCIL_ATTACHMENT,RBO,depth_target);
    if(gl.CheckFramebufferStatus(FBO)!=FBO_COMPLETE||!ok("render target"))return false;
    target_width=width;target_height=height;readback_ready=false;return true;
}
bool readback() {
    if(readback_ready)return true;
    if(!context||!target_width||!target_height)return false;
    raw_pixels.resize(size_t(target_width)*target_height);readback_pixels.resize(raw_pixels.size());
    gl.ReadPixels(0,0,target_width,target_height,BGRA,GL_UNSIGNED_BYTE,raw_pixels.data());
    if(!ok("readback"))return false;
    for(unsigned y=0;y<target_height;++y)
        std::copy_n(raw_pixels.data()+size_t(target_height-1-y)*target_width,target_width,readback_pixels.data()+size_t(y)*target_width);
    readback_ready=true;return true;
}
bool shaders() {
    const char *vertex=R"(#version 130
noperspective out vec4 color;
out vec2 uv;
void main(){gl_Position=gl_Vertex;color=gl_Color;uv=gl_MultiTexCoord0.xy;}
)";
    const char *fragment=R"(#version 130
uniform sampler2D image;
uniform int polygon,alpha_value,pixel_pass,texture_mode,highlight;
uniform vec3 toon[32];
noperspective in vec4 color;
in vec2 uv;
out vec4 output_color;
void main(){
    vec4 t=texture2D(image,uv);
    if(polygon==0){if(t.a<0.0001)discard;output_color=t*color;return;}
    float a=texture_mode==1?float(alpha_value):floor(((floor(t.a*31.0+0.5)+1.0)*(float(alpha_value)+1.0)-1.0)/32.0);
    if(a<0.5||(pixel_pass==1&&a<30.5)||(pixel_pass==2&&a>30.5))discard;
    vec3 c=texture_mode==1?mix(color.rgb,t.rgb,t.a):t.rgb*color.rgb;
    if(texture_mode==2){
        int intensity=min(511,int(floor(clamp(color.r,0.0,1.0)*511.0+0.0001)));
        vec3 shade=toon[intensity>>4];
        c=highlight!=0?clamp(t.rgb*(float(intensity>>3)/63.0)+shade,0.0,1.0):t.rgb*shade;
    }
    output_color=vec4(c,(a+1.0)/32.0);
}
)";
    GLuint vs=0,fs=0;
    auto compile=[&](GLenum type,const char *source) {
        GLuint id=gl.CreateShader(type);gl.ShaderSource(id,1,&source,nullptr);gl.CompileShader(id);
        GLint success=0;gl.GetShaderiv(id,COMPILE_STATUS,&success);
        if(!success){char log[2048]={};gl.GetShaderInfoLog(id,sizeof(log),nullptr,log);std::fprintf(stderr,"[OpenGL CPU] shader: %s\n",log);gl.DeleteShader(id);return GLuint(0);}
        return id;
    };
    vs=compile(VERTEX_SHADER,vertex);fs=compile(FRAGMENT_SHADER,fragment);
    if(!vs||!fs){if(vs)gl.DeleteShader(vs);if(fs)gl.DeleteShader(fs);return false;}
    program=gl.CreateProgram();gl.AttachShader(program,vs);gl.AttachShader(program,fs);gl.LinkProgram(program);
    gl.DeleteShader(vs);gl.DeleteShader(fs);GLint linked=0;gl.GetProgramiv(program,LINK_STATUS,&linked);
    if(!linked){char log[2048]={};gl.GetProgramInfoLog(program,sizeof(log),nullptr,log);std::fprintf(stderr,"[OpenGL CPU] link: %s\n",log);return false;}
    gl.UseProgram(program);
    gl.Uniform1i(gl.GetUniformLocation(program,"image"),0);
    u_polygon=gl.GetUniformLocation(program,"polygon");u_alpha=gl.GetUniformLocation(program,"alpha_value");
    u_pass=gl.GetUniformLocation(program,"pixel_pass");u_mode=gl.GetUniformLocation(program,"texture_mode");
    u_toon=gl.GetUniformLocation(program,"toon[0]");u_highlight=gl.GetUniformLocation(program,"highlight");
    return ok("shaders");
}
}

void close_worker();
bool open_worker(void *window) {
    close_worker();game_window=static_cast<HWND>(window);
    wchar_t path[32768]={};GetModuleFileNameW(nullptr,path,32768);
    auto *end=wcsrchr(path,L'\\');if(!end)return false;
    *end=0;std::wstring full=std::wstring(path)+L"\\renderers\\mesa\\opengl32.dll";
    library=LoadLibraryExW(full.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if(!library){std::fprintf(stderr,"[OpenGL CPU] Mesa runtime unavailable (%lu)\n",GetLastError());return false;}
    auto fail=[](){close_worker();return false;};
#define LOAD_WGL(name) gl.name=reinterpret_cast<decltype(gl.name)>(GetProcAddress(library,"wgl" #name));if(!gl.name)return fail();
    LOAD_WGL(ChoosePixelFormat) LOAD_WGL(SetPixelFormat) LOAD_WGL(CreateContext)
    LOAD_WGL(MakeCurrent) LOAD_WGL(DeleteContext) LOAD_WGL(GetProcAddress)
#undef LOAD_WGL
#define LOAD_BASE(name) gl.name=reinterpret_cast<decltype(gl.name)>(GetProcAddress(library,"gl" #name));if(!gl.name)return fail();
    GL_BASE(LOAD_BASE)
#undef LOAD_BASE
    // Scoped process settings: never installs a system driver or changes the registry.
    char previous[32768]={};DWORD len=GetEnvironmentVariableA("GALLIUM_DRIVER",previous,sizeof(previous));
    SetEnvironmentVariableA("GALLIUM_DRIVER","llvmpipe");
    WNDCLASSA cls{};cls.style=CS_OWNDC;cls.lpfnWndProc=DefWindowProcA;cls.hInstance=GetModuleHandleA(nullptr);cls.lpszClassName="SM64DSOpenGLSoftware";
    RegisterClassA(&cls);
    context_window=CreateWindowA(cls.lpszClassName,"",WS_POPUP,0,0,32,32,nullptr,nullptr,cls.hInstance,nullptr);
    context_dc=context_window?GetDC(context_window):nullptr;
    PIXELFORMATDESCRIPTOR p{};p.nSize=sizeof(p);p.nVersion=1;p.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;p.iPixelType=PFD_TYPE_RGBA;p.cColorBits=32;p.cDepthBits=24;p.cStencilBits=8;
    int format=context_dc?gl.ChoosePixelFormat(context_dc,&p):0;
    bool current=format&&gl.SetPixelFormat(context_dc,format,&p)&&(context=gl.CreateContext(context_dc))&&gl.MakeCurrent(context_dc,context);
    const char *renderer=current?reinterpret_cast<const char *>(gl.GetString(GL_RENDERER)):nullptr;
    const char *version=current?reinterpret_cast<const char *>(gl.GetString(GL_VERSION)):nullptr;
    if(renderer)std::snprintf(adapter_name,sizeof(adapter_name),"OpenGL CPU: %s",renderer);
    SetEnvironmentVariableA("GALLIUM_DRIVER",len&&len<sizeof(previous)?previous:nullptr);
    if(!renderer||!std::strstr(renderer,"llvmpipe")){std::fprintf(stderr,"[OpenGL CPU] CPU driver verification failed\n");return fail();}
#define LOAD_EXT(ret,name,args) gl.name=reinterpret_cast<decltype(gl.name)>(gl.GetProcAddress("gl" #name));if(!gl.name)return fail();
    GL_EXT(LOAD_EXT)
#undef LOAD_EXT
    if(!shaders())return fail();
    gl.GenFramebuffers(1,&framebuffer);gl.GenRenderbuffers(1,&depth_target);gl.GenTextures(1,&color_target);
    base_image=image(nullptr,SCREEN_W,SCREEN_H);map_image=image(nullptr,SCREEN_W,SCREEN_H);
    const uint32_t white=0xffffffff;white_image=image(&white,1,1);
    if(!base_image||!map_image||!white_image)return fail();
    const auto *extensions=reinterpret_cast<const char *>(gl.GetString(GL_EXTENSIONS));
    max_aniso=1;if(extensions&&std::strstr(extensions,"GL_EXT_texture_filter_anisotropic"))gl.GetFloatv(MAX_ANISO,&max_aniso);
    gl.BlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
    gl.Disable(GL_DITHER);gl.Disable(GL_CULL_FACE);
    epoch=texture_replacement_epoch();
    std::fprintf(stderr,"[OpenGL CPU] verified %s; %s\n",renderer,version);
    return ok("initialization");
}
void close_worker() {
    if(context) {
        gl.MakeCurrent(context_dc,context);
        for(auto &t:cache)gl.DeleteTextures(1,&t.second.id);
        if(base_image)gl.DeleteTextures(1,&base_image);if(map_image)gl.DeleteTextures(1,&map_image);
        if(white_image)gl.DeleteTextures(1,&white_image);if(color_target)gl.DeleteTextures(1,&color_target);
        if(framebuffer&&gl.DeleteFramebuffers)gl.DeleteFramebuffers(1,&framebuffer);
        if(gameplay_image)gl.DeleteTextures(1,&gameplay_image);
        if(gameplay_framebuffer&&gl.DeleteFramebuffers)gl.DeleteFramebuffers(1,&gameplay_framebuffer);
        if(depth_target&&gl.DeleteRenderbuffers)gl.DeleteRenderbuffers(1,&depth_target);
        if(program&&gl.DeleteProgram)gl.DeleteProgram(program);
        gl.MakeCurrent(nullptr,nullptr);gl.DeleteContext(context);
    }
    context=nullptr;
    if(context_dc&&context_window)ReleaseDC(context_window,context_dc);context_dc=nullptr;
    if(context_window)DestroyWindow(context_window);context_window=nullptr;
    if(library)FreeLibrary(library);library=nullptr;
    gl={};game_window=nullptr;framebuffer=color_target=depth_target=program=base_image=map_image=white_image=0;
    gameplay_image=gameplay_framebuffer=0;gameplay_width=gameplay_height=0;
    target_width=target_height=triangle_count=0;adapter_name[0]=0;cache.clear();missing.clear();cache_bytes=use_counter=0;
    readback_pixels.clear();raw_pixels.clear();readback_ready=false;anisotropic=0;stats={};
}
void glsw_set_menu_scene(bool active){menu_scene=active;}
void glsw_set_anisotropic_filtering(uint8_t value){anisotropic=value==2||value==4||value==8||value==16?uint8_t(std::min<float>(value,max_aniso)):0;}
uint8_t glsw_anisotropic_filtering(){return anisotropic;}
bool render_worker(const Framebuffer &base,const uint32_t *map,const VideoSettings &s,bool draw_geometry,
                 const uint32_t *foreground,const DualScreenLayout *screens,const Framebuffer *sub,bool engine_a_on_top,bool cache_gameplay) {
    stats={};auto start=std::chrono::steady_clock::now();
    if(!context||s.width<256||s.height<192||s.width>3840||s.height>2160||!resize(s.width,s.height))return false;
    readback_ready=false;gl.UseProgram(program);gl.BindFramebuffer(FBO,framebuffer);
    if(epoch!=texture_replacement_epoch()){
        gl.Finish();
        for(auto it=cache.begin();it!=cache.end();)if(std::get<0>(it->first)){gl.DeleteTextures(1,&it->second.id);cache_bytes-=it->second.bytes;it=cache.erase(it);}else ++it;
        epoch=texture_replacement_epoch();missing.clear();
    }
    gl.DepthMask(GL_TRUE);gl.StencilMask(255);gl.ColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    gl.ClearColor(0,0,0,1);gl.ClearDepth(1);gl.ClearStencil(0);gl.Clear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
    viewport(screens?(engine_a_on_top?screens->top:screens->bottom):ScreenRect{0,0,int(s.width),int(s.height)});
    static Framebuffer opaque;
    for(unsigned y=0;y<SCREEN_H;++y)for(unsigned x=0;x<SCREEN_W;++x)opaque.px[y][x]=base.px[y][x]|0xff000000u;
    gl.BindTexture(GL_TEXTURE_2D,base_image);gl.TexSubImage2D(GL_TEXTURE_2D,0,0,0,SCREEN_W,SCREEN_H,BGRA,GL_UNSIGNED_BYTE,opaque.px);
    overlay_state(false);quad(base_image,!s.upscale_2d);triangle_count=0;
    if(draw_geometry&&!geometry(screens!=nullptr))return false;
    if(draw_geometry&&foreground){
        gl.BindTexture(GL_TEXTURE_2D,base_image);gl.TexSubImage2D(GL_TEXTURE_2D,0,0,0,SCREEN_W,SCREEN_H,BGRA,GL_UNSIGNED_BYTE,foreground);
        overlay_state(true);quad(base_image);
    }
    if(screens){
        if(!sub)return false;
        viewport(engine_a_on_top?screens->bottom:screens->top);
        for(unsigned y=0;y<SCREEN_H;++y)for(unsigned x=0;x<SCREEN_W;++x)opaque.px[y][x]=sub->px[y][x]|0xff000000u;
        gl.BindTexture(GL_TEXTURE_2D,map_image);gl.TexSubImage2D(GL_TEXTURE_2D,0,0,0,SCREEN_W,SCREEN_H,BGRA,GL_UNSIGNED_BYTE,opaque.px);
        overlay_state(false);quad(map_image,!s.upscale_2d);
    }
    if(cache_gameplay && !screens) {
        if(!gameplay_image)gl.GenTextures(1,&gameplay_image);
        gl.BindTexture(GL_TEXTURE_2D,gameplay_image);
        if(gameplay_width!=target_width || gameplay_height!=target_height) {
            gl.TexImage2D(GL_TEXTURE_2D,0,RGBA16F,target_width,target_height,0,BGRA,GL_UNSIGNED_BYTE,nullptr);
            gameplay_width=target_width;gameplay_height=target_height;
        }
        gl.CopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,target_width,target_height);
    }
    if(!screens&&map&&s.minimap){
        gl.BindTexture(GL_TEXTURE_2D,map_image);gl.TexSubImage2D(GL_TEXTURE_2D,0,0,0,SCREEN_W,SCREEN_H,BGRA,GL_UNSIGNED_BYTE,map);
        overlay_state(true);
        const float x=s.minimap_x/1000.f,y=s.minimap_y/1000.f,size=s.minimap_scale/1000.f;
        quad(map_image,false,x*2-1,1-y*2,(x+size)*2-1,1-(y+size)*2);
    }
    stats.cache_bytes=cache_bytes;stats.cache_budget=cache_budget;
    stats.cpu_us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count();
    return ok("frame");
}
bool present_worker(unsigned width,unsigned height,bool preserve_aspect){
    if(!game_window||!width||!height||!readback())return false;
    HDC dc=GetDC(game_window);if(!dc)return false;
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=target_width;
    info.bmiHeader.biHeight=-LONG(target_height);info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    const auto r=preserve_aspect?screen_fit(target_width,target_height,{0,0,int(width),int(height)}):ScreenRect{0,0,int(width),int(height)};
    RECT black{0,0,LONG(width),LONG(height)};if(preserve_aspect)FillRect(dc,&black,static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    SetStretchBltMode(dc,COLORONCOLOR);
    int result=StretchDIBits(dc,r.x,r.y,r.width,r.height,0,0,target_width,target_height,readback_pixels.data(),&info,DIB_RGB_COLORS,SRCCOPY);
    ReleaseDC(game_window,dc);return result!=GDI_ERROR;
}
bool readback_worker(std::vector<uint32_t> &pixels,unsigned &width,unsigned &height){
    if(!readback())return false;pixels=readback_pixels;width=target_width;height=target_height;return true;
}

namespace {
// Created after io_init reserves the DS address ranges. A large main-thread
// stack can overlap those low addresses before main even starts. Mesa's LLVM
// compiler instead gets a dedicated stack whose allocation respects them.
struct Worker {
    HANDLE thread=nullptr,wake=nullptr,done=nullptr;
    std::function<void()> job;
    bool stop=false;
} worker;
DWORD WINAPI render_thread(void *) {
    for(;;) {
        if(WaitForSingleObject(worker.wake,INFINITE)!=WAIT_OBJECT_0)return 1;
        if(worker.stop)break;
        try {worker.job();}
        catch(...) {std::fprintf(stderr,"[OpenGL CPU] renderer task failed\n");}
        worker.job={};SetEvent(worker.done);
    }
    return 0;
}
bool submit(std::function<void()> job) {
    if(!worker.thread)return false;
    worker.job=std::move(job);
    return SetEvent(worker.wake)&&WaitForSingleObject(worker.done,INFINITE)==WAIT_OBJECT_0;
}
}
bool glsw_open(void *window) {
    glsw_close();
    worker.wake=CreateEventA(nullptr,FALSE,FALSE,nullptr);
    worker.done=CreateEventA(nullptr,FALSE,FALSE,nullptr);
    if(worker.wake&&worker.done)
        worker.thread=CreateThread(nullptr,8*1024*1024,render_thread,nullptr,STACK_SIZE_PARAM_IS_A_RESERVATION,nullptr);
    if(!worker.thread){glsw_close();return false;}
    bool result=false;submit([&]{result=open_worker(window);});
    if(!result)glsw_close();
    return result;
}
void glsw_close() {
    if(worker.thread) {
        submit([]{close_worker();});worker.stop=true;SetEvent(worker.wake);
        WaitForSingleObject(worker.thread,INFINITE);CloseHandle(worker.thread);
    }
    if(worker.wake)CloseHandle(worker.wake);
    if(worker.done)CloseHandle(worker.done);
    worker.thread=worker.wake=worker.done=nullptr;worker.stop=false;worker.job={};
}
bool glsw_render(const Framebuffer &base,const uint32_t *map,const VideoSettings &s,bool geometry,
                 const uint32_t *foreground,const DualScreenLayout *screens,const Framebuffer *sub,bool top,bool cache_gameplay) {
    bool result=false;return submit([&]{result=render_worker(base,map,s,geometry,foreground,screens,sub,top,cache_gameplay);})&&result;
}
bool glsw_present(unsigned width,unsigned height,bool aspect) {
    bool result=false;return submit([&]{result=present_worker(width,height,aspect);})&&result;
}
bool glsw_readback(std::vector<uint32_t> &pixels,unsigned &width,unsigned &height) {
    bool result=false;return submit([&]{result=readback_worker(pixels,width,height);})&&result;
}
bool glsw_readback_gameplay(std::vector<uint32_t> &pixels,unsigned &width,unsigned &height) {
    bool result=false;
    return submit([&]{
        if(!gameplay_image || !gameplay_width || !gameplay_height)return;
        if(!gameplay_framebuffer)gl.GenFramebuffers(1,&gameplay_framebuffer);
        gl.BindFramebuffer(FBO,gameplay_framebuffer);
        gl.FramebufferTexture2D(FBO,COLOR_ATTACHMENT,GL_TEXTURE_2D,gameplay_image,0);
        if(gl.CheckFramebufferStatus(FBO)==FBO_COMPLETE) {
            width=gameplay_width;height=gameplay_height;
            std::vector<uint32_t> raw(size_t(width)*height);pixels.resize(raw.size());
            gl.ReadPixels(0,0,width,height,BGRA,GL_UNSIGNED_BYTE,raw.data());
            for(unsigned y=0;y<height;++y)std::copy_n(raw.data()+size_t(height-1-y)*width,width,pixels.data()+size_t(y)*width);
            result=ok("gameplay preview readback");
        }
        gl.BindFramebuffer(FBO,framebuffer);
    })&&result;
}

const char *glsw_adapter(){return adapter_name;}
unsigned glsw_triangle_count(){return triangle_count;}
GpuFrameStats glsw_frame_stats(){return stats;}
void glsw_set_texture_budget_bytes(uint64_t bytes){cache_budget=std::max<uint64_t>(4096ull*4096*4,bytes);}
void glsw_force_texture_scan(bool force){force_scan=force;}
}
#endif

