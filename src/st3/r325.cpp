#include "types.h"
#include "main_mem.h"
#include "st_room.h"
#include "atari.h"
#include "event.h"
#include "flag_rsf.h"
#include "global.h"
#include "sce.h"
#include "sce_at.h"
#include "obj.h"
#include "model.h"
#include "etc_model.h"
#include "est.h"
#include "esp.h"
#include "TexRender.h"

// Room 3-25 (D:/Bio4/Prog/r325.cpp): the s00 event (Ashley taken away) with its two render-to-texture
// models, the red-eye texture swap and the Leon jacket toggles.

struct R325Work {
    TexRenderMng* tex[2];   // 0x000  render targets of parts 6 / 7
    u8 texTbl0[0x80];       // 0x008  blend table of tex[0]
    u8 texTbl1[0x80];       // 0x088  blend table of tex[1]
    void* tpl;              // 0x108  ev0002's original texture palette
};

static R325Work* r325_work;

// The original reads r4 although its prototype has one parameter (r40e).
void TexRenderModResP(cModel* m, int parts) asm("TexRenderModRes");

void R325EventS00();
extern "C" void Evt_R325S00_Func(Event* e);

void R325Init()
{
#line 35 "D:/Bio4/Prog/r325.cpp"
    r325_work = (R325Work*) MEM_CALLOC(sizeof(R325Work), 1, 0xd);
    EvtMgr.SetFunc("evt_r325s00_func", (void*) Evt_R325S00_Func);
    EvtMgr.SetFunc("evt_r325s99_func", (void*) Evt_R325S00_Func);
    if (RsfCheck(G_ROOM_ID, 0) == 0) {
        SceAtDataSet_exec(3, 0x12, 0, (TaskFunc) R325EventS00, 0, 1);
        EvtMgr.EvtReadAram("event/evd/r325s00.evd", 0, 0, 0, 0);
    }
    TexRenderInit(&r325_work->tex[0], 0xE0, 2);
    TexRenderInit(&r325_work->tex[1], 0xE0, 2);
}

void R325Main()
{
}

void R325EventS00()
{
    if (RsfCheck(G_ROOM_ID, 0) == 0) {
        RsfSet(G_ROOM_ID, 0);
        SceAtSetEnable(3, 0);
        EvtMgr.EvtReadExec("event/evd/r325s00.evd", 0, 0);
    }
}

