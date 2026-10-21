"""Keep the intro letter's 2D text aligned with its perspective 3D panel."""
import re


def transform(text, filename):
    statements = {
        'func_0201f138.cpp': ('func_0201f138', True),
        'func_0201ef38.c': ('func_0201ef38', False),
        'func_0201eaac.c': ('func_0201eaac', False),
    }
    name, enabled = statements[filename]
    text = 'extern "C" void pc_cutscene_letter_state(int);\n' + text
    if enabled:
        # Set after initialization: Message reset inside this function clears it.
        anchor = 'data_0209d45c |= 8;'
        assert text.count(anchor) == 1, 'Intro letter initialization changed'
        return text.replace(anchor, anchor + '\n    pc_cutscene_letter_state(1);')
    pattern = r'(void\s+' + name + r'\s*\([^)]*\)\s*\{)'
    text, count = re.subn(pattern, r'\1\n    pc_cutscene_letter_state(0);', text, count=1)
    assert count == 1, 'Intro text lifecycle hook changed: ' + name
    return text
