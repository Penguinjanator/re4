#include "atari.h"
#include "global.h"
#include "math_sub.h"
#include "rnd.h"
#include "esp.h"

struct Esp15Work {
    f32 Range;     // 0x00 half size of the box around the camera
    f32 Del_ratio;  // 0x04 1 - (distance ratio where the alpha starts fading)
    f32 Base_alpha;     // 0x08 base alpha
    f32 Min_y;    // 0x0C the sprite may not fall below this height (0 = none)
    u8 Room_del_frame;     // 0x10 fade in frames
    u8 Room_del_cnt;        // 0x11
};

// Camera-relative particle (rain / snow / dust): the position is wrapped so that it always
// stays inside a box around the camera.
class cEsp15 : public cEsp {
public:
    Esp15Work m_Free;  // 0xF8

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
};

cEsp* Esp15_Create()
{
    return new cEsp15;
}

void cEsp15::move()
{
    Esp15Work* w = &m_Free;
    Vec tmp;
    Vec dir;
    Vec sc;
    Vec sc2;
    f32 range;
    f32 half;
    f32 d;
    f32 oldY;
    int n;
    int flag1;
    int flag2;

    m_Col_a = w->Base_alpha;
    if (CommonMove()) {
        if (!AnmMove()) {
            PushEsp(this);
        } else {
            flag1 = 1;
            flag2 = 1;
            oldY = m_Pos.y;
            if (w->Min_y != 0.0f && m_Pos.y < w->Min_y) {
                flag1 = 0;
            }
            w->Base_alpha = m_Col_a;
            if (w->Room_del_frame != 0) {
                if (pG->Status_flg[1] & 0x02000000) {
                    w->Room_del_cnt++;
                } else if (w->Room_del_cnt != 0) {
                    w->Room_del_cnt--;
                }
                if (w->Room_del_cnt != 0) {
                    if (w->Room_del_cnt >= w->Room_del_frame) {
                        w->Room_del_cnt = w->Room_del_frame;
                    }
                    m_Col_a = m_Col_a * (1.0f - (f32)w->Room_del_cnt / w->Room_del_frame);
                }
            }
            range = w->Range;
            half = range * 0.6f;

            PSVECSubtract(&pG->Cam.param.at, &pG->Cam.param.pos, &dir);
            PSVECCrossProduct(&dir, &pG->Cam.up, &dir);
#line 111 "D:/Bio4/Prog/esp15.cpp"
            VECNormalize(&dir, &dir);
            PSVECSubtract(&m_Pos, &pG->Cam.param.pos, &tmp);
            d = PSVECDotProduct(&tmp, &dir);
            if (d >= 0.0f) {
                n = (int)((d + half) / (half * 2.0f));
            } else {
                n = (int)((d - half) / (half * 2.0f));
            }
            if (n != 0) {
                PSVECScale(&dir, &sc, -(half * 2.0f * (f32)n));
                PSVECAdd(&m_Pos, &sc, &m_Pos);
            }

#line 130 "D:/Bio4/Prog/esp15.cpp"
            VECNormalize(&pG->Cam.up, &dir);
            PSVECSubtract(&m_Pos, &pG->Cam.param.pos, &tmp);
            d = PSVECDotProduct(&tmp, &dir);
            if (d >= 0.0f) {
                n = (int)((d + half) / (half * 2.0f));
            } else {
                n = (int)((d - half) / (half * 2.0f));
            }
            if (n != 0) {
                PSVECScale(&dir, &sc2, -(half * 2.0f * (f32)n));
                PSVECAdd(&m_Pos, &sc2, &m_Pos);
            }

            PSVECSubtract(&pG->Cam.param.at, &pG->Cam.param.pos, &dir);
#line 152 "D:/Bio4/Prog/esp15.cpp"
            VECNormalize(&dir, &dir);
            PSVECSubtract(&m_Pos, &pG->Cam.param.pos, &tmp);
            d = PSVECDotProduct(&tmp, &dir);
            if (d >= 0.0f) {
                n = (int)(d / range);
            } else {
                n = -(int)(-d / range) - 1;
            }
            if (n != 0) {
                PSVECScale(&dir, &sc, -(range * (f32)n));
                PSVECAdd(&m_Pos, &sc, &m_Pos);
                PSVECSubtract(&m_Pos, &pG->Cam.param.pos, &tmp);
                d = PSVECDotProduct(&tmp, &dir);
            }
            if (d > range * w->Del_ratio) {
                m_Col_a = m_Col_a * (1.0f - (d - range * w->Del_ratio) / (range * (1.0f - w->Del_ratio)));
            }
            if (w->Min_y != 0.0f) {
                if (m_Pos.y < w->Min_y) {
                    flag2 = 0;
                }
                if (flag1 == 1 && flag2 == 0) {
                    m_Pos.y = oldY;
                }
            }
        }
    }
}

int cEsp15::SetFreeWork(EspGenWork* gen, u32* seed)
{
    Esp15Work* w = &m_Free;

    m_Del_far = (s8)gen->Work8[0] * 10;
    m_Del_near = (s8)gen->Work8[1] * 10;
    w->Del_ratio = (f32)(s8)gen->Work8[2] / 100.0f;
    if (w->Del_ratio > 1.0f) {
        w->Del_ratio = 1.0f;
    }
    w->Del_ratio = 1.0f - w->Del_ratio;
    w->Room_del_frame = gen->Work8[3];
    w->Range = gen->R_pos.z;
    m_Pos.x += w->Range * fRandSeed1_1(seed);
    m_Pos.y += w->Range * fRandSeed1_1(seed);
    m_Pos.z += w->Range * fRandSeed1_1(seed);
    w->Base_alpha = m_Col_a;
    w->Min_y = gen->Vec0.x;
    if (pGS->Status_flg[1] & 0x02000000) {
        m_Col_a = 0.0f;
        w->Room_del_cnt = w->Room_del_frame;
    }
    return 1;
}
