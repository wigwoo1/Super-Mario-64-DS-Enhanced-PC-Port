// Native-resolution D3D11 rasterization of the recovered game's geometry.
// DS command decode, lighting, clipping, UVs and original textures stay shared
// with the software reference. Only rasterization/presentation run on the GPU.
#include "ntr/gpu.h"
#include "ntr/gl_software.h"
#include "ntr/gx.h"
#include "fsr_bridge.h"
#include <chrono>
#include "ntr/rt.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <map>
#include <set>
#include <tuple>
#include <thread>
#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>

namespace ntr {
namespace {
using Microsoft::WRL::ComPtr;
struct Vertex { float p[4],color[4],uv[2],previous[4]; };
struct Texture {
    ComPtr<ID3D11Texture2D> resource; ComPtr<ID3D11ShaderResourceView> view;
    bool partial_alpha=false;unsigned width=0,height=0;uint64_t bytes=0,last_used=0;
};
struct FrameMaterial {
    const uint32_t *pixels=nullptr;int width=0,height=0;
    TexturePixelsInfo info;TextureReplacementRef replacement;
};
struct Group { FrameMaterial *material; unsigned first,count; uint32_t attributes; bool partial_alpha=false; uint8_t texture_address=3; };
ComPtr<ID3D11Device> device;
ComPtr<ID3D11DeviceContext> context;
ComPtr<IDXGISwapChain> swap;
ComPtr<IDXGISwapChain3> flip_swap;
std::array<ComPtr<ID3D11Resource>,2> flip_buffers;
std::array<ComPtr<ID3D11RenderTargetView>,2> flip_views;
struct RasterTarget {
    ComPtr<ID3D11Texture2D> color,depth;
    ComPtr<ID3D11RenderTargetView> color_view;
    ComPtr<ID3D11DepthStencilView> depth_view;
    ComPtr<ID3D11ShaderResourceView> color_sample,depth_sample;
    unsigned width=0,height=0;
};
RasterTarget output_raster,upscale_raster;
RasterTarget *raster=&output_raster;
ComPtr<ID3D11RenderTargetView> window_view;
ComPtr<ID3D11Texture2D> staging,gameplay_cache;
ComPtr<ID3D11VertexShader> vertex_shader;
ComPtr<ID3D11PixelShader> pixel_shader;
ComPtr<ID3D11PixelShader> scene_motion_shader;
ComPtr<ID3D11PixelShader> scene_reactive_shader;
ComPtr<ID3D11Texture2D> scene_surface_motion;
ComPtr<ID3D11Texture2D> scene_reactive,scene_opaque_color;
ComPtr<ID3D11RenderTargetView> scene_surface_motion_view;
ComPtr<ID3D11RenderTargetView> scene_reactive_view;
ComPtr<ID3D11ShaderResourceView> scene_surface_motion_sample;
unsigned scene_target_width=0,scene_target_height=0;
ComPtr<ID3D11PixelShader> motion_shader;
ComPtr<ID3D11InputLayout> layout;
ComPtr<ID3D11Buffer> vertex_buffer;
ComPtr<ID3D11Buffer> toon_params;
ComPtr<ID3D11Buffer> draw_params;
ComPtr<ID3D11Buffer> motion_params;
ComPtr<ID3D11Query> retirement_query;
ComPtr<ID3D11SamplerState> texture_samplers[16],anisotropic_samplers[4][16],nearest_clamp,linear_clamp;
ComPtr<ID3D11DepthStencilState> depth_on,depth_off;
ComPtr<ID3D11DepthStencilState> normal_depth[2][2],translucent_depth[2][2],shadow_mask[2],shadow_draw[2],shadow_exclude,shadow_clear;
ComPtr<ID3D11BlendState> blend_on,blend_off,color_off,blend_reactive;
ComPtr<ID3D11RasterizerState> rasterizer,wireframe;
Texture base_texture,map_texture,packed_map_texture,white_texture,hud_texture;
Texture motion_source;
struct MotionHistory {
    GxCameraFrame current{};
    DirectX::XMFLOAT4X4 reprojection{};
    float exposure=0;
} motion_history;
struct SceneCameraHistory {
    GxCameraFrame current{};
    GxCameraPrediction prediction{},previous_prediction{};
    DirectX::XMFLOAT4X4 reprojection{};
    bool valid=false,same_geometry=false;
} scene_camera_history;
struct SceneSampleOffset { float x=0,y=0; };
struct SceneDepthHistory {
    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11ShaderResourceView> sample;
    uint64_t frame=0,serial=0;
    SceneSampleOffset offset;
} scene_depth_history;
struct LiveFsrProbe {
    bool device=false,running=false,attempted=false,failed=false,bypassed=false;
    bool infinite_depth=false;
    unsigned frames=0;
    uint64_t last_frame=0,last_serial=0;
    float last_phase=0;
    GxCameraPrediction last_prediction{};
    unsigned width=0,height=0;
    unsigned phases=0;
    uint64_t started_tick=0;
    FsrMode mode=FsrMode::Off;
} live_fsr;
enum class FsrPresentation { Inactive, WarmUp, NewScene, RepeatScene };
ComPtr<ID3D11ComputeShader> scene_input_shaders[2];
struct SceneInputTargets {
    static constexpr unsigned live_count=3;
    static constexpr unsigned count=6;
    ComPtr<ID3D11Texture2D> textures[count];
    ComPtr<ID3D11UnorderedAccessView> views[count];
    ComPtr<ID3D11Buffer> camera;
    unsigned width=0,height=0;
    bool diagnostics=false;
} scene_inputs;
bool motion_failed=false;
void reset_scene_capture_targets();
uint64_t hud_texture_revision=~uint64_t(0);
using CacheKey=std::tuple<bool,uint64_t,uint32_t>;
std::map<CacheKey,Texture> texture_cache;
std::set<CacheKey> unavailable_replacements;
uint64_t cache_bytes=0,cache_budget=256ull*1024*1024,use_counter=0;
uint32_t cache_epoch=0;
GpuFrameStats frame_stats;
bool force_texture_scan=false;
bool using_gl_software=false;
uint8_t anisotropic_level=0;
unsigned window_width,window_height,buffer_capacity,triangles_drawn;
char adapter_name[160];

bool hr_ok(HRESULT hr, const char *operation) {
    if (SUCCEEDED(hr)) return true;
    std::fprintf(stderr,"[native GPU] %s failed: 0x%08lx\n",operation,static_cast<unsigned long>(hr));
    return false;
}
bool texture(Texture &out, const uint32_t *pixels, unsigned width, unsigned height,
             const TexturePixelsInfo *info=nullptr) {
    out.partial_alpha=false;
    if(info && info->immutable)out.partial_alpha=info->partial_alpha;
    else if(pixels) for(size_t i=0;i<static_cast<size_t>(width)*height;++i)
        out.partial_alpha|=(pixels[i]>>24)>0 && (pixels[i]>>24)<255;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width=width; desc.Height=height; desc.MipLevels=desc.ArraySize=1;
    desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM; desc.SampleDesc.Count=1;
    desc.Usage=D3D11_USAGE_DEFAULT; desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data{pixels,width*4,0};
    if(!hr_ok(device->CreateTexture2D(&desc,pixels?&data:nullptr,&out.resource),"texture") ||
        !hr_ok(device->CreateShaderResourceView(out.resource.Get(),nullptr,&out.view),"texture view"))return false;
    out.width=width;out.height=height;return true;
}
bool ensure_raster(RasterTarget &destination,unsigned width,unsigned height) {
    if(destination.color && destination.width==width && destination.height==height)return true;
    RasterTarget next;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width=width; desc.Height=height; desc.MipLevels=desc.ArraySize=1;
    desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM; desc.SampleDesc.Count=1;
    desc.Usage=D3D11_USAGE_DEFAULT;
    desc.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
    if (!hr_ok(device->CreateTexture2D(&desc,nullptr,&next.color),"render target") ||
        !hr_ok(device->CreateRenderTargetView(next.color.Get(),nullptr,&next.color_view),"render view") ||
        !hr_ok(device->CreateShaderResourceView(next.color.Get(),nullptr,&next.color_sample),"render sample")) return false;
    desc.Format=DXGI_FORMAT_R24G8_TYPELESS;
    desc.BindFlags=D3D11_BIND_DEPTH_STENCIL|D3D11_BIND_SHADER_RESOURCE;
    D3D11_DEPTH_STENCIL_VIEW_DESC dsv{};
    dsv.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsv.ViewDimension=D3D11_DSV_DIMENSION_TEXTURE2D;
    if(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&next.depth)) &&
       SUCCEEDED(device->CreateDepthStencilView(next.depth.Get(),&dsv,&next.depth_view))) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srv{};
        srv.Format=DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        srv.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;
        srv.Texture2D.MipLevels=1;
        if(FAILED(device->CreateShaderResourceView(next.depth.Get(),&srv,&next.depth_sample)))next.depth_sample.Reset();
    } else {
        next.depth.Reset();next.depth_view.Reset();
        desc.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;desc.BindFlags=D3D11_BIND_DEPTH_STENCIL;
        if(!hr_ok(device->CreateTexture2D(&desc,nullptr,&next.depth),"depth target") ||
           !hr_ok(device->CreateDepthStencilView(next.depth.Get(),nullptr,&next.depth_view),"depth view"))return false;
    }
    next.width=width;next.height=height;
    destination=std::move(next);
    std::fprintf(stderr,"[native GPU] raster target %ux%u, adapter %s\n",width,height,adapter_name);
    return true;
}
bool resize_target(unsigned width,unsigned height) {
    if(output_raster.color && output_raster.width==width && output_raster.height==height)return true;
    context->OMSetRenderTargets(0,nullptr,nullptr);
    ID3D11ShaderResourceView *null_view=nullptr;
    context->PSSetShaderResources(0,1,&null_view);
    staging.Reset();upscale_raster={};
    motion_source={};motion_history={};scene_camera_history={};scene_depth_history={};
    reset_scene_capture_targets();
    return ensure_raster(output_raster,width,height);
}
Vertex vertex(float x,float y,float z,float w,float u,float v,uint32_t color) {
    Vertex out{{x,y,z,w},{((color>>16)&255)/255.0f,((color>>8)&255)/255.0f,
        (color&255)/255.0f,((color>>24)&255)/255.0f},{u,v},{}};
    return out;
}
bool upload(const std::vector<Vertex> &vertices) {
    if (vertices.empty()) return true;
    if (vertices.size()>buffer_capacity) {
        vertex_buffer.Reset(); buffer_capacity=static_cast<unsigned>(vertices.size()+1024);
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth=buffer_capacity*sizeof(Vertex); desc.Usage=D3D11_USAGE_DYNAMIC;
        desc.BindFlags=D3D11_BIND_VERTEX_BUFFER; desc.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
        if (!hr_ok(device->CreateBuffer(&desc,nullptr,&vertex_buffer),"vertex buffer")) return false;
    }
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (!hr_ok(context->Map(vertex_buffer.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&mapped),"vertex upload")) return false;
    std::memcpy(mapped.pData,vertices.data(),vertices.size()*sizeof(Vertex));
    context->Unmap(vertex_buffer.Get(),0);
    ID3D11Buffer *buffer=vertex_buffer.Get(); const UINT stride=sizeof(Vertex),offset=0;
    context->IASetVertexBuffers(0,1,&buffer,&stride,&offset);
    return true;
}
void bind_texture(ID3D11ShaderResourceView *view,ID3D11SamplerState *sampler) {
    context->PSSetShaderResources(0,1,&view);
    context->PSSetSamplers(0,1,&sampler);
}
void parameters(unsigned alpha=31,unsigned filter=0,bool polygon=false,unsigned mode=0) {
    const unsigned values[4]={alpha,filter,polygon?1u:0u,mode};
    context->UpdateSubresource(draw_params.Get(),0,nullptr,values,0,0);
    ID3D11Buffer *buffer=draw_params.Get();context->PSSetConstantBuffers(0,1,&buffer);
}
bool quad(ID3D11ShaderResourceView *view,ID3D11SamplerState *sampler,
          float left=-1,float top=1,float right=1,float bottom=-1,bool opaque=false) {
    std::vector<Vertex> v{
        vertex(left,top,0,1,0,0,0xffffffff),vertex(right,top,0,1,1,0,0xffffffff),
        vertex(left,bottom,0,1,0,1,0xffffffff),vertex(left,bottom,0,1,0,1,0xffffffff),
        vertex(right,top,0,1,1,0,0xffffffff),vertex(right,bottom,0,1,1,1,0xffffffff)};
    if (!upload(v)) return false;
    parameters(31,0,false,opaque?4:0);bind_texture(view,sampler); context->Draw(6,0); return true;
}

bool draw_fsr_output(ID3D11ShaderResourceView *resolved) {
    if(!resolved)return false;
    ID3D11RenderTargetView *render_view=output_raster.color_view.Get();
    context->OMSetRenderTargets(1,&render_view,output_raster.depth_view.Get());
    const D3D11_VIEWPORT viewport{0,0,float(output_raster.width),float(output_raster.height),0,1};
    context->RSSetViewports(1,&viewport);
    context->PSSetShader(pixel_shader.Get(),nullptr,0);
    context->OMSetDepthStencilState(depth_off.Get(),0);
    context->OMSetBlendState(blend_off.Get(),nullptr,~0u);
    const bool drawn=quad(resolved,nearest_clamp.Get(),-1,1,1,-1,true);
    ID3D11ShaderResourceView *empty=nullptr;
    context->PSSetShaderResources(0,1,&empty);
    fsr_bridge_release_output();
    return drawn;
}

