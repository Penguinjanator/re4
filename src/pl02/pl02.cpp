// pl02 module (pl02.rel = pl0b.rel = pl0c.rel): Ada: the hair / holster cloth chains of costume 2, the
// player class with Leon's motion table and model set (body, hair, head, face shapes, hands).

#include "atari.h"
#include "light.h"
#include "pl_mod.h"
#include "pendulum.h"
#include "db_log.h"
#include "esp.h"

extern "C" {
void OSReport(const char* fmt, ...);
void PenClothMove(cModel* m, PenCloth* pInfo);   // game/pendulum.cpp
}
extern f32 adaHairMax[14];   // game/pl_cloth.cpp
extern f32 adaHairWindS[14];
extern f32 adaHairWindR[14];
extern CLOTH_AT_SET adaHairAt[6];

#define VALID_PTR(p) ((u32) (p) >= 0x80000000 && (u32) (p) <= 0x82FFFFFF)

// Store through a reference: a scalar (non-struct) MEM, so pG is reloaded after every store.
static inline void PSet(void*& d, void* v) { d = v; }
static inline void PSet(cModelInfo*& d, cModelInfo* v) { d = v; }

// adaHair (costume 2); the parts table and the holster's collision volume are globals (REL fields A = 0)
u8 adaHair2P[14] = {64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77};
static u8 adaHair2Up[14] = {0xFF, 64, 0xFF, 66, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 72, 0xFF, 74, 0xFF, 76};
static u8 adaHair2Dp[14] = {65, 0xFF, 67, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 73, 0xFF, 75, 0xFF, 77, 0xFF};

// adaHolster (costume 2)
static u8 adaHolsterP[5] = {26, 31, 78, 79, 80};
static u8 adaHolsterUp[5] = {0xFF, 0xFF, 0xFF, 78, 79};
static u8 adaHolsterDp[5] = {0xFF, 0xFF, 79, 80, 0xFF};
static f32 adaHolsterMax[5] = {0.2f, 0.2f, 1.0f, 1.0f, 1.0f};
CLOTH_AT_SET adaHolsterAt[1] = {
    {0x0000, 0x11, 0x11, 1.0f, 170.0f, {50.0f, -120.0f, 30.0f}, {0.0f, 0.0f, 0.0f}},
};

static void testHairSetAda2(cModel* pl, PlCloth* c)
{
    c->Num = 14;
    c->pCloth = adaHair2P;
    c->pLeft = 0;
    c->pRight = 0;
    c->pUpLeft = 0;
    c->pUpRight = 0;
    c->pParent = adaHair2Up;
    c->pChild = adaHair2Dp;
    c->pGravity = 0;
    c->pRate = 0;
    c->pWindSin = adaHairWindS;
    c->pWindRate = adaHairWindR;
    c->pMax = adaHairMax;
    c->pAtset = adaHairAt;
    c->At_num = 6;
    c->Gravity = 15.0f;
    c->Rate = 0.75f;
    c->Bundle_num = 4;
    c->WindSin = 0.0f;
    c->Stretchy = 1.0f;
    c->Move_rate = 0.5f;
    c->pModel = 0;
    c->Flag = 0x302;
    c->pPtbl = 0;
    PenClothSet(pl, (PenCloth*) c, 100.0f);
}

void testHairMoveAda2(cModel* pl, PlCloth* c)
{
    PenClothMove(pl, (PenCloth*) c);
}

void testHolsterSetAda2(cModel* pl, PlCloth* c)
{
    c->Num = 5;
    c->pCloth = adaHolsterP;
    c->pLeft = 0;
    c->pRight = 0;
    c->pUpLeft = 0;
    c->pUpRight = 0;
    c->pParent = adaHolsterUp;
    c->pChild = adaHolsterDp;
    c->pWindSin = 0;
    c->pWindRate = 0;
    c->pGravity = 0;
    c->pRate = 0;
    c->pMax = adaHolsterMax;
    c->pAtset = adaHolsterAt;
    c->At_num = 1;
    c->Gravity = 15.0f;
    c->Rate = 0.7f;
    c->WindSin = 0.0f;
    c->Stretchy = 1.0f;
    c->Move_rate = 0.3f;
    c->pModel = 0;
    c->Bundle_num = 0;
    c->Flag = 0x302;
    c->pPtbl = 0;
    PenClothSet(pl, (PenCloth*) c, 100.0f);
}

