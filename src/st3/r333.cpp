#include "types.h"
#include "main_mem.h"
#include "st_room.h"
#include "atari.h"
#include "map_obj.h"
#include "light.h"
#include "widget.h"
#include "event.h"
#include "card.h"
#include "sofdec.h"
#include "flag_rsf.h"
#include "global.h"
#include "main.h"
#include "main_sub.h"
#include "game.h"
#include "sce.h"
#include "sce_sys.h"
#include "sce_at.h"
#include "scroll.h"
#include "obj.h"
#include "em.h"
#include "em_wrap.h"
#include "pl0e.h"
#include "player.h"
#include "pl_sub.h"
#include "cam_ctrl.h"
#include "mes.h"
#include "item.h"
#include "snd.h"
#include "fade.h"
#include "view.h"
#include "dvd.h"
#include "option.h"
#include "cDataSwap.h"
#include "mercenaries.h"
#include "TexRender.h"
#include "id_sys.h"
#include "cockpit.h"
#include "act_btn.h"
#include "sscrn.h"
#include "est.h"
#include "esp.h"
#include "db_log.h"

// Room 3-33 (D:/Bio4/Prog/r333.cpp): the jet ski escape: the ride start (s00), the collapsing cave
// (fall_a..d), the escape event (s10) with the screen capture filter, and the game result screen.

extern "C" {
void Filter0bAllocBuf();
void Filter0bFreeBuf();
void Filter0bCapture();
void Filter0bSetAlpha(u8 alpha);
}

struct R333Work {
    TexRenderMng* tex;   // 0x00  the water render target
    cEmWrap em;          // 0x04  the jet ski (list 0xFE)
    u8 pad_10[0x50];
    int timer;           // 0x60  count-down frames when the s00 event started
    u32 se;              // 0x64  SndCall handle of the engine sound
    void* idData;        // 0x68  SS/<lang>/id333.dat
};

// The result id file (SS/<lang>/id333.dat): offsets of its texture and id blocks.
struct R333IdData {
    u8 pad_0[0x10];
    u32 ofsTex;   // 0x10  IdTexDataLoad(.., 7)
    u32 ofsId;    // 0x14  IdSys.set(.., 0xFF, 0x28, ..)
};

static R333Work* r333_work;

void R333EventS00();
void R333EventS10();
extern "C" void Evt_R333S00_Func(Event* e);
extern "C" void Evt_R333S10_Func(Event* e);
static void setTexRender();
static void exec_no_ret_exit();
static void exec_no_ret();
static void fall_eff();
static void fall_eff2();
static void fall_a();
static void fall_b();
static void fall_c();
static void fall_d();
static void r333_use_exec();
static void r333_useMes();
void ride();
static void gameResult();
static void exec_continue();
static void exec_die();
static void yure_task();
static void kazekiri_task();
void read_id_data();
void disp_id_data();
void erase_id_data();

// st3.cpp's count-down helpers
void st3_setCountDownTimer(int frame);
int st3_getCountDownTimer();
void st3_startCountDown();
void st3_checkCountDown();
void st3_endCountDown();

// 1 while the event is being skipped (EVT status bit 30).
static inline int r333_evtSkip(Event* e)
{
    int skip = 1;

    if ((e->status & 0x40000000) == 0) {
        skip = 0;
    }
    return skip;
}

