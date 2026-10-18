#ifndef SM64DS_OVERLAY_ID_H
#define SM64DS_OVERLAY_ID_H

/* The DS linker defines overlay_N as an absolute symbol with value N, so the
   original code uses &overlay_N to pass the table index to LoadOverlay. Host
   linkers do not support that absolute-symbol spelling; preserve the same
   index explicitly for the native port. */
#if defined(SM64DS_PLATFORM_PC)
#define OVERLAY_ID(id) (id)
#else
#define OVERLAY_ID(id) ((int)&overlay_##id)
#endif

#endif
