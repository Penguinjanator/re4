// game/esp4c.cpp: effect id 0x4C, the controller for est generator 45 (Espgen45: rain / snow /
// dust weather particles around the camera). It is invisible; every frame it feeds its position,
// size, colour and fade into the generator through the Estgen45Set* interface, and with Tool_flg
// bit0 also the extended parameter block (reflection type, shimmer, spread/damp, mask texture).

#include "atari.h"
#include "light.h"
#include "esp.h"
#include "espgen.h"

// Est generator 45 parameter block filled from this effect (game/espgen45.cpp).
struct Esp4cWork {
    u8 Type;      // 0x00
    u8 Refrect_type;        // 0x01
    u8 Spec_Tex;        // 0x02
    u8 wave_ratio_base;        // 0x03
    u16 Shimmer_pow1;       // 0x04
    u16 Shimmer_pow2;       // 0x06
    f32 spread;   // 0x08
    f32 damp;     // 0x0C
    Vec ang;      // 0x10
    u8 flag;      // 0x1C
    u8 MaskTex_id; // 0x1D  gen->MaskTex_id (unused after set-up)
};

// Weather (est generator 45) controller: pushes its color/size into the generator every frame.
class cEsp4c : public cEsp {
public:
    Esp4cWork m_Free;  // 0xF8

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
    virtual void Destruct();
};

// EspCreateTbl[0x4C] factory.
cEsp* Esp4c_Create()
{
    return new cEsp4c;
}

// Runs the base update but restores the colour (the generator does its own fading), then pushes
// target position (m_Pos x/z; y only with Tool_flg bit1), size, colour + fade steps and, with
// Tool_flg bit0, the Esp4cWork parameter block into Espgen45.
void cEsp4c::move()
{
    Esp4cWork* w = &m_Free;
    f32 r = m_Col_r;
    f32 g = m_Col_g;
    f32 b = m_Col_b;
    f32 a = m_Col_a;

    if (CommonMove()) {
        m_Col_r = r;
        m_Col_g = g;
        m_Col_b = b;
        m_Col_a = a;
        Estgen45SetTargetCamera(0);
        Estgen45SetTargetPos(m_Pos.x, m_Pos.z);
        if (m_Tool_flg & 2) {
            Estgen45SetTargetHeight(1);
            Estgen45SetHeight(m_Pos.y);
        } else {
            Estgen45SetTargetHeight(0);
        }
        Estgen45SetSizeOverWrite(1);
        Estgen45SetSize(m_Size_base_x * m_Size_mul);
        Estgen45SetColorMul(1);
        Estgen45SetColor((u8)m_Col_r, (u8)m_Col_g, (u8)m_Col_b, (u8)m_Col_a, m_Col_d_r, m_Col_d_g, m_Col_d_b, m_Col_d_a);
        if (m_Tool_flg & 1) {
            Estgen45SetColorOverWrite(1);
            Estgen45SetParamOverWrite(1);
            Estgen45SetParam(w);
        } else {
            Estgen45SetColorOverWrite(0);
            Estgen45SetParamOverWrite(0);
        }
    }
}

// Nothing to release.
void cEsp4c::Destruct()
{
}

// EspTransTbl[0x4C]: draws nothing.
void Esp4c_Trans()
{
}

// Fills Esp4cWork from the record: Type Work8[0] (2 = spread/damp from Work8[1..2]), specular
// texture Tex_id, shimmer powers prm 0xCE/0xD2, reflection type Work8[3], angles (degrees ->
// radians), mask texture when Tool_flg 0x4000; then applies the first frame at once.
int cEsp4c::SetFreeWork(EspGenWork* gen, u32* seed)
{
    Esp4cWork* w = &m_Free;

    w->wave_ratio_base = gen->WorkSp8[2];
    w->Type = gen->Work8[0];
    if (w->Type == 2) {
        w->spread = 0.5f - (f32)(s8)gen->Work8[1] * 0.005f;
        if (w->spread > 0.5f) {
            w->spread = 0.5f;
        }
        if (w->spread < 0.0f) {
            w->spread = 0.0f;
        }
        w->damp = 0.99f - gen->Work8[2] * 0.001f;
    }
    w->Spec_Tex = gen->Tex_id;
    w->Shimmer_pow1 = gen->prm.h.xCE;
    w->Shimmer_pow2 = gen->prm.h.xD2;
    w->Refrect_type = gen->Work8[3];
    w->ang = gen->Ang;
    PSVECScale(&w->ang, &w->ang, 3.14 / 180);
    if (gen->Tool_flg & 0x4000) {
        w->flag |= 2;
        w->MaskTex_id = gen->MaskTex_id;
        w->flag |= 1;
    }
    move();
    return 1;
}
