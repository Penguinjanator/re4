// game/hermite: 1-D Hermite curves (D:/Bio4/Prog/hermite.cpp). A Hermite1 is a list of keys
// (t, v, in/out tangents); the event fog/focus curves and camera paths evaluate them with
// Hermite_1CurveCalc, and the tool editors scale/translate/reverse them.
#include "types.h"
#include "hermite.h"
#include "main_mem.h"

// Empties the curve.
void Hermite_1Clear(Hermite1* pCurve)
{
    int i;

    for (i = 0; i < pCurve->num; i++) {
        memclr_asm(&pCurve->key[i], sizeof(HermiteKey));
    }
    pCurve->num = 0;
}

// 1 when t lies within the curve's key range.
int Hermite_1CurveRight(Hermite1* pCurve, f32 frame)
{
    if (pCurve == NULL || pCurve->num <= 0) {
        return 0;
    }
    if (frame < pCurve->key[0].t) {
        return 0;
    }
    if (frame > pCurve->key[pCurve->num - 1].t) {
        return 0;
    }
    return 1;
}

// Evaluates the curve at t into *out; 0 (no value) when t is outside the key range.
int Hermite_1CurveCalc(Hermite1* pCurve, f32 frame, f32* pS)
{
    if (pS == NULL) {
        return 0;
    }
    if (!Hermite_1CurveRight(pCurve, frame)) {
        return 0;
    }
    *pS = Hermite_1CurveCalc(pCurve, frame);
    return 1;
}

// Evaluates the curve at t (0 when no segment contains t).
f32 Hermite_1CurveCalc(Hermite1* pCurve, f32 frame)
{
    int num = pCurve->num;
    HermiteKey* k0 = NULL;
    HermiteKey* k1 = NULL;
    int found = 0;
    int i;
    f32 result;

    for (i = 0; i < num - 1; i++) {
        k0 = &pCurve->key[i];
        k1 = &pCurve->key[i + 1];
        if (frame >= k0->t && frame <= k1->t) {
            found = 1;
            break;
        }
    }
    if (found) {
        Hermite_1(k0, k1, frame, &result);
    } else {
        result = 0.0f;
    }
    return result;
}

// Scales the curve in time (about the first key) by sx and in value by sy, adjusting tangents.
void Hermite_1Scale(Hermite1* pScurve, f32 Hscale, f32 Vscale)
{
    int i;
    f32 base;

    base = pScurve->key[0].t;
    for (i = 0; i < pScurve->num; i++) {
        pScurve->key[i].t = (pScurve->key[i].t - base) * Hscale + base;
        pScurve->key[i].out /= Hscale;
        pScurve->key[i].in /= Hscale;
    }
    base = pScurve->key[0].v;
    for (i = 0; i < pScurve->num; i++) {
        pScurve->key[i].v = (pScurve->key[i].v - base) * Vscale + base;
        pScurve->key[i].out *= Vscale;
        pScurve->key[i].in *= Vscale;
    }
}

// Moves the curve so its first key is at (tx, ty).
void Hermite_1Trans(Hermite1* pScurve, f32 Xoffset, f32 Yoffset)
{
    int i;

    Xoffset -= pScurve->key[0].t;
    for (i = 0; i < pScurve->num; i++) {
        pScurve->key[i].t += Xoffset;
    }
    Yoffset -= pScurve->key[0].v;
    for (i = 0; i < pScurve->num; i++) {
        pScurve->key[i].v += Yoffset;
    }
}

// Reverses the curve in time (keys mirrored, tangents swapped and negated).
void Hermite_1Reverse(Hermite1* pScurve)
{
    int num = pScurve->num;
    HermiteKey* tmp = (HermiteKey*) Debug_alloc(num * sizeof(HermiteKey), 1);
    f32 t0, t1;
    int i;

    for (i = 0; i < num; i++) {
        tmp[i] = pScurve->key[i];
    }
    t0 = pScurve->key[0].t;
    t1 = pScurve->key[num - 1].t;
    for (i = 0; i < num; i++) {
        pScurve->key[i].t = t1 - tmp[num - 1 - i].t + t0;
        pScurve->key[i].v = tmp[num - 1 - i].v;
        pScurve->key[i].out = -tmp[num - 1 - i].in;
        pScurve->key[i].in = -tmp[num - 1 - i].out;
    }
    Debug_free(tmp);
}

// Cubic Hermite interpolation between two keys at time t.
void Hermite_1(HermiteKey* pH0, HermiteKey* pH1, f32 t, f32* pP)
{
    f32 dt = pH1->t - pH0->t;
    f32 s = (t - pH0->t) / dt;
    f32 s2 = s * s;
    f32 s3 = s * s2;
    f32 h01 = -(s3 + s3) + 3.0f * s2;
    f32 h11 = s3 - s2;
    f32 h10 = h11 - s2 + s;
    f32 h00 = -h01 + 1.0f;

    *pP = h00 * pH0->v + h01 * pH1->v + dt * (h10 * pH0->out + h11 * pH1->in);
}

// Derivative of the Hermite segment at time t.
void Hermite_1_dt(HermiteKey* pH0, HermiteKey* pH1, f32 t, f32* pT)
{
    f32 dt = pH1->t - pH0->t;
    f32 s = (t - pH0->t) / dt;
    f32 s2 = s * s;
    f32 dh11 = 3.0f * s2 - 2.0f * s;
    f32 dh10 = 3.0f * s2 - 2.0f * s - 2.0f * s + 1.0f;
    f32 dh00 = dh10 + dh11 - 1.0f;
    f32 dh01 = -dh00;

    *pT = dh00 * pH0->v + dh01 * pH1->v + dt * (dh10 * pH0->out + dh11 * pH1->in);
}
