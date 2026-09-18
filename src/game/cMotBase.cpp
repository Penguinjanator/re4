// game/cMotBase.cpp: cMotBase, the motion base follower: a target pose (pos / ang) integrated
// from the model's motion speed so the model can be blended toward where its animation says it
// should be (root motion), over `cnt` frames (0xFF = inactive).

#include "cMotBase.h"
#include "math_sub.h"

// Inactive, no model.
cMotBase::cMotBase()
{
    cnt = 0xFF;
    pModel = 0;
}

// Starts following: model, start pose p / r (also the "old" pose), blend over `c` frames.
void cMotBase::set(cMotModel* m, MotionData* data, Vec* p, Vec* r, u8 c)
{
    pModel = m;
    pos_old = *p;
    pos = pos_old;
    ang_old = *r;
    ang = ang_old;
    cnt = c;
}

// Same with the model's current motion; clears Mot_attr bit0 (the motion does not move the
// model itself while the base drives it).
void cMotBase::set(cMotModel* m, Vec* p, Vec* r, u8 hokan0)
{
    set(m, m->Motion.pMot, p, r, hokan0);
    m->Motion.Mot_attr &= ~1;
}

// add the model's movement since the last move to the followed pose
void cMotBase::adjust()
{
    Vec d;

    if (cnt == 0xFF) {
        return;
    }
    PSVECSubtract(&pModel->pos, &pos_old, &d);
    PSVECAdd(&pos, &d, &pos);
    ang.y += pModel->ang.y - ang_old.y;
}

// Per-frame: advances the target pose by the motion's translation / rotation speed (in model
// space), then either blends the model toward it over the remaining frames (setting Mot_attr
// bit0 back when done) or, at cnt 0, pins the model to it. Remembers the model pose for adjust.
void cMotBase::move()
{
    Vec spd;
    Vec rotSpd;

    if (cnt == 0xFF) {
        return;
    }
    MotionGetSpeed(pModel, &pModel->Motion, 0, &spd, &rotSpd);
    PSMTXMultVecSR(pModel->mat, &spd, &spd);
    PSVECAdd(&pos, &spd, &pos);
    PSVECAdd(&ang, &rotSpd, &ang);
    if (cnt != 0) {
        PSVECSubtract(&pos, &pModel->pos, &spd);
        PSVECScale(&spd, &spd, 1.0f / (f32) (int) cnt);
        PSVECAdd(&pModel->pos, &spd, &pModel->pos);
        pModel->ang.y += Muku2(pModel->ang.y, ang.y, PI / (f32) (int) cnt);
        cnt--;
        if (cnt == 0) {
            pModel->Motion.Mot_attr |= 1;
            cnt = 0xFF;
        }
    } else {
        pModel->pos = pos;
        pModel->ang = ang;
    }
    pos_old = pModel->pos;
    ang_old = pModel->ang;
}
