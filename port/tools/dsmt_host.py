"""Native DSMT scene integration; preserve original ROM source and data bytes."""
import argparse,hashlib,json,re,struct
from pathlib import Path
START=0x020ccb54;BSS=0x02103340;END=0x02104c40;BASE=0x020ad660
SUMMARY_START=0x0209b33c;SUMMARY_END=0x0209b3e0
SAVE_START=0x0209caa0;SAVE_END=SAVE_START+0x32c
MESSAGE_IDS_START=0x0208eeec;MESSAGE_IDS_END=0x0208f074
def tail_veneers(root):
    text=(root/'config/arm9/overlays/ov007/symbols.txt').read_text()
    raw=(root/'extracted/overlays/overlay_0007.bin').read_bytes()
    functions={int(a,16):(n,int(size,16)) for n,size,a in re.findall(
        r'^(\S+) kind:function\(arm,size=0x([0-9a-f]+)\) addr:0x([0-9a-f]+)',text,re.M)}
    result={}
    for address,(name,size) in functions.items():
        if size!=12 or not name.startswith('func_'):continue
        load,jump,target=struct.unpack_from('<3I',raw,address-BASE)
        if (load,jump)!=(0xe59fc000,0xe12fff1c):continue
        to=functions.get(target,('',0))[0]
        if to.startswith('func_'):result[name]=to
    return result
def transform_veneer(source,target):
    # A true LDR ip,[pc]; BX ip veneer preserves all arguments and returns.
    # A C void(void) wrapper cannot do that under the x86 stack ABI.
    return ('// Native equivalent of the independently verified ARM tail jump.\n'
        'extern "C" void '+target+'(void);\n'
        'extern "C" __declspec(naked) void '+source+'(void) { __asm { jmp '+target+' } }\n')
def self_relocations(root,raw):
    result={}
    for line in (root/'config/arm9/overlays/ov007/relocs.txt').read_text().splitlines():
        m=re.fullmatch(r'from:0x([0-9a-f]+) kind:load to:0x([0-9a-f]+) module:overlay\(7\)',line)
        if not m:continue
        address,target=(int(v,16) for v in m.groups())
        if START<=address<BSS and START<=target<END:
            if struct.unpack_from('<I',raw,address-BASE)[0]!=target:raise ValueError('DSMT relocation differs from original bytes')
            result[address]=target
    return result
def owned_names(root):
    names=[]
    for path,low,high in ((root/'config/arm9/overlays/ov007/symbols.txt',START,END),
                          (root/'config/arm9/symbols.txt',SUMMARY_START,SUMMARY_END),
                          (root/'config/arm9/symbols.txt',SAVE_START,SAVE_END),
                          (root/'config/arm9/symbols.txt',MESSAGE_IDS_START,MESSAGE_IDS_END)):
        for n,a in re.findall(r'^(\S+) kind:(?:data\([^)]*\)|bss) addr:0x([0-9a-f]+)',path.read_text(),re.M):
            if low<=int(a,16)<high:names.append((n,int(a,16)))
    return names
