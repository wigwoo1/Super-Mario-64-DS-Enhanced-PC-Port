"""Resolve approved dialogue at initialization, before layout or text commands."""
import re


def transform(text):
    update='void Message::Update()\n{'
    if update in text:
        if text.count(update)!=1:raise ValueError('Message update definition changed')
        return 'extern "C" void pc_dialogue_observe();\n'+text.replace(update,update+'\n    pc_dialogue_observe();',1)
    # These are the normal actor dialog, immediate text, and Peach letter paths.
    anchors={
        'void Message::DisplayText(unsigned short t)': '    p = func_02054d88() + 0x4000;',
        'void func_0201f138(void)': '    data_0209d6a8 = (u8)((((data_0209d6f0->m4 / 8) * 9) + 7) / 8);',
        'void func_0201f32c(': '    data_0209d6a8 = (u8)((*(u16 *)(entry + 4) + 7) / 8);',
    }
    for signature,anchor in anchors.items():
        if re.search(re.escape(signature)+r'[^;{}]*\{',text):
            if text.count(anchor)!=1:raise ValueError('Dialogue initialization anchor changed: '+signature)
            return 'extern "C" void pc_dialogue_begin();\n'+text.replace(anchor,'    pc_dialogue_begin();\n'+anchor,1)
    return text