bool prepare_motion_pass() {
    if(motion_failed || !raster->depth_sample)return false;
    if(motion_shader && motion_params && motion_source.resource)return true;
    if(!motion_shader) {
        constexpr char shader[]=R"(
Texture2D scene_color : register(t0);
Texture2D<float> scene_depth : register(t1);
SamplerState linear_color : register(s0);
cbuffer MotionParameters : register(b2) {
    row_major float4x4 current_to_previous;
    float2 dimensions;
    float exposure;
    float max_pixels;
};
float4 main(float4 pixel:SV_POSITION) : SV_TARGET {
    const float2 uv=pixel.xy/dimensions;
    const float4 center=scene_color.Sample(linear_color,uv);
    const float depth=scene_depth.Load(int3(int2(pixel.xy),0));
    if(depth>=0.9999999) return center;
    const float4 projected=mul(float4(uv*float2(2,-2)+float2(-1,1),depth,1),current_to_previous);
    if(projected.w<=0.0001) return center;
    float2 velocity=(projected.xy/projected.w*float2(0.5,-0.5)+0.5-uv)*exposure;
    const float length_pixels=length(velocity*dimensions);
    if(length_pixels<0.65) return center;
    velocity*=min(1.0,max_pixels/length_pixels);
    float4 total=center*5;
    float weight=5;
    [unroll] for(int i=1;i<=4;++i) {
        const float2 at=uv+velocity*(i/4.0);
        if(any(at<=0.0) || any(at>=1.0)) continue;
        const float z=scene_depth.Load(int3(int2(at*dimensions),0));
        if(z>=0.9999999 || abs(z-depth)>max(0.0008,(1-depth)*0.12)) continue;
        total+=scene_color.Sample(linear_color,at)*(5-i);
        weight+=5-i;
    }
    return total/weight;
})";
        ComPtr<ID3DBlob> code,errors;
        if(FAILED(D3DCompile(shader,sizeof(shader)-1,nullptr,nullptr,nullptr,"main","ps_4_0",
                             D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&errors))) {
            if(errors)std::fprintf(stderr,"[motion blur] %s",static_cast<const char *>(errors->GetBufferPointer()));
        } else device->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&motion_shader);
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth=80;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        if(FAILED(device->CreateBuffer(&desc,nullptr,&motion_params)))motion_params.Reset();
    }
    if(!motion_shader || !motion_params || !texture(motion_source,nullptr,raster->width,raster->height)) {
        motion_failed=true;
        motion_source={};
        std::fprintf(stderr,"[motion blur] unavailable on this device\n");
        return false;
    }
    return true;
}

bool build_camera_reprojection(const GxCameraFrame &camera,const GxCameraFrame &previous,
                               DirectX::XMFLOAT4X4 &reprojection,bool same_geometry=false) {
    if(!camera.owner || camera.owner!=previous.owner || camera.scene!=previous.scene ||
       camera.tick<previous.tick || (camera.tick==previous.tick && !same_geometry) ||
       camera.tick-previous.tick>8)return false;
    float distance=0,alignment=0;
    for(unsigned i=0;i<3;++i){
        const float delta=camera.eye[i]-previous.eye[i];distance+=delta*delta;
        alignment+=camera.forward[i]*previous.forward[i];
    }
    if(distance>64*64 || alignment<0.85f)return false; // Camera cut.
    if(camera.view_projection==previous.view_projection) {
        DirectX::XMStoreFloat4x4(&reprojection,DirectX::XMMatrixIdentity());
        return true;
    }
    const float aspect=float(SCREEN_W)*output_raster.height/(float(SCREEN_H)*output_raster.width);
    const auto adjusted=[aspect](const GxCameraFrame &frame){
        DirectX::XMFLOAT4X4 m;
        std::memcpy(m.m,frame.view_projection.data(),sizeof(m.m));
        for(auto &row:m.m) {
            row[0]*=aspect;
            row[2]=(row[2]+row[3])*0.5f;
        }
        return DirectX::XMLoadFloat4x4(&m);
    };
    DirectX::XMVECTOR determinant;
    const auto inverse=DirectX::XMMatrixInverse(&determinant,adjusted(camera));
    const float det=DirectX::XMVectorGetX(determinant);
    if(!std::isfinite(det) || std::fabs(det)<1e-8f)return false;
    DirectX::XMStoreFloat4x4(&reprojection,DirectX::XMMatrixMultiply(inverse,adjusted(previous)));
    return true;
}

void update_motion_history(const GxCameraFrame &camera) {
    if(camera.serial==motion_history.current.serial)return;
    const auto previous=motion_history.current;
    motion_history.current=camera;
    motion_history.exposure=0;
    if(camera.view_projection==previous.view_projection ||
       !build_camera_reprojection(camera,previous,motion_history.reprojection))return;
    motion_history.exposure=std::min(0.75f,1.0f/float(camera.tick-previous.tick));
}

void draw_motion_blur(bool enabled) {
    if(!enabled) {
        motion_history={};motion_source={};
        return;
    }
    update_motion_history(gx_display_camera());
    if(motion_history.exposure==0 || !prepare_motion_pass())return;
    context->OMSetRenderTargets(0,nullptr,nullptr);
    context->CopyResource(motion_source.resource.Get(),raster->color.Get());
    ID3D11RenderTargetView *rt=raster->color_view.Get();
    context->OMSetRenderTargets(1,&rt,nullptr);
    context->OMSetDepthStencilState(depth_off.Get(),0);
    context->OMSetBlendState(blend_off.Get(),nullptr,~0u);
    context->PSSetShader(motion_shader.Get(),nullptr,0);
    ID3D11ShaderResourceView *z=raster->depth_sample.Get();
    context->PSSetShaderResources(1,1,&z);
    struct Parameters {DirectX::XMFLOAT4X4 matrix;float width,height,exposure,max_pixels;};
    static_assert(sizeof(Parameters)==80,"Motion shader constant layout");
    const Parameters values{motion_history.reprojection,float(raster->width),float(raster->height),
                            motion_history.exposure,std::max(4.0f,raster->height/48.0f)};
    context->UpdateSubresource(motion_params.Get(),0,nullptr,&values,0,0);
    ID3D11Buffer *buffer=motion_params.Get();
    context->PSSetConstantBuffers(2,1,&buffer);
    frame_stats.motion_blur_applied=quad(motion_source.view.Get(),linear_clamp.Get());
    ID3D11ShaderResourceView *null_views[2]={nullptr,nullptr};
    context->PSSetShaderResources(0,2,null_views);
    context->PSSetShader(pixel_shader.Get(),nullptr,0);
}

// Convert the scene's BGRA color and D24 depth into resources suitable for a
// temporal pass. The capture is opt-in and reads back only the requested frame.
bool write_scene_input(ID3D11Texture2D *input,const char *prefix,const char *suffix,size_t pixel_bytes=4) {
    D3D11_TEXTURE2D_DESC desc{};
    input->GetDesc(&desc);
    desc.Usage=D3D11_USAGE_STAGING;
    desc.BindFlags=0;
    desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> readback;
    if(!hr_ok(device->CreateTexture2D(&desc,nullptr,&readback),"scene input readback"))return false;
    context->CopyResource(readback.Get(),input);
    char path[2048];
    const int length=std::snprintf(path,sizeof(path),"%s%s",prefix,suffix);
    if(length<0 || size_t(length)>=sizeof(path))return false;
    FILE *file=std::fopen(path,"wb");
    if(!file)return false;
    D3D11_MAPPED_SUBRESOURCE mapped{};
    bool written=hr_ok(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped),"scene input map");
    if(written) {
        const auto *bytes=static_cast<const unsigned char *>(mapped.pData);
        for(unsigned y=0;y<desc.Height && written;++y)
            written=std::fwrite(bytes+size_t(y)*mapped.RowPitch,pixel_bytes,desc.Width,file)==desc.Width;
        context->Unmap(readback.Get(),0);
    }
    return std::fclose(file)==0 && written;
}

bool prepare_scene_input_shader(bool diagnostics=false) {
    auto &shader=scene_input_shaders[diagnostics];
    if(shader)return true;
    constexpr char source[]=R"(
        Texture2D<float4> source_color : register(t0);
        Texture2D<float> source_depth : register(t1);
        Texture2D<float4> surface_motion : register(t2);
        RWTexture2D<float4> scene_color : register(u0);
        RWTexture2D<float> scene_depth : register(u1);
        RWTexture2D<float2> scene_motion : register(u2);
        #if DIAGNOSTIC_CAPTURE
        Texture2D<float> previous_depth : register(t3);
        RWTexture2D<float2> camera_motion : register(u3);
        RWTexture2D<float> history_reject : register(u4);
        RWTexture2D<float> history_depth : register(u5);
        #endif
        cbuffer SceneCamera : register(b0) {
            row_major float4x4 current_to_previous;
            uint camera_valid;
            uint history_valid;
            uint2 padding;
            float2 current_jitter;
            float2 previous_jitter;
        };
        [numthreads(8,8,1)]
        void main(uint3 id : SV_DispatchThreadID) {
            uint width, height;
            scene_color.GetDimensions(width,height);
            if(id.x>=width || id.y>=height)return;
            scene_color[id.xy]=source_color.Load(int3(id.xy,0));
            float depth=source_depth.Load(int3(id.xy,0));
            scene_depth[id.xy]=depth;
            const float2 dimensions=float2(width,height);
            // The stable motion buffer measures scene motion in an unjittered
            // raster. History depth was actually rasterized with its offset.
            const float2 uv=(float2(id.xy)+0.5-current_jitter)/dimensions;
            float2 motion=0;
            float4 previous=0;
            if(camera_valid && depth<0.9999999) {
                previous=mul(float4(uv*float2(2,-2)+float2(-1,1),depth,1),current_to_previous);
                if(previous.w>0.0001) {
                    const float2 previous_uv=previous.xy/previous.w*float2(0.5,-0.5)+0.5;
                    motion=(previous_uv-uv)*dimensions;
                }
            }
            #if DIAGNOSTIC_CAPTURE
            camera_motion[id.xy]=motion;
            #endif
            // Use a surface vector only if this opaque fragment still owns
            // the depth after the later translucent and shadow passes.
            const float4 surface=surface_motion.Load(int3(id.xy,0));
            if(camera_valid && surface.w>0 && abs(surface.w-1-depth)<0.000001) {
                #if DIAGNOSTIC_CAPTURE
                previous=float4(surface.xyz,1);
                #endif
                const float2 previous_uv=surface.xy*float2(0.5,-0.5)+0.5;
                motion=(previous_uv-uv)*dimensions;
            }
            scene_motion[id.xy]=motion;
            #if DIAGNOSTIC_CAPTURE
            float reject=1;
            if(history_valid && previous.w>0.0001) {
                const float3 projected=previous.xyz/previous.w;
                const float2 previous_uv=projected.xy*float2(0.5,-0.5)+0.5+previous_jitter/dimensions;
                if(all(previous_uv>=0) && all(previous_uv<1) && projected.z>=0 && projected.z<1) {
                    const float recorded=previous_depth.Load(int3(int2(previous_uv*dimensions),0));
                    // Point sampling avoids blending across silhouette edges.
                    // Compare non-linear D24 depth in the same projection.
                    reject=abs(recorded-projected.z)>0.00015?1:0;
                }
            }
            history_reject[id.xy]=reject;
            history_depth[id.xy]=history_valid?previous_depth.Load(int3(id.xy,0)):1;
            #endif
        }
    )";
    ComPtr<ID3DBlob> shader_code;
    const D3D_SHADER_MACRO macros[]={{"DIAGNOSTIC_CAPTURE",diagnostics?"1":"0"},{nullptr,nullptr}};
    return hr_ok(D3DCompile(source,sizeof(source)-1,nullptr,macros,nullptr,"main","cs_5_0",
                           D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&shader_code,nullptr),"scene input shader compile") &&
           hr_ok(device->CreateComputeShader(shader_code->GetBufferPointer(),shader_code->GetBufferSize(),
                                             nullptr,&shader),"scene input shader");
}

struct SceneCameraParameters {
    DirectX::XMFLOAT4X4 matrix;
    unsigned valid,depth_valid,padding[2];
    SceneSampleOffset current_jitter,previous_jitter;
};
static_assert(sizeof(SceneCameraParameters)==96,"Scene camera shader constant layout");

