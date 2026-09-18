#include "atari.h"
#include "global.h"
#include "math_sub.h"
#include "esp.h"

struct Esp07Work {
    Vec RefRate;    // 0x00 x: horizontal damping, y: vertical damping (gen->xD8.. * 0.1)
    u8 GndEstOwner;      // 0x0C est on floor hit (gen->xC8)
    u8 GndEstNo;     // 0x0D (gen->xC9)
    u8 WallEstOwner;     // 0x0E est on wall hit (gen->xCA)
    u8 WallEstNo;    // 0x0F (gen->xCB)
    u32 HitType;   // 0x10 0: floor (cached), 1: floor, 2: wall (gen->xFC)
    u32 EstCall;   // 0x14 0: bounce, 1: est + die, 2: est + bounce, 3: die (gen->xFD)
    u32 Flg;     // 0x18 bit0: stopped, bit1: floor height cached
    f32 GndHeight;    // 0x1C
    u8 SeType;     // 0x20 (gen->xFE)
};

// Bouncing particle: checks the floor (or walls) every frame, bounces / spawns an est / dies.
class cEsp07 : public cEsp {
public:
    Esp07Work m_Free;  // 0xF8

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
};

extern "C" {
void Esp07_ChkGnd(cEsp07* esp, f32 floorY);
void Esp07_HitGndLight(cEsp07* esp);
void Esp07_HitGnd(cEsp07* esp);
void Esp07_HitWall(cEsp07* esp);
}

cEsp* Esp07_Create()
{
    return new cEsp07;
}

void Esp07_ChkGnd(cEsp07* esp, f32 floorY)
{
    Esp07Work* w = &esp->m_Free;
    f32 half;

    half = esp->m_Size_base_y * 0.5f * esp->m_Size_mul;
    if ((pG->flags_64 & 0x00800000) && !(pG->flags_60 & 0x00010000)) {
        floorY = 0.0f;
    }
    if (esp->m_Pos.y - half < floorY) {
        esp->m_Pos.y = floorY + half;
        if (w->SeType != 0) {
            Vec d;
            f32 dist;

            PSVECSubtract(&pG->Cam.param.pos, &esp->m_Pos, &d);
            dist = PSVECMag(&d);
            if (w->SeType == 3 || dist < 8000.0f) {
                EspCallSeType(w->SeType, &esp->m_Pos);
            }
        }
        if (w->EstCall == 1 || w->EstCall == 2) {
            Vec rot;
            Vec p;

            if (esp->m_Speed.x == 0.0f && esp->m_Speed.z == 0.0f) {
                rot.x = rot.y = rot.z = 0.0f;
            } else {
                rot.x = rot.z = 0.0f;
                rot.y = atan2f(esp->m_Speed.x, esp->m_Speed.z);
            }
            p = esp->m_Pos;
            p.y = floorY + 65.0f;
            EstSet(0, -1, &p, &rot, w->GndEstOwner, w->GndEstNo, esp->info.Core_flg, esp->info.Core_kind, esp->info.Core_pEm, NULL);
            if (w->EstCall != 2) {
                PushEsp(esp);
                return;
            }
        }
        if (w->EstCall == 3) {
            PushEsp(esp);
            return;
        }
        esp->m_Speed.x = esp->m_Speed.x * w->RefRate.x;
        esp->m_Speed.y = esp->m_Speed.y * -w->RefRate.y;
        esp->m_Speed.z = esp->m_Speed.z * w->RefRate.x;
        if (PSVECMag(&esp->m_Speed) < 10.0f) {
            w->Flg = 1;
            PSVECScale(&esp->m_Speed, &esp->m_Speed, 0.0f);
            PSVECScale(&esp->m_Speed_plus, &esp->m_Speed_plus, 0.0f);
        }
    }
}

void Esp07_HitGndLight(cEsp07* esp)
{
    Esp07Work* w = &esp->m_Free;
    u32 attr;

    if (!(w->Flg & 2)) {
        w->Flg |= 2;
        w->GndHeight = SatMgr.getFloor(&esp->m_Pos, 600.0f, 100000.0f, &attr, 0);
    }
    Esp07_ChkGnd(esp, w->GndHeight);
}