void R333Init()
{
    int zero;

#line 103 "D:/Bio4/Prog/r333.cpp"
    r333_work = (R333Work*) MEM_CALLOC(sizeof(R333Work), 1, 0xd);
    read_id_data();
    if (pG->room_id_prev == 0xFFF) {
        if ((pG->flags_5018 & 0x04000000) == 0) {
            BitOn(pG->flags_5018, 0x04000000);
            SubCharInit(1, &pPL->pos, pPL->rot.y);
            SubCharCtrl(1, 0);
        }
        ItemMgr.get(0x88, 0);
    }
    if (pG->x4F9F == 2) {
        SceExec(0x12, (TaskFunc) R333EventS10, 0, 0, 2, 0);
    }
    zero = 0;
    pG->flags_54 &= ~0x400;
    EstSet(0, -1, 0, 0, 1, 0xB, 1, 2, (u32) zero, (void*) zero);
    EvtMgr.SetFunc("evt_r333s00_func", (void*) Evt_R333S00_Func);
    EvtMgr.SetFunc("evt_r333s10_func", (void*) Evt_R333S10_Func);
    SceAtDataSet_exec(0xE, 0x12, 0, (TaskFunc) r333_useMes, 0, 1);
    SceExec(0x12, (TaskFunc) r333_use_exec, 0, 0, 2, 0);
    SceExec(0x12, (TaskFunc) yure_task, 0, 0, 2, 0);
    SceExec(0x12, (TaskFunc) kazekiri_task, 0, 0, 2, 0);
    if (RsfCheck(G_ROOM_ID, 0) == 0) {
        SceAtDataSet_exec(1, 0x12, 0, (TaskFunc) R333EventS00, 0, 1);
        EvtMgr.EvtReadAram("event/evd/r333s00.evd", 0, 0, 0, 0);
    }
    if (RsfCheck(G_ROOM_ID, 1) == 0) {
        SceAtDataSet_exec(2, 0x12, 0, (TaskFunc) R333EventS10, 0, 1);
        EvtMgr.EvtReadAram("event/evd/r333s10.evd", 0, 0, 0, 0);
    }
    if (RsfCheck(G_ROOM_ID, 3) == 0) {
        SceAtDataSet_exec(4, 0x12, 0, (TaskFunc) exec_no_ret, 0, 1);
    } else {
        EstSet(0, -1, 0, 0, 1, 1, 1, 5, (u32) zero, (void*) zero);
    }
    zero = 0;
    if (RsfCheck(G_ROOM_ID, 2) == 0) {
        SceAtDataSet_exec(0x11, 0x12, 0, (TaskFunc) exec_continue, 0, 1);
    }
    ZFAR = 100000000.0f;
    r333_work->em.setEm(0xFE, -1, 1, 1, 1);
    {
        Vec pos;

        pos.x = -674136.0f;
        pos.y = -26675.0f;
        pos.z = -551098.0f;
        r333_work->em.setPos(&pos);
    }
    {
        Vec ang;

        ang.y = 2.2f;
        ang.x = 0.0f;
        ang.z = 0.0f;
        r333_work->em.setAng(&ang);
    }
    ((cPl0e*) r333_work->em.getPtr())->setRail(ROOM_ARC_PTR(pG->pRoomArc, 0x1F));
    setTexRender();
    EstSet(0, -1, 0, 0, 1, 0x10, 0x801, 3, (u32) zero, (void*) zero);
    BitOn(pG->flags_170, 0x00040000);
    pG->flags_58 |= 0x01000000;
    SceAtDataSet_exec(0, 0x12, 0, (TaskFunc) fall_eff, 0, 1);
    SceAtDataSet_exec(0x10, 0x12, 0, (TaskFunc) fall_eff2, 0, 1);
    SceAtDataSet_exec(6, 0x12, 0, (TaskFunc) fall_a, 0, 1);
    SceAtDataSet_exec(8, 0x12, 0, (TaskFunc) fall_b, 0, 1);
    SceAtDataSet_exec(0xA, 0x12, 0, (TaskFunc) fall_c, 0, 1);
    SceAtDataSet_exec(0xC, 0x12, 0, (TaskFunc) fall_d, 0, 1);
    SmdSetTrans(0x70, 0);
    SmdSetTrans(0x74, 0);
    SmdSetTrans(0x78, 0);
    SmdSetTrans(0x71, 0);
    SmdSetTrans(0x75, 0);
    SmdSetTrans(0x79, 0);
    SmdSetTrans(0x72, 0);
    SmdSetTrans(0x76, 0);
    SmdSetTrans(0x7A, 0);
    if (pG->room_id_prev == 0xFFF) {
        pG->room_id_prev = 0x331;
        st3_setCountDownTimer(3600);
    }
    st3_startCountDown();
    if (pG->flags_54 & 0x00080000) {
        if (st3_getCountDownTimer() <= 2699) {
            st3_setCountDownTimer(2700);
        }
    }
    r333_work->se = SndCall(6, 6, 0, 0, 0, 0);
}

