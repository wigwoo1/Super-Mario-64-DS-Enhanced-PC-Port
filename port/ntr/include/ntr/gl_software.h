#pragma once
#include "ntr/gpu.h"
namespace ntr {
bool glsw_open(void *window);
void glsw_close();
void glsw_set_menu_scene(bool active);
void glsw_set_letter_overlay(bool active);
void glsw_set_anisotropic_filtering(uint8_t level);
uint8_t glsw_anisotropic_filtering();
bool glsw_render(const Framebuffer &,const uint32_t *,const VideoSettings &,bool,
                 const uint32_t *,const DualScreenLayout *,const Framebuffer *,bool,bool,const uint32_t *);
bool glsw_present(unsigned width,unsigned height,bool preserve_aspect);
bool glsw_readback(std::vector<uint32_t> &,unsigned &,unsigned &);
bool glsw_readback_gameplay(std::vector<uint32_t> &,unsigned &,unsigned &);
const char *glsw_adapter();
unsigned glsw_triangle_count();
GpuFrameStats glsw_frame_stats();
void glsw_set_texture_budget_bytes(uint64_t bytes);
void glsw_force_texture_scan(bool force);
}
