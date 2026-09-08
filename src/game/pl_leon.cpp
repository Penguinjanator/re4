// game/pl_leon.cpp: Leon player class: model set (body, hands, head, face), motion table, cloth.

#include "atari.h"
#include "light.h"
#include "player.h"
#include "global.h"
#include "db_log.h"
#include "main.h"
#include "snd.h"
#include "pl_cloth.h"

extern "C" {
void OSReport(const char* fmt, ...);
void EspDataLoad(void* data, int a, int b);     // game/eff_sys.cpp
int SubCharCheckCtrl();                         // game/pl_sub.cpp
void SubCharCtrl(int a, int b);                 // game/pl_sub.cpp
}
u32 SubCharGetStatus();                         // game/pl_npc.cpp
void ShapeSet(void* info, int a, void* data, int b);  // game/shape.cpp
void ShapeEnd(void* info);

extern cModel* pSUB;
extern u8 pl_fs_tbl[];   // game/foot_shadow_tbl.cpp (incomplete type: full address, not @sda21)

#define HALT()                                                    \
    do {                                                          \
        OSReport("HALT %s(%d)\n", __FILE__, __LINE__);            \
        *(volatile u32*) 0x11111111 = 0;                          \
    } while (0)

#define VALID_PTR(p) ((u32) (p) >= 0x80000000 && (u32) (p) <= 0x82FFFFFF)

// Store through a reference: a scalar (non-struct) MEM, so pG is reloaded after every store.
static inline void PSet(void*& d, void* v) { d = v; }

cPlLeon::cPlLeon()
{
    PlArc* arc;

    init0();
    setModel();
    weaponRelease();
    weaponLoad(pG->wep_no, pG->wep_type);
    weaponInit();
    init1();
    setMotion();
    arc = pG->pPlArc;
    EspDataLoad(PL_ARC_PTR(arc, 0x1A), 3, 0);
    startUp();
    pFootShadowTbl = pl_fs_tbl;
}

void cPlLeon::setMotion()
{
    PSet(pMotTbl[0x5F], PL_ARC_PTR(pG->pPlArc, 0x32));
    PSet(pMotTbl[0x60], PL_ARC_PTR(pG->pPlArc, 0x33));
    PSet(pMotTbl[0x61], PL_ARC_PTR(pG->pPlArc, 0x34));
    PSet(pMotTbl[0x62], PL_ARC_PTR(pG->pPlArc, 0x35));
    PSet(pMotTbl[0x63], PL_ARC_PTR(pG->pPlArc, 0x36));
    PSet(pMotTbl[0x64], PL_ARC_PTR(pG->pPlArc, 0x37));
    PSet(pMotTbl[0x65], PL_ARC_PTR(pG->pPlArc, 0x38));
    PSet(pMotTbl[0x66], PL_ARC_PTR(pG->pPlArc, 0x39));
    PSet(pMotTbl[0x6B], PL_ARC_PTR(pG->pPlArc, 0x3A));
    PSet(pMotTbl[0x6C], PL_ARC_PTR(pG->pPlArc, 0x3B));
    PSet(pMotTbl[0x67], PL_ARC_PTR(pG->pPlArc, 0x3C));
    PSet(pMotTbl[0x68], PL_ARC_PTR(pG->pPlArc, 0x3D));
    PSet(pMotTbl[0x69], PL_ARC_PTR(pG->pPlArc, 0x3E));
    PSet(pMotTbl[0x6A], PL_ARC_PTR(pG->pPlArc, 0x3F));
}

void cPlLeon::move()
{
    cPlayer::move();
}

