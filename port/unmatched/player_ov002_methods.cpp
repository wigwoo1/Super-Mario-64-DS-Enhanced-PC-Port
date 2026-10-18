// Host extraction of the two ov002 Player routines needed by Gate 9.
// Bodies are carried over from the recovered Player aggregate; the aggregate
// itself has function-local class declarations that MSVC cannot compile.
#include "Player.h"
#include "SharedFilePtr.h"

#define LAUNDER16(p) ((u16 *)(int)(p))

extern "C" {
extern void _ZN15dExtFrameCtrl_c8LoadFileER13SharedFilePtr(void *);
extern int _ZNK6Player14GetBodyModelIDEjb(void *, u32, int);
extern void _ZN9ModelAnim7SetAnimEP8BCA_Filei5Fix12IiEj(void *, void *, int, Fix12i, u32);
extern void _ZN15TextureSequence7SetFileER8BTP_Filei5Fix12IiEj(
    TextureSequence *, BTP_File *, s32, s32, u16);
extern void _ZN15dExtFrameCtrl_c12SetAnimationEti5Fix12IiEt(dExtFrameCtrl_c *, u16, s32, s32, u16);
extern u8 data_0209f2d8;
extern void *data_ov002_020ff480[];
extern void *data_ov002_020ff2f0[];

void _ZN6Player7SetAnimEji5Fix12IiEj(char *self, u32 a, int b, Fix12i c, u16 d)
{
    u32 charId;
    void *sfp;
    u32 sl;
    int field8;
    u32 newVal;
    int id;
    int flag;
    int param2;
    void *modelPtr;

    flag = (data_0209f2d8 == 1);
    if (flag && ((int)a < 0x11 || (int)a >= 0x72)) {
        u16 *f = LAUNDER16(self + 0x6ce);
        *f = (u16)(*f | 0x80);
        return;
    }

    newVal = a << 2;
    if (newVal != *(u32 *)(self + 0x63c)) {
        ((SharedFilePtr *)(data_ov002_020ff480[
            *(u32 *)(self + 0x63c) + (*(int *)(self + 8) & 3)]))->Release();
        sfp = data_ov002_020ff480[newVal + (*(int *)(self + 8) & 3)];
        _ZN15dExtFrameCtrl_c8LoadFileER13SharedFilePtr(sfp);
        id = _ZNK6Player14GetBodyModelIDEjb(self, *(int *)(self + 8) & 0xff, 0);
        modelPtr = ((void **)(self + 0xdc))[id];
        *(int *)((char *)modelPtr + 0x60) = 0;

        if ((*(u16 *)(self + 0x73c) & 7) == 4) {
            charId = *(u8 *)(self + 0x6dd);
            if (charId == *(u32 *)(self + 8)) charId = *(u8 *)(self + 0x6dc);
            ((SharedFilePtr *)(data_ov002_020ff480[
                *(u32 *)(self + 0x63c) + charId]))->Release();
            sfp = data_ov002_020ff480[newVal + charId];
            _ZN15dExtFrameCtrl_c8LoadFileER13SharedFilePtr(sfp);
            id = _ZNK6Player14GetBodyModelIDEjb(self, charId & 0xff, 0);
            _ZN9ModelAnim7SetAnimEP8BCA_Filei5Fix12IiEj(
                ((void **)(self + 0xdc))[id], *(void **)((char *)sfp + 4), b, c, d);
            *(u16 *)(self + 0x73e) = (u16)(*(u16 *)(self + 0x73e) + 1);
        }
    }

    field8 = *(int *)(self + 8);
    sl = a * 4;
    id = _ZNK6Player14GetBodyModelIDEjb(self, (u32)(field8 & 0xff), 0);
    _ZN9ModelAnim7SetAnimEP8BCA_Filei5Fix12IiEj(
        ((void **)(self + 0xdc))[id],
        *(void **)((char *)data_ov002_020ff480[(field8 & 3) + a * 4] + 4), b, c, d);

    field8 = *(int *)(self + 8);
    if ((int)sl >= 0x60) sl = 0x60;
    sl += (field8 & 3);
    param2 = ((int)sl >> 2) == 1 ? 0x40000000 : 0;
    _ZN15TextureSequence7SetFileER8BTP_Filei5Fix12IiEj(
        (TextureSequence *)(self + 0x1dc + field8 * 0x14),
        (BTP_File *)*(void **)((char *)data_ov002_020ff2f0[sl] + 4),
        param2, 0x1000, 0);
    *(u32 *)(self + 0x63c) = newVal;
}

int func_ov002_020cd28c(char *self)
{
    _ZN6Player7SetAnimEji5Fix12IiEj(self, 0xaf, 0x40000000, 0x1000, 0);
    *(u8 *)(self + 0x6e3) = 7;
    return 7;
}

void _ZN15TextureSequence7SetFileER8BTP_Filei5Fix12IiEj(
    TextureSequence *self, BTP_File *file, s32 flags, s32 speed, u16 startFrame)
{
    if (file == self->file) {
        self->SetFlags(flags);
        self->speed = speed;
    } else {
        self->file = file;
        _ZN15dExtFrameCtrl_c12SetAnimationEti5Fix12IiEt(
            self, file->unk_00, flags, speed, startFrame);
    }
}

int _ZN6Player11ChangeStateERNS_5StateE(Player *self, Player::State &state)
{
    return self->Player::ChangeState(state);
}

int _ZN6Player8HasNoCapEv(Player *self)
{
    extern signed char data_0209f2f8;
    extern unsigned char data_0209f2d8;
    extern int _ZN6Player7IsStateERNS_5StateE(void *, void *);
    extern int _ZN8SaveData16HasPlayerLostCapEv(void);
    extern Player::State data_ov002_0211058c;
    if (data_0209f2f8 == 0x1f) return 0;
    int r = (data_0209f2d8 == 2);
    if (!r) {
        if (!_ZN6Player7IsStateERNS_5StateE(self, &data_ov002_0211058c)) {
            r = (data_0209f2d8 == 1);
            if (!r && _ZN8SaveData16HasPlayerLostCapEv()) return 1;
        }
    }
    if (self->param1 == 3) return 0;
    return self->mHasNoCap != 0;
}

void Player::func_ov002_020c9e18()
{
    if (mIsNoControl != 1) return;
    mIsNoControl = 0;
    *(unsigned int *)((char *)this + 0x2ec) &= ~4u;
}

}
