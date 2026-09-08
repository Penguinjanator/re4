#include "atari.h"
#include "light.h"
#include "math_sub.h"
#include "rnd.h"
#include "esp.h"
#include "cloth.h"

// Cloth sheet: a Cloth grid attached to the effect position, waving with a sine field.
class cEsp4e : public cEsp {
public:
    Cloth* cloth;      // 0xF8
    GXTexObj tex;      // 0xFC
    GXTlutObj tlut;    // 0x11C
    f32 angX;          // 0x128 wave phase along x
    f32 angY;          // 0x12C wave phase along y
    u8 waveSpdX;       // 0x130
    u8 ampX;           // 0x131
    u8 waveSpdY;       // 0x132
    u8 ampY;           // 0x133
    f32 freqX;         // 0x134
    f32 freqY;         // 0x138
    f32 power;         // 0x13C
    f32 ang;           // 0x140 global phase
    f32 angSpd;        // 0x144
    f32 powerRate;     // 0x148
    f32 rnd;           // 0x14C random factor

    virtual ~cEsp4e();
    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
    virtual void Destruct();
};

extern "C" {
cEsp* Esp4e_Create();
void Esp4e_Trans();
}

cEsp* Esp4e_Create()
{
    return new cEsp4e;
}

void cEsp4e::move()
{
    Vec pos0 = pos;
    Vec sp;
    Cloth* c;
    f32 base;
    f32 rand;
    f32 stepY;
    f32 stepX;
    f32 wx;
    f32 wy;
    f32 ay;
    f32 ax;
    f32 s;
    u32 i;
    u32 j;

    if (!CommonMove()) {
        return;
    }
    pos = pos0;
    c = cloth;
    if (c == NULL) {
        return;
    }
    if (parent != pEffParentWorld) {
        Vec p;
        Vec r;
        Mtx m;
        PSMTXMultVec(parent->mat, &pos, &p);
        low_RotMatrix(m, &rot);
        PSMTXConcat(parent->mat, m, m);
        Matrix2AxisAngle(m, &r);
        c->SetPosAng(r, p);
    }
    {
        Mtx m;
        PSVECScale(&spd, &sp, 0.1f);
        RotMatrix(m, &rot);
        PSMTXInverse(m, m);
        PSMTXMultVec(m, &sp, &sp);
    }
    c->colR = (u8) colR;
    c->colG = (u8) colG;
    c->colB = (u8) colB;
    c->colA = (u8) colA;
    {
        static f32 DAMPING = 0.98f;
        c->calcSpeed(DAMPING);
    }
    c->move();
    c->calcNormal();

    ang += angSpd;
    rand = rnd;
    ang = LIMIT_ANGLE(ang);
    s = SINF(ang);
    s = SINF(ang * 0.3f) * (powerRate * s) + 1.0f;
    angX = (f32) waveSpdX * 0.01f + angX;
    angY = (f32) waveSpdY * 0.01f + angY;
    stepY = freqX / c->ny;
    stepX = freqY / c->nx;
    wx = (f32) waveSpdY * 0.025f;
    wy = (f32) ampY * 0.025f;
    PSVECScale(&sp, &sp, s);
    ay = angX;
    angX = LIMIT_ANGLE(angX);
    angY = LIMIT_ANGLE(angY);

    for (i = 0; i < c->ny; i++) {
        base = SINF(ay) * wx;
        ay += stepY * rand * fRand0_1() + stepY;
        ax = angY;
        for (j = 0; j < c->nx; j++) {
            f32 v = SINF(ax) * wy;
            f32 st = stepX * rand * fRand0_1() + stepX;
            v = base + v;
            c->disturbance(v * s + power * s, j, i);
            ax += st;
            PSVECAdd(&c->spd[j + c->nx * i], &sp, &c->spd[j + c->nx * i]);
        }
    }
}

void cEsp4e::Destruct()
{
    if (cloth) {
        cloth->Destroy();
    }
}

void Esp4e_Trans()
{
}

int cEsp4e::SetFreeWork(EspGenWork* gen, u32* seed)
{
    void* tpl;
    int ci;
    int nx;
    int ny;
    f32 w;
    f32 h;
    f32 d;
    int flag;

    if (!EspGetTplAddr(anmNo, &tpl)) {
        pLog->err(0, 0, "ESP4e : tex init invalid.");
        return 0;
    }
    if (!PullCloth(&cloth)) {
        pLog->err(0, 0, "ESP4e : init invalid.");
        return 0;
    }
    d = 60.857143f;
    ci = ClothTexSetUp(tpl, &tex, 0, &tlut);
    nx = (int) (sizeX / 200.0f * 36.0f);
    ny = (int) (sizeY / 200.0f * 24.0f);
    w = gen->xD8 * 0.1f + 1.0f;
    h = gen->xDC * 0.1f + 1.0f;
    if (w == 0.0f) {
        w = 0.001f;
    }
    if (h == 0.0f) {
        h = 0.001f;
    }
    flag = (gen->flags & 1) != 0;
    if (nx < 2) {
        nx = 2;
    }
    if (ny < 2) {
        ny = 2;
    }
    if (nx > 100) {
        nx = 100;
    }
    if (ny > 50) {
        ny = 50;
    }
    if (ci) {
        cloth->Set(rot, pos, nx, ny, w, &tex, h * (3000.0f / d / 23.0f), NULL, d, &tlut, flag);
    } else {
        cloth->Set(rot, pos, nx, ny, w, &tex, h * (3000.0f / d / 23.0f), NULL, d, NULL, flag);
    }
    if (gen->xC2) {
        cloth->x74 = 1;
    }
    waveSpdX = gen->xC8;
    waveSpdY = gen->xC9;
    ampX = gen->xCA;
    ampY = gen->xCB;
    freqX = (f32) (int) (gen->prm.w.xCC + 1) * 0.5f;
    freqY = (f32) (int) (gen->prm.w.xD0 + 1) * 0.5f;
    power = (f32) (int) gen->xD4 * 0.025f;
    angSpd = (f32) (gen->xFC + 1) * 0.0025f;
    powerRate = (f32) (gen->xFD + 1) * 0.07f;
    rnd = (f32) (gen->xFE + 1) * 0.2f;
    return 1;
}

cEsp4e::~cEsp4e()
{
}
