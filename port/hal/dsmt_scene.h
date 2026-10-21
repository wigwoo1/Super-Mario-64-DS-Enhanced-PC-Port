#pragma once
int pc_title_text_mode_get();
void pc_title_text_mode_set(int enabled);
bool pc_dsmt_requested();
bool pc_dsmt_active();
void *pc_create_dsmt_scene();
void pc_dsmt_trace(const char *step);
void pc_dsmt_phase(int phase);
int pc_dsmt_input_update();
extern "C" void pc_dsmt_graph_prepare();
