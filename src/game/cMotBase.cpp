#include "cMotBase.h"
#include "math_sub.h"

cMotBase::cMotBase()
{
    cnt = 0xFF;
    pModel = 0;
}

void cMotBase::set(cMotModel* m, MotionData* data, Vec* p, Vec* r, u8 c)
{
    pModel = m;
    pos_old = *p;
    pos = pos_old;
    ang_old = *r;
    ang = ang_old;
    cnt = c;
}

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