void cPlLeon::setModel()
{
    cModelInfo* info;
    PlArc* arc;

    arc = pG->pPlArc;
    info = (cModelInfo*) modelInit(PL_ARC_PTR(arc, 4), PL_ARC_PTR(arc, 5));
    if (!VALID_PTR(info)) {
        goto failed;
    }
    switch (pG->costume) {
    case 0:
    case 1:
    case 2:
    case 3:
        arc = pG->pPlArc;
        info = ModInfoMgr.create(PL_ARC_PTR(arc, 0xA), PL_ARC_PTR(arc, 5));
        if (!VALID_PTR(info)) {
            goto failed;
        }
        addModel(info);
        break;
    }
    arc = pG->pPlArc;
    info = ModInfoMgr.create(PL_ARC_PTR(arc, 0xD), PL_ARC_PTR(arc, 5));
    if (!VALID_PTR(info)) {
        goto failed;
    }
    addModel(info);
    pFace->pFace = info;
    if (VALID_PTR(pFace->pFace)) {
        pFace->pFace->x5C = 0.0f;
        pFace->pFace->x84 = 0.0f;
        pFace->pFace->x70 = 0.0f;
    }
    arc = pG->pPlArc;
    info = ModInfoMgr.create(PL_ARC_PTR(arc, 8), PL_ARC_PTR(arc, 7));
    if (!VALID_PTR(info)) {
        goto failed;
    }
    addModel(info);
    pFace->pShape = info;
    pFace->pHeadData = PL_ARC_PTR(pG->pPlArc, 8);
    arc = pG->pPlArc;
    info = ModInfoMgr.create(PL_ARC_PTR(arc, 6), PL_ARC_PTR(arc, 7));
    if (!VALID_PTR(info)) {
        goto failed;
    }
    addModel(info);
    pFace->pHair = info;
    arc = pG->pPlArc;
    info = ModInfoMgr.create(PL_ARC_PTR(arc, 9), PL_ARC_PTR(arc, 7));
    if (!VALID_PTR(info)) {
        goto failed;
    }
    addModel(info);
    info->flags |= 0x40;
    pFace->pEye = info;
    switch (pG->costume) {
    case 1:
    case 2:
    case 3:
        arc = pG->pPlArc;
        info = ModInfoMgr.create(PL_ARC_PTR(arc, 0x10), PL_ARC_PTR(arc, 5));
        if (!VALID_PTR(info)) {
            goto failed;
        }
        addModel(info);
        break;
    }
    if (pG->flags_51C0 & 0x20) {
        setWound();
    }
    x12D = 1;
    setFace(0);
    setRightHand(0);
    setLeftHand(1);
    return;

failed:
    pLog->err(0, 0, "cPlLeon::setModel() failed.");
}

void cPlLeon::setWound()
{
    cModelInfo* info;
    PlArc* arc = pG->pPlArc;

    info = ModInfoMgr.create(PL_ARC_PTR(arc, 0xE), PL_ARC_PTR(arc, 0xF));
    if (!VALID_PTR(info)) {
        pLog->err(0, 0, "cPlLeon::setModel() failed.");
    } else {
        addModel(info);
    }
}

void cPlLeon::setRightHand(int no)
{
    cModelInfo* info;
    PlArc* arc;

    if (pFace->pRight) {
        deleteModelInfo(pFace->pRight);
        pFace->pRight = 0;
        pFace->pRightData = 0;
    }
    switch (no) {
    case 0:
        arc = pG->pPlArc;
        no = (int) PL_ARC_PTR(arc, 0x12);
        break;
    case 1:
        no = (int) pFace->pRightDataAlt;
        break;
    }
    arc = pG->pPlArc;
    info = ModInfoMgr.create((void*) no, PL_ARC_PTR(arc, 0x11));
    if (info) {
        addModel(info);
        pFace->pRight = info;
        pFace->pRightData = (void*) no;
    }
    if (info == 0) {
#line 353 "D:/Bio4/Prog/pl_leon.cpp"
        HALT();
    }
}

