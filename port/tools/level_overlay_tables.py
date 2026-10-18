from pathlib import Path
import struct, sys
root=Path(sys.argv[1]); out=Path(sys.argv[2]); image=(root/'extracted/arm9_dec.bin').read_bytes(); base=0x02004000
# Bound the 7x7 ID table using its neighboring known labels: 0x02075804..0x020758c8.
group=struct.unpack_from('<49I', image, 0x02075804-base)
# Level IDs for 52 selectable stages. Use the next table's symbol boundary at 0x02075998.
levels=struct.unpack_from('<52I', image, 0x020758c8-base)
indices=image[0x02075998-base:0x02075998-base+52*7]
lines=['// Generated from extracted ARM9 bytes; do not edit.', '#include <cstdint>', 'extern "C" {', 'struct OvTbl { unsigned int t[7]; };', 'OvTbl data_02075804[7] = {']
for i in range(7): lines.append('    {{'+', '.join(str(v) for v in group[i*7:(i+1)*7])+'}},')
lines += ['};', 'unsigned char data_02075998[52][7] = {']
for i in range(52): lines.append('    {'+', '.join(str(v) for v in indices[i*7:(i+1)*7])+'},')
lines += ['};', 'int data_020758c8[52] = {'+', '.join(str(v if v<0x80000000 else v-0x100000000) for v in levels)+'};', '}']
out.parent.mkdir(parents=True, exist_ok=True); out.write_text('\n'.join(lines)+'\n', encoding='ascii')