void R333Main()
{
    if (RsfCheck(G_ROOM_ID, 1) == 0) {
        st3_checkCountDown();
    }
    if (CheckDoorJumpWithAshley() == 1) {
        SceAtSetEnable(0xF, 1);
    } else {
        SceAtSetEnable(0xF, 0);
    }
    if (DebugTrg(0)) {
        SceExec(0x12, (TaskFunc) exec_die, 0, 0, 2, 0);
    }
}

// The ride starts (s00): the jet ski motion, then the "hold on" prompt for 150 frames.
void R333EventS00()
{
    if ((s16) pG->pl_life > 0) {
        if (RsfCheck(G_ROOM_ID, 0) == 0) {
            cPl0e* boat;
            u32 i;

            RsfSet(G_ROOM_ID, 0);
            SceAtSetEnable(1, 0);
            boat = (cPl0e*) r333_work->em.getPtr();
            boat->stopEngine();
            SndRoomStrVolSet(10, 600);
            EvtMgr.EvtReadExec("event/evd/r333s00.evd", 0, 0);
            SndRoomStrVolReset(200);
            boat->set2ndRail();
            BitOn(pG->flags_170, 0x00040000);
            pG->flags_58 |= 0x01000000;
            SceSleep(15);
            i = 0;
            do {
                ActBtn.set(0x31, 5, 0, 0, 2, 0x10, 0, 0);
                i++;
                SceSleep(1);
            } while (i <= 0x95);
        }
    }
}

// The escape (s10): the filter capture event, then the result screen.
void R333EventS10()
{
    if ((s16) pG->pl_life > 0) {
        if (RsfCheck(G_ROOM_ID, 1) == 0) {
            RsfSet(G_ROOM_ID, 1);
            SceAtSetEnable(2, 0);
            SndRoomStrStop(0);
            SndRoomBgmStop(0, 0);
            EffectEspDelete(1, 2, 0, 0);
            EffectEspgenDelete(1, 2, 0);
            EffectEfmDelete(1, 2, 0);
            ((cPl0e*) r333_work->em.getPtr())->stopEngine();
            SceEventStart(0);
            EvtMgr.EvtReadExec("event/evd/r333s10.evd", 0, 0);
            SceSleep(90);
            SetGameTime();
            SceExec(0x12, (TaskFunc) gameResult, 0, 0, 2, 0);
            SceEventEnd(0);
        }
    }
}

extern "C" void Evt_R333S00_Func(Event* e)
{
    void* mod;

    switch (e->funcMode) {
    case 0:
        SmdSetTrans(3, 0);
        r333_work->timer = st3_getCountDownTimer();
        break;
    case 1:
        switch (e->cut) {
        case 0:
        case 1:
            BitOn(pG->flags_5010, 0x800);
            BitOff(pG->flags_170, 0x00040000);
            BitOff(pG->flags_58, 0x01000000);
            break;
        default:
            BitOn(pG->flags_170, 0x00040000);
            BitOn(pG->flags_58, 0x01000000);
            BitOff(pG->flags_5010, 0x800);
            break;
        }
        if (e->cut == 0) {
            if (e->frame == 0) {
                if (e->GetMod(&mod, "pl0000", 0, 0) == 1) {
                    ((cModel*) mod)->be_flag |= 0x00100000;
                }
                if (e->GetMod(&mod, "pl0100", 0, 0) == 1) {
                    ((cModel*) mod)->be_flag |= 0x00100000;
                }
            }
        }
        break;
    case 2:
        SmdSetTrans(3, 1);
        st3_setCountDownTimer(r333_work->timer - e->maxTotalFrame);
        st3_startCountDown();
        break;
    }
}

