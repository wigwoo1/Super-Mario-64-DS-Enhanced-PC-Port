"""Copy a native QA save to a NEW directory with character caps unlocked.

Only the separate copy is edited. Yoshi remains the selected character and
all other progress is preserved. Never point this tool at a user's save folder.
"""
import argparse
import pathlib
import struct
import zlib


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('source',type=pathlib.Path)
    p.add_argument('destination',type=pathlib.Path)
    args=p.parse_args()
    assert not args.destination.exists(),'destination must be a new QA directory'
    copies={}
    for name in ('slot1.sav','slot1.sav.bak','minigames.sav','minigames.sav.bak'):
        raw=bytearray((args.source/name).read_bytes())
        magic,slot,size,checksum,reserved=struct.unpack_from('<8s4I',raw)
        assert magic==b'SM64PC01' and reserved==0 and len(raw)==24+size,name
        struct.pack_into('<I',raw,16,0)
        assert zlib.crc32(raw)==checksum,name+' checksum'
        if name.startswith('slot1'):
            assert slot==0 and size==68 and raw[24:28]==b'8000',name
            assert raw[24+0x41]==3,'fixture must keep Yoshi selected'
            flags,=struct.unpack_from('<I',raw,24+8)
            struct.pack_into('<I',raw,24+8,flags|7)
        else:
            assert slot==3 and size==740 and raw[24:28]==b'5000',name
        struct.pack_into('<I',raw,16,zlib.crc32(raw))
        copies[name]=raw
    args.destination.mkdir(parents=True)
    for name,raw in copies.items():(args.destination/name).write_bytes(raw)
    print('PASS: separate QA save created; Mario/Luigi/Wario unlock bits set, '
          'Yoshi selected, checksums valid; source untouched')


if __name__=='__main__':main()