bool prepare_scene_input_targets(unsigned width,unsigned height,bool diagnostics) {
    if(scene_inputs.width==width && scene_inputs.height==height && scene_inputs.diagnostics==diagnostics)return true;
    SceneInputTargets next;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width=width;desc.Height=height;
    desc.MipLevels=desc.ArraySize=1;
    desc.SampleDesc.Count=1;
    desc.Usage=D3D11_USAGE_DEFAULT;
    desc.BindFlags=D3D11_BIND_UNORDERED_ACCESS|D3D11_BIND_SHADER_RESOURCE;
    const DXGI_FORMAT formats[]={DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R32_FLOAT,
                                 DXGI_FORMAT_R32G32_FLOAT,DXGI_FORMAT_R32G32_FLOAT,
                                 DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R32_FLOAT};
    const unsigned count=diagnostics?SceneInputTargets::count:SceneInputTargets::live_count;
    for(unsigned i=0;i<count;++i) {
        desc.Format=formats[i];
        if(!hr_ok(device->CreateTexture2D(&desc,nullptr,&next.textures[i]),"scene input texture") ||
           !hr_ok(device->CreateUnorderedAccessView(next.textures[i].Get(),nullptr,&next.views[i]),"scene input view"))
            return false;
    }
    D3D11_BUFFER_DESC constants{};
    constants.ByteWidth=sizeof(SceneCameraParameters);
    constants.Usage=D3D11_USAGE_DEFAULT;
    constants.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
    if(!hr_ok(device->CreateBuffer(&constants,nullptr,&next.camera),"scene input camera constants"))
        return false;
    next.width=width;next.height=height;next.diagnostics=diagnostics;
    scene_inputs=std::move(next);
    return true;
}

bool capture_scene_inputs(const char *prefix,SceneSampleOffset offset,const FsrFrame *live_frame=nullptr,
                          bool warm_up=false) {
    const bool diagnostics=prefix!=nullptr;
    if(!raster->color_sample || !raster->depth_sample || !scene_surface_motion_sample || !scene_reactive ||
       (diagnostics && !scene_opaque_color) || !prepare_scene_input_shader(diagnostics) ||
       !prepare_scene_input_targets(raster->width,raster->height,diagnostics))return false;
    const unsigned input_count=diagnostics?4:3;
    const unsigned output_count=diagnostics?SceneInputTargets::count:SceneInputTargets::live_count;
    const auto &outputs=scene_inputs.textures;
    const bool motion_valid=scene_camera_history.valid && (!live_frame || !live_frame->reset);
    const SceneCameraParameters values{scene_camera_history.reprojection,unsigned(motion_valid),
                                       unsigned(scene_depth_history.sample!=nullptr),{},offset,scene_depth_history.offset};
    ID3D11Buffer *camera_buffer=scene_inputs.camera.Get();
    context->UpdateSubresource(camera_buffer,0,nullptr,&values,0,0);

    context->OMSetRenderTargets(0,nullptr,nullptr);
    ID3D11ShaderResourceView *inputs[]={raster->color_sample.Get(),raster->depth_sample.Get(),scene_surface_motion_sample.Get(),
                                      scene_depth_history.sample.Get()};
    ID3D11UnorderedAccessView *outputs_views[SceneInputTargets::count];
    for(unsigned i=0;i<output_count;++i)outputs_views[i]=scene_inputs.views[i].Get();
    context->CSSetShader(scene_input_shaders[diagnostics].Get(),nullptr,0);
    context->CSSetConstantBuffers(0,1,&camera_buffer);
    context->CSSetShaderResources(0,input_count,inputs);
    context->CSSetUnorderedAccessViews(0,output_count,outputs_views,nullptr);
    context->Dispatch((raster->width+7)/8,(raster->height+7)/8,1);
    ID3D11ShaderResourceView *no_inputs[4]{};
    ID3D11UnorderedAccessView *no_outputs[SceneInputTargets::count]{};
    ID3D11Buffer *no_buffer=nullptr;
    context->CSSetShaderResources(0,input_count,no_inputs);
    context->CSSetUnorderedAccessViews(0,output_count,no_outputs,nullptr);
    context->CSSetConstantBuffers(0,1,&no_buffer);
    context->CSSetShader(nullptr,nullptr,0);
    ID3D11RenderTargetView *render_view=raster->color_view.Get();
    context->OMSetRenderTargets(1,&render_view,raster->depth_view.Get());
    if(live_frame) {
        ID3D11Resource *sources[]={outputs[0].Get(),outputs[1].Get(),outputs[2].Get(),scene_reactive.Get()};
        if(warm_up)return fsr_bridge_prime(sources,*live_frame);
        return draw_fsr_output(fsr_bridge_dispatch(sources,*live_frame));
    }
    return write_scene_input(outputs[0].Get(),prefix,"-color.rgba") &&
           write_scene_input(outputs[1].Get(),prefix,"-depth.f32") &&
           write_scene_input(outputs[3].Get(),prefix,"-camera-motion.f32",8) &&
           write_scene_input(outputs[2].Get(),prefix,"-motion.f32",8) &&
           write_scene_input(outputs[4].Get(),prefix,"-history-reject.f32") &&
           write_scene_input(outputs[5].Get(),prefix,"-previous-depth.f32") &&
           write_scene_input(scene_reactive.Get(),prefix,"-reactive.r8",1) &&
           write_scene_input(scene_opaque_color.Get(),prefix,"-opaque-color.bgra") &&
           write_scene_input(scene_surface_motion.Get(),prefix,"-surface-motion.f32",16);
}

using MotionKey=std::tuple<uintptr_t,uintptr_t,unsigned,unsigned>;
MotionKey motion_key(const GxMotionKey &value) {
    return {value.instance,value.mesh,value.draw,value.primitive};
}

auto previous_model_triangles() {
    size_t count=0;
    const auto *previous=gx_previous_motion_triangles(count);
    std::map<MotionKey,const GxMotionTriangle *> history;
    for(size_t i=0;i<count;++i)history.emplace(motion_key(previous[i].key),previous+i);
    return history;
}

bool matching_model_triangle(const GxMotionTriangle &a,const GxMotionTriangle &b) {
    return a.local_vertices==b.local_vertices && a.viewport==b.viewport;
}

void trace_model_motion(const char *prefix) {
    if(!scene_camera_history.valid)return;
    size_t current_count=0,previous_count=0;
    const auto *current=gx_display_motion_triangles(current_count);
    const auto *previous=gx_previous_motion_triangles(previous_count);
    const auto history=previous_model_triangles();
    char path[2048];
    const int length=std::snprintf(path,sizeof(path),"%s-model-motion.csv",prefix);
    if(length<0 || size_t(length)>=sizeof(path))return;
    FILE *file=std::fopen(path,"w");
    if(!file)return;
    std::fputs("instance,mesh,draw,primitive,triangle,vertex,x,y,previous_x,previous_y,"
               "camera_only_x,camera_only_y\n",file);
    const float width=float(raster->width),height=float(raster->height);
    const float aspect=(float(SCREEN_W)*height)/(float(SCREEN_H)*width);
    const auto reprojection=DirectX::XMLoadFloat4x4(&scene_camera_history.reprojection);
    size_t matched=0,topology_rejected=0;
    for(size_t i=0;i<current_count;++i) {
        const auto &now=current[i];
        const auto found=history.find(motion_key(now.key));
        if(found==history.end())continue;
        const auto &before=*found->second;
        if(!matching_model_triangle(now,before)) {
            ++topology_rejected;
            continue;
        }
        // The captured camera matrix describes the full main gameplay viewport.
        if(now.viewport!=std::array<int,4>{0,0,SCREEN_W,SCREEN_H})continue;
        ++matched;
        for(unsigned j=0;j<now.clip.size();++j) {
            const auto &a=now.clip[j],&b=before.clip[j];
            if(a[3]<=0.0001f || b[3]<=0.0001f)continue;
            const float x=a[0]*aspect/a[3],y=a[1]/a[3],z=(a[2]/a[3]+1)*0.5f;
            DirectX::XMFLOAT4 projected{};
            DirectX::XMStoreFloat4(&projected,DirectX::XMVector4Transform(
                DirectX::XMVectorSet(x,y,z,1),reprojection));
            if(projected.w<=0.0001f)continue;
            std::fprintf(file,"%zx,%zx,%u,%u,%zu,%u,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
                size_t(now.key.instance),size_t(now.key.mesh),now.key.draw,now.key.primitive,now.triangle_index,j,
                (x+1)*width*0.5f,(1-y)*height*0.5f,
                (b[0]*aspect/b[3]+1)*width*0.5f,(1-b[1]/b[3])*height*0.5f,
                (projected.x/projected.w+1)*width*0.5f,(1-projected.y/projected.w)*height*0.5f);
        }
    }
    std::fclose(file);
    std::fprintf(stderr,"[scene model motion] current=%zu previous=%zu matched=%zu topology_rejected=%zu\n",
                 current_count,previous_count,matched,topology_rejected);
}

const char *scene_capture_prefix() {
    static const char *prefix=[]() -> const char * {
        const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
        return automation && *automation=='1'?std::getenv("SM64DS_PC_FSR_SCENE_CAPTURE"):nullptr;
    }();
    return prefix && *prefix?prefix:nullptr;
}

uint64_t scene_capture_tick() {
    static const auto tick=[] {
        const char *value=std::getenv("SM64DS_PC_FSR_SCENE_CAPTURE_TICK");
        return value?std::strtoull(value,nullptr,10):0ull;
    }();
    return tick;
}

uint64_t live_fsr_start_tick() {
    static const uint64_t tick=[] {
        const auto *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
        const auto *helper=std::getenv("SM64DS_PC_FSR_HELPER");
        const auto *start=std::getenv("SM64DS_PC_FSR_LIVE_START");
        return automation && std::strcmp(automation,"1")==0 && helper && *helper && start?
               std::strtoull(start,nullptr,10):0ull;
    }();
    return tick;
}

SceneSampleOffset fsr_sample_offset(uint64_t frame_index,unsigned phases=8) {
    const unsigned index=unsigned(frame_index%phases)+1;
    const auto halton=[](unsigned i,unsigned base) {
        float result=0;
        for(float scale=1.0f/base;i;scale/=base,i/=base)
            result+=scale*float(i%base);
        return result-0.5f;
    };
    return {halton(index,2),halton(index,3)};
}

SceneSampleOffset scene_sample_offset(bool eligible) {
    static const bool enabled=[] {
        const auto *value=std::getenv("SM64DS_PC_FSR_SCENE_JITTER");
        return value && std::strcmp(value,"1")==0;
    }();
    const auto tick=scene_capture_tick();
    const auto frame=rt_frame();
    if(!enabled || !eligible || !scene_capture_prefix() || !tick || !frame ||
       (frame!=tick && frame!=tick-1))return {};
    return fsr_sample_offset(frame-1);
}

void remember_scene_depth(bool eligible,SceneSampleOffset offset) {
    const auto tick=scene_capture_tick();
    if(!eligible || !scene_capture_prefix() || !tick || rt_frame()!=tick-1 || !raster->depth_sample)return;
    D3D11_TEXTURE2D_DESC desc{};
    raster->depth->GetDesc(&desc);
    desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    D3D11_SHADER_RESOURCE_VIEW_DESC srv{};
    raster->depth_sample->GetDesc(&srv);
    SceneDepthHistory history;
    if(!hr_ok(device->CreateTexture2D(&desc,nullptr,&history.texture),"previous scene depth") ||
       !hr_ok(device->CreateShaderResourceView(history.texture.Get(),&srv,&history.sample),"previous scene depth view"))
        return;
    context->OMSetRenderTargets(0,nullptr,nullptr);
    context->CopyResource(history.texture.Get(),raster->depth.Get());
    ID3D11RenderTargetView *render_view=raster->color_view.Get();
    context->OMSetRenderTargets(1,&render_view,raster->depth_view.Get());
    history.frame=rt_frame();
    history.serial=gx_display_camera().serial;
    history.offset=offset;
    scene_depth_history=std::move(history);
}

void reset_scene_capture_targets() {
    scene_surface_motion_sample.Reset();scene_surface_motion_view.Reset();scene_surface_motion.Reset();
    scene_reactive_view.Reset();scene_reactive.Reset();scene_opaque_color.Reset();
    scene_target_width=scene_target_height=0;
    scene_inputs={};
}

bool same_prediction(const GxCameraPrediction &a,const GxCameraPrediction &b) {
    if(a.fraction==0 || b.fraction==0)return a.fraction==b.fraction;
    return a.clip_transform==b.clip_transform;
}

void update_scene_camera(bool gameplay_camera,const GxCameraPrediction &prediction) {
    if(!scene_capture_prefix() && !live_fsr.device)return;
    if(!gameplay_camera) {
        scene_camera_history={};
        scene_depth_history={};
        return;
    }
    auto camera=gx_display_camera();
    auto &history=scene_camera_history;
    const bool same_geometry=camera.serial==history.current.serial;
    if(same_geometry && same_prediction(prediction,history.prediction))return;
    if(prediction.fraction!=0) {
        DirectX::XMFLOAT4X4 view,delta;
        std::memcpy(&view,camera.view_projection.data(),sizeof(view));
        std::memcpy(&delta,prediction.clip_transform.data(),sizeof(delta));
        DirectX::XMStoreFloat4x4(&view,DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&view),
                                                                DirectX::XMLoadFloat4x4(&delta)));
        std::memcpy(camera.view_projection.data(),&view,sizeof(view));
    }
    history.valid=build_camera_reprojection(camera,history.current,history.reprojection,same_geometry);
    history.same_geometry=same_geometry;
    history.previous_prediction=history.prediction;
    history.prediction=prediction;
    history.current=camera;
}