extern "C" void Evt_R333S10_Func(Event* e)
{
    static int alpha = 230;
    static int alphaTime = 50;
    void* mod;

    switch (e->funcMode) {
    case 0: {
        u32 i;

        for (i = 0; i <= 0x1F; i++) {
            u8 no = i + 0xC;

            EffectEspDelete(0, no, 0, 0);
            EffectEspgenDelete(0, no, 0);
            EffectEfmDelete(0, no, 0);
        }
        EffectEspDelete(0x4001, 0, 0, 0);
        EffectEspgenDelete(0x4001, 0, 0);
        EffectEfmDelete(0x4001, 0, 0);
        pG->flags_170 &= ~0x00040000;
        Filter0bAllocBuf();
        SmdSetTrans(3, 0);
        st3_endCountDown();
        break;
    }
    case 1:
        switch (e->cut) {
        case 0:
        case 1:
            BitOn(pG->flags_5010, 0x800);
            BitOn(pG->flags_58, 0x01000000);
            SmdSetTrans(0xCE, 0);
            break;
        case 2:
            BitOn(pG->flags_5010, 0x800);
            BitOn(pG->flags_58, 0x01000000);
            SmdSetTrans(0xCE, 1);
            break;
        default:
            BitOn(pG->flags_5010, 0x800);
            BitOff(pG->flags_58, 0x01000000);
            SmdSetTrans(0xCE, 1);
            break;
        }
        if (e->cut == 0x10 || e->cut == 0x12) {
            if (e->frame == 0) {
                void* m;

                if (e->GetMod(&m, "pl0100", 0, 0) == 1) {
                    ((cObj*) m)->o18.flags |= 0x40;
                }
            }
        } else {
            if (e->frame == 0) {
                void* m;

                if (e->GetMod(&m, "pl0100", 0, 0) == 1) {
                    ((cObj*) m)->o18.flags &= ~0x40;
                }
            }
        }
        switch (e->cut) {
        case 0:
            if (e->frame == 0) {
                if (e->GetMod(&mod, "pl0000", 0, 0) == 1) {
                    ((cModel*) mod)->be_flag |= 0x00100000;
                }
                if (e->GetMod(&mod, "pl0100", 0, 0) == 1) {
                    ((cModel*) mod)->be_flag |= 0x00100000;
                }
                if (e->GetMod(&mod, "evm8100", 0, 0) == 1) {
                    ((cModel*) mod)->be_flag |= 0x80;
                }
            }
            break;
        case 0xD:
            if (e->frame == e->maxFrame - 1) {
                Filter0bCapture();
            }
            break;
        case 0xE:
            if (e->frame <= alphaTime) {
                Filter0bSetAlpha((u8) alpha - e->frame * alpha / alphaTime);
            }
            break;
        case 0x13:
            if (e->frame == 380) {
                int skip = r333_evtSkip(e);

                if (skip == 0) {
                    FadeSetW(2, e->maxFrame - 380, 0, 0);
                }
            }
            break;
        }
        break;
    case 2:
        Filter0bFreeBuf();
        SmdSetTrans(3, 1);
        break;
    }
}

// The water rendered to texture, blended into scroll object 7.
static void setTexRender()
{
    static u8 texTbl[0x20];
    cObj* obj;
    u8* tbl = texTbl;

    if (GetTexRenderMgr(&r333_work->tex)) {
        tbl[0] = 1;
        tbl[1] = 0;
        tbl[4] = 0xF7;
        tbl[5] = r333_work->tex->texId;
        IntSet(r333_work->tex->repType, 1);
        EstSet(0, -1, 0, 0, 1, 0, r333_work->tex->mask | 1, 0, 0, 0);
    } else {
        pLog->err(0, 0, "setTexRender() : Manager alloc failed!!");
    }
    obj = SmdGetObjPtr(7);
    obj->pInfo->setTexBlendTbl(tbl);
    obj->pInfo->setBlendRatio(0xFF);
    obj->pInfo->color[3] = 0xF0;
    obj->x136 = 2;
    obj->x137 = 0x1E;
    obj->x138 = 0x80;
}