extern "C" void Evt_R325S00_Func(Event* e)
{
    void* mod;
    void* info;
    void* bin;

    switch (e->funcMode) {
    case 0:
        setRoomEtcDisp(0, 0, 1);
        pG->Room_flg[0] &= ~0x80000000;
        break;
    case 1:
        if (e->NowCut == 7) {
            if (e->NowFrame == 0) {
                if (e->GetMod(&mod, "pl0000", 0, 0) == 1) {
                    ((cObj*) mod)->o18.be_flag |= 0x40;
                }
            }
        } else {
            if (e->NowFrame == 0) {
                if (e->GetMod(&mod, "pl0000", 0, 0) == 1) {
                    ((cObj*) mod)->o18.be_flag &= ~0x40;
                }
            }
        }
        if (e->NowCut == 4) {
            if (e->NowFrame == 0) {
                if (e->GetMod(&mod, "pl0000", 0, 0) == 1) {
                    TexRenderModSet((cModel*) mod, 6, r325_work->texTbl0, r325_work->tex[0], 0, 0, 1, 1, 1.0f);
                }
                EffectEspDelete(r325_work->tex[0]->mask | 0x3001, 0, 0, 0);
                EffectEspgenDelete(r325_work->tex[0]->mask | 0x3001, 0, 0);
                EffectEfmDelete(r325_work->tex[0]->mask | 0x3001, 0, 0);
                EstSet(0, -1, 0, 0, 1, 0, r325_work->tex[0]->mask | 0x3001, 0, 0, 0);
            }
        } else {
            if (e->NowFrame == 0) {
                if (e->GetMod(&mod, "pl0000", 0, 0) == 1) {
                    TexRenderModResP((cModel*) mod, 6);
                }
                EffectEspDelete(r325_work->tex[0]->mask | 0x3001, 0, 0, 0);
                EffectEspgenDelete(r325_work->tex[0]->mask | 0x3001, 0, 0);
                EffectEfmDelete(r325_work->tex[0]->mask | 0x3001, 0, 0);
            }
        }
        if (pG->game_costume == 1) {
            if (e->NowFrame == 0) {
                if (e->GetMod(&mod, "pl0000", 0, 0) == 1) {
                    ModelInfoSetTrans((cModel*) mod, 6, 0);
                }
            }
        }
        if (e->NowCut == 7) {
            if (e->NowFrame == 0) {
                if (e->GetMod(&mod, "pl0000", 0, 0) == 1) {
                    TexRenderModSet((cModel*) mod, 7, r325_work->texTbl1, r325_work->tex[1], 0, 0, 1, 1, 1.0f);
                }
                EffectEspDelete(r325_work->tex[1]->mask | 0x3001, 0, 0, 0);
                EffectEspgenDelete(r325_work->tex[1]->mask | 0x3001, 0, 0);
                EffectEfmDelete(r325_work->tex[1]->mask | 0x3001, 0, 0);
                EstSet(0, -1, 0, 0, 1, 1, r325_work->tex[1]->mask | 0x3001, 0, 0, 0);
            }
        } else {
            if (e->NowFrame == 0) {
                if (e->GetMod(&mod, "pl0000", 0, 0) == 1) {
                    TexRenderModResP((cModel*) mod, 7);
                }
                EffectEspDelete(r325_work->tex[1]->mask | 0x3001, 0, 0, 0);
                EffectEspgenDelete(r325_work->tex[1]->mask | 0x3001, 0, 0);
                EffectEfmDelete(r325_work->tex[1]->mask | 0x3001, 0, 0);
            }
        }
        switch (e->NowCut) {
        case 0:
            if (e->NowFrame == 0) {
                if (e->GetMod(&info, "ev0002", 0, 0) == 1) {
                    if ((int) pG->Room_flg[0] >= 0) {
                        BitOn(pG->Room_flg[0], 0x80000000);
                        r325_work->tpl = ((cModelInfo*) info)->tpl_addr;
                    }
                }
            }
            break;
        case 7:
            if (e->NowFrame == 0) {
                if (e->GetMod(&info, "ev0002", 0, 0) == 1) {
                    if (EvtMgr.GetBin(&bin, "event/model/ev0000/ev0002_red_eye.tpl", 0) == 1) {
                        ((cModelInfo*) info)->setTplAddr(bin);
                    }
                }
            }
            break;
        default:
            if (e->NowFrame == 0) {
                if (e->GetMod(&info, "ev0002", 0, 0) == 1) {
                    ((cModelInfo*) info)->setTplAddr(r325_work->tpl);
                }
            }
            break;
        }
        if (e->NowCut == 8) {
            if (e->NowFrame == 0) {
                if (e->GetMod(&mod, "pl0200", 0, 0) == 1) {
                    Obj18Work* w = &((cObj*) mod)->o18;

                    if (w && w->child) {
                        ((cObj*) mod)->o18.ObjChainFlagCommon |= 0x04000000;
                        w->child->be_flag &= ~2;
                    }
                }
            }
        } else {
            if (e->NowFrame == 0) {
                if (e->GetMod(&mod, "pl0200", 0, 0) == 1) {
                    Obj18Work* w = &((cObj*) mod)->o18;

                    if (w && w->child) {
                        ((cObj*) mod)->o18.ObjChainFlagCommon &= ~0x04000000;
                        w->child->be_flag |= 2;
                    }
                }
            }
        }
        break;
    case 2:
        setRoomEtcDisp(0, 1, 1);
        break;
    }
}
