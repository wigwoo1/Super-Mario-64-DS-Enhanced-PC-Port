"""Adapt the PC course picker without modifying the retail source."""
import sys
from pathlib import Path
def transform(text):
    needle='LoadLevelNoReturn(t, data_ov003_020b1181[i << 3], 1, 0);'
    if text.count(needle)!=1:raise ValueError('PC course-picker level handoff changed')
    return '#include "hal/pc_session.h"\n'+text.replace(needle,'if (!pc_prepare_level_session()) { pc_report_save_error(); return 1; }\n                '+needle.replace(', 1, 0);', ', pc_test_mission(), 0);'))
if __name__=='__main__':
    source,output=map(Path,sys.argv[1:]);output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(transform(source.read_text()))
