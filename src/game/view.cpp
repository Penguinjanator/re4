#include "types.h"
#include "vec.h"
#include "global.h"
#include "math_sub.h"
#include "db_log.h"
#include "view.h"

#define VIEW_ASPECT 1.33333333f

VIEW View;
u8 ViewHit[0xD00];

void VIEW::gameInit(Camera* cam)
{
    pCam = cam;
    roomInit();
}

void VIEW::roomInit()
{
    init();
}

void VIEW::init()
{
    initPerspective(pCam->param.fovy, VIEW_ASPECT, ZNEAR, ZFAR);
    orientation();
    fovyOld = pCam->param.fovy;
    zfarOld = zfar;
}

void VIEW::move()
{
    if (fovyOld != pCam->param.fovy || zfarOld != zfar) {
        initPerspective(pCam->param.fovy, VIEW_ASPECT, ZNEAR, zfar);
    }
    orientation();
    fovyOld = pCam->param.fovy;
    zfarOld = zfar;
}

void VIEW::setFarPlane(f32 z)
{
    zfar = z;
}

// initPerspective: the original's algorithm -- three Vec temporaries (the second normal block uses
// t1/t3), `Vec q[4]` for the sphere points, the frustum points halved with an indexed loop, and
// the circumsphere numerators written with the point differences inline (recomputed from the q
// copies after the PSVECSquareMag calls).
// Shape (from the target's asm): block 1 addresses `b->point[k]` inline; `c = &local; *c = localFull;`
// (c set before the copy loop so cse cannot fold `c + K` to `this + K` after it); block 2 through 14
// pointer locals p0..p7/n0..n5 assigned in the halving loop's preheader; the halving loop as
// `px[i * 3] *= 0.5f; py[i * 3] *= 0.5f;` (two bases, one index giv -> lfsx/stfsx); `b = &localFull`
// re-assigned for the sphere block; q copies field-wise (a struct copy forces `&b->point[k]` into a
// pseudo that cse folds to `this + K` and gcse then hoists).
void VIEW::initPerspective(f32 fovy_, f32 aspect_, f32 znear_, f32 zfar_)
{
    Vec t1;
    Vec t2;
    Vec t3;
    Vec q[4];
    ViewFrustum* b;
    ViewFrustum* c;
    Vec* p0;
    Vec* p1;
    Vec* p2;
    Vec* p3;
    Vec* p4;
    Vec* p5;
    Vec* p6;
    Vec* p7;
    Vec* n0;
    Vec* n1;
    Vec* n2;
    Vec* n3;
    Vec* n4;
    Vec* n5;
    f32 t;
    f32 h;
    f32 w;
    f32 zn;
    f32 zf;
    f32 det;
    f32 d0;
    f32 d1;
    f32 d2;
    int i;

    fovy = fovy_;
    aspect = aspect_;
    znear = znear_;
    zfar = zfar_;
    t = sinf(fovy * 0.5f * 3.1415927f / 180.0f) / cosf(fovy * 0.5f * 3.1415927f / 180.0f);
    b = &localFull;
    zn = znear;
    h = zn * t;
    w = h * aspect_;
    b->point[0].x = w;
    b->point[0].y = h;
    b->point[0].z = -zn;
    b->point[1].x = -w;
    b->point[1].y = h;
    b->point[1].z = -zn;
    b->point[2].x = -w;
    b->point[2].y = -h;
    b->point[2].z = -zn;
    b->point[3].x = w;
    b->point[3].y = -h;
    b->point[3].z = -zn;
    zf = zfar;
    h = zf * t;
    w = h * aspect_;
    b->point[4].x = w;
    b->point[4].y = h;
    b->point[4].z = -zf;
    b->point[5].x = -w;
    b->point[5].y = h;
    b->point[5].z = -zf;
    b->point[6].x = -w;
    b->point[6].y = -h;
    b->point[6].z = -zf;
    b->point[7].x = w;
    b->point[7].y = -h;
    b->point[7].z = -zf;

#line 190 "D:/Bio4/Prog/view.cpp"
    PSVECSubtract(&b->point[1], &b->point[0], &t1);
    PSVECSubtract(&b->point[3], &b->point[0], &t2);
    PSVECCrossProduct(&t1, &t2, &b->normal[0]);
    VECNormalize(&b->normal[0], &b->normal[0]);

    PSVECSubtract(&b->point[3], &b->point[0], &t1);
    PSVECSubtract(&b->point[4], &b->point[0], &t2);
    PSVECCrossProduct(&t1, &t2, &b->normal[1]);
    VECNormalize(&b->normal[1], &b->normal[1]);

    PSVECSubtract(&b->point[4], &b->point[0], &t1);
    PSVECSubtract(&b->point[1], &b->point[0], &t2);
    PSVECCrossProduct(&t1, &t2, &b->normal[2]);
    VECNormalize(&b->normal[2], &b->normal[2]);

    PSVECSubtract(&b->point[5], &b->point[1], &t1);
    PSVECSubtract(&b->point[2], &b->point[1], &t2);
    PSVECCrossProduct(&t1, &t2, &b->normal[3]);
    VECNormalize(&b->normal[3], &b->normal[3]);

    PSVECSubtract(&b->point[6], &b->point[2], &t1);
    PSVECSubtract(&b->point[3], &b->point[2], &t2);
    PSVECCrossProduct(&t1, &t2, &b->normal[4]);
    VECNormalize(&b->normal[4], &b->normal[4]);

    PSVECSubtract(&b->point[7], &b->point[4], &t1);
    PSVECSubtract(&b->point[5], &b->point[4], &t2);
    PSVECCrossProduct(&t1, &t2, &b->normal[5]);
    VECNormalize(&b->normal[5], &b->normal[5]);

    c = &local;
    *c = localFull;
    p0 = &c->point[0];
    p1 = &c->point[1];
    p2 = &c->point[2];
    p3 = &c->point[3];
    p4 = &c->point[4];
    p5 = &c->point[5];
    p6 = &c->point[6];
    p7 = &c->point[7];
    n0 = &c->normal[0];
    n1 = &c->normal[1];
    n2 = &c->normal[2];
    n3 = &c->normal[3];
    n4 = &c->normal[4];
    n5 = &c->normal[5];
    {
        f32* px = &p0->x;
        f32* py = &p0->y;

        for (i = 0; i < 8; i++) {
            px[i * 3] *= 0.5f;
            py[i * 3] *= 0.5f;
        }
    }
#line 236 "D:/Bio4/Prog/view.cpp"
    PSVECSubtract(p1, p0, &t1);
    PSVECSubtract(p3, p0, &t3);
    PSVECCrossProduct(&t1, &t3, n0);
    VECNormalize(n0, n0);

    PSVECSubtract(p3, p0, &t1);
    PSVECSubtract(p4, p0, &t3);
    PSVECCrossProduct(&t1, &t3, n1);
    VECNormalize(n1, n1);

    PSVECSubtract(p4, p0, &t1);
    PSVECSubtract(p1, p0, &t3);
    PSVECCrossProduct(&t1, &t3, n2);
    VECNormalize(n2, n2);

    PSVECSubtract(p5, p1, &t1);
    PSVECSubtract(p2, p1, &t3);
    PSVECCrossProduct(&t1, &t3, n3);
    VECNormalize(n3, n3);

    PSVECSubtract(p6, p2, &t1);
    PSVECSubtract(p3, p2, &t3);
    PSVECCrossProduct(&t1, &t3, n4);
    VECNormalize(n4, n4);

    PSVECSubtract(p7, p4, &t1);
    PSVECSubtract(p5, p4, &t3);
    PSVECCrossProduct(&t1, &t3, n5);
    VECNormalize(n5, n5);

    orientation();

    b = &localFull;
    q[0].x = b->point[0].x;
    q[0].y = b->point[0].y;
    q[0].z = b->point[0].z;
    q[1].x = b->point[4].x;
    q[1].y = b->point[4].y;
    q[1].z = b->point[4].z;
    q[2].x = b->point[5].x;
    q[2].y = b->point[5].y;
    q[2].z = b->point[5].z;
    q[3].x = b->point[6].x;
    q[3].y = b->point[6].y;
    q[3].z = b->point[6].z;
    det = (q[1].x - q[0].x) * (q[2].y - q[1].y) * (q[3].z - q[2].z) + (q[2].x - q[1].x) * (q[3].y - q[2].y) * (q[1].z - q[0].z) +
          (q[3].x - q[2].x) * (q[1].y - q[0].y) * (q[2].z - q[1].z) - (q[1].x - q[0].x) * (q[3].y - q[2].y) * (q[2].z - q[1].z) -
          (q[2].x - q[1].x) * (q[1].y - q[0].y) * (q[3].z - q[2].z) - (q[3].x - q[2].x) * (q[2].y - q[1].y) * (q[1].z - q[0].z);
    d0 = PSVECSquareMag(&q[0]) - PSVECSquareMag(&q[1]);
    d1 = PSVECSquareMag(&q[1]) - PSVECSquareMag(&q[2]);
    d2 = PSVECSquareMag(&q[2]) - PSVECSquareMag(&q[3]);
    det = det + det;
    sphere.center.x = (d0 * ((q[3].y - q[2].y) * (q[2].z - q[1].z) - (q[2].y - q[1].y) * (q[3].z - q[2].z)) +
                       d1 * ((q[1].y - q[0].y) * (q[3].z - q[2].z) - (q[3].y - q[2].y) * (q[1].z - q[0].z)) +
                       d2 * ((q[2].y - q[1].y) * (q[1].z - q[0].z) - (q[1].y - q[0].y) * (q[2].z - q[1].z))) /
                      det;
    sphere.center.y = (d0 * ((q[3].z - q[2].z) * (q[2].x - q[1].x) - (q[2].z - q[1].z) * (q[3].x - q[2].x)) +
                       d1 * ((q[1].z - q[0].z) * (q[3].x - q[2].x) - (q[3].z - q[2].z) * (q[1].x - q[0].x)) +
                       d2 * ((q[2].z - q[1].z) * (q[1].x - q[0].x) - (q[1].z - q[0].z) * (q[2].x - q[1].x))) /
                      det;
    sphere.center.z = (d0 * ((q[3].x - q[2].x) * (q[2].y - q[1].y) - (q[2].x - q[1].x) * (q[3].y - q[2].y)) +
                       d1 * ((q[1].x - q[0].x) * (q[3].y - q[2].y) - (q[3].x - q[2].x) * (q[1].y - q[0].y)) +
                       d2 * ((q[2].x - q[1].x) * (q[1].y - q[0].y) - (q[1].x - q[0].x) * (q[2].y - q[1].y))) /
                      det;
    sphere.radius = PSVECDistance(&sphere.center, &q[0]);
}

void VIEW::orientation()
{
    Mtx* m = &pG->Cam.mat;
    u32 i;

    for (i = 0; i < 6; i++) {
        PSMTXMultVecSR(*m, &localFull.normal[i], &worldFull.normal[i]);
        PSMTXMultVecSR(*m, &local.normal[i], &world.normal[i]);
    }
    for (i = 0; i < 8; i++) {
        PSMTXMultVec(*m, &localFull.point[i], &worldFull.point[i]);
        PSMTXMultVec(*m, &local.point[i], &world.point[i]);
    }
    sphereWorld = sphere;
    PSMTXMultVec(*m, &sphere.center, &sphereWorld.center);
}