void testHolsterMoveAda2(cModel* pl, PlCloth* c)
{
    PenClothMove(pl, (PenCloth*) c);
}

void PlClothSetAda2(cModel* pl, PlCloth* ribbon, PlCloth* dress, PlCloth* hair, int evt)
{
    testHairSetAda2(pl, hair);
    testHolsterSetAda2(pl, dress);
}

void PlClothMoveAda2(cModel* pl, PlCloth* ribbon, PlCloth* dress, PlCloth* hair)
{
    testHairMoveAda2(pl, hair);
    testHolsterMoveAda2(pl, dress);
    pl->be_flag &= ~0x00E00000;
}

void PlClothSetAda3(cModel* pl, PlCloth* ribbon, PlCloth* dress, PlCloth* hair, int evt)
{
}

void PlClothMoveAda3(cModel* pl, PlCloth* ribbon, PlCloth* dress, PlCloth* hair)
{
}

cPlAda::cPlAda()
{
    PlArc* arc;

    init0();
    setModel();
    weaponRelease();
    weaponLoad(pG->weapon_no, pG->weapon_type);
    weaponInit();
    init1();
    setMotion();
    arc = pG->pPlayer;
    EspDataLoad((u32) PL_ARC_PTR(arc, 0x1A), 3, 0);
    startUp();
    if (pG->pl_costume == 1) {
        EstSet((int) this, -1, 0, 0, 0, 0x59, 0x800, 0, 0, 0);
    }
    pFootShadowTbl = pl_fs_tbl;
}

void cPlAda::setMotion()
{
    PSet(pMotTbl[0x5F], PL_ARC(0x32));
    PSet(pMotTbl[0x60], PL_ARC(0x33));
    PSet(pMotTbl[0x61], PL_ARC(0x34));
    PSet(pMotTbl[0x62], PL_ARC(0x35));
    PSet(pMotTbl[0x63], PL_ARC(0x36));
    PSet(pMotTbl[0x64], PL_ARC(0x37));
    PSet(pMotTbl[0x65], PL_ARC(0x38));
    PSet(pMotTbl[0x66], PL_ARC(0x39));
    PSet(pMotTbl[0x6B], PL_ARC(0x3A));
    PSet(pMotTbl[0x6C], PL_ARC(0x3B));
    PSet(pMotTbl[0x67], PL_ARC(0x3C));
    PSet(pMotTbl[0x68], PL_ARC(0x3D));
    PSet(pMotTbl[0x69], PL_ARC(0x3E));
    PSet(pMotTbl[0x6A], PL_ARC(0x3F));
}

void cPlAda::move()
{
    cPlayer::move();
}

void cPlAda::setModel()
{
    cModelInfo* info;

    info = (cModelInfo*) modelInit(PL_ARC(4), PL_ARC(5));
    if (!VALID_PTR(info)) {
        pLog->err(0, 0, "cPlAda::setModel() failed.");
        return;
    }
    info = ModInfoMgr.create(PL_ARC(6), PL_ARC(7));
    if (!VALID_PTR(info)) {
        pLog->err(0, 0, "cPlAshley::setModel() failed.");
        return;
    }
    addModel(info);
    PSet(Body->pHair, info);
    info = ModInfoMgr.create(PL_ARC(8), PL_ARC(7));
    if (!VALID_PTR(info)) {
        pLog->err(0, 0, "cPlLeon::setModel() failed.");
        return;
    }
    addModel(info);
    PSet(Body->pShape, info);
    PSet(Body->pHeadData, PL_ARC(8));
    info = ModInfoMgr.create(PL_ARC(9), PL_ARC(0xA));
    if (!VALID_PTR(info)) {
        pLog->err(0, 0, "cPlAshley::setModel() failed.");
        return;
    }
    info->be_flag |= 0x40;
    addModel(info);
    TevScaleGroup = 1;
    setFace(0);
    setRightHand(0);
    setLeftHand(0);
}

