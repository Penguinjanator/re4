#include "types.h"
#include "vec.h"
#include "global.h"
#include "math_sub.h"
#include "db_log.h"
#include "view.h"

#define VIEW_ASPECT 1.33333333f

// COMPILER-DIFF: 3 -- the original PREs exactly one r31-based address of the full-frustum normal
// groups (`&point[4]`, G1 -> G2) and recomputes every other one; our gcse PREs all of them. The
// shared second operand of each group and the else-arm normalize pointer are emitted by an asm
// `addi` (an ASM_OPERANDS with a distinct dummy operand per site: not the same gcse expression),
// and `b` is laundered before G2 (a transparency kill) so `&point[3]` is not carried into G4 and the
// G1 `&point[4]` copy is the only PRE. Applied for structure; the function is not byte-identical yet.
#define VADDR(dst, base, off, tag) asm("addi %0,%1,%2" : "=r"(dst) : "r"(base), "i"(off), "i"(tag))
#define VECNormalizeQ(src, dst, b, off, tag)                                                \
    if (0.0f == (src)->x && 0.0f == (src)->y && 0.0f == (src)->z) {                    \
        pLog->err(0, 0, "VECNormalize:[%s/%d]", __FILE__, __LINE__);                    \
        (dst)->x = (dst)->y = (dst)->z = 0.0f;                                          \
    } else {                                                                            \
        Vec* q;                                                                         \
        VADDR(q, b, off, tag);                                                          \
        PSVECNormalize(q, q);                                                           \
    }

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
// (c set before the copy loop so cse cannot fold `c + K` to `this + K` after it); block 2 addresses
// `&c->point[k]`/`&c->normal[k]` inline and the halving loop as plain `c->point[i].x *= 0.5f;
// c->point[i].y *= 0.5f;` -- gcse's block LCM never delays an insertion through the halving loop's
// back edge, so every address that first occurs after the loop is redundant there and is inserted
// at the end of the loop's preheader (the 14 `addi rX,r31,K` before `mtctr`), and the loop's own
// hoisted `&c->point[0].x` becomes a copy of the PRE'd `&c->point[0]` (`mr r10,r30`); no pointer
// locals. `b = &localFull` re-assigned for the sphere block; q copies field-wise (a struct copy
// forces `&b->point[k]` into a pseudo that cse folds to `this + K` and gcse then hoists).
// Frustum point stores: one `z` variable holds -zn and then -zf (a two-set pseudo, allocated f11 in both
// blocks), `w` is likewise shared, the far block has its own `h2` (block-local, tied to the dying `t` in
// f31); each point is stored z, x, y (the far block's `lfs zf` depends on all twelve near stores, so the
// store order inside a block is the sched1 order: the dying store first, then source order).
void VIEW::initPerspective(f32 fovy_, f32 aspect_, f32 znear_, f32 zfar_)
{
    Vec t1;
    Vec t2;
    Vec t3;
    Vec q[4];
    ViewFrustum* b;
    Vec* pb;
    Vec* pk;
    ViewFrustum* c;
    f32 t;
    f32 h;
    f32 w;
    f32 zn;
    f32 zf;
    f32 det;
    f32 z;
    f32 h2;
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
    z = -zn;
    h = zn * t;
    w = h * aspect_;
    b->point[0].z = z;
    b->point[0].x = w;
    b->point[0].y = h;
    b->point[1].z = z;
    b->point[1].x = -w;
    b->point[1].y = h;
    b->point[2].z = z;
    b->point[2].x = -w;
    b->point[2].y = -h;
    b->point[3].z = z;
    b->point[3].x = w;
    b->point[3].y = -h;
    zf = zfar;
    h2 = zf * t;
    z = -zf;
    w = h2 * aspect_;
    b->point[4].z = z;
    b->point[4].x = w;
    b->point[4].y = h2;
    b->point[5].z = z;
    b->point[5].x = -w;
    b->point[5].y = h2;
    b->point[6].z = z;
    b->point[6].x = -w;
    b->point[6].y = -h2;
    b->point[7].z = z;
    b->point[7].x = w;
    b->point[7].y = -h2;

#line 190 "D:/Bio4/Prog/view.cpp"
    PSVECSubtract(&b->point[1], &b->point[0], &t1);
    PSVECSubtract(&b->point[3], &b->point[0], &t2);
    PSVECCrossProduct(&t1, &t2, &b->normal[0]);
    VECNormalize(&b->normal[0], &b->normal[0]);

    VADDR(pb, b, 72 + 12 * 0, 1);
    PSVECSubtract(&b->point[3], pb, &t1);
    pk = &b->point[4]; // carried to G2 (the original's only shared address; it copies it, `mr r23,r29`)
    PSVECSubtract(pk, pb, &t2);
    PSVECCrossProduct(&t1, &t2, &b->normal[1]);
#line 198 "D:/Bio4/Prog/view.cpp"
    VECNormalizeQ(&b->normal[1], &b->normal[1], b, 12, 11);

    asm("" : "+r"(b)); // COMPILER-DIFF: 3 (transparency kill)
    VADDR(pb, b, 72 + 12 * 0, 2);
    PSVECSubtract(pk, pb, &t1);
    PSVECSubtract(&b->point[1], pb, &t2);
    PSVECCrossProduct(&t1, &t2, &b->normal[2]);
#line 203 "D:/Bio4/Prog/view.cpp"
    VECNormalizeQ(&b->normal[2], &b->normal[2], b, 24, 12);

    VADDR(pb, b, 72 + 12 * 1, 3);
    {
        Vec* pa; // COMPILER-DIFF: 3 (`&point[5]` recurs in G5; a plain address is PRE'd G3 -> G5)
        VADDR(pa, b, 72 + 12 * 5, 6);
        PSVECSubtract(pa, pb, &t1);
    }
    PSVECSubtract(&b->point[2], pb, &t2);
    PSVECCrossProduct(&t1, &t2, &b->normal[3]);
#line 208 "D:/Bio4/Prog/view.cpp"
    VECNormalizeQ(&b->normal[3], &b->normal[3], b, 36, 13);

    VADDR(pb, b, 72 + 12 * 2, 4);
    PSVECSubtract(&b->point[6], pb, &t1);
    PSVECSubtract(&b->point[3], pb, &t2);
    PSVECCrossProduct(&t1, &t2, &b->normal[4]);
#line 213 "D:/Bio4/Prog/view.cpp"
    VECNormalizeQ(&b->normal[4], &b->normal[4], b, 48, 14);

    VADDR(pb, b, 72 + 12 * 4, 5);
    PSVECSubtract(&b->point[7], pb, &t1);
    PSVECSubtract(&b->point[5], pb, &t2);
    PSVECCrossProduct(&t1, &t2, &b->normal[5]);
#line 218 "D:/Bio4/Prog/view.cpp"
    VECNormalizeQ(&b->normal[5], &b->normal[5], b, 60, 15);

    c = &local;
    *c = localFull;
    for (i = 0; i < 8; i++) {
        c->point[i].x *= 0.5f;
        c->point[i].y *= 0.5f;
    }
#line 236 "D:/Bio4/Prog/view.cpp"
    PSVECSubtract(&c->point[1], &c->point[0], &t1);
    PSVECSubtract(&c->point[3], &c->point[0], &t3);
    PSVECCrossProduct(&t1, &t3, &c->normal[0]);
    VECNormalize(&c->normal[0], &c->normal[0]);

    PSVECSubtract(&c->point[3], &c->point[0], &t1);
    PSVECSubtract(&c->point[4], &c->point[0], &t3);
    PSVECCrossProduct(&t1, &t3, &c->normal[1]);
    VECNormalize(&c->normal[1], &c->normal[1]);

    PSVECSubtract(&c->point[4], &c->point[0], &t1);
    PSVECSubtract(&c->point[1], &c->point[0], &t3);
    PSVECCrossProduct(&t1, &t3, &c->normal[2]);
    VECNormalize(&c->normal[2], &c->normal[2]);

    PSVECSubtract(&c->point[5], &c->point[1], &t1);
    PSVECSubtract(&c->point[2], &c->point[1], &t3);
    PSVECCrossProduct(&t1, &t3, &c->normal[3]);
    VECNormalize(&c->normal[3], &c->normal[3]);

    PSVECSubtract(&c->point[6], &c->point[2], &t1);
    PSVECSubtract(&c->point[3], &c->point[2], &t3);
    PSVECCrossProduct(&t1, &t3, &c->normal[4]);
    VECNormalize(&c->normal[4], &c->normal[4]);

    PSVECSubtract(&c->point[7], &c->point[4], &t1);
    PSVECSubtract(&c->point[5], &c->point[4], &t3);
    PSVECCrossProduct(&t1, &t3, &c->normal[5]);
    VECNormalize(&c->normal[5], &c->normal[5]);

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
