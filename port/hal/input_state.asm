; Four original {held, pressed} halfword pairs, with shared interior labels.
.386
.model flat
.data
ALIGN 4
PUBLIC _data_020a0e58, _data_020a0e5a, _data_020a0e5c, _data_020a0e5e
PUBLIC _pc_input_state_end
_data_020a0e58 WORD 0
_data_020a0e5a WORD 0
_data_020a0e5c WORD 0
_data_020a0e5e WORD 0
WORD 0, 0, 0, 0
_pc_input_state_end LABEL BYTE
; The camera publishes its angle at raw-record +0x10. Offline input history
; copies it into each 0x24-byte player record, where GetAngleToCamera reads it.
; Every named interior view must refer to that same allocation.
ALIGN 4
PUBLIC _data_020a1040, _data_020a1044, _data_020a1046, _data_020a1048
PUBLIC _data_020a104c, _data_020a104e, _data_020a1050, _data_020a1052
PUBLIC _pc_raw_input_end
_data_020a1040 DWORD 0
_data_020a1044 WORD 0
_data_020a1046 WORD 0
_data_020a1048 DWORD 0
_data_020a104c WORD 0
_data_020a104e WORD 0
_data_020a1050 WORD 0
_data_020a1052 BYTE 18 DUP (0)
_pc_raw_input_end LABEL BYTE
ALIGN 4
PUBLIC _data_020a1154, _data_020a1160, _data_020a1162, _data_020a1164
PUBLIC _data_020a1166, _data_020a1178, _data_020a117c, _data_020a11a0
PUBLIC _data_020a11c4, _pc_input_history_end
_data_020a1154 BYTE 12 DUP (0)
_data_020a1160 WORD 0
_data_020a1162 WORD 0
_data_020a1164 WORD 0
_data_020a1166 BYTE 18 DUP (0)
_data_020a1178 DWORD 0
_data_020a117c BYTE 36 DUP (0)
_data_020a11a0 BYTE 36 DUP (0)
_data_020a11c4 BYTE 32 DUP (0)
_pc_input_history_end LABEL BYTE
END
