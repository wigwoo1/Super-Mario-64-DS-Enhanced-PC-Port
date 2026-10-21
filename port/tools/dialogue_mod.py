"""Export one private dialogue draft or compile explicitly approved replacements.

Drafts include original ROM text and must stay outside public source/releases.
Compiled packs contain only the approved entries; keep them local until reviewed.
"""
import argparse
import json
from pathlib import Path
import struct
import sys
import zlib

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
import message_bank as MB

FORMAT='sm64ds-approved-dialogue-v1'
LANGUAGES={'eng':1,'frn':2,'gmn':3,'itl':4,'spn':5}


def protected(payload):
    out=bytearray();i=0
    while i<len(payload):
        c=payload[i];i+=1
        if c==0xfe:
            n=payload[i]
            if n<3 or i+n-1>len(payload):raise ValueError('Malformed command')
            out.extend(payload[i-1:i+n-1]);i+=n-1
        elif c>=0xd0:out.append(c)
    return bytes(out)


def signature(message):
    return zlib.crc32(bytes.fromhex(message['info_hex'])+MB.encode_message(message['text']))


def draft(bank,language,ids,context):
    return {'format':FORMAT,'language':LANGUAGES[language],'messages':[
        {'id':n,'context':context,'original':bank['messages'][n]['text'],
         'info_hex':bank['messages'][n]['info_hex'],
         'original_crc32':f"{signature(bank['messages'][n]):08x}",
         'approved':False,'replacement':None} for n in ids]}


def compile_pack(bank,document,widths):
    if document.get('format')!=FORMAT or document.get('language') not in LANGUAGES.values():raise ValueError('Unknown draft format/language')
    result=bytearray(b'SM64TXT1'+struct.pack('<II',document['language'],len(document['messages'])))
    seen=set()
    for item in document['messages']:
        n=item['id']
        if not isinstance(n,int) or n in seen or not 0<=n<len(bank['messages']):raise ValueError('Invalid or duplicate message ID')
        seen.add(n);message=bank['messages'][n]
        if item.get('approved') is not True or not isinstance(item.get('replacement'),str):raise ValueError(f'Message {n}: replacement must be explicitly approved')
        if item.get('original')!=message['text'] or item.get('info_hex')!=message['info_hex'] or item.get('original_crc32')!=f'{signature(message):08x}':raise ValueError(f'Message {n}: original text changed; export a fresh draft')
        original=MB.encode_message(message['text']);edited=MB.encode_message(item['replacement'])
        if len(edited)>8192 or protected(original)!=protected(edited):raise ValueError(f'Message {n}: preserve command, icon and line/page tokens exactly')
        maximum=min(255,int.from_bytes(bytes.fromhex(message['info_hex'])[:2],'little'))
        width=0;glyphs=0;i=0
        while i<len(edited):
            c=edited[i];i+=1
            if c==0xfe:i+=edited[i]-1
            elif c in (0xfd,0xff):
                if width>maximum:raise ValueError(f'Message {n}: line is {width}px; box allows {maximum}px')
                width=0;glyphs=0
            else:
                glyphs+=1
                if glyphs>255:raise ValueError(f'Message {n}: line exceeds the native 255-glyph counter')
                width+=widths[c]
        result+=struct.pack('<III',n,signature(message),len(edited))+edited
    if len(seen)>4096 or len(result)>4*1024*1024:raise ValueError('Pack is too large')
    return bytes(result)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('command',choices=['export','compile']);p.add_argument('--bank',type=Path,required=True)
    p.add_argument('--language',choices=LANGUAGES,default='eng');p.add_argument('--id',type=lambda x:int(x,0),action='append')
    p.add_argument('--context',default='');p.add_argument('--draft',type=Path)
    p.add_argument('--arm9',type=Path,default=ROOT/'extracted/arm9_dec.bin');p.add_argument('--output',type=Path,required=True)
    p.add_argument('--arm9-base',type=lambda x:int(x,0),default=0x02004000,
                   help='ARM9 RAM load address; verified Europe rev0 default is 0x02004000')
    a=p.parse_args();bank=MB.parse_bank(a.bank.read_bytes())
    if a.command=='export':
        if not a.id:p.error('Export at least one --id')
        a.output.write_text(json.dumps(draft(bank,a.language,a.id,a.context),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    else:
        if not a.draft:p.error('Compile requires --draft')
        document=json.loads(a.draft.read_text(encoding='utf-8-sig'))
        if document['language']!=LANGUAGES[a.language]:p.error('Draft language differs from selected bank')
        offset=0x0208f074-a.arm9_base
        if offset<0:raise ValueError('ARM9 base is after the font table')
        widths=a.arm9.read_bytes()[offset:offset+256]
        if len(widths)!=256:raise ValueError('ARM9 font-width table unavailable')
        a.output.write_bytes(compile_pack(bank,document,widths))
    print(f'Wrote {a.output}')


if __name__=='__main__':main()
