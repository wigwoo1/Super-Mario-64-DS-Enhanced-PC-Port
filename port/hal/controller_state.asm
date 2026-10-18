; Four 24-byte retail processed-controller records. Interior symbols must
; alias this allocation: halfword/byte labels cannot each get four-byte padding.
.386
.model flat
.data
ALIGN 4
PUBLIC _data_0209f498, _data_0209f49c, _data_0209f49e
PUBLIC _data_0209f4a0, _data_0209f4a2, _data_0209f4a4, _data_0209f4a6
PUBLIC _data_0209f4a8, _data_0209f4a9, _data_0209f4ab, _data_0209f4ac, _data_0209f4ae
PUBLIC _pc_controller_state_end
_data_0209f498 WORD 0,0
_data_0209f49c WORD 0
_data_0209f49e WORD 0
_data_0209f4a0 WORD 0
_data_0209f4a2 WORD 0
_data_0209f4a4 WORD 0
_data_0209f4a6 WORD 0
_data_0209f4a8 BYTE 0
_data_0209f4a9 BYTE 0,0
_data_0209f4ab BYTE 0
_data_0209f4ac BYTE 0,0
_data_0209f4ae BYTE 0,0
BYTE 72 DUP(0)
_pc_controller_state_end LABEL BYTE
END
