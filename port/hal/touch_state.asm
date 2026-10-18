; Four retail {held, edge, x, y} byte records, with canonical interior labels.
.386
.model flat
.data
ALIGN 4
PUBLIC _gTouchHeld, _gTouchEdge, _gTouchX, _gTouchY, _pc_touch_state_end
_gTouchHeld BYTE 0
_gTouchEdge BYTE 0
_gTouchX BYTE 0
_gTouchY BYTE 0
BYTE 12 DUP (0)
_pc_touch_state_end LABEL BYTE
END