def transform(text,name):
    if name=='func_ov007_020b8548.c':
        anchor='    data_ov007_0210345c = p1;'
        if text.count(anchor)!=1:raise ValueError('Title font capture anchor changed')
        return ('extern void pc_settings_capture_title_font(const void *, unsigned int);\n'+
                text.replace(anchor,anchor+'\n    pc_settings_capture_title_font((const void *)p1, (unsigned short)p2);',1))
    if name=='func_ov007_020b7eec.c':
        end=text.rfind('}')
        if end<0 or 'void func_ov007_020b7eec(unsigned char *t)' not in text:
            raise ValueError('Title option text renderer changed')
        return ('void pc_title_text_label_draw(void);\n'+text[:end]+
                '  pc_title_text_label_draw();\n'+text[end:])
    if name=='func_ov007_020aed98.cpp':
        anchors=[('    for (i = 0; i < 0x18; i++) {',0),('    for (i = 0; i < 9; i++) {',2)]
        # There are two 24-element loops: hit testing and the state step.
        a=anchors[0][0]
        if text.count(a)!=2:raise ValueError('DSMT update loop changed')
        first=text.index(a);second=text.index(a,first+1)
        text=text[:second]+'    pc_dsmt_phase(1);\n'+text[second:]
        text=text[:first]+'    pc_dsmt_phase(0);\n'+text[first:]
        a=anchors[1][0]
        if text.count(a)!=1:raise ValueError('DSMT update tail changed')
        return '#include "hal/dsmt_scene.h"\n'+text.replace(a,'    pc_dsmt_phase(2);\n'+a,1)
    if name=='dScBoot_c.cpp':
        needle='StartSceneFade(2, 0, 0);'
        if text.count(needle)!=1:raise ValueError('Native boot title handoff changed')
        load='    func_0201a2f8();'
        if text.count(load)!=1:raise ValueError('Synchronous boot resource load changed')
        text=text.replace(load,load+'\n    if (pc_dsmt_requested()) { pc_dsmt_trace("boot message bank"); LoadMessageBankForLanguage(); }',1)
        return '#include "hal/dsmt_scene.h"\n'+text.replace(needle,'StartSceneFade(pc_dsmt_requested() ? 1 : 2, 0, 0);')
    if name=='dScDSMT_c.cpp':
        # This title scene owns the actual native option command callback.
        # Reuse its animated On/Off widgets for the persisted dialogue mode.
        for old,new in [('        TurnBacklightOn();','        pc_title_text_mode_set(1);'),
                        ('        TurnBacklightOff();','        pc_title_text_mode_set(0);'),
                        ('((u8 *)data_0209b340)[9] = data_0208ee3c[0];',
                         '((u8 *)data_0209b340)[9] = pc_title_text_mode_get();')]:
            if text.count(old)!=1:raise ValueError('Title dialogue option anchor changed: '+old)
            text=text.replace(old,new,1)
        init='    func_ov007_020b7138(&data_ov007_02103290, data_0209b340);'
        text=text.replace(init,'    ((u8 *)data_0209b340)[9] = pc_title_text_mode_get();\n'+init,1)
        input_block='    func_0203da9c();\n    {\n        u16 *p = func_0203dabc();\n        int arg4;\n        func_0203da9c();\n        arg4 = func_0203dae4();\n        result = func_ov007_020b7090(p[0], p[1], p[2], p[3], arg4);\n    }'
        if text.count(input_block)!=1:raise ValueError('DSMT input ABI anchor changed')
        text=text.replace(input_block,'    result = pc_dsmt_input_update();',1)
        signature='extern "C" int *dScDSMT_c_classInit(void)'
        if text.count(signature)!=1:raise ValueError('DSMT factory changed')
        start=text.index('{',text.index(signature));depth=1;end=start+1
        while depth:
            depth+=(text[end]=='{')-(text[end]=='}');end+=1
        text=text[:start]+'{ return static_cast<int *>(pc_create_dsmt_scene()); }'+text[end:]
        text='#include "hal/dsmt_scene.h"\n'+text
        for overlay in (100,102):text=text.replace('(int)&OVERLAY_'+str(overlay)+'_ID',str(overlay))
        for needle,step in [('s32 dScDSMT_c::InitResources()\n{','InitResources'),('    func_ov007_020b7138(&data_ov007_02103290, data_0209b340);','UI initialize'),('    func_ov007_020b7090(0, 0, 0, 0, 0);','first UI update'),('    Sound::LoadInitialGroup(2);','sound group')]:
            if text.count(needle)!=1:raise ValueError('DSMT initialization anchor changed: '+step)
            text=text.replace(needle,needle+'\n    pc_dsmt_trace("'+step+'");' if needle.endswith('{') else '    pc_dsmt_trace("'+step+'");\n'+needle,1)
    return text
