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

// initPerspective (551 words, was 611): the two normal blocks are written out (VECNormalize's
// __LINE__ is 193/198/.../218 and 239/.../264 in the original, 5 lines per normal). OPEN: the
// original computes -znear/-h/-w once each and stores them through chains (4/2/2 stores from one
// register), keeps &normal[k]/&point[k] in callee-saved r14-r27 across the second normal block,
// halves the points with an indexed `lfsx/stfsx` loop, and its sphere block builds a `Vec* p[4]`
// table on the stack (frame+56/80/92 and &localFull) -- a different algorithm from the det/d0..d2
// form below (its pool has 1.0, 2pi, 12.0, 1/1024, pi/2 and both double-conversion magics that
// ours lacks: .rodata 0x80 vs 0x48).
void VIEW::initPerspective(f32 fovy_, f32 aspect_, f32 znear_, f32 zfar_)
{
    Vec t1;
    Vec t2;
    Vec q[4];
    ViewFrustum* b;
    f32 t;
    f32 h;
    f32 w;
    int i;

    fovy = fovy_;
    aspect = aspect_;
    znear = znear_;
    zfar = zfar_;
    t = sinf(fovy * 0.5f * 3.1415927f / 180.0f) / cosf(fovy * 0.5f * 3.1415927f / 180.0f);
    b = &localFull;
    h = znear * t;
    w = h * aspect;
    b->point[0].x = w;
    b->point[0].y = h;
    b->point[0].z = -znear;
    b->point[1].x = -w;
    b->point[1].y = h;
    b->point[1].z = -znear;
    b->point[2].x = -w;
    b->point[2].y = -h;
    b->point[2].z = -znear;
    b->point[3].x = w;
    b->point[3].y = -h;
    b->point[3].z = -znear;
    h = zfar * t;
    w = h * aspect;
    b->point[4].x = w;
    b->point[4].y = h;
    b->point[4].z = -zfar;
    b->point[5].x = -w;
    b->point[5].y = h;
    b->point[5].z = -zfar;
    b->point[6].x = -w;
    b->point[6].y = -h;
    b->point[6].z = -zfar;
    b->point[7].x = w;
    b->point[7].y = -h;
    b->point[7].z = -zfar;

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

    local = localFull;
    b = &local;
    for (i = 0; i < 8; i++) {
        b->point[i].x *= 0.5f;
        b->point[i].y *= 0.5f;
    }
#line 236 "D:/Bio4/Prog/view.cpp"
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

    orientation();

    b = &localFull;
    q[0] = b->point[0];
    q[1] = b->point[4];
    q[2] = b->point[5];
    q[3] = b->point[6];
    {
        f32 ax = q[1].x - q[0].x, ay = q[1].y - q[0].y, az = q[1].z - q[0].z;
        f32 bx = q[2].x - q[1].x, by = q[2].y - q[1].y, bz = q[2].z - q[1].z;
        f32 cx = q[3].x - q[2].x, cy = q[3].y - q[2].y, cz = q[3].z - q[2].z;
        f32 det = ax * by * cz + bx * cy * az + cx * ay * bz - ax * cy * bz - bx * ay * cz - cx * by * az;
        f32 d0 = PSVECSquareMag(&q[0]) - PSVECSquareMag(&q[1]);
        f32 d1 = PSVECSquareMag(&q[1]) - PSVECSquareMag(&q[2]);
        f32 d2 = PSVECSquareMag(&q[2]) - PSVECSquareMag(&q[3]);
        det = det + det;
        sphere.center.x = (d0 * (by * cz - cy * bz) + d1 * (cy * az - ay * cz) + d2 * (ay * bz - by * az)) / det;
        sphere.center.y = (d0 * (cx * bz - bx * cz) + d1 * (ax * cz - cx * az) + d2 * (bx * az - ax * bz)) / det;
        sphere.center.z = (d0 * (bx * cy - cx * by) + d1 * (cx * ay - ax * cy) + d2 * (ax * by - bx * ay)) / det;
    }
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
