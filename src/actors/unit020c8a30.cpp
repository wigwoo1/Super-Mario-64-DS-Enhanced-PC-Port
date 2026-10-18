//cpp
/*
 * ov006 0x020c8a30..0x020c8dd4 (.text), ten functions: the small sprite
 * pool whose reset function (0x020c8a9c) is called by the Jump, Jump2,
 * Trampoline and Trampoline2 minigame scenes.
 *
 * The pool is a static array of three 0x20-byte records at
 * data_ov006_021404d8 (a counter at data_ov006_021404c4 that is stamped into
 * each claimed record, capped at 3, and an optional sprite table pointer at
 * data_ov006_021404c8 that overrides the per-language table). The ROM
 * carries no RTTI and no vtable for the record type, so the unit has no
 * evidenced class name; the functions keep their address names.
 *
 *   0x020c8a30  draw every record, last to first          (3 x 0x020c8af4)
 *   0x020c8a64  update every record                       (3 x 0x020c8ba8)
 *   0x020c8a9c  reset the pool: clear each record's active flag via
 *               0x020c8c6c, then store the counter and sprite table
 *   0x020c8af4  draw one record through Hud_RenderSprite
 *   0x020c8ba8  update one record (moves it; clears its active flag when
 *               its timer runs out)
 *   0x020c8c6c  clear one record's active flag
 *   0x020c8c78  claim a free record at (x, y); called from
 *               dMgJump3DMario_c and from the unit at 0x020c8dd4
 *   0x020c8da8  array destructor for the pool (__cxa_vec_cleanup)
 *   0x020c8dcc  empty record destructor
 *   0x020c8dd0  empty record constructor
 *
 * The last three are what the module's static initialiser (__sinit at
 * 0x0212f660) passes to __cxa_vec_ctor and registers with the exit chain.
 *
 * Folded from ten one-function shards, one per address-named function
 * (func_ov006_020c8a30 .. func_ov006_020c8dd0); the one C++ shard,
 * func_ov006_020c8ba8, is the only one that needed C++ (it calls the
 * reference-taking ApproachLinear helpers). The run sits between the
 * promoted dMgJump3DMario_c unit (ends 0x020c8a30) and the unpromoted
 * Player/dMgTrmpln2Mario_c/dMgTrmpln3DMario_c unit (starts 0x020c8dd4)
 * with no gap on either side.
 *
 * Functions are written highest address first: mwccarm emits .text in
 * reverse source order.
 */

typedef long long s64;

struct Ent_ov006 {
    int x;
    int y;
    int f08;
    int f0c;
    int f10;
    int f14;
    short f18;
    short f1a;
    short f1c;
    short f1e;
};

extern int ApproachLinear(int&, int, int);
extern int ApproachLinear2(short&, short, short);