bool create_scene_capture_targets(const RasterTarget &source) {
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width=source.width;desc.Height=source.height;
    desc.MipLevels=desc.ArraySize=desc.SampleDesc.Count=1;
    desc.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;
    desc.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
    if(!hr_ok(device->CreateTexture2D(&desc,nullptr,&scene_surface_motion),"surface motion texture") ||
       !hr_ok(device->CreateRenderTargetView(scene_surface_motion.Get(),nullptr,&scene_surface_motion_view),"surface motion target") ||
       !hr_ok(device->CreateShaderResourceView(scene_surface_motion.Get(),nullptr,&scene_surface_motion_sample),"surface motion view")) {
        reset_scene_capture_targets();
        return false;
    }
    desc.Format=DXGI_FORMAT_R8_UNORM;
    if(!hr_ok(device->CreateTexture2D(&desc,nullptr,&scene_reactive),"reactive mask texture") ||
       !hr_ok(device->CreateRenderTargetView(scene_reactive.Get(),nullptr,&scene_reactive_view),"reactive mask target")) {
        reset_scene_capture_targets();
        return false;
    }
    scene_target_width=source.width;scene_target_height=source.height;
    return true;
}

bool prepare_scene_capture_targets(const RasterTarget &source,bool diagnostics=false) {
    if(scene_target_width!=source.width || scene_target_height!=source.height) {
        reset_scene_capture_targets();
        if(!create_scene_capture_targets(source))return false;
    }
    if(diagnostics && !scene_opaque_color) {
        D3D11_TEXTURE2D_DESC desc{};
        source.color->GetDesc(&desc);
        desc.BindFlags=0;
        if(!hr_ok(device->CreateTexture2D(&desc,nullptr,&scene_opaque_color),"opaque scene snapshot"))
            return false;
    }
    const float clear[4]={};
    context->ClearRenderTargetView(scene_surface_motion_view.Get(),clear);
    context->ClearRenderTargetView(scene_reactive_view.Get(),clear);
    return true;
}

const char *prepare_scene_capture(bool eligible_history) {
    const auto *prefix=scene_capture_prefix();
    const auto tick=scene_capture_tick();
    static bool attempted=false;
    if(!prefix || !eligible_history || attempted || !tick || rt_frame()!=tick)return nullptr;
    attempted=true;
    if(!scene_camera_history.valid || scene_depth_history.frame+1!=rt_frame() ||
       scene_depth_history.serial+1!=scene_camera_history.current.serial)
        scene_depth_history={};
    return prepare_scene_capture_targets(output_raster,true)?prefix:nullptr;
}

bool live_fsr_quality() {
    static const bool quality=[] {
        const char *mode=std::getenv("SM64DS_PC_FSR_QUALITY");
        return mode && std::strcmp(mode,"1")==0;
    }();
    return quality;
}

float fsr_upscale_ratio(FsrMode mode) {
    switch(mode) {
    case FsrMode::Quality: return 1.5f;
    case FsrMode::Balanced: return 1.7f;
    case FsrMode::Performance: return 2.0f;
    default: return 1.0f;
    }
}

bool trace_fsr_frames() {
    static const bool trace=[] {
        const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
        return automation && *automation=='1';
    }();
    return trace;
}

void stop_live_fsr(bool failed=false) {
    fsr_bridge_stop();
    live_fsr.running=false;
    live_fsr.failed=failed;
}

FsrPresentation prepare_live_fsr(bool eligible,FsrMode requested,const GxCameraPrediction &prediction) {
    const auto tick=live_fsr_start_tick(),frame=rt_frame();
    constexpr uint64_t lead_ticks=120;
    if(!live_fsr.device)return FsrPresentation::Inactive;
    const auto mode=tick?(live_fsr_quality()?FsrMode::Quality:FsrMode::NativeAA):requested;
    if(live_fsr.mode!=mode || live_fsr.width!=output_raster.width || live_fsr.height!=output_raster.height) {
        stop_live_fsr();
        const bool available=live_fsr.device;
        live_fsr={};live_fsr.device=available;
        live_fsr.mode=mode;live_fsr.width=output_raster.width;live_fsr.height=output_raster.height;
    }
    if(mode==FsrMode::Off || (tick && frame<tick && tick-frame>lead_ticks))return FsrPresentation::Inactive;
    const auto camera=gx_display_camera();
    const bool camera_ok=camera.owner && !std::isnan(camera.far_plane) &&
        camera.near_plane>0 && camera.far_plane>camera.near_plane && camera.vertical_fov>0;
    if(!eligible || !camera_ok) {
        live_fsr.bypassed=true;
        return FsrPresentation::Inactive;
    }
    if(live_fsr.running && std::isinf(camera.far_plane)!=live_fsr.infinite_depth) {
        stop_live_fsr();
        live_fsr.attempted=false;live_fsr.frames=live_fsr.phases=0;
    }
    const bool upscale=fsr_upscales(mode);
    auto &scene=upscale?upscale_raster:output_raster;
    if(!live_fsr.attempted && (!tick || frame<tick)) {
        live_fsr.attempted=true;
        live_fsr.started_tick=frame;
        std::fprintf(stderr,"[FSR bridge] warm-up tick=%llu camera near=%.6f far=%.6f fov=%.6f\n",
                     static_cast<unsigned long long>(frame),
                     double(camera.near_plane),double(camera.far_plane),double(camera.vertical_fov));
        const float ratio=fsr_upscale_ratio(mode);
        const bool allocated=!upscale || ensure_raster(scene,unsigned(float(output_raster.width)/ratio),
                                                       unsigned(float(output_raster.height)/ratio));
        live_fsr.running=allocated && fsr_bridge_start(scene.width,scene.height,output_raster.width,output_raster.height,
                                                       tick?fsr_probe_frames:0);
        live_fsr.infinite_depth=std::isinf(camera.far_plane);
        if(live_fsr.running && prepare_scene_capture_targets(scene))return FsrPresentation::WarmUp;
        stop_live_fsr(true);
    }
    if(!live_fsr.running || frame<tick)return FsrPresentation::Inactive;
    const bool alive=fsr_bridge_alive();
    const bool ready=live_fsr.phases || fsr_bridge_ready();
    if(alive && ready) {
        if(!live_fsr.phases) {
            live_fsr.phases=fsr_bridge_jitter_phases();
            std::fprintf(stderr,"[FSR bridge] jitter phases=%u\n",live_fsr.phases);
        }
        if(live_fsr.frames && !live_fsr.bypassed && camera.serial==live_fsr.last_serial &&
           same_prediction(live_fsr.last_prediction,prediction))
            return FsrPresentation::RepeatScene;
        if(live_fsr.phases && prepare_scene_capture_targets(scene))return FsrPresentation::NewScene;
    } else if(alive && !tick && frame-live_fsr.started_tick<240) {
        return FsrPresentation::Inactive;
    }
    std::fprintf(stderr,"[FSR bridge] fallback tick=%llu helper_ready=%u\n",
                 static_cast<unsigned long long>(frame),unsigned(ready));
    stop_live_fsr(true);
    return FsrPresentation::Inactive;
}

bool finish_live_fsr(SceneSampleOffset offset,const GxCameraPrediction &prediction,bool warm_up) {
    const auto camera=gx_display_camera();
    const uint64_t now=rt_frame();
    const float phase=rt_presentation_phase();
    const bool gap=live_fsr.frames && (now<live_fsr.last_frame || now-live_fsr.last_frame>8);
    const bool reset=!live_fsr.frames || live_fsr.bypassed || gap ||
        !scene_camera_history.valid ||
        (camera.serial!=live_fsr.last_serial && camera.serial!=live_fsr.last_serial+1);
    const float ticks=reset?1.0f:std::max(0.25f,float(now-live_fsr.last_frame)+phase-live_fsr.last_phase);
    const FsrFrame frame{offset.x,offset.y,camera.near_plane,camera.far_plane,camera.vertical_fov,
                         ticks*(1000.0f/60.0f),unsigned(reset)};
    const bool drawn=capture_scene_inputs(nullptr,offset,&frame,warm_up);
    if(warm_up && drawn)return true;
    if(drawn) {
        ++live_fsr.frames;
        live_fsr.bypassed=false;
        live_fsr.last_frame=now;
        live_fsr.last_serial=camera.serial;
        live_fsr.last_phase=phase;
        live_fsr.last_prediction=prediction;
        if(trace_fsr_frames())
            std::fprintf(stderr,"[FSR bridge] completed tick=%llu serial=%llu index=%u reset=%u elapsed_ms=%.3f phase=%.2f prediction=%.4f\n",
                         static_cast<unsigned long long>(now),static_cast<unsigned long long>(camera.serial),
                         live_fsr.frames,unsigned(reset),double(frame.elapsed_ms),double(phase),double(prediction.fraction));
    }
    if(!drawn || (live_fsr_start_tick() && live_fsr.frames==fsr_probe_frames))stop_live_fsr(!drawn);
    return drawn;
}

