#include "types.h"
#include "vec.h"
#include "global.h"
#include "camera.h"
#include "quake.h"

extern "C" void* memset(void* dst, int c, unsigned int n);

QuakeWork Quake;
static Vec QuakeOfsOld[2];

void QuakeMove()
{
    QuakeScheduler();
    if (Quake.active) {
        QuakeMain();
    }
}

void QuakeInit()
{
    int i;
    QuakeEntry* e = Quake.ent;

    for (i = 0; i < 16; i++, e++) {
        e->Be_flg = 0;
        e->No = 0;
        e->Delay = 0;
        e->Time = 0;
        e->Scale = 0.0f;
        e->Axis = 0;
    }
    Quake.rnd_idx = 0;
}

void QuakeExec(u8 id, u16 delay, s16 time, f32 power, u8 axis)
{
    int i;
    QuakeEntry* e = Quake.ent;

    for (i = 0; i < 16; i++, e++) {
        if (!(e->Be_flg & 1)) {
            e->Be_flg = 1;
            e->No = id;
            e->Delay = delay;
            e->Time = time;
            e->Scale = power;
            e->Axis = axis;
            break;
        }
    }
}

static void QuakeKill(u8 id)
{
    int i;
    QuakeEntry* e = Quake.ent;

    for (i = 0; i < 16; i++, e++) {
        if ((e->Be_flg & 1) && e->No == id) {
            e->Be_flg = 0;
            e->No = 0;
            e->Delay = 0;
            e->Time = 0;
            e->Scale = 0.0f;
        }
    }
}

void QuakeScheduler()
{
    int i;
    QuakeEntry* e;

    Quake.active = 0;
    Quake.axis = 0;
    Quake.power = 0.0f;
    e = Quake.ent;
    for (i = 0; i < 16; i++, e++) {
        if (e->Be_flg & 1) {
            if (e->Delay != 0) {
                e->Delay--;
            } else if (e->Time == 0) {
                e->Be_flg = 0;
                e->No = 0;
                e->Delay = 0;
                e->Time = 0;
                e->Scale = 0.0f;
                e->Axis = 0;
            } else {
                Quake.active = 1;
                if (Quake.power < e->Scale) {
                    Quake.power = e->Scale;
                    Quake.axis |= e->Axis;
                }
                e->Time--;
            }
        }
    }
}

void QuakeMain()
{
    static s8 rnd_tbl[16] = {0, -1, 1, 2, -1, 0, 1, -1, 1, -1, 0, 1, -1, -2, 0, 1};
    GlobalWork* g = pG;
    Camera* cam = &g->Cam;
    Vec ofs = {0.0f, 0.0f, 0.0f};

    if (Quake.axis & 1) {
        Quake.rnd_idx = (Quake.rnd_idx + 1) & 0xF;
        ofs.x = (f32) rnd_tbl[Quake.rnd_idx] * Quake.power;
    }
    if (Quake.axis & 2) {
        Quake.rnd_idx = (Quake.rnd_idx + 1) & 0xF;
        ofs.y = (f32) rnd_tbl[Quake.rnd_idx] * Quake.power;
    }
    if (Quake.axis & 4) {
        Quake.rnd_idx = (Quake.rnd_idx + 1) & 0xF;
        ofs.z = (f32) rnd_tbl[Quake.rnd_idx] * Quake.power;
    }
    PSMTXMultVecSR(cam->mat, &ofs, &ofs);
    PSVECAdd(&g->Cam.param.pos, &ofs, &g->Cam.param.pos);
    PSVECAdd(&g->Cam.param.at, &ofs, &g->Cam.param.at);
    CameraSetOrientationUp(cam);
}