void cPlAda::setRightHand(int no)
{
    cModelInfo* info;
    void* data;

    if (Body->pRight) {
        deleteModelInfo(Body->pRight);
        Body->pRight = 0;
        Body->pRightData = 0;
    }
    switch ((u32) no) {
    case 0:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        data = PL_ARC(0x11);
        break;
    case 1:
        data = Body->pWepHand;
        break;
    default:
        data = (void*) no;
        break;
    }
    if ((info = ModInfoMgr.create(data, PL_ARC(5))) != 0) {
        addModel(info);
        Body->pRight = info;
        Body->pRightData = data;
    }
}

void cPlAda::setLeftHand(u32 no)
{
    cModelInfo* info;
    void* data;

    if (Body->pLeft) {
        deleteModelInfo(Body->pLeft);
        Body->pLeft = 0;
        Body->pLeftData = 0;
    }
    if (no == 0x63) {
        no = Body->oldLhandNo;
    }
    switch (no) {
    case 0:
        data = PL_ARC(0x12);
        break;
    case 1:
        data = PL_ARC(0x13);
        break;
    case 2:
        data = PL_ARC(0x14);
        break;
    case 4:
        data = PL_ARC(0x15);
        break;
    default:
        data = (void*) no;
        break;
    }
    Body->oldLhandNo = Body->nowLhandNo;
    Body->nowLhandNo = no;
    info = ModInfoMgr.create(data, PL_ARC_PTR(pGS->pPlayer, 5));
    if (info == 0) {
        pLog->err(0, 0, "cPlLeon::setLeftHand() ModInfoMgr.create() failed");
    } else {
        addModel(info);
        Body->pLeft = info;
        Body->pLeftData = data;
    }
}

void cPlAda::setFace(int no)
{
    void* data = 0;
    void* shape = Body->pShape;

    if (shape == 0) {
        return;
    }
    switch (no) {
    case 0:
    default:
        ShapeEnd(shape);
        break;
    case 1:
        data = PL_ARC(0x62);
        break;
    case 2:
        data = PL_ARC(0x63);
        break;
    }
    if (no != 0) {
        ShapeSet(Body->pShape, 0, data, 2);
    }
}

void cPlAda::setHead(int no)
{
    cModelInfo* info;

    if (no != 0) {
        return;
    }
    if (Body->pHair == 0) {
        return;
    }
    deleteModelInfo(Body->pHair);
    Body->pHair = 0;
    info = ModInfoMgr.create(PL_ARC_PTR(pGS->pPlayer, 0xB), PL_ARC_PTR(pGS->pPlayer, 7));
    if (info) {
        addModel(info);
    }
}

void cPlAda::setHead(void* bin, void* tpl)
{
    cModelInfo* info;

    if (Body->pHair == 0) {
        return;
    }
    deleteModelInfo(Body->pHair);
    Body->pHair = 0;
    info = ModInfoMgr.create(bin, tpl);
    if (info) {
        addModel(info);
    }
}

void cPlAda::initCloth()
{
    if (pG->pl_costume != 1) {
        PlClothSetAda2(this, &adaRibbon, &adaDress, &adaHair, 0);
    } else {
        PlClothSetAda3(this, &adaRibbon, &adaDress, &adaHair, 0);
    }
}

void cPlAda::moveCloth()
{
    if (pG->pl_costume != 1) {
        PlClothMoveAda2(this, &adaRibbon, &adaDress, &adaHair);
    } else {
        PlClothMoveAda3(this, &adaRibbon, &adaDress, &adaHair);
    }
}

void Pl02Init(cEm* em)
{
    new (em) cPlAda();
}

extern "C" void _prolog()
{
    PlInitFunc = Pl02Init;
    OSReport("Pl02 ADA prolog Ok\n");
}

extern "C" void _epilog()
{
}

extern "C" void _unresolved()
{
}
