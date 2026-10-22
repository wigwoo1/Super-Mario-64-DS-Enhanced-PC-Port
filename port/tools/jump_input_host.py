"""Opt-in, read-only observation of the original jump decision."""

def transform(text):
    for signature, label in (("int Player::St_Wait_Main()", "wait"),
                             ("int Player::St_Walk_Main()", "walk"),
                             ("int func_ov002_020d36d8(char* c, int arg)", "gate")):
        anchor = signature + '\n{'
        assert text.count(anchor) == 1, 'Jump trace anchor changed: ' + signature
        owner = 'c' if label == 'gate' else 'this'
        text = text.replace(anchor, anchor + '\n    pc_trace_jump_input(' + owner + ',"' + label + '");', 1)
    return 'extern "C" void pc_trace_jump_input(const void *, const char *);\n' + text