void finish_scene_capture(const char *prefix,SceneSampleOffset offset) {
    if(!prefix)return;
    ID3D11RenderTargetView *render_view=raster->color_view.Get();
    context->OMSetRenderTargets(1,&render_view,raster->depth_view.Get());
    context->PSSetShader(pixel_shader.Get(),nullptr,0);
    const bool captured=capture_scene_inputs(prefix,offset);
    std::fprintf(stderr,"[scene input probe] tick=%llu raster=%ux%u triangles=%u captured=%u camera_valid=%u depth_history=%u\n",
        static_cast<unsigned long long>(rt_frame()),raster->width,raster->height,triangles_drawn,unsigned(captured),
        unsigned(scene_camera_history.valid),unsigned(scene_depth_history.sample!=nullptr));
    std::fprintf(stderr,"[scene input probe] jitter %.9g %.9g previous_jitter %.9g %.9g\n",
                 double(offset.x),double(offset.y),double(scene_depth_history.offset.x),double(scene_depth_history.offset.y));
    scene_depth_history={};
    if(captured && scene_camera_history.valid) {
        trace_model_motion(prefix);
        std::fputs("[scene input probe] current_to_previous",stderr);
        for(const auto &row:scene_camera_history.reprojection.m)
            for(float value:row)std::fprintf(stderr," %.9g",double(value));
        std::fputc('\n',stderr);
    }
}
bool retire_commands() {
    ID3D11ShaderResourceView *null_view=nullptr;context->PSSetShaderResources(0,1,&null_view);
    context->End(retirement_query.Get());context->Flush();
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
    for(;;) {
        BOOL done=FALSE;const HRESULT hr=context->GetData(retirement_query.Get(),&done,sizeof(done),0);
        if(FAILED(hr))return hr_ok(hr,"texture retirement");
        if(hr==S_OK && done)return true;
        if(std::chrono::steady_clock::now()>=deadline) {
            std::fprintf(stderr,"[native GPU] texture retirement timed out\n");return false;
        }
        std::this_thread::yield();
    }
}
bool evict_for(uint64_t incoming) {
    const bool needs_eviction=!texture_cache.empty() &&
        (cache_bytes+incoming>cache_budget || texture_cache.size()>=1024);
    // D3D11 commands retain resource references after map erasure. Retire
    // outstanding draws before releasing LRU entries, bounding queued payload
    // too. No wait occurs while the working set fits the cache.
    if(needs_eviction && !retire_commands())return false;
    while(!texture_cache.empty() &&
          (cache_bytes+incoming>cache_budget || texture_cache.size()>=1024)) {
        auto oldest=std::min_element(texture_cache.begin(),texture_cache.end(),
            [](const auto &a,const auto &b){return a.second.last_used<b.second.last_used;});
        cache_bytes-=oldest->second.bytes;texture_cache.erase(oldest);++frame_stats.evictions;
    }
    return true;
}
bool sync_replacements() {
    if(cache_epoch!=texture_replacement_epoch()) {
        const bool has_replacements=std::any_of(texture_cache.begin(),texture_cache.end(),
            [](const auto &entry){return std::get<0>(entry.first);});
        if(has_replacements && !retire_commands())return false;
        for(auto i=texture_cache.begin();i!=texture_cache.end();) {
            if(std::get<0>(i->first)) {cache_bytes-=i->second.bytes;i=texture_cache.erase(i);}
            else ++i;
        }
        unavailable_replacements.clear();cache_epoch=texture_replacement_epoch();
    }
    return evict_for(0);
}
Texture *cache_upload(const CacheKey &key,const uint32_t *pixels,int width,int height,
                      const TexturePixelsInfo *info=nullptr) {
    const uint64_t bytes=static_cast<uint64_t>(width)*height*4;
    if(bytes>cache_budget)return nullptr;
    if(!evict_for(bytes))return nullptr;
    Texture tex;
    if(!texture(tex,pixels,width,height,info))return nullptr;
    tex.bytes=bytes;tex.last_used=++use_counter;
    auto result=texture_cache.emplace(key,std::move(tex));
    cache_bytes+=bytes;frame_stats.upload_bytes+=bytes;++frame_stats.uploads;
    return &result.first->second;
}
// Resolve at draw time. Geometry groups retain CPU descriptors, never pointers
// to cache entries: an LRU eviction between the two alpha passes stays safe.
Texture *resolve_material(const FrameMaterial &m) {
    if(!m.pixels)return &white_texture;
    const auto &r=m.replacement;
    const auto route=texture_replacement_route(texture_quality(),r.role);
    if(texture_backend()==TextureBackend::NativeGpu && r.eligible && r.key && route!=TextureReplacementRoute::None) {
        const CacheKey key{true,r.key,static_cast<uint32_t>(route)};
        auto cached=texture_cache.find(key);
        if(cached!=texture_cache.end()) {
            cached->second.last_used=++use_counter;++frame_stats.cache_hits;return &cached->second;
        }
        if(!unavailable_replacements.count(key)) {
            ++frame_stats.replacement_attempts;
            std::vector<uint32_t> staging_pixels;int width=0,height=0;
            if(texture_load_replacement_pixels(r.key,route,staging_pixels,width,height)) {
                frame_stats.staging_peak_bytes=std::max(frame_stats.staging_peak_bytes,
                    static_cast<uint64_t>(staging_pixels.size())*4);
                if(auto *result=cache_upload(key,staging_pixels.data(),width,height)) {
                    ++frame_stats.replacement_uploads;return result;
                }
            }
            // Missing, invalid, over-limit or failed uploads use original art.
            // A settings change starts a fresh epoch and retries these keys.
            unavailable_replacements.insert(key);
        }
    }
    const CacheKey key{false,m.info.key,0};auto cached=texture_cache.find(key);
    if(cached!=texture_cache.end()) {
        cached->second.last_used=++use_counter;++frame_stats.cache_hits;return &cached->second;
    }
    return cache_upload(key,m.pixels,m.width,m.height,&m.info);
}
using FrameKey=std::tuple<const uint32_t *,int,int,uint64_t,TextureRole,bool>;
FrameMaterial *material(const GxTriangle &t,std::map<FrameKey,FrameMaterial> &frame_cache) {
    const auto pointer=std::make_tuple(t.tex,t.tw,t.th,t.replacement.key,t.replacement.role,t.replacement.eligible);
    auto found=frame_cache.find(pointer);
    if(found!=frame_cache.end())return &found->second;
    FrameMaterial m;m.pixels=t.tw>0 && t.th>0?t.tex:nullptr;m.width=t.tw;m.height=t.th;
    m.info=t.pixels_info;m.replacement=t.replacement;
    if(m.pixels) {
        if(m.info.immutable && force_texture_scan) {
            m.info.key=texture_pixels_key(t.tex,t.tw,t.th);
            frame_stats.hash_texels+=static_cast<uint64_t>(t.tw)*t.th;
        } else if(!m.info.immutable) {
            m.info=texture_pixels_info(t.tex,t.tw,t.th);
            frame_stats.hash_texels+=static_cast<uint64_t>(t.tw)*t.th;
        } else ++frame_stats.identity_uses;
    }
    return &frame_cache.emplace(pointer,m).first->second;
}
bool menu_scene=false,letter_overlay=false;
bool geometry(bool dual_screen,const GxCameraPrediction &prediction,bool capture_motion,
              SceneSampleOffset offset,ID3D11Texture2D *opaque_snapshot) {
    if(const char *path=std::getenv("SM64DS_PC_POLYGON_DUMP")) {
        static bool dumped=false;
        const char *when=std::getenv("SM64DS_PC_POLYGON_DUMP_TICK");
        if(!dumped && path[0] && rt_frame()>=(when?std::strtoull(when,nullptr,10):0)) {
            FILE *file=std::fopen(path,"wb");
            if(file) {
                size_t n=0;const auto *list=gx_display_polygons(n);
                std::fputs("index,attributes,mode,id,alpha,width,height,tex_alpha_min,tex_alpha_max,color,x0,y0,z0,x1,y1,z1,x2,y2,z2\n",file);
                for(size_t i=0;i<n;++i) {
                    const auto &t=list[i];unsigned amin=255,amax=0;
                    if(t.tex) for(int p=0;p<t.tw*t.th;++p) { amin=std::min(amin,t.tex[p]>>24);amax=std::max(amax,t.tex[p]>>24); }
                    else amax=255;
                    std::fprintf(file,"%zu,%08x,%u,%u,%u,%d,%d,%u,%u,%08x",i,t.attributes,
                        (t.attributes>>4)&3,(t.attributes>>24)&63,(t.attributes>>16)&31,t.tw,t.th,amin,amax,t.v[0].color);
                    for(const auto &v:t.v) std::fprintf(file,",%.7g,%.7g,%.7g",v.x,v.y,v.z);
                    std::fputc('\n',file);
                }
                std::fclose(file);dumped=true;
            }
        }
    }
    std::map<FrameKey,FrameMaterial> frame_cache;
    // Fullscreen mask-clear primitive stays in this same vertex buffer. Only
    // stencil bit 6 is cleared; the lower six polygon-ID bits must survive.
    std::vector<Vertex> vertices{
        vertex(-1,1,0,1,0,0,0xffffffff),vertex(1,1,0,1,1,0,0xffffffff),
        vertex(-1,-1,0,1,0,1,0xffffffff),vertex(-1,-1,0,1,0,1,0xffffffff),
        vertex(1,1,0,1,1,0,0xffffffff),vertex(1,-1,0,1,1,1,0xffffffff)};
    std::vector<Group> groups;
    const float jitter_x=2*offset.x/raster->width,jitter_y=-2*offset.y/raster->height;
    size_t count=0; const GxTriangle *source=gx_display_polygons(count);
    std::vector<const GxMotionTriangle *> previous_for_draw;
    DirectX::XMFLOAT4X4 previous_delta{};
    const bool predict_previous=scene_camera_history.previous_prediction.fraction!=0;
    if(capture_motion && scene_camera_history.valid) {
        previous_for_draw.resize(count);
        if(predict_previous)
            std::memcpy(&previous_delta,scene_camera_history.previous_prediction.clip_transform.data(),
                        sizeof(previous_delta));
        size_t current_count=0;
        const auto *current=gx_display_motion_triangles(current_count);
        const auto history=scene_camera_history.same_geometry
            ? decltype(previous_model_triangles()){} : previous_model_triangles();
        for(size_t i=0;i<current_count;++i) {
            const auto &now=current[i];
            if(now.triangle_index>=count || now.viewport!=std::array<int,4>{0,0,SCREEN_W,SCREEN_H})
                continue;
            if(scene_camera_history.same_geometry) {
                previous_for_draw[now.triangle_index]=&now;
                continue;
            }
            const auto found=history.find(motion_key(now.key));
            if(found!=history.end() && matching_model_triangle(now,*found->second))
                previous_for_draw[now.triangle_index]=found->second;
        }
    }
    for(size_t i=0;i<count;++i)if(((source[i].attributes>>4)&3)==2) {
        uint32_t values[36]={};bool highlight=false;
        gx_read_toon_table(values,highlight);values[32]=highlight?1:0;
        context->UpdateSubresource(toon_params.Get(),0,nullptr,values,0,0);
        ID3D11Buffer *buffer=toon_params.Get();context->PSSetConstantBuffers(1,1,&buffer);
        break;
    }
    for(size_t i=0;i<count;++i) {
        auto t=source[i]; GxVertex clipped[12];
        const bool predicted=gx_apply_camera_prediction(t,prediction);
        if(predicted)++frame_stats.camera_predicted_triangles;
        const auto *previous=previous_for_draw.empty()?nullptr:previous_for_draw[i];
        std::array<DirectX::XMFLOAT4,3> previous_clip;
        if(previous) {
            for(unsigned k=0;k<previous_clip.size();++k) {
                const auto &clip=previous->clip[k];
                previous_clip[k]={clip[0],clip[1],clip[2],clip[3]};
                if(predict_previous) {
                    DirectX::XMStoreFloat4(&previous_clip[k],DirectX::XMVector4Transform(
                        DirectX::XMLoadFloat4(&previous_clip[k]),DirectX::XMLoadFloat4x4(&previous_delta)));
                }
            }
        }
        float aspect=1;
        if(t.perspective && !menu_scene && !dual_screen) {
            // Keep vertical FOV and original proportions. Widen the horizontal
            // frustum before clipping so widescreen reveals real extra geometry.
            aspect=(float(SCREEN_W)*output_raster.height)/(float(SCREEN_H)*output_raster.width);
            for(auto &v:t.v) v.clip_x*=aspect;
        }
        if((jitter_x!=0 || jitter_y!=0) && t.viewport_x==0 && t.viewport_y==0 &&
           t.viewport_w==SCREEN_W && t.viewport_h==SCREEN_H) {
            for(auto &v:t.v) {
                v.clip_x+=jitter_x*v.w;
                v.clip_y+=jitter_y*v.w;
            }
        }
        std::array<float,3> weights[12];
        const unsigned n=gx_clip_polygon(t,clipped,previous?weights:nullptr);
        for(unsigned j=1;j+1<n;++j) {
            const GxVertex v[3]={clipped[0],clipped[j],clipped[j+1]};
            const float area=(v[1].x-v[0].x)*(v[2].y-v[0].y)-(v[1].y-v[0].y)*(v[2].x-v[0].x);
            if(std::fabs(area)<1e-6f || !(t.attributes&(area<0?(1u<<7):(1u<<6)))) continue;
            auto *m=material(t,frame_cache);
            if(groups.empty() || groups.back().material!=m || groups.back().attributes!=t.attributes || groups.back().texture_address!=t.texture_address)
                groups.push_back({m,static_cast<unsigned>(vertices.size()),0,t.attributes,false,t.texture_address});
            for(unsigned index:{0u,j,j+1}) {
                const auto &a=clipped[index];
                // Preserve the retail viewport, mapping its logical 256x192
                // coordinates directly to the full-resolution raster target.
                const float x=a.clip_x*t.viewport_w/SCREEN_W+a.w*(2.0f*t.viewport_x/SCREEN_W+float(t.viewport_w)/SCREEN_W-1);
                const float y=a.clip_y*t.viewport_h/SCREEN_H+a.w*(1-2.0f*t.viewport_y/SCREEN_H-float(t.viewport_h)/SCREEN_H);
                vertices.push_back(vertex(x,y,(a.clip_z+a.w)*0.5f,a.w,
                    t.tw>0?a.u/t.tw:0,t.th>0?a.v/t.th:0,a.color));
                if(previous) {
                    auto &p=vertices.back().previous;
                    for(unsigned k=0;k<3;++k) {
                        const auto &clip=previous_clip[k];
                        p[0]+=clip.x*weights[index][k]*aspect;
                        p[1]+=clip.y*weights[index][k];
                        p[2]+=(clip.z+clip.w)*0.5f*weights[index][k];
                        p[3]+=clip.w*weights[index][k];
                    }
                }
                if(((t.attributes>>4)&3)==2)vertices.back().color[0]=a.toon_intensity/511;
            }
            groups.back().count+=3; ++triangles_drawn;
        }
    }
    if(groups.empty()) {
        if(opaque_snapshot) {
            context->OMSetRenderTargets(0,nullptr,nullptr);
            context->CopyResource(opaque_snapshot,raster->color.Get());
        }
        return true;
    }
    if(!upload(vertices)) return false;
    bool rendered_shadow=false;
    // Hardware renders opaque pixels first, followed by translucent polygons
    // in submission order. This also places shadow volumes after their targets.
    for(unsigned pass=0;pass<2;++pass) {
        if(capture_motion) {
            if(pass && opaque_snapshot) {
                context->OMSetRenderTargets(0,nullptr,nullptr);
                context->CopyResource(opaque_snapshot,raster->color.Get());
            }
            ID3D11RenderTargetView *targets[]={raster->color_view.Get(),
                pass?scene_reactive_view.Get():scene_surface_motion_view.Get()};
            context->OMSetRenderTargets(2,targets,raster->depth_view.Get());
            context->PSSetShader(pass?scene_reactive_shader.Get():scene_motion_shader.Get(),nullptr,0);
        }
        for(auto &group:groups) {
            const unsigned mode=(group.attributes>>4)&3,id=(group.attributes>>24)&63;
            const unsigned alpha=(group.attributes>>16)&31;
            const unsigned equal=(group.attributes>>14)&1;
            const bool shadow=mode==3;
            const bool translucent=alpha>0 && alpha<31;
            if(!pass && (shadow || translucent)) continue;
            if(pass && !shadow && !translucent && !(mode!=1 && group.partial_alpha)) continue;
            auto *resolved=resolve_material(*group.material);
            if(!resolved)return false;
            group.partial_alpha=resolved->partial_alpha;
            const bool partial=mode!=1 && group.partial_alpha;
            auto *view=resolved->view.Get();
            context->RSSetState(alpha? rasterizer.Get():wireframe.Get());
            parameters(alpha?alpha:31,shadow?0:partial&&!translucent?(pass?2:1):0,true,mode);
            const unsigned aniso_index=anisotropic_level==2?0:anisotropic_level==4?1:anisotropic_level==8?2:anisotropic_level==16?3:4;
            auto sampler_state=aniso_index<4?anisotropic_samplers[aniso_index][group.texture_address&15].Get():texture_samplers[group.texture_address&15].Get();
            bind_texture(view,sampler_state);
            if(shadow) {
                context->OMSetBlendState(color_off.Get(),nullptr,~0u);
                if(!id) {
                    if(rendered_shadow) {
                        context->RSSetState(rasterizer.Get());
                        context->OMSetDepthStencilState(shadow_clear.Get(),0);
                        parameters();bind_texture(white_texture.view.Get(),nearest_clamp.Get());
                        context->Draw(6,0);
                        parameters(alpha?alpha:31,0,true,mode);
                        bind_texture(view,sampler_state);
                        rendered_shadow=false;
                    }
                    // Mask volumes mark pixels only on DEPTH FAIL, without writing
                    // depth or color. Drawing these as black surfaces hid Mario.
                    context->OMSetDepthStencilState(shadow_mask[equal].Get(),0x40);
                    context->Draw(group.count,group.first);
                } else {
                    // Exclude pixels whose stored polygon ID is the caster's ID.
                    context->OMSetDepthStencilState(shadow_exclude.Get(),id);
                    context->Draw(group.count,group.first);
                    // NOT_EQUAL against ref bit6=0 selects mask bit6=1, then REPLACE
                    // simultaneously consumes that bit and stores the shadow ID.
                    context->OMSetDepthStencilState(shadow_draw[equal].Get(),id);
                    context->OMSetBlendState(capture_motion?blend_reactive.Get():blend_on.Get(),nullptr,~0u);
                    context->Draw(group.count,group.first);
                    rendered_shadow=true;
                }
            } else {
                const unsigned write_depth=!pass || (group.attributes&(1u<<11));
                // Bit7 distinguishes a previous translucent pixel from an opaque
                // receiver with the same six-bit ID. DS rejects repeat blending
                // within one translucent object, including Peach during her fade.
                context->OMSetDepthStencilState(pass?translucent_depth[write_depth?1:0][equal].Get():normal_depth[write_depth?1:0][equal].Get(),pass?(0x80|id):id);
                context->OMSetBlendState(pass?(capture_motion?blend_reactive.Get():blend_on.Get()):blend_off.Get(),nullptr,~0u);
                context->Draw(group.count,group.first);
            }
        }
    }
    context->RSSetState(rasterizer.Get());
    return true;
}
}

