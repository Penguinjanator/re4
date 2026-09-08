#include "atari.h"
#include "light.h"
#include "esp.h"
#include "espgen.h"
#include "math_sub.h"
#include "db_log.h"

extern "C" {
void espgen10_Update(EspgenWork* w);
void espgen10_Move00(EspgenWork* w);
void espgen10_Move01(EspgenWork* w);
}

// Effect controller 10: plays an effect sequence (EspSeqData) record by record.
struct Espgen10Work {
    EspSeqData* head;  // 0x14
    cModel* model;     // 0x18
    u32 serial;        // 0x1C model serial the controller was set up with
    u16 cnt;           // 0x20 frame counter
    u8 no;             // 0x22 next record
    u8 flags;          // 0x23 bit0: parts matrix fixed, bit1: pass the rotation on
    u16 parts;         // 0x24 parts number (0xFE: free position, 0xFF: none)
    u8 pad_26[2];
    u32 seed;          // 0x28
    Mtx mtx;           // 0x2C
    Vec pos;           // 0x5C
    Vec rot;           // 0x68
    u8 pad_74[0x90 - 0x74];
    EspSeqOpt* p8;          // 0x90
};

int EspgenDataSet(EspSeqData* head, int no, EspInfo* info, u32* seed, cModel* model, u16 parts, Mtx* mtx, Vec* pos,
                  Vec* rot, EspSeqOpt* p8, int flag)
{
    EspGenWork* rec = &head->rec[no];
    int ret = 1;

    if (info->x0 & 0x1000) {
        cModel** list = EspEvModList;
        u32 no = rec->x6;
        if (no > 0x7F) {
            model = NULL;
        } else {
            model = list[no];
        }
    }

    switch (rec->type) {
    case 0: {
        cEsp* esp;
        if (flag == 0) {
            pos = NULL;
        }
        if (EspSeqSet(rec, info, seed, model, mtx, 0, &esp, p8, pos, 0.0f)) {
            goto ok;
        }
        break;
    }
    case 1:
        if (EspgenSeqSet(head, no, info, model, parts, mtx, pos, rot, p8, flag)) {
            goto ok;
        }
        break;
    default:
        pLog->err(0, 0, "ESP_CTRL : KIND[%d] is invalid.", rec->type);
        break;
    }
    ret = 0;
ok:
    return ret;
}

void SetEspCore(EspgenWork* w, u16 a, u32 b, u8 c, u32 d, u8 e)
{
    w->info.x0 = a;
    w->info.x2 = c;
    w->info.x4 = b;
    w->info.x8 = d;
    w->info.x3 = e;
}

int PullEspEspgen(EspgenWork** out, u16 a, int c, u32 b, u32 d, u8 e, int front)
{
    int ret;

    if (front == 1) {
        ret = PullEspgenFront(out);
    } else {
        ret = PullEspgen(out);
    }
    if (ret) {
        SetEspCore(*out, a, b, c, d, e);
    }
    return ret;
}

void espgen10_Update(EspgenWork* w)
{
    Espgen10Work* p = (Espgen10Work*) w->work;
    EspSeqData* head = p->head;
    EspGenWork* rec = &head->rec[p->no];
    cModel* model = p->model;

    if (model != NULL) {
        if ((model->be_flag & 0x201) != 1 || model->serial != p->serial) {
            PushEspgen(w);
            return;
        }
    }
    if ((p->parts >= 0xF8 && p->parts <= 0xFD) || p->parts == 0xFF) {
        pLog->err(0, 0, "ESP_CTRL10 : PARTS_NO[%x] invalid.", p->parts);
        PushEspgen(w);
        return;
    }
    if (p->parts == 0xFE) {
        PSMTXIdentity(p->mtx);
        RotMatrix(p->mtx, &p->rot);
        p->mtx[0][3] = p->pos.x;
        p->mtx[1][3] = p->pos.y;
        p->mtx[2][3] = p->pos.z;
    } else {
        if (p->model == NULL) {
            pLog->err(0, 0, "ESP_CTRL10 : PARTS_NO is set but No Parent.");
            PushEspgen(w);
            return;
        }
        if (!(p->flags & 1)) {
            cModel* part;
            Vec ofs;
            Vec r;

            if (p->parts >= model->nParts) {
                pLog->err(0, 0, "ESP_CTRL10 : PARTS_NO[%d] is invalid(MAX:%d).", p->parts, model->nParts);
                PushEspgen(w);
                return;
            }
            part = model->getPartsPtr(p->parts);
            PSMTXIdentity(p->mtx);
            PSVECAdd(&p->rot, &model->rot, &r);
            RotMatrix(p->mtx, &r);
            PSMTXMultVecSR(p->mtx, &p->pos, &ofs);
            p->mtx[0][3] = part->mat[0][3] + ofs.x;
            p->mtx[1][3] = part->mat[1][3] + ofs.y;
            p->mtx[2][3] = part->mat[2][3] + ofs.z;
            if (!(head->flags & 1)) {
                p->flags |= 1;
            }
        }
    }
    if (rec->x4 < p->cnt) {
        pLog->err(0, 0, "ESP_ESTSET : DATA[%d] is no SORT.", p->no);
        PushEspgen(w);
        return;
    }
    while (rec->x4 == p->cnt) {
        int flag = 0;
        if (p->flags & 2) {
            flag = 1;
        }
        if (!EspgenDataSet(head, p->no, &w->info, &p->seed, p->model, p->parts, &p->mtx, &p->pos, &p->rot, p->p8,
                           flag)) {
            return;
        }
        p->no++;
        rec++;
        if (p->no >= head->num) {
            PushEspgen(w);
            break;
        }
    }
    p->cnt++;
}

void espgen10_Move00(EspgenWork* w)
{
    espgen10_Update(w);
    w->step = 1;
}

void espgen10_Move01(EspgenWork* w)
{
    espgen10_Update(w);
}

void Espgen10_Move(EspgenWork* w)
{
    static void (*Espgen10MoveTbl[])(EspgenWork*) = {espgen10_Move00, espgen10_Move01};

    Espgen10MoveTbl[w->step](w);
}
