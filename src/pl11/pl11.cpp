// pl11 module (pl11.rel = pl15.rel): Ashley in the knight armour, a partner (cSubChar) with its own
// model set, face shapes and hand models built from the partner archive.
//
// cSubAshley (pl_mod.h) replaces the DOL's partner class for the armour costume: the same
// cSubChar routines (game/pl_npc.cpp), only modelSet / setFace / setHand differ (the armour body,
// its head shapes and two hand model slots m_pModRHand / m_pModLHand). Pl11Init is registered as EmInitFunc
// (the partner is enemy id 3 in the em list), constructs her, builds the models, runs
// cSubChar::init and clears Status_flg[1] bit17.

#include "atari.h"
#include "light.h"
#include "pl_mod.h"
#include "db_log.h"
#include "esp.h"

#undef ARC
#define ARC(no) SUB_ARC(this, no)

// EmInitFunc of the module: placement-constructs the partner in the cEm work, builds her models,
// runs the cSubChar init and clears Status_flg[1] bit17.
static void Pl11Init(cEm* em)
{
    cSubAshley* sub = new (em) cSubAshley();

    sub->modelSet();
    sub->init();
    StaFlagOff(pG, STA_CRITICAL);
}

// Constructor: hp from the saved ashley_life, the light area on, the players' foot shadow table,
// the partner effects (archive 0x11 as group 4), and registers herself as pSUB.
cSubAshley::cSubAshley()
{
    hp = pGS->ashley_life;
    litArea.on(1);
    pFootShadowTbl = pl_fs_tbl;
    EspDataLoad((u32) ARC(0x11), 4, 0);
    pSUB = this;
}

// Builds the armour model set from the partner archive: the body (4/5) as the base model, the
// head with the face shapes (7/0xB, m_pFace), the hair (6/8) and two armour parts (9, 0xA with
// texture 5), then the hands (setHand(0)).
void cSubAshley::modelSet()
{
    cModelInfo* info;

    if (modelInit(ARC(4), ARC(5)) == 0) {
        pLog->err(0, 0, "cSubChar::modelSet() failed.");
    }
    info = ModInfoMgr.create(ARC(7), ARC(0xB));
    m_pFace = info;
    if (info) {
        addModel(info);
    }
    info = ModInfoMgr.create(ARC(6), ARC(8));
    if (info) {
        addModel(info);
    }
    info = ModInfoMgr.create(ARC(9), ARC(5));
    if (info) {
        addModel(info);
    }
    info = ModInfoMgr.create(ARC(0xA), ARC(5));
    if (info) {
        addModel(info);
    }
    setHand(0);
}

// Face expression on the head shapes: 0 ends the blend (neutral), 1 / 2 the shapes 0x6C / 0x6D
// with blend type 2, 3 / 4 the same shapes with type 6 (held).
void cSubAshley::setFace(int no)
{
    void* data = 0;
    int type = 0;

    if (m_pFace == 0) {
        return;
    }
    switch ((u32) no) {
    case 0:
        break;
    case 1:
        data = ARC(0x6C);
        type = 2;
        break;
    case 2:
        data = ARC(0x6D);
        type = 2;
        break;
    case 3:
        data = ARC(0x6C);
        type = 6;
        break;
    case 4:
        data = ARC(0x6D);
        type = 6;
        break;
    }
    if (no != 0) {
        ShapeSet(m_pFace, 0, data, type);
    } else {
        ShapeEnd(m_pFace);
    }
}

// Hand models: m_pModRHand (right) 0xC / 0xD / 0xE and m_pModLHand (left) 0xF / 0x10 / 0xF for
// hand set 0 (open) / 1 / 3, all with the body texture 5.
void cSubAshley::setHand(int no)
{
    cModelInfo* info;
    void* data;

    if (m_pModRHand) {
        deleteModelInfo(m_pModRHand);
        m_pModRHand = 0;
    }
    switch ((u32) no) {
    case 0:
    default:
        data = ARC(0xC);
        break;
    case 1:
        data = ARC(0xD);
        break;
    case 3:
        data = ARC(0xE);
        break;
    }
    m_pModRHand = ModInfoMgr.create(data, ARC(5));
    if (m_pModRHand) {
        addModel(m_pModRHand);
    }
    if (m_pModLHand) {
        deleteModelInfo(m_pModLHand);
        m_pModLHand = 0;
    }
    switch ((u32) no) {
    case 0:
    default:
        data = ARC(0xF);
        break;
    case 1:
        data = ARC(0x10);
        break;
    case 3:
        data = ARC(0xF);
        break;
    }
    info = ModInfoMgr.create(data, ARC(5));
    m_pModLHand = info;
    if (info) {
        addModel(info);
    }
}

// REL entry: registers the partner constructor as the enemy init function.
extern "C" void _prolog()
{
    EmInitFunc = Pl11Init;
}

// REL exit: nothing to free.
extern "C" void _epilog()
{
}

// Target of every unresolved cross-module branch (snmakerel patches them to `bl _unresolved`).
extern "C" void _unresolved()
{
}
