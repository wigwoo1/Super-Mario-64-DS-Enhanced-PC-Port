//cpp
/* Slot machine scene tail (MG_SLOT1): the scene factory + InitResources.
 * The scene's other methods live in src/actors/dScMgSlots.cpp; Behavior
 * sits between the two units at ov006:0x0210c9e0 and stays a draft shard
 * (notes/mwccarm-codegen.md section 6cm: the ROM recomputes idx*4 inside
 * the touch-hit block -- a placement cell 2004/b56 cannot reach).
 *
 * The factory leads the file on purpose: when it was defined after
 * InitResources, mwccarm emitted the extern "C" function's .text section
 * ahead of the member's, and the linker needs the member's section first
 * (measured; do not reorder). The allocation, the base C2 call and the
 * three vptr stores are written out by hand: the ROM has
 * no dScMgSlot1_c C1, so `new dScMgSlot1_c` cannot link.
 * Still raw: pad_4684, pad_4690 and pad_469c are embedded objects of
 * unknown type, set up by the unnamed func_ov006_0210c2b0 and func_ov006_0210c208.
 */

#include "types.h"
#include "decl_common.h"
#include "dScMgSlot1_c.h"

extern int data_ov004_020bc8b4;

extern "C" {

extern u32 LoadCompressedFileAt(int fileID, void *target);
extern char *_ZN2G213GetBG2CharPtrEv(void);
extern int LoadFile(int handle);
extern void _ZN2GX10LoadBGPlttEPKvjj(const void *p, u32 a, u32 b);
extern char *_ZN2G212GetBG2ScrPtrEv(void);
extern char *_ZN2G212GetBG3ScrPtrEv(void);
extern char *_ZN3G2S13GetBG2CharPtrEv(void);
extern char *_ZN3G2S13GetBG0CharPtrEv(void);
extern void _ZN3GXS10LoadBGPlttEPKvjj(const void *p, u32 a, u32 b);
extern char *_ZN3G2S12GetBG0ScrPtrEv(void);
extern char *_ZN3G2S12GetBG1ScrPtrEv(void);
extern char *_ZN3G2S12GetBG2ScrPtrEv(void);
extern char *_ZN3G2S12GetBG3ScrPtrEv(void);
extern void MultiStore16(u16 val, char *dst, int nbytes);
extern int GetGameLanguage(void);
extern void _ZN2GX11LoadOBJPlttEPKvjj(const void *p, u32 a, u32 b);
extern void _ZN3GXS11LoadOBJPlttEPKvjj(const void *p, u32 a, u32 b);
extern void _ZN3G2x13SetBlendAlphaEPVttttj(volatile u16 *p, u16 a, u16 b, u16 c, u16 d);

extern unsigned char data_0209d45c;
extern unsigned char data_0209d454;
extern int data_0208ee44;

}

extern "C" {
extern void *_ZN11dScMgBase_cC2Ev(void *);
extern void func_ov006_0210c2b0(char *p);
extern void func_ov006_0210c208(char *p);
extern void *_ZTV12dScMgSlot1_c;
extern void *_ZTVN12dScMgSlot1_c9betIcon_cE;
extern void *_ZTV9dThIcon_c;
void *dScMgSlot1_c_classInit(void);
/* The name is reconstructed; the ROM proves the class, not the spelling. */
void *dScMgSlot1_c_classInit(void)
{
    dScMgSlot1_c *scene =
        (dScMgSlot1_c *)_ZN7fBase_cnwEj(sizeof(dScMgSlot1_c));
    if (scene) {
        _ZN11dScMgBase_cC2Ev(scene);
        *(void **)scene = &_ZTV12dScMgSlot1_c;
        dScMgSlot1_c::betIcon_c *icon = &scene->mBetIcon;
        *(void **)icon = &_ZTV9dThIcon_c;
        *(void **)icon = &_ZTVN12dScMgSlot1_c9betIcon_cE;
        func_ov006_0210c2b0((char *)scene->pad_4684);
        func_ov006_0210c2b0((char *)scene->pad_4690);
        func_ov006_0210c208((char *)scene->pad_469c);
    }
    return scene;
}
}

struct Obj {
    virtual void m00();
    virtual void m04();
    virtual void m08();
    virtual void m0c();
    virtual void m10();
    virtual void m14();
    virtual void m18();
    virtual void m1c();
    virtual void m20();
    virtual void m24();
    virtual void m28();
    virtual void m2c();
    virtual void m30();
    virtual void m34();
    virtual void m38();
    virtual void m3c();
    virtual void m40();
    virtual void m44();
    virtual void m48(int a);   /* vtable offset 0x48 */
};