static bool gpu_open_renderer(void *window,bool warp,VideoRenderer renderer,bool request_fsr) {
    const char *force=std::getenv("SM64DS_PC_GPU_FORCE_TEXTURE_SCAN");
    force_texture_scan=force && force[0]=='1';
    gpu_close();
    if(renderer==VideoRenderer::OpenGLSoftware) {
        using_gl_software=glsw_open(window);
        return using_gl_software;
    }
    const D3D_FEATURE_LEVEL levels[]={D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_10_1,D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL selected{};
    live_fsr.device=!warp && request_fsr &&
        fsr_helper_available() && fsr_device_open(&device,&context);
    HRESULT hr=live_fsr.device?S_OK:D3D11CreateDevice(nullptr,warp?D3D_DRIVER_TYPE_WARP:D3D_DRIVER_TYPE_HARDWARE,nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,levels,3,D3D11_SDK_VERSION,&device,&selected,&context);
    if(FAILED(hr) && !warp) {
        warp=true;
        std::fprintf(stderr,"[native GPU] hardware device unavailable; trying Windows WARP\n");
        hr=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            levels,3,D3D11_SDK_VERSION,&device,&selected,&context);
    }
    if(!hr_ok(hr,"device")) { gpu_close();return false; }
    selected=device->GetFeatureLevel();
    ComPtr<IDXGIDevice> dxgi; ComPtr<IDXGIAdapter> adapter; ComPtr<IDXGIFactory> factory;
    if(FAILED(device.As(&dxgi)) || FAILED(dxgi->GetAdapter(&adapter)) ||
        FAILED(adapter->GetParent(IID_PPV_ARGS(&factory)))) { gpu_close();return false; }
    cache_budget=256ull*1024*1024;
    if(!warp) {
        ComPtr<IDXGIAdapter3> memory_adapter;DXGI_QUERY_VIDEO_MEMORY_INFO memory{};
        if(SUCCEEDED(adapter.As(&memory_adapter)) &&
           SUCCEEDED(memory_adapter->QueryVideoMemoryInfo(0,DXGI_MEMORY_SEGMENT_GROUP_LOCAL,&memory)))
            cache_budget=std::clamp<uint64_t>(memory.Budget/2,128ull*1024*1024,4ull*1024*1024*1024);
    }
    DXGI_ADAPTER_DESC desc{}; adapter->GetDesc(&desc);
    WideCharToMultiByte(CP_UTF8,0,desc.Description,-1,adapter_name,sizeof(adapter_name),nullptr,nullptr);
    if(window && live_fsr.device) {
        if(!fsr_open_swap_chain(factory.Get(),window,&flip_swap) || FAILED(flip_swap.As(&swap))) {
            gpu_close();
            return false;
        }
        DXGI_SWAP_CHAIN_DESC1 desc1{};
        if(SUCCEEDED(flip_swap->GetDesc1(&desc1))) {
            window_width=desc1.Width;
            window_height=desc1.Height;
        }
        factory->MakeWindowAssociation(static_cast<HWND>(window),DXGI_MWA_NO_ALT_ENTER);
        std::fprintf(stderr,"[native GPU] FSR D3D12 flip swap chain ready\n");
    } else if(window) {
        DXGI_SWAP_CHAIN_DESC swap_desc{};
        swap_desc.BufferDesc.Width=swap_desc.BufferDesc.Height=1;
        swap_desc.BufferDesc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
        swap_desc.SampleDesc.Count=1; swap_desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_desc.BufferCount=2; swap_desc.OutputWindow=static_cast<HWND>(window);
        swap_desc.Windowed=TRUE; swap_desc.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
        if(!hr_ok(factory->CreateSwapChain(device.Get(),&swap_desc,&swap),"swap chain")) { gpu_close();return false; }
        factory->MakeWindowAssociation(static_cast<HWND>(window),DXGI_MWA_NO_ALT_ENTER);
    }
    D3D11_QUERY_DESC retirement{};retirement.Query=D3D11_QUERY_EVENT;
    if(!hr_ok(device->CreateQuery(&retirement,&retirement_query),"retirement query")) {gpu_close();return false;}
    const char *shader=R"(
Texture2D image : register(t0); SamplerState sample_image : register(s0);
cbuffer DrawParameters : register(b0) { uint polygon_alpha; uint pixel_pass; uint is_polygon; uint texture_mode; };
cbuffer ToonParameters : register(b1) { uint4 toon_colors[8]; uint highlight; uint3 toon_padding; };
struct Input { float4 p:POSITION; float4 color:COLOR; float2 uv:TEXCOORD; float4 prev:TEXCOORD1; };
struct Output { float4 p:SV_POSITION; noperspective float4 color:COLOR; float2 uv:TEXCOORD; float4 prev:TEXCOORD1; };
Output vs_main(Input i) { Output o; o.p=i.p; o.color=i.color; o.uv=i.uv; o.prev=i.prev; return o; }
float4 ps_main(Output i):SV_TARGET {
    float4 t=image.Sample(sample_image,i.uv);
    if(!is_polygon) {
        if(texture_mode==4)return float4(t.rgb,1);
        clip(t.a-0.0001);return t*i.color;
    }
    float alpha=texture_mode==1 ? polygon_alpha : floor(((floor(t.a*31+0.5)+1)*(polygon_alpha+1)-1)/32);
    clip(alpha-0.5);
    if(pixel_pass==1) clip(alpha-30.5);
    if(pixel_pass==2) clip(30.5-alpha);
    float3 color=texture_mode==1?lerp(i.color.rgb,t.rgb,t.a):t.rgb*i.color.rgb;
    if(texture_mode==2) {
        uint intensity=min(511u,(uint)floor(saturate(i.color.r)*511+0.0001));
        uint index=intensity>>4;
        uint packed=toon_colors[index/4][index%4];
        float3 shade=float3((packed>>16)&255,(packed>>8)&255,packed&255)/255.0;
        color=highlight?saturate(t.rgb*((intensity>>3)/63.0)+shade):t.rgb*shade;
    }
    // DS blending uses (alpha+1)/32; opaque alpha31 remains exactly one.
    return float4(color,(alpha+1)/32);
}
struct SceneOutput { float4 color:SV_TARGET0; float4 surface:SV_TARGET1; };
SceneOutput ps_scene_motion(Output i) {
    SceneOutput o;
    o.color=ps_main(i);
    o.surface=i.prev.w>0.0001?float4(i.prev.xyz/i.prev.w,1+i.p.z):0;
    return o;
}
struct ReactiveOutput { float4 color:SV_TARGET0; float reactive:SV_TARGET1; };
ReactiveOutput ps_scene_reactive(Output i) {
    ReactiveOutput o;
    o.color=ps_main(i);
    // Shadow volumes modify the existing receiver. They do not represent
    // alpha-blended material coverage of their own.
    o.reactive=is_polygon && texture_mode!=3?min(o.color.a,0.9):0;
    return o;
}
)";
    ComPtr<ID3DBlob> vs,ps,ps_scene,ps_reactive,errors;
    auto compile=[&](const char *entry,const char *profile,ComPtr<ID3DBlob> &out) {
        const HRESULT result=D3DCompile(shader,std::strlen(shader),nullptr,nullptr,nullptr,entry,profile,
            D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&out,&errors);
        if(FAILED(result) && errors) std::fprintf(stderr,"[native GPU] shader: %s\n",static_cast<const char *>(errors->GetBufferPointer()));
        return hr_ok(result,"shader compile");
    };
    if(!compile("vs_main","vs_4_0",vs) || !compile("ps_main","ps_4_0",ps) ||
        !compile("ps_scene_motion","ps_4_0",ps_scene) || !compile("ps_scene_reactive","ps_4_0",ps_reactive) ||
        FAILED(device->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&vertex_shader)) ||
        FAILED(device->CreatePixelShader(ps_scene->GetBufferPointer(),ps_scene->GetBufferSize(),nullptr,&scene_motion_shader)) ||
        FAILED(device->CreatePixelShader(ps_reactive->GetBufferPointer(),ps_reactive->GetBufferSize(),nullptr,&scene_reactive_shader)) ||
        FAILED(device->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&pixel_shader))) { gpu_close();return false; }
    const D3D11_INPUT_ELEMENT_DESC elements[]={
        {"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,32,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"TEXCOORD",1,DXGI_FORMAT_R32G32B32A32_FLOAT,0,40,D3D11_INPUT_PER_VERTEX_DATA,0}};
    if(FAILED(device->CreateInputLayout(elements,4,vs->GetBufferPointer(),vs->GetBufferSize(),&layout))) { gpu_close();return false; }
    D3D11_BUFFER_DESC constants{};constants.ByteWidth=16;constants.Usage=D3D11_USAGE_DEFAULT;
    constants.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
    if(FAILED(device->CreateBuffer(&constants,nullptr,&draw_params))) { gpu_close();return false; }
    constants.ByteWidth=36*sizeof(uint32_t);
    if(FAILED(device->CreateBuffer(&constants,nullptr,&toon_params))) {gpu_close();return false;}
    D3D11_SAMPLER_DESC sampler{};
    sampler.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;
    sampler.MaxLOD=D3D11_FLOAT32_MAX;
    for(unsigned flags=0;flags<16;++flags) {
        sampler.AddressU=!(flags&1)?D3D11_TEXTURE_ADDRESS_CLAMP:flags&4?D3D11_TEXTURE_ADDRESS_MIRROR:D3D11_TEXTURE_ADDRESS_WRAP;
        sampler.AddressV=!(flags&2)?D3D11_TEXTURE_ADDRESS_CLAMP:flags&8?D3D11_TEXTURE_ADDRESS_MIRROR:D3D11_TEXTURE_ADDRESS_WRAP;
        if(FAILED(device->CreateSamplerState(&sampler,&texture_samplers[flags]))) {gpu_close();return false;}
    }
    const uint8_t caps[]={2,4,8,16};
    bool anisotropic_available=true;
    for(unsigned level=0;level<4 && anisotropic_available;++level) for(unsigned flags=0;flags<16;++flags) {
        sampler.Filter=D3D11_FILTER_ANISOTROPIC;
        sampler.MaxAnisotropy=caps[level];
        sampler.AddressU=!(flags&1)?D3D11_TEXTURE_ADDRESS_CLAMP:flags&4?D3D11_TEXTURE_ADDRESS_MIRROR:D3D11_TEXTURE_ADDRESS_WRAP;
        sampler.AddressV=!(flags&2)?D3D11_TEXTURE_ADDRESS_CLAMP:flags&8?D3D11_TEXTURE_ADDRESS_MIRROR:D3D11_TEXTURE_ADDRESS_WRAP;
        if(FAILED(device->CreateSamplerState(&sampler,&anisotropic_samplers[level][flags]))) {
            anisotropic_available=false;
            break;
        }
    }
    if(!anisotropic_available) {
        for(auto &level:anisotropic_samplers) for(auto &state:level) state.Reset();
        anisotropic_level=0;
        std::fprintf(stderr,"[native GPU] anisotropic filtering unavailable; using original sampling\n");
    }
    sampler.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampler.MaxAnisotropy=1;
    sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
    if(FAILED(device->CreateSamplerState(&sampler,&nearest_clamp))) { gpu_close();return false; }
    sampler.Filter=D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    if(FAILED(device->CreateSamplerState(&sampler,&linear_clamp))) { gpu_close();return false; }
    D3D11_DEPTH_STENCIL_DESC ds{}; ds.DepthEnable=TRUE;ds.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;ds.DepthFunc=D3D11_COMPARISON_LESS;
    if(FAILED(device->CreateDepthStencilState(&ds,&depth_on))) { gpu_close();return false; }
    ds.DepthEnable=FALSE;ds.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;
    if(FAILED(device->CreateDepthStencilState(&ds,&depth_off))) { gpu_close();return false; }
    for(unsigned write=0;write<2;++write) for(unsigned equal=0;equal<2;++equal) {
        D3D11_DEPTH_STENCIL_DESC n{};
        n.DepthEnable=TRUE;n.DepthWriteMask=write?D3D11_DEPTH_WRITE_MASK_ALL:D3D11_DEPTH_WRITE_MASK_ZERO;
        n.DepthFunc=equal?D3D11_COMPARISON_EQUAL:D3D11_COMPARISON_LESS;
        n.StencilEnable=TRUE;n.StencilReadMask=0xbf;n.StencilWriteMask=0xbf;
        n.FrontFace={D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_REPLACE,D3D11_COMPARISON_ALWAYS};
        n.BackFace=n.FrontFace;
        if(FAILED(device->CreateDepthStencilState(&n,&normal_depth[write][equal]))) { gpu_close();return false; }
        n.FrontFace.StencilFunc=n.BackFace.StencilFunc=D3D11_COMPARISON_NOT_EQUAL;
        if(FAILED(device->CreateDepthStencilState(&n,&translucent_depth[write][equal]))) { gpu_close();return false; }
    }
    for(unsigned equal=0;equal<2;++equal) {
        D3D11_DEPTH_STENCIL_DESC n{};
        n.DepthEnable=TRUE;n.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;
        n.DepthFunc=equal?D3D11_COMPARISON_EQUAL:D3D11_COMPARISON_LESS;
        n.StencilEnable=TRUE;n.StencilReadMask=0x40;n.StencilWriteMask=0x40;
        n.FrontFace={D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_REPLACE,D3D11_STENCIL_OP_KEEP,D3D11_COMPARISON_ALWAYS};
        n.BackFace=n.FrontFace;
        if(FAILED(device->CreateDepthStencilState(&n,&shadow_mask[equal]))) { gpu_close();return false; }
        n.StencilWriteMask=0x7f;
        n.FrontFace={D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_REPLACE,D3D11_COMPARISON_NOT_EQUAL};
        n.BackFace=n.FrontFace;
        if(FAILED(device->CreateDepthStencilState(&n,&shadow_draw[equal]))) { gpu_close();return false; }
    }
    D3D11_DEPTH_STENCIL_DESC stencil{};stencil.DepthEnable=FALSE;stencil.StencilEnable=TRUE;
    stencil.StencilReadMask=0x3f;stencil.StencilWriteMask=0x40;
    stencil.FrontFace={D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_ZERO,D3D11_COMPARISON_EQUAL};
    stencil.BackFace=stencil.FrontFace;
    if(FAILED(device->CreateDepthStencilState(&stencil,&shadow_exclude))) { gpu_close();return false; }
    stencil.FrontFace.StencilFunc=stencil.BackFace.StencilFunc=D3D11_COMPARISON_ALWAYS;
    if(FAILED(device->CreateDepthStencilState(&stencil,&shadow_clear))) { gpu_close();return false; }
    D3D11_BLEND_DESC blend{}; auto &b=blend.RenderTarget[0];
    b.RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;
    if(FAILED(device->CreateBlendState(&blend,&blend_off))) { gpu_close();return false; }
    b.RenderTargetWriteMask=0;
    if(FAILED(device->CreateBlendState(&blend,&color_off))) { gpu_close();return false; }
    b.RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;
    b.BlendEnable=TRUE;b.SrcBlend=D3D11_BLEND_SRC_ALPHA;b.DestBlend=D3D11_BLEND_INV_SRC_ALPHA;b.BlendOp=D3D11_BLEND_OP_ADD;
    b.SrcBlendAlpha=D3D11_BLEND_ONE;b.DestBlendAlpha=D3D11_BLEND_INV_SRC_ALPHA;b.BlendOpAlpha=D3D11_BLEND_OP_ADD;
    if(FAILED(device->CreateBlendState(&blend,&blend_on))) { gpu_close();return false; }
    blend.IndependentBlendEnable=TRUE;
    auto &reactive=blend.RenderTarget[1];
    reactive=b;
    reactive.SrcBlend=reactive.DestBlend=D3D11_BLEND_ONE;
    reactive.BlendOp=D3D11_BLEND_OP_MAX;
    reactive.RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_RED;
    if(FAILED(device->CreateBlendState(&blend,&blend_reactive))) { gpu_close();return false; }
    D3D11_RASTERIZER_DESC raster{};raster.FillMode=D3D11_FILL_SOLID;raster.CullMode=D3D11_CULL_NONE;raster.DepthClipEnable=TRUE;
    const uint32_t white=0xffffffff;
    if(FAILED(device->CreateRasterizerState(&raster,&rasterizer)) ||
        !texture(base_texture,nullptr,SCREEN_W,SCREEN_H) || !texture(map_texture,nullptr,SCREEN_W,SCREEN_H) ||
        !texture(white_texture,&white,1,1)) { gpu_close();return false; }
    raster.FillMode=D3D11_FILL_WIREFRAME;
    if(FAILED(device->CreateRasterizerState(&raster,&wireframe))) { gpu_close();return false; }
    if(live_fsr.device) {
        live_fsr.device=prepare_scene_input_shader();
        std::fprintf(stderr,"[FSR bridge] scene shader prepared=%u\n",unsigned(live_fsr.device));
    }
    gx_set_temporal_motion(live_fsr.device);
    std::fprintf(stderr,"[native GPU] D3D11 ready: %s, feature %04x\n",adapter_name,selected);
    return true;
}
bool gpu_open(void *window,bool warp,VideoRenderer renderer,bool request_fsr) {
    const bool wants_fsr=renderer==VideoRenderer::D3D11 && (request_fsr || live_fsr_start_tick());
    if(gpu_open_renderer(window,warp,renderer,wants_fsr))return true;
    return wants_fsr && gpu_open_renderer(window,warp,renderer,false);
}

bool gpu_recover_presentation(void *window) {
    if(flip_swap && fsr_device_lost())
        return gpu_open_renderer(window,false,VideoRenderer::D3D11,false);
    gpu_close();
    return false;
}

static void reset_flip_buffers() {
    for(auto &view:flip_views)view.Reset();
    for(auto &buffer:flip_buffers)buffer.Reset();
}

void gpu_close() {
    glsw_close();using_gl_software=false;
    if(context) { context->ClearState();context->Flush(); }
    window_view.Reset();
    reset_flip_buffers();
    if(flip_swap)fsr_drain_present_queue();
    flip_swap.Reset();swap.Reset();
    fsr_device_close();live_fsr={};gx_set_temporal_motion(false);
    texture_cache.clear();cache_bytes=0;use_counter=0;cache_epoch=0;unavailable_replacements.clear();
    base_texture={};map_texture={};packed_map_texture={};white_texture={};hud_texture={};hud_texture_revision=~uint64_t(0);
    gameplay_cache.Reset();staging.Reset();output_raster={};upscale_raster={};raster=&output_raster;
    motion_source={};motion_history={};scene_camera_history={};scene_depth_history={};motion_shader.Reset();motion_params.Reset();motion_failed=false;
    reset_scene_capture_targets();scene_motion_shader.Reset();scene_reactive_shader.Reset();blend_reactive.Reset();
    for(auto &shader:scene_input_shaders)shader.Reset();
    retirement_query.Reset();vertex_shader.Reset();pixel_shader.Reset();layout.Reset();vertex_buffer.Reset();draw_params.Reset();toon_params.Reset();
    for(auto &sampler:texture_samplers)sampler.Reset();
    for(auto &level:anisotropic_samplers) for(auto &sampler:level) sampler.Reset();
    anisotropic_level=0;
    nearest_clamp.Reset();linear_clamp.Reset();depth_on.Reset();depth_off.Reset();
    for(unsigned i=0;i<2;++i) { shadow_mask[i].Reset();shadow_draw[i].Reset();for(auto &state:normal_depth[i]) state.Reset();for(auto &state:translucent_depth[i]) state.Reset(); }
    shadow_exclude.Reset();shadow_clear.Reset();
    blend_on.Reset();blend_off.Reset();color_off.Reset();rasterizer.Reset();wireframe.Reset();context.Reset();device.Reset();
    window_width=window_height=buffer_capacity=triangles_drawn=0;
    adapter_name[0]=0;
}
void gpu_set_menu_scene(bool active) {menu_scene=active;glsw_set_menu_scene(active);}
void gpu_set_letter_overlay(bool active) {letter_overlay=active;glsw_set_letter_overlay(active);}
const char *gpu_fsr_notice() {
    return !live_fsr.device || live_fsr.failed?"FSR UNAVAILABLE":"";
}
void gpu_set_anisotropic_filtering(uint8_t level) {
    if(using_gl_software){glsw_set_anisotropic_filtering(level);return;}
    if(level!=2 && level!=4 && level!=8 && level!=16) level=0;
    bool available=level==0;
    if(level==2) available=anisotropic_samplers[0][0]!=nullptr;
    if(level==4) available=anisotropic_samplers[1][0]!=nullptr;
    if(level==8) available=anisotropic_samplers[2][0]!=nullptr;
    if(level==16) available=anisotropic_samplers[3][0]!=nullptr;
    anisotropic_level=available?level:0;
}
uint8_t gpu_anisotropic_filtering() {return using_gl_software?glsw_anisotropic_filtering():anisotropic_level;}
bool gpu_render(const Framebuffer &base,const uint32_t *map,const VideoSettings &s,bool draw_geometry,const uint32_t *foreground,
                const DualScreenLayout *screens,const Framebuffer *sub,bool engine_a_on_top,bool cache_gameplay,
                const uint32_t *hud,unsigned hud_width,unsigned hud_height,uint64_t hud_revision,bool motion_gameplay,
                unsigned map_width,unsigned map_height) {
    if(map && (!map_width || !map_height || map_width>s.width || map_height>s.height))return false;
    const bool gameplay_camera=motion_gameplay && draw_geometry && !screens && !menu_scene && !letter_overlay;
    const auto prediction=gameplay_camera && s.target_fps>60?gx_camera_prediction():GxCameraPrediction{};
    if(using_gl_software)return glsw_render(base,map,s,draw_geometry,foreground,screens,sub,
        engine_a_on_top,cache_gameplay,hud,hud_width,hud_height,hud_revision,map_width,map_height,prediction);
    frame_stats={};
    frame_stats.camera_prediction=prediction.fraction;
    struct Timer {
        std::chrono::steady_clock::time_point start=std::chrono::steady_clock::now();
        ~Timer() {frame_stats.cache_bytes=cache_bytes;frame_stats.cache_budget=cache_budget;frame_stats.cpu_us=static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count());}
    } timer;
    if(!device || s.width<256 || s.width>3840 || s.height<192 || s.height>2160 ||
        !resize_target(s.width,s.height)) return false;
    if(hud && (hud_width!=s.width || hud_height!=s.height))return false;
    if(screens && !sub)return false;
    if(!sync_replacements())return false;
    ID3D11ShaderResourceView *null_view=nullptr;context->PSSetShaderResources(0,1,&null_view);
    context->RSSetState(rasterizer.Get());context->IASetInputLayout(layout.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vertex_shader.Get(),nullptr,0);context->PSSetShader(pixel_shader.Get(),nullptr,0);
    // The main view includes the DS backdrop even where the transparent map
    // scanout marks alpha zero. Match the original display's RGB presentation;
    // only the separate minimap overlay should use that alpha as transparency.
    static Framebuffer opaque_base;
    for(unsigned y=0;y<SCREEN_H;++y) for(unsigned x=0;x<SCREEN_W;++x)
        opaque_base.px[y][x]=base.px[y][x]|0xff000000u;
    context->UpdateSubresource(base_texture.resource.Get(),0,nullptr,opaque_base.px,SCREEN_W*4,0);
    triangles_drawn=0;
    const bool eligible_history=gameplay_camera && prediction.fraction==0;
    update_scene_camera(gameplay_camera,live_fsr.device?prediction:GxCameraPrediction{});
    const bool wants_upscale=live_fsr_start_tick()?live_fsr_quality():fsr_upscales(s.fsr);
    const auto live=prepare_live_fsr(gameplay_camera && !(wants_upscale && s.motion_blur),s.fsr,prediction);
    const bool live_frame=live==FsrPresentation::NewScene;
    const bool repeated=live==FsrPresentation::RepeatScene;
    const bool warm_up=live==FsrPresentation::WarmUp;
    const bool upscale=wants_upscale && live!=FsrPresentation::Inactive;
    const char *scene_capture=live==FsrPresentation::Inactive?prepare_scene_capture(eligible_history):nullptr;
    const auto offset=live_frame || repeated?fsr_sample_offset(live_fsr.frames-unsigned(repeated),live_fsr.phases):
        scene_sample_offset(!warm_up && eligible_history);
    const auto render_scene=[&](bool capture,SceneSampleOffset sample,ID3D11Texture2D *opaque_snapshot=nullptr) {
        ID3D11RenderTargetView *view=raster->color_view.Get();
        context->OMSetRenderTargets(1,&view,raster->depth_view.Get());
        D3D11_VIEWPORT viewport{0,0,float(raster->width),float(raster->height),0,1};
        if(screens) {
            const auto &screen=engine_a_on_top?screens->top:screens->bottom;
            viewport={float(screen.x),float(screen.y),float(screen.width),float(screen.height),0,1};
        }
        context->RSSetViewports(1,&viewport);
        context->OMSetDepthStencilState(depth_off.Get(),0);context->OMSetBlendState(blend_off.Get(),nullptr,~0u);
        context->PSSetShader(pixel_shader.Get(),nullptr,0);
        const float black[4]={0,0,0,1};
        context->ClearRenderTargetView(view,black);
        context->ClearDepthStencilView(raster->depth_view.Get(),D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL,1,0);
        triangles_drawn=0;
        return quad(base_texture.view.Get(),s.upscale_2d?nearest_clamp.Get():linear_clamp.Get()) &&
            (!draw_geometry || geometry(screens!=nullptr,prediction,capture,sample,opaque_snapshot));
    };
    bool resolved=false;
    const bool repeat_needs_depth=repeated && s.motion_blur;
    if(!repeated || repeat_needs_depth) {
        raster=upscale?&upscale_raster:&output_raster;
        const bool rendered=render_scene(live_frame || warm_up || scene_capture,offset,
                                         scene_capture?scene_opaque_color.Get():nullptr);
        if(rendered && (live_frame || warm_up))resolved=finish_live_fsr(offset,prediction,warm_up);
        raster=&output_raster;
        if(!rendered)return false;
    }
    if(repeated && (resolved=draw_fsr_output(fsr_bridge_output())) && trace_fsr_frames())
        std::fprintf(stderr,"[FSR bridge] reused tick=%llu serial=%llu phase=%.2f\n",
                     static_cast<unsigned long long>(rt_frame()),static_cast<unsigned long long>(live_fsr.last_serial),
                     double(rt_presentation_phase()));
    if(((upscale && warm_up) || (!resolved && (upscale || (repeated && !repeat_needs_depth)))) &&
       !render_scene(false,{}))return false;
    finish_scene_capture(scene_capture,offset);
    remember_scene_depth(eligible_history,offset);
    draw_motion_blur(s.motion_blur && gameplay_camera);
    if(draw_geometry && foreground) {
        context->OMSetDepthStencilState(depth_off.Get(),0);context->OMSetBlendState(blend_on.Get(),nullptr,~0u);
        context->UpdateSubresource(base_texture.resource.Get(),0,nullptr,foreground,SCREEN_W*4,0);
        const float aspect=letter_overlay && !screens && !menu_scene
            ? float(SCREEN_W)*s.height/(float(SCREEN_H)*s.width) : 1.0f;
        if(!quad(base_texture.view.Get(),nearest_clamp.Get(),-aspect,1,aspect,-1)) return false;
    }
    if(screens && (engine_a_on_top?screens->bottom:screens->top).width>0 && (engine_a_on_top?screens->bottom:screens->top).height>0) {
        const auto &r=engine_a_on_top?screens->bottom:screens->top;
        D3D11_VIEWPORT sub_view{float(r.x),float(r.y),float(r.width),float(r.height),0,1};
        context->RSSetViewports(1,&sub_view);
        context->OMSetDepthStencilState(depth_off.Get(),0);context->OMSetBlendState(blend_off.Get(),nullptr,~0u);
        for(unsigned y=0;y<SCREEN_H;++y) for(unsigned x=0;x<SCREEN_W;++x)
            opaque_base.px[y][x]=sub->px[y][x]|0xff000000u;
        context->UpdateSubresource(map_texture.resource.Get(),0,nullptr,opaque_base.px,SCREEN_W*4,0);
        if(!quad(map_texture.view.Get(),s.upscale_2d?nearest_clamp.Get():linear_clamp.Get())) return false;
    }
    if(cache_gameplay && !screens) {
        D3D11_TEXTURE2D_DESC desc{},old{};raster->color->GetDesc(&desc);
        if(gameplay_cache)gameplay_cache->GetDesc(&old);
        if(!gameplay_cache || old.Width!=desc.Width || old.Height!=desc.Height) {
            gameplay_cache.Reset();desc.BindFlags=0;
            if(!hr_ok(device->CreateTexture2D(&desc,nullptr,&gameplay_cache),"gameplay preview cache"))return false;
        }
        context->CopyResource(gameplay_cache.Get(),raster->color.Get());
    }
    if(!screens && map && s.minimap) {
        auto *minimap=&map_texture;
        if(map_width!=SCREEN_W || map_height!=SCREEN_H) {
            if(packed_map_texture.width!=map_width || packed_map_texture.height!=map_height)
                if(!texture(packed_map_texture,nullptr,map_width,map_height))return false;
            minimap=&packed_map_texture;
        }
        context->OMSetDepthStencilState(depth_off.Get(),0);context->OMSetBlendState(blend_on.Get(),nullptr,~0u);
        context->UpdateSubresource(minimap->resource.Get(),0,nullptr,map,map_width*4,0);
        const float x=s.minimap_x/1000.0f,y=s.minimap_y/1000.0f,size=s.minimap_scale/1000.0f;
        // Preserve the old overlay's proportional layout in both dimensions.
        if(!quad(minimap->view.Get(),nearest_clamp.Get(),x*2-1,1-y*2,(x+size)*2-1,1-(y+size)*2)) return false;
    }
    if(!screens && hud) {
        context->OMSetDepthStencilState(depth_off.Get(),0);context->OMSetBlendState(blend_on.Get(),nullptr,~0u);
        if(!hud_texture.resource || hud_texture.width!=hud_width || hud_texture.height!=hud_height) {
            if(!texture(hud_texture,nullptr,hud_width,hud_height))return false;
            hud_texture_revision=~uint64_t(0);
        }
        if(hud_texture_revision!=hud_revision) {
            context->UpdateSubresource(hud_texture.resource.Get(),0,nullptr,hud,hud_width*4,0);
            hud_texture_revision=hud_revision;
        }
        if(!quad(hud_texture.view.Get(),nearest_clamp.Get(),-1,1,1,-1))return false;
    }
    context->PSSetShaderResources(0,1,&null_view);
    return true;
}
bool gpu_present(unsigned width,unsigned height,bool preserve_aspect) {
    if(using_gl_software)return glsw_present(width,height,preserve_aspect);
    if(!swap || !output_raster.color || !width || !height) return false;
    if(width!=window_width || height!=window_height) {
        context->OMSetRenderTargets(0,nullptr,nullptr);window_view.Reset();
        reset_flip_buffers();
        if(flip_swap && !fsr_drain_present_queue())return false;
        if(!hr_ok(swap->ResizeBuffers(0,width,height,DXGI_FORMAT_UNKNOWN,0),"window resize")) return false;
        window_width=width;window_height=height;
    }
    ID3D11Resource *wrapped=nullptr;
    ID3D11RenderTargetView *rt=nullptr;
    if(flip_swap) {
        const unsigned index=flip_swap->GetCurrentBackBufferIndex();
        if(index>=flip_buffers.size())return false;
        auto &buffer=flip_buffers[index];
        auto &view=flip_views[index];
        if(!view && (!fsr_wrap_back_buffer(flip_swap.Get(),index,&buffer) ||
                     FAILED(device->CreateRenderTargetView(buffer.Get(),nullptr,&view))))return false;
        wrapped=buffer.Get();rt=view.Get();
        fsr_acquire_back_buffer(wrapped);
    } else {
        if(!window_view) {
            ComPtr<ID3D11Texture2D> buffer;
            if(FAILED(swap->GetBuffer(0,IID_PPV_ARGS(&buffer))) ||
               FAILED(device->CreateRenderTargetView(buffer.Get(),nullptr,&window_view)))return false;
        }
        rt=window_view.Get();
    }
    context->OMSetRenderTargets(1,&rt,nullptr);
    const float black[4]={0,0,0,1};context->ClearRenderTargetView(rt,black);
    const auto r=preserve_aspect?screen_fit(raster->width,raster->height,{0,0,int(width),int(height)}):ScreenRect{0,0,int(width),int(height)};
    D3D11_VIEWPORT vp{float(r.x),float(r.y),float(r.width),float(r.height),0,1};context->RSSetViewports(1,&vp);
    context->OMSetDepthStencilState(depth_off.Get(),0);context->OMSetBlendState(blend_off.Get(),nullptr,~0u);
    const bool drawn=quad(raster->color_sample.Get(),linear_clamp.Get());
    ID3D11ShaderResourceView *null_view=nullptr;context->PSSetShaderResources(0,1,&null_view);
    if(wrapped) {
        context->OMSetRenderTargets(0,nullptr,nullptr);
        fsr_release_back_buffer(wrapped);
    }
    if(!drawn)return false;
    if(flip_swap)fsr_test_device_loss(rt_frame());
    const auto presented=swap->Present(0,0);
    if(FAILED(presented) && flip_swap)fsr_report_device_status("swap chain present");
    return hr_ok(presented,"present");
}
static bool read_texture(ID3D11Texture2D *texture,std::vector<uint32_t> &pixels,unsigned &width,unsigned &height) {
    if(!texture) return false;
    D3D11_TEXTURE2D_DESC desc{},old{};texture->GetDesc(&desc);
    if(staging)staging->GetDesc(&old);
    if(!staging || old.Width!=desc.Width || old.Height!=desc.Height) {
        staging.Reset();
        desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        if(!hr_ok(device->CreateTexture2D(&desc,nullptr,&staging),"capture staging")) return false;
    }
    context->CopyResource(staging.Get(),texture);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if(!hr_ok(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped),"capture readback")) return false;
    width=desc.Width;height=desc.Height;pixels.resize(static_cast<size_t>(width)*height);
    for(unsigned y=0;y<height;++y) std::memcpy(pixels.data()+static_cast<size_t>(y)*width,
        static_cast<const char *>(mapped.pData)+y*mapped.RowPitch,width*4);
    context->Unmap(staging.Get(),0);
    return true;
}
bool gpu_readback(std::vector<uint32_t> &pixels,unsigned &width,unsigned &height) {
    return using_gl_software?glsw_readback(pixels,width,height):read_texture(raster->color.Get(),pixels,width,height);
}
bool gpu_readback_gameplay(std::vector<uint32_t> &pixels,unsigned &width,unsigned &height) {
    return using_gl_software?glsw_readback_gameplay(pixels,width,height):read_texture(gameplay_cache.Get(),pixels,width,height);
}
const char *gpu_adapter() { return using_gl_software?glsw_adapter():adapter_name; }
unsigned gpu_triangle_count() { return using_gl_software?glsw_triangle_count():triangles_drawn; }
GpuFrameStats gpu_frame_stats() {return using_gl_software?glsw_frame_stats():frame_stats;}
void gpu_force_texture_scan(bool force) {force_texture_scan=force;glsw_force_texture_scan(force);}
void gpu_set_texture_budget_bytes(uint64_t bytes) {cache_budget=std::max<uint64_t>(64ull*1024*1024,bytes);glsw_set_texture_budget_bytes(bytes);}
}
#else
namespace ntr {
bool gpu_open(void *,bool,VideoRenderer,bool) { return false; }
bool gpu_recover_presentation(void *) { return false; }
const char *gpu_fsr_notice() { return "FSR UNAVAILABLE"; }
void gpu_close() {}
void gpu_set_anisotropic_filtering(uint8_t) {}
uint8_t gpu_anisotropic_filtering() { return 0; }
bool gpu_render(const Framebuffer &,const uint32_t *,const VideoSettings &,bool,const uint32_t *,
                const DualScreenLayout *,const Framebuffer *,bool,bool,const uint32_t *,unsigned,unsigned,uint64_t,bool,
                unsigned,unsigned) { return false; }
void gpu_set_menu_scene(bool) {}
void gpu_set_letter_overlay(bool) {}
bool gpu_present(unsigned,unsigned,bool) { return false; }
bool gpu_readback(std::vector<uint32_t> &,unsigned &,unsigned &) { return false; }
bool gpu_readback_gameplay(std::vector<uint32_t> &,unsigned &,unsigned &) { return false; }
const char *gpu_adapter() { return "unavailable"; }
unsigned gpu_triangle_count() { return 0; }
GpuFrameStats gpu_frame_stats() {return {};}
void gpu_force_texture_scan(bool) {}
void gpu_set_texture_budget_bytes(uint64_t) {}
}
#endif
