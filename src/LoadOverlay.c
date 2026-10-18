#include "types.h"
#if defined(SM64DS_PLATFORM_PC)
#include <stdio.h>
#include <stdlib.h>
#if defined(SM64DS_GAME_TARGET)
extern int pc_load_level_overlay(int id);
#endif
#define PC_LOAD_OVERLAY_TRACE(label) do { \
    const char *trace = getenv("SM64DS_PC_TRACE"); \
    if (trace && trace[0] && trace[0] != '0') { \
        fprintf(stderr, "[sm64ds] LoadOverlay: %s\\n", label); \
        fflush(stderr); \
    } \
} while (0)
#else
#define PC_LOAD_OVERLAY_TRACE(label) ((void)0)
#endif
void func_02018c00(void *dst, int a, int id);
int func_0203d7b8(void);
struct Pair;
void func_0205e088(struct Pair *dst, char *src);
void Crash(void);
void FS_LoadOverlay(int a, int id);
struct Region
{
  u32 valid;
  u32 start;
  u32 size;
};
extern struct Region data_0209d3c4[12];
struct Pair
{
  int a;
  int b;
};
void LoadOverlay(int id)
{
#if defined(SM64DS_PLATFORM_PC)
  /* The host target statically links the recovered base, boot-menu and
     message-overlay code. Their machine code and overlay data are represented
     by native objects, so they need no DS RAM-residency operation here. */
  if (id == 0 || id == 1 || id == 2 || id == 3 || id == 4 || id == 5 || id == 6 || id == 7) {
    PC_LOAD_OVERLAY_TRACE("overlay code/data linked into host executable");
    return;
  }
#if defined(SM64DS_GAME_TARGET)
  if (id >= 8 && id <= 59) {
    if (!pc_load_level_overlay(id)) {
      fprintf(stderr, "[sm64ds] LoadOverlay: level data load failed for overlay %d\n", id);
      fflush(stderr);
    }
    return;
  }
#endif
  {
    const char *trace = getenv("SM64DS_PC_TRACE");
    if (trace && trace[0] && trace[0] != '0') {
      fprintf(stderr, "[sm64ds] LoadOverlay: dynamic ID %d requested; native loader is not ready\n", id);
      fflush(stderr);
    }
  }
  return;
#else
  PC_LOAD_OVERLAY_TRACE("begin");
  u32 start;
  struct Region *slot;
  u32 i;
  u32 total;
  struct Pair pair;
  char buf[0x24];
  struct Region *r;
  u32 end;
  func_02018c00(buf, 0, id);
  PC_LOAD_OVERLAY_TRACE("overlay info fetched");
  if (func_0203d7b8() != 0)
  {
    return;
  }
  func_0205e088(&pair, buf);
  PC_LOAD_OVERLAY_TRACE("overlay source resolved");
  slot = 0;
  total = (*((u32 *) (buf + 8))) + (*((u32 *) (buf + 0xc)));
  start = *((u32 *) (buf + 4));
  end = start + total;
  for (r = data_0209d3c4, i = 0; i < 12; r++, i++)
  {
    if (r->valid == 0xffffffff)
    {
      if (slot == 0)
      {
        slot = r;
      }
    }
    else
      if (((r->start + r->size) > start) && (end > r->start))
    {
      Crash();
    }
  }

  if (slot != 0)
  {
    slot->valid = id;
    slot->start = start;
    slot->size = total;
  }
  PC_LOAD_OVERLAY_TRACE("overlay slot reserved");
  FS_LoadOverlay(0, id);
  PC_LOAD_OVERLAY_TRACE("overlay bytes loaded");
  func_0203d7b8();
#endif
}