void cPlLeon::setLeftHand(u32 no)
{
    cModelInfo* info;
    void* data;
    PlArc* arc;

    if (pFace->pLeft) {
        deleteModelInfo(pFace->pLeft);
        pFace->pLeft = 0;
        pFace->pLeftData = 0;
    }
    if (no == 0x63) {
        no = pFace->leftNoPrev;
    }
    switch (no) {
    case 0:
        arc = pG->pPlArc;
        data = PL_ARC_PTR(arc, 0x14);
        break;
    case 1:
        arc = pG->pPlArc;
        data = PL_ARC_PTR(arc, 0x15);
        break;
    case 2:
        arc = pG->pPlArc;
        data = PL_ARC_PTR(arc, 0x16);
        break;
    case 3:
        arc = pG->pPlArc;
        data = PL_ARC_PTR(arc, 0x17);
        break;
    case 4:
        arc = pG->pPlArc;
        data = PL_ARC_PTR(arc, 0x18);
        break;
    case 5:
        arc = pG->pPlArc;
        data = PL_ARC_PTR(arc, 0x19);
        break;
    default:
        data = (void*) no;
        break;
    }
    pFace->leftNoPrev = pFace->leftNo;
    pFace->leftNo = no;
    arc = pG->pPlArc;
    info = ModInfoMgr.create(data, PL_ARC_PTR(arc, 0x11));
    if (info == 0) {
        pLog->err(0, 0, "cPlLeon::setLeftHand() ModInfoMgr.create() failed");
    } else {
        addModel(info);
        pFace->pLeft = info;
        pFace->pLeftData = data;
    }
}

void cPlLeon::setFace(int no)
{
    void* data = 0;
    PlArc* arc;

    if (pFace->pShape == 0) {
        return;
    }
    switch (no) {
    case 1:
        arc = pG->pPlArc;
        data = PL_ARC_PTR(arc, 0x62);
        break;
    case 2:
        arc = pG->pPlArc;
        data = PL_ARC_PTR(arc, 0x63);
        break;
    default:
        ShapeEnd(pFace->pShape);
        break;
    }
    if (no != 0) {
        ShapeSet(pFace->pShape, 0, data, 2);
    }
}

void cPlLeon::setHead(int no)
{
    cModelInfo* info;
    PlArc* arc;

    if (no != 0) {
        return;
    }
    if (pFace->pShape == 0) {
        return;
    }
    deleteModelInfo(pFace->pShape);
    pFace->pShape = 0;
    deleteModelInfo(pFace->pHair);
    pFace->pHair = 0;
    deleteModelInfo(pFace->pEye);
    pFace->pEye = 0;
    arc = pG->pPlArc;
    info = ModInfoMgr.create(PL_ARC_PTR(arc, 0xB), PL_ARC_PTR(arc, 7));
    if (info) {
        addModel(info);
    }
}

void cPlLeon::setHead(void* bin, void* tpl)
{
    cModelInfo* info;

    if (pFace->pShape == 0) {
        return;
    }
    deleteModelInfo(pFace->pShape);
    pFace->pShape = 0;
    deleteModelInfo(pFace->pHair);
    pFace->pHair = 0;
    deleteModelInfo(pFace->pEye);
    pFace->pEye = 0;
    info = ModInfoMgr.create(bin, tpl);
    if (info) {
        addModel(info);
    }
}

int cPlLeon::checkXbutton()
{
    if (xButtonWait) {
        xButtonWait--;
    }
    if (pSUB && pSUB->id == 3) {
        pG->flags_5010 |= 4;
        if (xButtonWait == 0 && SubCharCheckCtrl() && (Key.trg & 0x200)) {
            if (SubCharGetStatus() & 0x40000000) {
                SndCall(1, 0x37, &pParts->worldPos, 0, 0, 0);
                SubCharCtrl(1, 0);
            } else {
                SndCall(1, 0x36, &pParts->worldPos, 0, 0, 0);
                SubCharCtrl(0, 0);
            }
            xButtonWait = 8;
            pG->flags_500C |= 0x800000;
            return 1;
        }
    }
    return 0;
}

void cPlLeon::initCloth()
{
    if (pG->costume != 2) {
        PlClothSetLeon(this, &leonHair, &leonJacket, &leonHolster);
    }
}

void cPlLeon::moveCloth()
{
    if (pG->costume != 2) {
        PlClothMoveLeon(this, &leonHair, &leonJacket, &leonHolster);
    }
}
