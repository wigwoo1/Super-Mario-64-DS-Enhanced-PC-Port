; One native GX bank-state allocation with the ROM's interior halfword labels.
; No executable ARM/x86 code: these labels preserve aggregate/field aliasing.
.386
.model flat
.data
ALIGN 4
PUBLIC _data_020a6088, _data_020a608a, _data_020a608c, _data_020a608e
PUBLIC _data_020a6090, _data_020a6092, _data_020a6094, _data_020a6096
PUBLIC _data_020a6098, _data_020a609a, _data_020a609c, _data_020a609e
PUBLIC _data_020a60a0
_data_020a6088 WORD 0
_data_020a608a WORD 0
_data_020a608c WORD 0
_data_020a608e WORD 0
_data_020a6090 WORD 0
_data_020a6092 WORD 0
_data_020a6094 WORD 0
_data_020a6096 WORD 0
_data_020a6098 WORD 0
_data_020a609a WORD 0
_data_020a609c WORD 0
_data_020a609e WORD 0
_data_020a60a0 WORD 0
ALIGN 4
END