static void exec_no_ret_exit()
{
    CamCtrl.Comeback(0);
    SceEventEnd(0);
    SceAtSetEnable(5, 1);
}

// The way back collapses (camera cut 10).
static void exec_no_ret()
{
    int zero = 0;

    BitOn(pG->door_unlock[1], 0x4000);
    RsfSet(G_ROOM_ID, 3);
    SceEventStart(1);
    SndStrReq(1, 0x3A, 0x80000003, 0, 0, 0.0f);
    EstSet(0, -1, 0, 0, 1, 1, 1, 5, (u32) zero, (void*) zero);
    CamCtrl.CutCall(0xA);
    SceSetEventCancel(1, (TaskFunc) exec_no_ret_exit, 0, -1, 1);
    while (!CamCtrl.IsMotionEnd()) {
        SceSleep(1);
    }
    SceSetEventCancel(0, 0, 0, -1, 1);
    exec_no_ret_exit();
}

static void fall_eff()
{
    SndCall(6, 0, 0, 0, 0, 0);
    EstSet(0, -1, 0, 0, 1, 7, 1, 0, 0, 0);
}

static void fall_eff2()
{
    SndCall(6, 0, 0, 0, 0, 0);
    EstSet(0, -1, 0, 0, 1, 0xA, 1, 0, 0, 0);
}

static void fall_a()
{
    EstSet(0, -1, 0, 0, 1, 5, 1, 0, 0, 0);
    SndCall(6, 4, 0, 0, 0, 0);
    SceSleep(47);
    SndCall(6, 5, 0, 0, 0, 0);
}

static void fall_b()
{
    EstSet(0, -1, 0, 0, 1, 4, 1, 0, 0, 0);
    SndCall(6, 4, 0, 0, 0, 0);
    SceSleep(47);
    SndCall(6, 5, 0, 0, 0, 0);
}

static void fall_c()
{
    EstSet(0, -1, 0, 0, 1, 2, 1, 0, 0, 0);
    SndCall(6, 4, 0, 0, 0, 0);
    SceSleep(47);
    SndCall(6, 5, 0, 0, 0, 0);
}

static void fall_d()
{
    EstSet(0, -1, 0, 0, 1, 3, 1, 0, 0, 0);
    SndCall(6, 4, 0, 0, 0, 0);
    SceSleep(47);
    SndCall(6, 5, 0, 0, 0, 0);
}

// Waits for the jet ski key to be used.
static void r333_use_exec()
{
    while (ItemMgr.check(0x88) != 1) {
        SceSleep(1);
    }
    ride();
}

static void r333_useMes()
{
    if (CheckDoorJumpWithAshley() == 1) {
        if (ItemMgr.num(0x88) != 0) {
            SubScreenOpen(0x80, 1);
        }
    } else {
        cMes.MesSet(0x67, 100, 0x150 - cMes.getWork()->lineSpace - cMes.getWork()->fontH - 1, 1, 0, 0, 4);
    }
}

// Leon gets on the jet ski.
void ride()
{
    SceMesSet(0, 0, 1, 0x64, 0x150 - cMes.getWork()->lineSpace - cMes.getWork()->fontH - 1);
    pG->flags_174 |= 0x80000000;
    EffectEspDelete(1, 5, 0, 0);
    EffectEspgenDelete(1, 5, 0);
    EffectEfmDelete(1, 5, 0);
    SndStrReq(1, 0x74, 0x80000003, 0, 0, 0.0f);
    ((cPl0e*) r333_work->em.getPtr())->setRide();
    SndStop(r333_work->se, 0);
    if (pG->x4F9F == 2) {
        SceExec(0x12, (TaskFunc) R333EventS00, 0, 0, 2, 0);
    }
}

// The result screen after the escape movie: the game result, the extras unlocked, the save question.
struct SystemWorkPtr {
    SystemWork* p;
};
#define pSysS (((SystemWorkPtr*) &pSys)->p)