extern "C" {
int GetGameLanguage(void);
void Hud_RenderSprite(void* a0, int a1, int a2, int a3, int a4);
typedef void (*dtor_t)(void *);
void __cxa_vec_cleanup(void *block, unsigned int n, unsigned int size, dtor_t dtor);
extern short data_02082214[];
extern void **data_ov006_0213b0d8[];
extern int data_ov006_021404c4;
extern void **data_ov006_021404c8;
extern struct Ent_ov006 data_ov006_021404d8[3];
extern int data_ov006_02140518[];

void func_ov006_020c8af4(char *c);
void func_ov006_020c8ba8(char *c);
void func_ov006_020c8c6c(short *p);
void func_ov006_020c8dcc(void);

// @symbol func_ov006_020c8dd0
void func_ov006_020c8dd0(void)
{
}

// @symbol func_ov006_020c8dcc
void func_ov006_020c8dcc(void)
{
}

// @symbol func_ov006_020c8da8
void func_ov006_020c8da8(void)
{
    __cxa_vec_cleanup(data_ov006_021404d8, 3, 0x20, (dtor_t)func_ov006_020c8dcc);
}

// @symbol func_ov006_020c8c78
void func_ov006_020c8c78(int x, int y)
{
    int i;
    int v;

    if (data_ov006_021404c4 >= 3) {
        return;
    }
    for (i = 0; i < 3; i++) {
        if (data_ov006_021404d8[i].f18 != 0) {
            continue;
        }
        data_ov006_021404d8[i].f1a = data_ov006_021404c4;
        data_ov006_021404d8[i].x = x << 12;
        v = data_ov006_021404d8[i].x;
        if (v < 0x20000) {
            v = 0x20000;
        } else if (v > 0xE0000) {
            v = 0xE0000;
        }
        data_ov006_021404d8[i].x = v;
        data_ov006_021404d8[i].y = y << 12;
        data_ov006_021404d8[i].f08 = 0;
        data_ov006_021404d8[i].f0c = -0x400;
        data_ov006_021404d8[i].f18 = 1;
        data_ov006_021404d8[i].f1c = 0;
        data_ov006_021404d8[i].f14 = 0x1000;
        data_ov006_021404d8[i].f10 = 0x30000;
        data_ov006_021404d8[i].f1e = 0x5A;
        data_ov006_021404c4++;
        return;
    }
}

// @symbol func_ov006_020c8c6c
void func_ov006_020c8c6c(short *p)
{
    p[12] = 0;
}

// @symbol func_ov006_020c8ba8
void func_ov006_020c8ba8(char* c)
{
    if (*(short*)(c+0x18) == 0) return;
    *(short*)(((int)c + 0x1c)) += 0x200;
    *(int*)(c+0x10) = (int)(((long long)*(int*)(c+0x10) * *(int*)(c+0x14) + 0x800) >> 12);
    *(int*)(((int)c + 0xc)) -= 0x40;
    ApproachLinear(*(int*)(c+0x14), 0x800, 0x10);
    if (ApproachLinear2(*(short*)(c+0x1e), 0, 1) != 0) {
        if (*(short*)(c+0x1a) != 2)
            *(short*)(c+0x18) = 0;
    } else {
        *(int*)(c) += *(int*)(c+0x8);
        *(int*)(((int)c + 0x4)) += *(int*)(c+0xc);
    }
}

// @symbol func_ov006_020c8af4
void func_ov006_020c8af4(char *c)
{
    void *x;
    int t;
    int prod;
    int pos;
    if (*(short*)(c + 0x18) == 0) return;
    if (data_ov006_021404c8 != 0) {
        x = data_ov006_021404c8[*(short*)(c + 0x1a)];
    } else {
        int idx2 = GetGameLanguage();
        x = data_ov006_0213b0d8[idx2][*(short*)(c + 0x1a)];
    }
    t = data_02082214[(*(unsigned short*)(c + 0x1c) >> 4) * 2];
    prod = (int)(((s64)*(int*)(c + 0x10) * t + 0x800) >> 12);
    pos = *(int*)c + prod;
    Hud_RenderSprite(x, pos >> 12, *(int*)(c + 4) >> 12, -1, -1);
}

// @symbol func_ov006_020c8a9c
void func_ov006_020c8a9c(int a0,int a1){
  int i;
  short* p=(short*)data_ov006_021404d8;
  for(i=0;i<3;i++){
    func_ov006_020c8c6c(p);
    p=(short*)((char*)p+0x20);
  }
  data_ov006_021404c4=a0;
  data_ov006_021404c8=(void**)a1;
}

// @symbol func_ov006_020c8a64
void func_ov006_020c8a64(void){
  int i = 0;
  char *p = (char*)data_ov006_021404d8;
  for(;;){
    func_ov006_020c8ba8(p);
    i++;
    p += 0x20;
    if(i >= 3) break;
  }
}

// @symbol func_ov006_020c8a30
void func_ov006_020c8a30(void){
  int i;
  char *p=(char*)data_ov006_02140518;
  for(i=2;i>=0;i--){
    func_ov006_020c8af4(p);
    p-=0x20;
  }
}
}