void Esp07_HitGnd(cEsp07* esp)
{
    u32 attr;

    Esp07_ChkGnd(esp, SatMgr.getFloor(&esp->m_Pos, 600.0f, 100000.0f, &attr, 0));
}

void Esp07_HitWall(cEsp07* esp)
{
    Esp07Work* w = &esp->m_Free;
    Vec refl;
    Vec hit;
    Vec n2;
    Vec next;
    Vec nrm;
    Vec rot;
    f32 mag;

    PSVECAdd(&esp->m_Pos, &esp->m_Speed, &next);
    if (SatMgr.hitCheck(&esp->m_Pos, &next, &hit, &nrm, 0, 0)) {
        esp->m_Pos = hit;
        PSVECAdd(&nrm, &esp->m_Pos, &esp->m_Pos);
        if (w->SeType != 0) {
            EspCallSeType(w->SeType, &esp->m_Pos);
        }
        if (w->EstCall == 1 || w->EstCall == 2) {
            if (esp->m_Speed.x == 0.0f && esp->m_Speed.z == 0.0f) {
                rot.x = rot.y = rot.z = 0.0f;
            } else {
                rot.x = rot.z = 0.0f;
                rot.y = atan2f(esp->m_Speed.x, esp->m_Speed.z);
            }
            if (nrm.y > 0.98f) {
                EstSet(0, -1, &esp->m_Pos, &rot, w->GndEstOwner, w->GndEstNo, esp->info.Core_flg, esp->info.Core_kind, esp->info.Core_pEm, NULL);
            } else {
                EstSet(0, -1, &esp->m_Pos, &rot, w->WallEstOwner, w->WallEstNo, esp->info.Core_flg, esp->info.Core_kind, esp->info.Core_pEm, NULL);
            }
            if (w->EstCall != 2) {
                PushEsp(esp);
                return;
            }
        }
        if (w->EstCall == 3) {
            PushEsp(esp);
            return;
        }
        mag = RootSumSquare3(&esp->m_Speed);
        n2.x = -nrm.x;
        n2.y = -nrm.y;
        n2.z = -nrm.z;
        C_VECReflect(&esp->m_Speed, &n2, &refl);
        PSVECScale(&refl, &esp->m_Speed, mag * w->RefRate.y);
        PSVECScale(&esp->m_Ang_plus, &esp->m_Ang_plus, -0.8f);
    }
}

void cEsp07::move()
{
    Esp07Work* w = &m_Free;

    if (CommonMove()) {
        if (!(w->Flg & 1)) {
            switch (w->HitType) {
            case 0:
                Esp07_HitGndLight(this);
                break;
            case 1:
                Esp07_HitGnd(this);
                break;
            case 2:
                Esp07_HitWall(this);
                break;
            default:
                pLog->err(0, 0, "ESP_07 : HitType[%x] is invalid.", w->HitType);
                PushEsp(this);
                return;
            }
        }
        if (!AnmMove()) {
            PushEsp(this);
        }
    }
}

int cEsp07::SetFreeWork(EspGenWork* gen, u32* seed)
{
    Esp07Work* w = &m_Free;

    w->RefRate = *(Vec*)&gen->xD8;
    PSVECScale(&w->RefRate, &w->RefRate, 0.1f);
    w->GndEstOwner = gen->xC8;
    w->GndEstNo = gen->xC9;
    w->WallEstOwner = gen->xCA;
    w->WallEstNo = gen->xCB;
    w->HitType = gen->xFC;
    w->EstCall = gen->xFD;
    w->SeType = gen->xFE;
    if (w->HitType > 2) {
        pLog->err(0, 0, "ESP_07 : HitType[%x] is invalid.", w->HitType);
        return 0;
    }
    if (w->EstCall > 3) {
        pLog->err(0, 0, "ESP_07 : EstCall[%x] is invalid.", w->EstCall);
        return 0;
    }
    if (w->SeType > 3) {
        pLog->err(0, 0, "ESP_07 : SeType[%d] is invalid.", w->SeType);
        return 0;
    }
    if (m_Life_max == 0) {
        m_Life_max = 0x80;
    }
    return 1;
}
