// game/light06: light type 6, linear fade (D:/Bio4/Prog/light06.cpp): the brightness rate starts at
// `start` and moves by `speed` per frame, clamped to 0..1.
#include "light.h"

struct Light06Work {
    f32 start;  // 0x00 initial rate
    f32 speed;  // 0x04 rate change per frame (sign gives direction)
    f32 rate;   // 0x08 current brightness rate 0..1
};

// LightFuncTbl[6]: Rno0 0 loads the start rate, 1 fades and writes DispCol = Col * rate.
// Fade light: brightness rate moves from `start` by `speed` and is clamped to [0, 1].
void Light06_Move(cLight* pLi)
{
    Light06Work* w = (Light06Work*)pLi->work;

    switch (pLi->Rno0) {
    case 0:
        w->rate = w->start;
        pLi->Rno0 = 1;
    case 1:
        w->rate += w->speed;
        if (w->speed > 0.0f) {
            if (w->rate > 1.0f) {
                w->rate = 1.0f;
            }
        } else {
            if (w->rate < 0.0f) {
                w->rate = 0.0f;
            }
        }
        pLi->DispCol.r = (u8)(w->rate * pLi->Col.r);
        pLi->DispCol.g = (u8)(w->rate * pLi->Col.g);
        pLi->DispCol.b = (u8)(w->rate * pLi->Col.b);
        break;
    }
}
