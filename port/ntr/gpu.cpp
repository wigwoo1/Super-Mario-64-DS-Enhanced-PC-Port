// Native-resolution D3D11 rasterization of the recovered game's geometry.
// DS command decode, lighting, clipping, UVs and original textures stay shared
// with the software reference. Only rasterization/presentation run on the GPU.
#include "ntr/gpu.h"
#include "ntr/gl_software.h"
#include "ntr/gx.h"
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
#include <wrl/client.h>

namespace ntr {
namespace {
using Microsoft::WRL::ComPtr;
struct Vertex { float p[4],color[4],uv[2]; };
struct Texture {
    ComPtr<ID3D11Texture2D> resource; ComPtr<ID3D11ShaderResourceView> view;
    bool partial_alpha=false;uint64_t bytes=0,last_used=0;
};
struct FrameMaterial {
    const uint32_t *pixels=nullptr;int width=0,height=0;
    TexturePixelsInfo info;TextureReplacementRef replacement;
};
struct Group { FrameMaterial *material; unsigned first,count; uint32_t attributes; bool partial_alpha=false; uint8_t texture_address=3; };
ComPtr<ID3D11Device> device;
ComPtr<ID3D11DeviceContext> context;
ComPtr<IDXGISwapChain> swap;
ComPtr<ID3D11RenderTargetView> window_view, target_view;
ComPtr<ID3D11Texture2D> target, depth, staging;
ComPtr<ID3D11Texture2D> gameplay_cache;
ComPtr<ID3D11DepthStencilView> depth_view;
ComPtr<ID3D11ShaderResourceView> target_sample;
ComPtr<ID3D11VertexShader> vertex_shader;
ComPtr<ID3D11PixelShader> pixel_shader;
ComPtr<ID3D11InputLayout> layout;
ComPtr<ID3D11Buffer> vertex_buffer;
ComPtr<ID3D11Buffer> toon_params;
ComPtr<ID3D11Buffer> draw_params;
ComPtr<ID3D11Query> retirement_query;
ComPtr<ID3D11SamplerState> texture_samplers[16],anisotropic_samplers[4][16],nearest_clamp,linear_clamp;
ComPtr<ID3D11DepthStencilState> depth_on,depth_off;
ComPtr<ID3D11DepthStencilState> normal_depth[2][2],translucent_depth[2][2],shadow_mask[2],shadow_draw[2],shadow_exclude,shadow_clear;
ComPtr<ID3D11BlendState> blend_on,blend_off,color_off;
ComPtr<ID3D11RasterizerState> rasterizer,wireframe;
Texture base_texture,map_texture,white_texture;
using CacheKey=std::tuple<bool,uint64_t,uint32_t>;
std::map<CacheKey,Texture> texture_cache;
std::set<uint64_t> unavailable_replacements;
uint64_t cache_bytes=0,cache_budget=256ull*1024*1024,use_counter=0;
uint32_t cache_epoch=0;
GpuFrameStats frame_stats;
bool force_texture_scan=false;
bool using_gl_software=false;
uint8_t anisotropic_level=0;
unsigned target_width,target_height,window_width,window_height,buffer_capacity,triangles_drawn;
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
    return hr_ok(device->CreateTexture2D(&desc,pixels?&data:nullptr,&out.resource),"texture") &&
        hr_ok(device->CreateShaderResourceView(out.resource.Get(),nullptr,&out.view),"texture view");
}
bool resize_target(unsigned width,unsigned height) {
    if (target && width==target_width && height==target_height) return true;
    context->OMSetRenderTargets(0,nullptr,nullptr);
    ID3D11ShaderResourceView *null_view=nullptr;
    context->PSSetShaderResources(0,1,&null_view);
    target.Reset(); target_view.Reset(); target_sample.Reset();
    depth.Reset(); depth_view.Reset(); staging.Reset();
    target_width=target_height=0;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width=width; desc.Height=height; desc.MipLevels=desc.ArraySize=1;
    desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM; desc.SampleDesc.Count=1;
    desc.Usage=D3D11_USAGE_DEFAULT;
    desc.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
    if (!hr_ok(device->CreateTexture2D(&desc,nullptr,&target),"render target") ||
        !hr_ok(device->CreateRenderTargetView(target.Get(),nullptr,&target_view),"render view") ||
        !hr_ok(device->CreateShaderResourceView(target.Get(),nullptr,&target_sample),"render sample")) return false;
    desc.Format=DXGI_FORMAT_D24_UNORM_S8_UINT; desc.BindFlags=D3D11_BIND_DEPTH_STENCIL;
    if (!hr_ok(device->CreateTexture2D(&desc,nullptr,&depth),"depth target") ||
        !hr_ok(device->CreateDepthStencilView(depth.Get(),nullptr,&depth_view),"depth view")) return false;
    target_width=width; target_height=height;
    std::fprintf(stderr,"[native GPU] raster target %ux%u, adapter %s\n",width,height,adapter_name);
    return true;
}
Vertex vertex(float x,float y,float z,float w,float u,float v,uint32_t color) {
    Vertex out{{x,y,z,w},{((color>>16)&255)/255.0f,((color>>8)&255)/255.0f,
        (color&255)/255.0f,((color>>24)&255)/255.0f},{u,v}};
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
          float left=-1,float top=1,float right=1,float bottom=-1) {
    std::vector<Vertex> v{
        vertex(left,top,0,1,0,0,0xffffffff),vertex(right,top,0,1,1,0,0xffffffff),
        vertex(left,bottom,0,1,0,1,0xffffffff),vertex(left,bottom,0,1,0,1,0xffffffff),
        vertex(right,top,0,1,1,0,0xffffffff),vertex(right,bottom,0,1,1,1,0xffffffff)};
    if (!upload(v)) return false;
    parameters();bind_texture(view,sampler); context->Draw(6,0); return true;
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
    if(texture_backend()==TextureBackend::NativeGpu && texture_upscaled() && r.eligible &&
       (r.role==TextureRole::Level || r.role==TextureRole::Object)) {
        const CacheKey key{true,r.key,cache_epoch};
        auto cached=texture_cache.find(key);
        if(cached!=texture_cache.end()) {
            cached->second.last_used=++use_counter;++frame_stats.cache_hits;return &cached->second;
        }
        if(!unavailable_replacements.count(r.key)) {
            ++frame_stats.replacement_attempts;
            std::vector<uint32_t> staging_pixels;int width=0,height=0;
            if(texture_load_replacement_pixels(r.key,staging_pixels,width,height)) {
                frame_stats.staging_peak_bytes=std::max(frame_stats.staging_peak_bytes,
                    static_cast<uint64_t>(staging_pixels.size())*4);
                if(auto *result=cache_upload(key,staging_pixels.data(),width,height)) {
                    ++frame_stats.replacement_uploads;return result;
                }
            }
            // Missing, invalid, over-limit or failed uploads use original art.
            // A settings change starts a fresh epoch and retries these keys.
            unavailable_replacements.insert(r.key);
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
bool geometry(bool dual_screen=false) {
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
    size_t count=0; const GxTriangle *source=gx_display_polygons(count);
    for(size_t i=0;i<count;++i)if(((source[i].attributes>>4)&3)==2) {
        uint32_t values[36]={};bool highlight=false;
        gx_read_toon_table(values,highlight);values[32]=highlight?1:0;
        context->UpdateSubresource(toon_params.Get(),0,nullptr,values,0,0);
        ID3D11Buffer *buffer=toon_params.Get();context->PSSetConstantBuffers(1,1,&buffer);
        break;
    }
    for(size_t i=0;i<count;++i) {
        auto t=source[i]; GxVertex clipped[12];
        if(t.perspective && !menu_scene && !dual_screen) {
            // Keep vertical FOV and original proportions. Widen the horizontal
            // frustum before clipping so widescreen reveals real extra geometry.
            const float aspect=(float(SCREEN_W)*target_height)/(float(SCREEN_H)*target_width);
            for(auto &v:t.v) v.clip_x*=aspect;
        }
        const unsigned n=gx_clip_polygon(t,clipped);
        for(unsigned j=1;j+1<n;++j) {
            const GxVertex v[3]={clipped[0],clipped[j],clipped[j+1]};
            const float area=(v[1].x-v[0].x)*(v[2].y-v[0].y)-(v[1].y-v[0].y)*(v[2].x-v[0].x);
            if(std::fabs(area)<1e-6f || !(t.attributes&(area<0?(1u<<7):(1u<<6)))) continue;
            auto *m=material(t,frame_cache);
            if(groups.empty() || groups.back().material!=m || groups.back().attributes!=t.attributes || groups.back().texture_address!=t.texture_address)
                groups.push_back({m,static_cast<unsigned>(vertices.size()),0,t.attributes,false,t.texture_address});
            for(const auto &a:v) {
                // Preserve the retail viewport, mapping its logical 256x192
                // coordinates directly to the full-resolution raster target.
                const float x=a.clip_x*t.viewport_w/SCREEN_W+a.w*(2.0f*t.viewport_x/SCREEN_W+float(t.viewport_w)/SCREEN_W-1);
                const float y=a.clip_y*t.viewport_h/SCREEN_H+a.w*(1-2.0f*t.viewport_y/SCREEN_H-float(t.viewport_h)/SCREEN_H);
                vertices.push_back(vertex(x,y,(a.clip_z+a.w)*0.5f,a.w,
                    t.tw>0?a.u/t.tw:0,t.th>0?a.v/t.th:0,a.color));
                if(((t.attributes>>4)&3)==2)vertices.back().color[0]=a.toon_intensity/511;
            }
            groups.back().count+=3; ++triangles_drawn;
        }
    }
    if(groups.empty()) return true;
    if(!upload(vertices)) return false;
    bool rendered_shadow=false;
    // Hardware renders opaque pixels first, followed by translucent polygons
    // in submission order. This also places shadow volumes after their targets.
    for(unsigned pass=0;pass<2;++pass) for(auto &group:groups) {
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
                context->OMSetBlendState(blend_on.Get(),nullptr,~0u);
                context->Draw(group.count,group.first);
                rendered_shadow=true;
            }
        } else {
            const unsigned write_depth=!pass || (group.attributes&(1u<<11));
            // Bit7 distinguishes a previous translucent pixel from an opaque
            // receiver with the same six-bit ID. DS rejects repeat blending
            // within one translucent object, including Peach during her fade.
            context->OMSetDepthStencilState(pass?translucent_depth[write_depth?1:0][equal].Get():normal_depth[write_depth?1:0][equal].Get(),pass?(0x80|id):id);
            context->OMSetBlendState(pass?blend_on.Get():blend_off.Get(),nullptr,~0u);
            context->Draw(group.count,group.first);
        }
    }
    context->RSSetState(rasterizer.Get());
    return true;
}
}

bool gpu_open(void *window,bool warp,VideoRenderer renderer) {
    const char *force=std::getenv("SM64DS_PC_GPU_FORCE_TEXTURE_SCAN");
    force_texture_scan=force && force[0]=='1';
    gpu_close();
    if(renderer==VideoRenderer::OpenGLSoftware) {
        using_gl_software=glsw_open(window);
        return using_gl_software;
    }
    const D3D_FEATURE_LEVEL levels[]={D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_10_1,D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL selected{};
    HRESULT hr=D3D11CreateDevice(nullptr,warp?D3D_DRIVER_TYPE_WARP:D3D_DRIVER_TYPE_HARDWARE,nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,levels,3,D3D11_SDK_VERSION,&device,&selected,&context);
    if(FAILED(hr) && !warp) {
        warp=true;
        std::fprintf(stderr,"[native GPU] hardware device unavailable; trying Windows WARP\n");
        hr=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            levels,3,D3D11_SDK_VERSION,&device,&selected,&context);
    }
    if(!hr_ok(hr,"device")) { gpu_close();return false; }
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
    if(window) {
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
struct Input { float4 p:POSITION; float4 color:COLOR; float2 uv:TEXCOORD; };
struct Output { float4 p:SV_POSITION; noperspective float4 color:COLOR; float2 uv:TEXCOORD; };
Output vs_main(Input i) { Output o; o.p=i.p; o.color=i.color; o.uv=i.uv; return o; }
float4 ps_main(Output i):SV_TARGET {
    float4 t=image.Sample(sample_image,i.uv);
    if(!is_polygon) { clip(t.a-0.0001);return t*i.color; }
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
)";
    ComPtr<ID3DBlob> vs,ps,errors;
    auto compile=[&](const char *entry,const char *profile,ComPtr<ID3DBlob> &out) {
        const HRESULT result=D3DCompile(shader,std::strlen(shader),nullptr,nullptr,nullptr,entry,profile,
            D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&out,&errors);
        if(FAILED(result) && errors) std::fprintf(stderr,"[native GPU] shader: %s\n",static_cast<const char *>(errors->GetBufferPointer()));
        return hr_ok(result,"shader compile");
    };
    if(!compile("vs_main","vs_4_0",vs) || !compile("ps_main","ps_4_0",ps) ||
        FAILED(device->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&vertex_shader)) ||
        FAILED(device->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&pixel_shader))) { gpu_close();return false; }
    const D3D11_INPUT_ELEMENT_DESC elements[]={
        {"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,32,D3D11_INPUT_PER_VERTEX_DATA,0}};
    if(FAILED(device->CreateInputLayout(elements,3,vs->GetBufferPointer(),vs->GetBufferSize(),&layout))) { gpu_close();return false; }
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
    D3D11_RASTERIZER_DESC raster{};raster.FillMode=D3D11_FILL_SOLID;raster.CullMode=D3D11_CULL_NONE;raster.DepthClipEnable=TRUE;
    const uint32_t white=0xffffffff;
    if(FAILED(device->CreateRasterizerState(&raster,&rasterizer)) ||
        !texture(base_texture,nullptr,SCREEN_W,SCREEN_H) || !texture(map_texture,nullptr,SCREEN_W,SCREEN_H) ||
        !texture(white_texture,&white,1,1)) { gpu_close();return false; }
    raster.FillMode=D3D11_FILL_WIREFRAME;
    if(FAILED(device->CreateRasterizerState(&raster,&wireframe))) { gpu_close();return false; }
    std::fprintf(stderr,"[native GPU] D3D11 ready: %s, feature %04x\n",adapter_name,selected);
    return true;
}
void gpu_close() {
    glsw_close();using_gl_software=false;
    if(context) { context->ClearState();context->Flush(); }
    texture_cache.clear();cache_bytes=0;use_counter=0;cache_epoch=0;unavailable_replacements.clear();
    base_texture={};map_texture={};white_texture={};
    gameplay_cache.Reset();target_sample.Reset();staging.Reset();target_view.Reset();target.Reset();depth_view.Reset();depth.Reset();
    retirement_query.Reset();window_view.Reset();swap.Reset();vertex_shader.Reset();pixel_shader.Reset();layout.Reset();vertex_buffer.Reset();draw_params.Reset();toon_params.Reset();
    for(auto &sampler:texture_samplers)sampler.Reset();
    for(auto &level:anisotropic_samplers) for(auto &sampler:level) sampler.Reset();
    anisotropic_level=0;
    nearest_clamp.Reset();linear_clamp.Reset();depth_on.Reset();depth_off.Reset();
    for(unsigned i=0;i<2;++i) { shadow_mask[i].Reset();shadow_draw[i].Reset();for(auto &state:normal_depth[i]) state.Reset();for(auto &state:translucent_depth[i]) state.Reset(); }
    shadow_exclude.Reset();shadow_clear.Reset();
    blend_on.Reset();blend_off.Reset();color_off.Reset();rasterizer.Reset();wireframe.Reset();context.Reset();device.Reset();
    target_width=target_height=window_width=window_height=buffer_capacity=triangles_drawn=0;
    adapter_name[0]=0;
}
void gpu_set_menu_scene(bool active) {menu_scene=active;glsw_set_menu_scene(active);}
void gpu_set_letter_overlay(bool active) {letter_overlay=active;glsw_set_letter_overlay(active);}
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
                const DualScreenLayout *screens,const Framebuffer *sub,bool engine_a_on_top,bool cache_gameplay,const uint32_t *hud) {
    if(using_gl_software)return glsw_render(base,map,s,draw_geometry,foreground,screens,sub,engine_a_on_top,cache_gameplay,hud);
    frame_stats={};
    struct Timer {
        std::chrono::steady_clock::time_point start=std::chrono::steady_clock::now();
        ~Timer() {frame_stats.cache_bytes=cache_bytes;frame_stats.cache_budget=cache_budget;frame_stats.cpu_us=static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count());}
    } timer;
    if(!device || s.width<256 || s.width>3840 || s.height<192 || s.height>2160 ||
        !resize_target(s.width,s.height)) return false;
    if(!sync_replacements())return false;
    ID3D11ShaderResourceView *null_view=nullptr;context->PSSetShaderResources(0,1,&null_view);
    ID3D11RenderTargetView *rt=target_view.Get(); context->OMSetRenderTargets(1,&rt,depth_view.Get());
    D3D11_VIEWPORT viewport{0,0,float(s.width),float(s.height),0,1};context->RSSetViewports(1,&viewport);
    context->RSSetState(rasterizer.Get());context->IASetInputLayout(layout.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vertex_shader.Get(),nullptr,0);context->PSSetShader(pixel_shader.Get(),nullptr,0);
    context->OMSetDepthStencilState(depth_off.Get(),0);context->OMSetBlendState(blend_off.Get(),nullptr,~0u);
    const float black[4]={0,0,0,1};context->ClearRenderTargetView(rt,black);
    context->ClearDepthStencilView(depth_view.Get(),D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL,1,0);
    if(screens) {
        if(!sub) return false;
        const auto &r=engine_a_on_top?screens->top:screens->bottom;
        D3D11_VIEWPORT main_view{float(r.x),float(r.y),float(r.width),float(r.height),0,1};
        context->RSSetViewports(1,&main_view);
    }
    // The main view includes the DS backdrop even where the transparent map
    // scanout marks alpha zero. Match the original display's RGB presentation;
    // only the separate minimap overlay should use that alpha as transparency.
    static Framebuffer opaque_base;
    for(unsigned y=0;y<SCREEN_H;++y) for(unsigned x=0;x<SCREEN_W;++x)
        opaque_base.px[y][x]=base.px[y][x]|0xff000000u;
    context->UpdateSubresource(base_texture.resource.Get(),0,nullptr,opaque_base.px,SCREEN_W*4,0);
    if(!quad(base_texture.view.Get(),s.upscale_2d?nearest_clamp.Get():linear_clamp.Get())) return false;
    triangles_drawn=0;
    if(draw_geometry && !geometry(screens!=nullptr)) return false;
    if(draw_geometry && foreground) {
        context->OMSetDepthStencilState(depth_off.Get(),0);context->OMSetBlendState(blend_on.Get(),nullptr,~0u);
        context->UpdateSubresource(base_texture.resource.Get(),0,nullptr,foreground,SCREEN_W*4,0);
        const float aspect=letter_overlay && !screens && !menu_scene
            ? float(SCREEN_W)*s.height/(float(SCREEN_H)*s.width) : 1.0f;
        if(!quad(base_texture.view.Get(),nearest_clamp.Get(),-aspect,1,aspect,-1)) return false;
    }
    if(screens) {
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
        D3D11_TEXTURE2D_DESC desc{},old{};target->GetDesc(&desc);
        if(gameplay_cache)gameplay_cache->GetDesc(&old);
        if(!gameplay_cache || old.Width!=desc.Width || old.Height!=desc.Height) {
            gameplay_cache.Reset();desc.BindFlags=0;
            if(!hr_ok(device->CreateTexture2D(&desc,nullptr,&gameplay_cache),"gameplay preview cache"))return false;
        }
        context->CopyResource(gameplay_cache.Get(),target.Get());
    }
    if(!screens && map && s.minimap) {
        context->OMSetDepthStencilState(depth_off.Get(),0);context->OMSetBlendState(blend_on.Get(),nullptr,~0u);
        context->UpdateSubresource(map_texture.resource.Get(),0,nullptr,map,SCREEN_W*4,0);
        const float x=s.minimap_x/1000.0f,y=s.minimap_y/1000.0f,size=s.minimap_scale/1000.0f;
        // Preserve the old overlay's proportional layout in both dimensions.
        if(!quad(map_texture.view.Get(),nearest_clamp.Get(),x*2-1,1-y*2,(x+size)*2-1,1-(y+size)*2)) return false;
    }
    if(!screens && hud) {
        context->OMSetDepthStencilState(depth_off.Get(),0);context->OMSetBlendState(blend_on.Get(),nullptr,~0u);
        context->UpdateSubresource(map_texture.resource.Get(),0,nullptr,hud,SCREEN_W*4,0);
        if(!quad(map_texture.view.Get(),nearest_clamp.Get(),-1,1,1,-1))return false;
    }
    context->PSSetShaderResources(0,1,&null_view);
    return true;
}
bool gpu_present(unsigned width,unsigned height,bool preserve_aspect) {
    if(using_gl_software)return glsw_present(width,height,preserve_aspect);
    if(!swap || !target || !width || !height) return false;
    if(width!=window_width || height!=window_height) {
        context->OMSetRenderTargets(0,nullptr,nullptr);window_view.Reset();
        if(!hr_ok(swap->ResizeBuffers(0,width,height,DXGI_FORMAT_UNKNOWN,0),"window resize")) return false;
        ComPtr<ID3D11Texture2D> buffer;
        if(FAILED(swap->GetBuffer(0,IID_PPV_ARGS(&buffer))) || FAILED(device->CreateRenderTargetView(buffer.Get(),nullptr,&window_view))) return false;
        window_width=width;window_height=height;
    }
    ID3D11RenderTargetView *rt=window_view.Get();context->OMSetRenderTargets(1,&rt,nullptr);
    const float black[4]={0,0,0,1};context->ClearRenderTargetView(rt,black);
    const auto r=preserve_aspect?screen_fit(target_width,target_height,{0,0,int(width),int(height)}):ScreenRect{0,0,int(width),int(height)};
    D3D11_VIEWPORT vp{float(r.x),float(r.y),float(r.width),float(r.height),0,1};context->RSSetViewports(1,&vp);
    context->OMSetDepthStencilState(depth_off.Get(),0);context->OMSetBlendState(blend_off.Get(),nullptr,~0u);
    if(!quad(target_sample.Get(),linear_clamp.Get())) return false;
    ID3D11ShaderResourceView *null_view=nullptr;context->PSSetShaderResources(0,1,&null_view);
    return hr_ok(swap->Present(0,0),"present");
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
    return using_gl_software?glsw_readback(pixels,width,height):read_texture(target.Get(),pixels,width,height);
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
bool gpu_open(void *,bool,VideoRenderer) { return false; }
void gpu_close() {}
void gpu_set_anisotropic_filtering(uint8_t) {}
uint8_t gpu_anisotropic_filtering() { return 0; }
bool gpu_render(const Framebuffer &,const uint32_t *,const VideoSettings &,bool,const uint32_t *,const DualScreenLayout *,const Framebuffer *,bool,bool,const uint32_t *) { return false; }
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
