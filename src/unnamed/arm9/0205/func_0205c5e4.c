extern unsigned int _ZN3IRQ7DisableEv();
extern void _ZN3IRQ7RestoreEj(unsigned int);
extern void OS_SleepThread(unsigned short*);
extern void func_0205c788(char*, int);
#if defined(SM64DS_PLATFORM_PC)
extern int func_0205c448(char*);
extern int func_0205c410(char*);
extern int func_0205c378(int*);
extern int func_0205c264(int*);
extern int func_0205c048(unsigned char*);
extern int func_0205bc88(char*);
extern int func_0205bc0c(char*);
extern int func_0205bbe4(void*);
extern int func_0205bbdc(void);

static unsigned int pc_callback(char* self, int idx) {
    switch (idx) {
    case 0: return (unsigned int)func_0205c448(self);
    case 1: return (unsigned int)func_0205c410(self);
    case 2: return (unsigned int)func_0205c378((int*)self);
    case 3: return (unsigned int)func_0205c264((int*)self);
    case 4: return (unsigned int)func_0205c048((unsigned char*)self);
    case 5: return (unsigned int)func_0205bc88(self);
    case 6: return (unsigned int)func_0205bc0c(self);
    case 7: return (unsigned int)func_0205bbe4((void*)self);
    case 8: return (unsigned int)func_0205bbdc();
    default: return 1;
    }
}
#else
extern unsigned int (*data_02086758[])(char*);
#endif

/* The hardware register block lives at r + 0x10 / r + 0x4c.  mwccarm folds a
   plain (u32*)(r + off) straight into the ldr/str addressing mode; the ROM
   instead materialises the address into a scratch register first.  Widening
   the address through a 64-bit value forces that explicit base add. */
#define REG(x) ((unsigned int*)(x))
#define REGU(x) ((unsigned int*)(x))

int func_0205c5e4(char* self, int idx) {
    char* r;
    unsigned int flags;
    int ret;
    int mask;
    unsigned int irq;
    int enabled;
    int busy;

    flags = *(unsigned int*)(self + 0xc);
    enabled = 1;
    r = *(char**)(self + 8);
    mask = 1 << idx;
    if ((flags & 4) == 0)
        enabled = 0;

    if (enabled) {
        *REGU(r + 0x10) |= 0x200;
    } else {
        *REGU(r + 0x10) |= 0x100;
    }

    if (*(unsigned int*)(r + 0x4c) & mask) {
        ret = (*(int(**)(char*, int))(r + 0x48))(self, idx);
        switch (ret) {
        case 0:
        case 1:
        case 4:
            *(int*)(self + 0x14) = ret;
            break;
        case 8:
            *REG(r + 0x4c) &= ~mask;
            ret = 7;
            break;
        }
    } else {
        ret = 7;
    }

    if (ret == 7) {
#if defined(SM64DS_PLATFORM_PC)
        ret = (int)pc_callback(self, idx);
#else
        ret = data_02086758[idx](self);
#endif
    }

    if (ret == 6) {
        int t = (*(unsigned int*)(self + 0xc) & 4) ? 1 : 0;
        if (t != 0) {
            unsigned short* p;
            irq = _ZN3IRQ7DisableEv();
            p = (unsigned short*)(r + 0xc);
            goto chk;
            do {
                OS_SleepThread(p);
chk:
                busy = (*(unsigned int*)(r + 0x10) & 0x200) ? 1 : 0;
            } while (busy != 0);
            ret = *(int*)(self + 0x14);
            _ZN3IRQ7RestoreEj(irq);
        }
    } else {
        int t2 = (*(unsigned int*)(self + 0xc) & 4) ? 1 : 0;
        if (t2 == 0) {
            *REG(r + 0x10) &= ~0x100;
            func_0205c788(self, ret);
        } else {
            *REG(r + 0x10) &= ~0x200;
            *(int*)(self + 0x14) = ret;
        }
    }

    return ret;
}