s32 dScMgSlot1_c::InitResources()
{
    char *c = (char *)this;
    volatile unsigned short fill;
    int i, j;

    *(volatile u16 *)0x400000c = (*(volatile u16 *)0x400000c & ~3) | 2;
    *(volatile u16 *)0x400000c = (*(volatile u16 *)0x400000c & 0x43) | 0x1210;
    *(volatile u16 *)0x400000c = *(volatile u16 *)0x400000c & ~0x40;
    *(volatile u16 *)0x400000e = (*(volatile u16 *)0x400000e & ~3) | 3;
    *(volatile u16 *)0x400000e = (*(volatile u16 *)0x400000e & 0x43) | 0x1310;
    *(volatile u16 *)0x400000e = *(volatile u16 *)0x400000e & ~0x40;

    LoadCompressedFileAt(0x7d, (void *)func_02054d88());
    LoadCompressedFileAt(0x7b, _ZN2G213GetBG2CharPtrEv() + 0x7800);
    {
        int h = LoadFile(0x7e);
        _ZN2GX10LoadBGPlttEPKvjj((const void *)h, 0x60, 0x1a0);
        Deallocate((void *)h);
    }
    LoadCompressedFileAt(0x7c, _ZN2G212GetBG2ScrPtrEv());
    LoadCompressedFileAt(0x7f, _ZN2G212GetBG3ScrPtrEv());

    data_0209d45c = 0x1c;

    *(volatile u16 *)0x4001008 = (*(volatile u16 *)0x4001008 & ~3) | 1;
    *(volatile u16 *)0x4001008 = (*(volatile u16 *)0x4001008 & 0x43) | 0x1410;
    *(volatile u16 *)0x4001008 = *(volatile u16 *)0x4001008 & ~0x40;
    *(volatile u16 *)0x400100a = (*(volatile u16 *)0x400100a & ~3) | 1;
    *(volatile u16 *)0x400100a = (*(volatile u16 *)0x400100a & 0x43) | 0x4010;
    *(volatile u16 *)0x4001008 = *(volatile u16 *)0x4001008 & ~0x40;
    SetSubBg1Offset(0x100, 0);

    *(volatile u16 *)0x400100c = (*(volatile u16 *)0x400100c & ~3) | 2;
    *(volatile u16 *)0x400100c = (*(volatile u16 *)0x400100c & 0x43) | 0x1218;
    *(volatile u16 *)0x400100c = *(volatile u16 *)0x400100c & ~0x40;
    *(volatile u16 *)0x400100e = (*(volatile u16 *)0x400100e & ~3) | 3;
    *(volatile u16 *)0x400100e = (*(volatile u16 *)0x400100e & 0x43) | 0x1310;
    *(volatile u16 *)0x400100e = *(volatile u16 *)0x400100e & ~0x40;

    LoadCompressedFileAt(0x72, _ZN3G2S13GetBG2CharPtrEv());
    LoadCompressedFileAt(0x7b, _ZN3G2S13GetBG0CharPtrEv() + 0x7800);
    {
        int h = LoadFile(0x73);
        _ZN3GXS10LoadBGPlttEPKvjj((const void *)h, 0x60, 0x1a0);
        Deallocate((void *)h);
    }
    LoadCompressedFileAt(4, _ZN3G2S12GetBG0ScrPtrEv());
    LoadCompressedFileAt(1, _ZN3G2S12GetBG1ScrPtrEv() + 0x800);
    LoadCompressedFileAt(0x74, _ZN3G2S12GetBG2ScrPtrEv());

    {
        char *d3 = _ZN3G2S12GetBG3ScrPtrEv();
        fill = 0x43e4;
        MultiStore16(fill, d3, 0x800);
    }

    LoadCompressedFileAt(data_ov006_0213e628[GetGameLanguage()], (void *)0x6400000);
    LoadCompressedFileAt(data_ov006_0213e628[GetGameLanguage()], (void *)0x6600000);

    {
        int h = LoadFile(0xf1);
        _ZN2GX11LoadOBJPlttEPKvjj((const void *)h, 0, 0x40);
        _ZN3GXS11LoadOBJPlttEPKvjj((const void *)h, 0, 0x40);
        Deallocate((void *)h);
    }

    data_0209d454 = 0x1d;
    *(volatile u16 *)0x4000050 = 0;
    _ZN3G2x13SetBlendAlphaEPVttttj((volatile u16 *)0x4001050, 1, 0x1c, 7, 9);

    data_ov004_020bc88c = 0x80;
    data_ov004_020bc860 = 0x40;
    data_ov004_020bc878 = 0x80;
    data_ov004_020bc890 = 0x40;
    data_ov004_020bc8b8 = 0x80;
    data_ov004_020bc8b4 = 0x40;

    if ((int)param1 & 0xff) {
        unsigned char *tbl = data_ov006_0213ea20;
        char *p = c;
        for (j = 0; j < 3; j++) {
            for (i = 0; i < 0x15; i++) {
                *(u8 *)(p + 0x46c0 + i) = tbl[i];
            }
            *(u8 *)(c + 0x46ff + j) = 0;
            ((u32 *)(c + 0x46a4))[j] = 0x40000;
            *(u8 *)(c + 0x4702 + j) = 1;
            tbl += 0x15;
            p += 0x15;
        }
    } else {
        int k;
        unsigned char *tbl = data_ov006_0213e9e0;
        char *p = c;
        for (k = 0; k < 3; k++) {
            for (i = 0; i < 0x15; i++) {
                *(u8 *)(p + 0x46c0 + i) = tbl[i];
            }
            *(u8 *)(c + 0x46ff + k) = 0;
            ((u32 *)(c + 0x46a4))[k] = 0x40000;
            *(u8 *)(c + 0x4702 + k) = 1;
            tbl += 0x15;
            p += 0x15;
        }
    }

    unk_4709 = 5;
    unk_46bc = 0xa;
    func_ov004_020adb1c(unk_46bc);
    func_ov006_0210c478(c + 0x4660);
    ((struct Obj *)c)->m48(3);
    func_ov004_020b04d0(0x20);
    data_0208ee44 = 1;
    return 1;
}
