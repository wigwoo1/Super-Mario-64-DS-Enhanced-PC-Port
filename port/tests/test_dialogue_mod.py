import copy
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import dialogue_mod as M
import dialogue_host


class ApprovedDialogue(unittest.TestCase):
    def setUp(self):
        self.bank={'messages':[{'index':0,'info_hex':'50000200','text':'TEST<cmd:FE0301>\nYES<icon:A>'}]}
        self.draft=M.draft(self.bank,'eng',[0],'Synthetic test')
        self.item=self.draft['messages'][0]
        self.item.update(approved=True,replacement='OK<cmd:FE0301>\nGO<icon:A>')
        self.widths=bytes([8]*256)

    def test_approved_binary_and_original_binding(self):
        pack=M.compile_pack(self.bank,self.draft,self.widths)
        self.assertEqual(pack[:8],b'SM64TXT1')
        self.assertEqual(struct.unpack_from('<IIIII',pack,8),(1,1,0,M.signature(self.bank['messages'][0]),len(M.MB.encode_message(self.item['replacement']))))
        self.item['approved']=False
        with self.assertRaisesRegex(ValueError,'approved'):M.compile_pack(self.bank,self.draft,self.widths)
        self.item['approved']=True;self.item['original']='changed'
        with self.assertRaisesRegex(ValueError,'original text changed'):M.compile_pack(self.bank,self.draft,self.widths)

    def test_commands_pages_icons_width_and_duplicates(self):
        for text in ['OK<cmd:FE0302>\nGO<icon:A>','OK<cmd:FE0301>GO<icon:A>','OK<cmd:FE0301>\nGO<icon:B>','A'*20+'<cmd:FE0301>\nGO<icon:A>']:
            with self.subTest(text=text):
                self.item['replacement']=text
                with self.assertRaises(ValueError):M.compile_pack(self.bank,self.draft,self.widths)
        self.item['replacement']='OK<cmd:FE0301>\nGO<icon:A>'
        self.item['replacement']='A'*256+'<cmd:FE0301>\nGO<icon:A>'
        with self.assertRaisesRegex(ValueError,'255-glyph'):M.compile_pack(self.bank,self.draft,bytes(256))
        self.item['replacement']='OK<cmd:FE0301>\nGO<icon:A>'
        self.draft['messages'].append(copy.deepcopy(self.item))
        with self.assertRaisesRegex(ValueError,'duplicate'):M.compile_pack(self.bank,self.draft,self.widths)

    def test_real_entry_hooks_run_before_layout(self):
        root=Path(__file__).resolve().parents[2]
        for relative in ['src/_ZN7Message11DisplayTextEt.cpp','src/unnamed/arm9/0201/func_0201f138.cpp','src/unnamed/arm9/0201/func_0201f32c.c']:
            source=(root/relative).read_text()
            transformed=dialogue_host.transform(source)
            self.assertEqual(transformed.count('    pc_dialogue_begin();'),1,relative)
            self.assertLess(transformed.index('    pc_dialogue_begin();'),transformed.index('    p = func_02054d88()') if '    p = func_02054d88()' in transformed else transformed.index('    data_0209d6a8 = (u8)('))
        self.assertEqual(dialogue_host.transform('extern void func_0201f32c(int);\n'), 'extern void func_0201f32c(int);\n')
        source=(root/'src/_ZN7Message6UpdateEv.cpp').read_text()
        self.assertEqual(dialogue_host.transform(source).count('    pc_dialogue_observe();'),1)


if __name__=='__main__':unittest.main()
