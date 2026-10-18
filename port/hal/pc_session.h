#pragma once
// Current PC course picker has no file-select scene yet. Initialize a pristine
// live block from native slot1 (retail slot0), then use its current character.
// Existing live progress is preserved. Read errors cancel course launch.
bool pc_prepare_level_session();
void pc_report_save_error();
int pc_test_mission();
unsigned pc_test_conditional_goomba_param(unsigned parameter, int level);
