#pragma once
extern "C" {
int pc_wanted_settings_open(void *scene);
int pc_wanted_settings_update(void *scene);
int pc_wanted_settings_render();
void pc_wanted_settings_prepare();
void pc_wanted_settings_reset();
int pc_wanted_settings_page();
int pc_wanted_settings_row();
}