def generate(root,out):
    raw=(root/'extracted/overlays/overlay_0007.bin').read_bytes()
    if len(raw)!=BSS-BASE:raise ValueError('Unexpected extracted DSMT image size')
    labels={}
    for n,a in owned_names(root):labels.setdefault(a,[]).append(n)
    # Only these eight independently identified callback slots are rebased here.
    # Address-shaped packed pixels/indices elsewhere are deliberately untouched.
    native={0x02103254:('func_ov007_020c3e4c',0x020c3e4c),0x02103258:('func_ov007_020c3e64',0x020c3e64),
            0x02103290:('func_ov007_020ccab4',0x020ccab4),0x02103294:('func_ov007_020cca98',0x020cca98),
            0x02103298:('func_ov007_020cca80',0x020cca80),0x0210329c:('func_ov007_020cca74',0x020cca74),
            0x021032a0:('func_ov007_020cca68',0x020cca68),0x021032a4:('func_ov007_020cc600',0x020cc600)}
    for a,(name,expected) in native.items():
        if struct.unpack_from('<I',raw,a-BASE)[0]!=expected:raise ValueError(f'Callback field changed: {a:08x}')
    self_refs=self_relocations(root,raw)
    lines=['.386','.model flat','.data','ALIGN 4','PUBLIC _pc_dsmt_rom_data','_pc_dsmt_rom_data LABEL BYTE']
    for name,_ in native.values():lines.append('EXTERN _'+name+':PROC')
    def emit(low,high,contents):
        pos=low
        while pos<high:
            for name in labels.get(pos,[]):lines.extend(['PUBLIC _'+name,'_'+name+' LABEL BYTE'])
            if pos in native:
                lines.append('DD OFFSET _'+native[pos][0]);pos+=4;continue
            if pos in self_refs:
                lines.append(f'DD OFFSET _pc_dsmt_rom_data+0{self_refs[pos]-START:x}h');pos+=4;continue
            count=min(16,high-pos)
            for i in range(1,count):
                if pos+i in labels or pos+i in native or pos+i in self_refs:count=i;break
            values=contents[pos-low:pos-low+count]
            lines.append('DB '+','.join(f'0{b:02x}h' for b in values));pos+=count
    emit(START,END,raw[START-BASE:]+bytes(END-BSS))
    lines+=['ALIGN 4','PUBLIC _pc_dsmt_save_summary','_pc_dsmt_save_summary LABEL BYTE']
    emit(SUMMARY_START,SUMMARY_END,bytes(SUMMARY_END-SUMMARY_START))
    lines+=['ALIGN 4','PUBLIC _pc_native_live_save','_pc_native_live_save LABEL BYTE']
    emit(SAVE_START,SAVE_END,bytes(SAVE_END-SAVE_START))
    # Retain the old HAL allocation's spare capacity, without aliasing unrelated
    # ARM9 globals after the actual 0x32c-byte save block into that padding.
    lines+=['DB 0d4h DUP (0)']
    # Two interleaved halfword columns, including internal labels, form one
    # immutable lookup table. Never infer its extent from the next label.
    arm=(root/'extracted/arm9_dec.bin').read_bytes()
    message_ids=arm[MESSAGE_IDS_START-0x02004000:MESSAGE_IDS_END-0x02004000]
    pairs=list(struct.iter_unpack('<HH',message_ids))
    if pairs[0]!=(0,0) or pairs[-1]!=(8000,630) or any(a[0]>=b[0] for a,b in zip(pairs,pairs[1:])):
        raise ValueError('Original message ID table boundary/order changed')
    lines+=['ALIGN 4','PUBLIC _pc_message_id_table','_pc_message_id_table LABEL BYTE']
    emit(MESSAGE_IDS_START,MESSAGE_IDS_END,message_ids)
    lines+=['END']
    out.parent.mkdir(parents=True,exist_ok=True);text='\n'.join(lines)+'\n'
    if not out.exists() or out.read_text()!=text:out.write_text(text)
    report={'OriginalOverlaySHA256':hashlib.sha256(raw).hexdigest(),'DataStart':hex(START),'DataEnd':hex(END),
            'CanonicalLabels':owned_names(root),'ExplicitCallbacks':{hex(a):v for a,v in native.items()},
            'SourceDeclaredSelfRelocations':{hex(a):hex(t) for a,t in self_refs.items()},
            'NativeTailVeneers':tail_veneers(root),
            'Limits':['Other embedded ROM pointers preserved; consumer-based relocation still required before full UI use',
                      'Original DSMT vtable bytes are data only; native factory creates native scene/callback vtables']}
    out.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('root',type=Path);parser.add_argument('output',type=Path);parser.add_argument('--boot-source',type=Path);args=parser.parse_args()
    if args.boot_source:
        text=transform(args.boot_source.read_text(),'dScBoot_c.cpp');args.output.parent.mkdir(parents=True,exist_ok=True)
        if not args.output.exists() or args.output.read_text()!=text:args.output.write_text(text)
    else:generate(args.root,args.output)