// One frame's wait on fade `no` (r31c FadeWait): the index is a separate `addi` on the array base,
// shared by the two waits of the omake path (`Fade+0x48` in r31), folded at the single-use ones.
static inline void r333_fadeWait(int no)
{
    if (Fade[no].flags & 1) {
        SceSleep(1);
    }
}

static void gameResult()
{
    static int FADE_TIME = 15;
    static char data_name[] = "SS/___/result.dat";
    static u32 MARGIN = 0x20000;
    static int sel;
    static u32 stop_bak;
    static u32 disp_bak;
    static u32 stop_bak2;
    static u32 disp_bak2;
    cDataSwap swap;
    u32 size;
    void* data;
    GameResult* res;

    SceEventStart(0);
    disp_bak = pG->flags_58;
    BitSet(pG->flags_58, 0xFFFFFFFF);
    BitOff(pG->flags_58, 0x2000);
    BitOff(pG->flags_58, 0x800);
    BitOff(pG->flags_58, 0x10000);
    stop_bak = pG->flags_170;
    BitSet(pG->flags_170, 0xFFFFFFFF);
    BitOff(pG->flags_170, 0x00800000);
    BitOff(pG->flags_170, 0x80000000);
    BitOff(pG->flags_170, 0x40);
    SceSleep(2);
    systemVISetBlack(1);
    FadeKill(2);
    ScreenReSize(0x200, 0x1C0);
    Sofdec.Initialize("movie/ending.sfd", 0);
    SceSleep(1);
    while (Sofdec.isPlay()) {
        SceSleep(1);
    }
    // Loop-note barrier (the r40e gameResult idiom): lifeMeterDisp's `li r4,0` and the counter's `li`
    // are issued after the calls before them.
    do {
        systemVISetBlack(1);
        ScreenReSize(0x280, 0x1C0);
        systemVISetBlack(0);
    } while (0);
    Cckpt.lifeMeterDisp(0);
    {
        int i;

        i = 305;
        disp_id_data();
        while (i != 0) {
            SceSleep(1);
            i--;
        }
        FadeSetW(2, 0, 0, 0);
    }
    SceSleep(1);
    erase_id_data();
    SceEventEnd(0);
    disp_bak2 = disp_bak;
    stop_bak2 = stop_bak;
    U32Set(pG->flags_58, disp_bak);
    pG->flags_170 = stop_bak;
    OpeSetOpenTerm(0x17, -675462.0f, -26062.0f, -552700.0f, 0.665f);
    U32Set(pG->flags_58, disp_bak2);
    pG->flags_170 = stop_bak2;
    SceEventStart(0);
    setLangExt3(data_name + 3);
    Dvd.FileExistCheck(data_name, &size);
    size += 8;
    size += MARGIN;
    swap.SwapOut((u32) pG->pRoomArc, size, 0);
    res = new GameResult;
#line 958 "D:/Bio4/Prog/r333.cpp"
    Dvd.ReadCheck(DvdReadN(data_name, 0, 0, 0, 0, 5, __FILE__, __LINE__), 0, 0, &data);
    res->init(data);
    SndStrReq(0, 0x3A, 0x80000003, 0, 0, 0.0f);
    FadeKillAll();
    FadeSetW(0x80000002, FADE_TIME, 0, 0);
    r333_fadeWait(2);
    while (res->move() == 0) {
        SceSleep(1);
    }
    SndStrReq(0, 0x3A, 4, 800, 0, 0.0f);
    FadeSetW(2, FADE_TIME, 0, 0);
    r333_fadeWait(2);
    if ((pSys->x4 & 0x40000000) == 0) {
        res->omake_init(data);
        FadeSetW(0x80000002, FADE_TIME, 0, 0);
        r333_fadeWait(2);
        while (res->omake_move() == 0) {
            SceSleep(1);
        }
        FadeSetW(2, FADE_TIME, 0, 0);
        r333_fadeWait(2);
    }
    // The selection is stored through a pointer taken before the calls: its `sel@ha` is a pseudo set
    // ahead of SceMesSet (r30), where `sel = f()` legitimises the address after the call (`li r9`).
    int* pSel = &sel;

    SceMesSet(0x80, 1, 1, 0x64, 0x150 - cMes.getWork()->lineSpace - cMes.getWork()->fontH - 1);
    *pSel = SceMesGetSelection();
    res->quit();
    delete res;
    swap.SwapIn();
    U16Set(pG->x4F8E, pG->x4F8E + 1);
    if (pG->x4F8E > 99) {
        pG->x4F8E = 99;
    }
    BitOn(pSys->x4, 0x80000000);
    BitOn(pSys->x4, 0x40000000);
    BitOn(pSys->x4, 0x00800000);
    if ((pSys->x4 & 0x00400000) == 0) {
        MercSaveWork save;
        int i;

        pSys->x4 |= 0x00400000;
        // Struct-member view of pSys (pGS): the element store is not disjoint from the pointer load,
        // so pSys is reloaded per iteration and the address stays `(pSys + 0x10) + i*4` (`stwx`).
        for (i = 0; i < 4; i++) {
            pSysS->x10[i] = 0;
        }
        for (i = 0; i < 2; i++) {
            pSysS->x20[i] = 0;
        }
        MercSysGetSaveWork(&save);
        for (i = 0; i < 4; i++) {
            save.stage[i].score = 0;
            save.stage[i].mode = 0;
        }
        MercSysSetSaveWork(&save);
    }
    if (sel == 1) {
        CardSave(0, 0x12);
        SceSleep(1);
    }
    U32Set(pG->flags_58, disp_bak);
    U32Set(pG->flags_170, stop_bak);
    pG->flags_54 |= 0x04000000;
}

