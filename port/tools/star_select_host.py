"""Native ABI adaptation for the original mission-selection scene."""
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[2]

def transform(text):
    if 'void* dScStarSel_c_classInit(void)' in text:
        return """#include "dScStarSel_c.h"
#include <cstddef>
#include "ntr/video.h"
#include "ntr/rt.h"
#include <cstdio>
#include <cstdlib>
static unsigned pc_selection=0,pc_phase=0,pc_mode=0,pc_confirm=0,pc_ready=0;
static uint64_t pc_observed_tick=0;
static bool pc_observed=false;
extern "C" void pc_star_select_observe(unsigned selection,unsigned phase,unsigned mode,unsigned confirm,unsigned ready) {
    if(std::getenv("SM64DS_PC_STATE_CAPTURE") && (!pc_observed || selection!=pc_selection || phase!=pc_phase || mode!=pc_mode || confirm!=pc_confirm || ready!=pc_ready))
        std::fprintf(stderr,"[native mission observation] tick=%llu selected=%u phase=%u mode=%u confirm=%u ready=%u\\n",
            static_cast<unsigned long long>(ntr::rt_frame()),selection,phase,mode,confirm,ready);
    pc_selection=selection;pc_phase=phase;pc_mode=mode;pc_confirm=confirm;pc_ready=ready;
    pc_observed_tick=ntr::rt_frame();pc_observed=true;
}
extern "C" int pc_star_select_status(unsigned *selection,unsigned *phase,unsigned *mode,unsigned *confirm,unsigned *ready) {
    if(!pc_observed || ntr::rt_frame()-pc_observed_tick>1)return 0;
    *selection=pc_selection;*phase=pc_phase;*mode=pc_mode;*confirm=pc_confirm;*ready=pc_ready;
    return 1;
}
extern "C" void pc_star_select_forget() {pc_observed=false;}
static_assert(sizeof(dScStarSel_c)==0x13c, "star selection scene size");
static_assert(offsetof(dScStarSel_c, models)==0x64, "star selection model offset");
extern "C" void *dScStarSel_c_classInit() {
    auto *scene = new dScStarSel_c;
    if(scene) ntr::video_set_star_select_active(true);
    return scene;
}
"""
    if 's32 dScStarSel_c::InitResources()' in text:
        # The development course picker skips DSMT's message bank preload.
        # Use the original loader when entering the scene from that route.
        text='extern "C" { extern void *data_0209d708; void func_0201fe08(); }\n'+text
        assert text.count('    LoadTextNarcs();')==1
        text=text.replace('    LoadTextNarcs();','    LoadTextNarcs();\n    if (!data_0209d708) func_0201fe08();')
    if 's32 dScStarSel_c::Render()' in text:
        # The shared overlay image already rebases its own sprite tables.
        # Keep descriptors and pointer arrays in that one original image.
        symbols=dict(re.findall(r'^(\S+) kind:data\([^)]*\) addr:0x([0-9a-fA-F]+)',
                    (ROOT/'config/arm9/overlays/ov001/symbols.txt').read_text(),re.M))
        pattern=r'extern (u8|void \*)\s*(data_ov001_[0-9a-f]+|_ZN3OAM\w+)(\[\])?;'
        def native_symbol(m):
            name=m[2];address=symbols[name]
            expr=f'(static_cast<void **>(port_overlay1_address(0x{address}u)))' if m[3] else f'(*static_cast<unsigned char *>(port_overlay1_address(0x{address}u)))'
            return f'#define {name} {expr}'
        text,count=re.subn(pattern,native_symbol,text)
        assert count==17, ('star selection sprite declarations changed',count)
        text='extern "C" void *port_overlay1_address(unsigned int);\n'+text
    if 's32 dScStarSel_c::CleanupResources()' in text:
        text='#include "ntr/video.h"\nextern "C" void pc_star_select_forget();\n'+text
        anchor='s32 dScStarSel_c::CleanupResources() {'
        assert text.count(anchor)==1
        text=text.replace(anchor,anchor+'\n    pc_star_select_forget();\n    ntr::video_set_star_select_active(false);')
    if 's32 dScStarSel_c::Behavior()' in text:
        text='#include <cstdio>\n#include <cstdlib>\n'+text
        anchor='data_0209f1f0 = FB(this, 0x115) + 1;'
        assert text.count(anchor)==1
        text=text.replace(anchor,anchor+'\n                    if(std::getenv("SM64DS_PC_STATE_CAPTURE")) std::fprintf(stderr,"[native mission selection] confirmed mission=%u level=%d\\n",unsigned(data_0209f1f0),int(data_02092110));')
        assert text.count('data_0209f5bc->v5()')==1
        text='#include "dFader_c.h"\n'+text.replace('data_0209f5bc->v5()', 'reinterpret_cast<dFader_c *>(data_0209f5bc)->IsAtStart()')
        anchor='s32 dScStarSel_c::Behavior()\n{'
        assert text.count(anchor)==1
        text='extern "C" void pc_star_select_observe(unsigned,unsigned,unsigned,unsigned,unsigned);\n'+text
        text=text.replace(anchor,anchor+'\n    pc_star_select_observe(FB(this,0x115),FB(this,0x135),FB(this,0x133),FB(this,0x119),reinterpret_cast<dFader_c *>(data_0209f5bc)->IsAtStart()!=0);')
    if 'int func_02043098(int idx, int b, int c, int d)' in text:
        assert text.count('else if (idx == 3 || idx == 4)')==1
        text=text.replace('else if (idx == 3 || idx == 4)', 'else if (idx == 3)')
        text=text.replace('ntr_input_set_menu_navigation(idx == 2)', 'ntr_input_set_menu_navigation(idx == 2 || idx == 4)')
    return text
