//cpp
// @symbol func_ov006_020e5450
/* recovered: Shell Smash (dScMgCurling2_c): a per-index update with sqrt and atan2 over the shell records.
   Matched under mwccarm 2004/b56. Two spellings below are codegen levers, not style:
   the contact angle's cosine and sine get explicit long long copies (cw, sw), and the
   copies are cast where they are made. Without the explicit cast the sine's sign word
   is hoisted above the hit stone's table reads. The hit stone's vex is computed before
   the contact table words are read; that keeps both loads below the hit stone's chain.
   The other levers (one rel/k/c/s set for both stones, pointer locals for the fields
   written after a call, the hitAngle reference) are described in notes/mwccarm-codegen.md 6cz. */
#include "dScMgCurling2_c.h"

extern "C" {
extern int _ZN4cstd4sqrtEy(u64 v);
extern short _ZN4cstd5atan2E5Fix12IiES1_(int y, int x);
extern void func_02012718(void *id, int v);
extern s16 data_02082214[];
}

#define FMUL(a, b) ((int)(((long long)(a) * (b) + 0x800) >> 12))

extern "C" void func_ov006_020e5450(dScMgCurling2_c *self, int idx)
{
    long long sw;
    long long cw;
    int i;
    int dx;
    int dy;

    for (i = 0; i < 11; i++) {
        if (self->mStone[i].active == 0) continue;
        if (idx == i) continue;
        if (self->mStone[i].state == 0) continue;
        if (self->mStone[i].state == 3) continue;
        dy = self->mStone[i].y;
        dx = self->mStone[i].x - self->mStone[idx].x;
        dy -= self->mStone[idx].y;
        if ((_ZN4cstd4sqrtEy((u64)((long long)dx * dx + (long long)dy * dy)) >> 12) >= 0x18) continue;
        {
            int nex;
            int ney;
            u16 ang;
            u16 rel;
            int k;
            int E;
            int c;
            int s;
            int sP;
            int cP;
            int vmx;
            int vmy;
            int vex;
            int vey;
            int yi;
            int xi;
            int *pHitX;
            int *pX;
            int *pHitY;

            dx = self->mStone[i].x - self->mStone[idx].x;
            dy = self->mStone[i].y - self->mStone[idx].y;
            pHitX = &self->mStone[i].x;
            pX = &self->mStone[idx].x;
            pHitY = &self->mStone[i].y;
            ang = _ZN4cstd5atan2E5Fix12IiES1_(dy, dx);
            u16 &hitAngle = self->mStone[i].angle;
            E = (ang >> 4) * 2;
            rel = self->mStone[idx].angle - ang;
            k = (rel >> 4) * 2;
            c = data_02082214[k + 1];
            s = data_02082214[k];
            vmx = FMUL(c, self->mStone[idx].speed);
            vmy = FMUL(s, self->mStone[idx].speed);
            rel = self->mStone[i].angle - ang;
            k = (rel >> 4) * 2;
            c = data_02082214[k + 1];
            s = data_02082214[k];
            vex = FMUL(c, self->mStone[i].speed);
            sP = data_02082214[E];
            cP = data_02082214[E + 1];
            vey = FMUL(s, self->mStone[i].speed);
            cw = (long long)cP;
            dy = (int)((cw * vex + 0x800) >> 12) - FMUL(sP, vmy);
            sw = (long long)sP;
            dx = (int)((sw * vex + 0x800) >> 12) + FMUL(cP, vmy);
            nex = FMUL(cP, vmx) - FMUL(sP, vey);
            ney = (int)((sw * vmx + 0x800) >> 12) + FMUL(cP, vey);
            self->mStone[idx].angle = _ZN4cstd5atan2E5Fix12IiES1_(dx, dy);
            self->mStone[idx].speed = _ZN4cstd4sqrtEy((u64)((long long)dy * dy + (long long)dx * dx));
            self->mStone[idx].x = self->mStone[i].x - FMUL(cP, 0x1b000);
            self->mStone[idx].y = self->mStone[i].y - (int)((sw * 0x1b000 + 0x800) >> 12);
            xi = self->mStone[idx].x >> 12;
            yi = self->mStone[idx].y >> 12;
            if (xi - 0xc < 0) {
                xi = self->mStone[idx].x - 0xc000;
                *pHitX += xi;
                self->mStone[idx].x = 0xc000;
            }
            if (xi + 0xc > 0x100) {
                *pHitX += self->mStone[idx].x - 0xf4000;
                self->mStone[idx].x = 0xf4000;
            }
            if (yi - 0xc < -0xe0) {
                self->mStone[idx].y = -0xd4000;
                *pHitY = self->mStone[idx].y + 0x18000;
            }
            hitAngle = _ZN4cstd5atan2E5Fix12IiES1_(ney, nex);
            self->mStone[i].speed = _ZN4cstd4sqrtEy((u64)((long long)nex * nex + (long long)ney * ney));
            self->mStone[idx].state = 1;
            self->mStone[i].state = 1;
            if (self->mStone[i].speed >= 0x3800) {
                self->mStone[i].fast = 1;
            } else {
                self->mStone[i].fast = 0;
            }
            func_02012718((void *) 0xe8, *pX);
            self->SpawnValue(idx, i);
            return;
        }
    }
}