static void exec_continue()
{
    RsfSet(G_ROOM_ID, 2);
    GameSaveSave(&GameSave, pSaveData, -1);
}

// Debug: the death camera (cut 13).
static void exec_die()
{
    int zero = 0;

    SceEventStart(1);
    BitOn(pG->flags_5010, 0x800);
    BitOff(pG->flags_170, 0x00040000);
    BitOff(pG->flags_58, 0x01000000);
    SmdSetTrans(3, 0);
    EstSet(0, -1, 0, 0, 1, 0x14, 1, 4, (u32) zero, (void*) zero);
    CamCtrl.CutCall(0xD);
    while (!CamCtrl.IsMotionEnd()) {
        SceSleep(1);
    }
    SceSleep(200);
    CamCtrl.Comeback(0);
    SceEventEnd(0);
    BitOff(pG->flags_5010, 0x800);
    BitOn(pG->flags_170, 0x00040000);
    BitOn(pG->flags_58, 0x01000000);
    SmdSetTrans(3, 1);
    EffectEspDelete(1, 4, 0, 0);
    EffectEspgenDelete(1, 4, 0);
    EffectEfmDelete(1, 4, 0);
}

static void yure_task()
{
}

// The wind noise while the camera is not in an event.
static void kazekiri_task()
{
    for (;;) {
        if ((int) pG->sceat_x17C < 0) {
            SndCall(6, 3, 0, 0, 0, 0);
            SceSleep(30);
        }
        SceSleep(1);
    }
}

void read_id_data()
{
    static char id_name[] = "SS/___/id333.dat";
    void* data;

    setLangExt3(id_name + 3);
#line 1146 "D:/Bio4/Prog/r333.cpp"
    Dvd.ReadCheck(DvdReadN(id_name, 0, 0, 0, 0, 5, __FILE__, __LINE__), 0, 0, &data);
    r333_work->idData = data;
}

// The result id table (SS/<lang>/id333.dat: texture block at 0x10, id block at 0x14).
void disp_id_data()
{
    R333IdData* d = (R333IdData*) r333_work->idData;

    IdTexDataLoad((void*) (d->ofsTex + (u32) d), 7);
    IdSys.set((void*) (d->ofsId + (u32) d), 0xFF, 0x28, 0x13, 6, 0);
}

void erase_id_data()
{
    IdTexRelease(7);
    IdSys.kill(0xFF, 0x28);
}
