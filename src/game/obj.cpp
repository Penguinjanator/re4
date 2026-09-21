// game/obj: the object manager (D:/Bio4/Prog/obj.cpp). ObjMgr (cObjMgr) owns the pool of 0x3D8-byte
// cObj works; construct() placement-news the per-id class (obj00..objBull, ids 0..0x3F) so the
// virtual move() dispatches to the unit that implements it, move() runs every alive object once a
// frame (objMove), destroy() frees an object's model resources first.
#include "atari.h"
#include "event.h"
#include "obj.h"
#include "global.h"
#include "db_log.h"
#include "va_ppc.h"
#include "main_mem.h"
#include "pl_wep.h"
#include "at_mod.h"
#include <dolphin/os.h>
#include "shape.h"

// Map object manager (ObjMgr): 0x3D8-byte cObj works, constructed by id (construct), moved once per
// frame (move / objMove). The per-id classes live in the obj* units; only their constructors are
// needed here.

extern "C" {
void objMove(cObj* p);
}

// Per-id classes constructed by cObjMgr::construct. Each declares its `move` so the vtable stays
// with the unit that defines it (a class without a key function would emit a linkonce copy here).
class cObj00 : public cObj {
public:
    virtual void move();
};
class cObj01 : public cObj {
public:
    virtual void move();
};
class cObjScr : public cObj {
public:
    cObjScr();
    virtual void move();
};
class cObj03 : public cObj {
public:
    cObj03();
    virtual void move();
};
class cObj04 : public cObj {
public:
    virtual void move();
};
class cObj05 : public cObj {
public:
    virtual void move();
};
class cObjBox : public cObj {
public:
    cObjBox();
    virtual void move();
};
class cObj08 : public cObj {
public:
    virtual void move();
};
class cObj09 : public cObj {
public:
    virtual void move();
};
class cWepItem : public cObj {
public:
    virtual void move();
};
class cObj12 : public cObj {
public:
    virtual void move();
};
class cObjLadder : public cObj {
public:
    virtual void move();
};
class cObjBell : public cObj {
public:
    virtual void move();
};
class cObjGatling : public cObj {
public:
    virtual void move();
};
class cObj16 : public cObj {
public:
    virtual void move();
};
class cObj18 : public cObj {
public:
    virtual void move();
};
class cItemObj : public cObj {
public:
    cItemObj();
    virtual void move();
};
class cObjGrenade : public cObj {
public:
    cObjGrenade();
    virtual void move();
};
class cObjSpear : public cObj {
public:
    virtual void move();
};
class cObj1c : public cObj {
public:
    virtual void move();
};
class cObjChain : public cObj {
public:
    virtual void move();
};
class cObjPillar : public cObj {
public:
    virtual void move();
};
class cObjObaModel : public cObj {
public:
    virtual void move();
};
class cObj26 : public cObj {
public:
    virtual void move();
};
class cObjGreFire : public cObj {
public:
    cObjGreFire();
    virtual void move();
};
class cObjGreLight : public cObj {
public:
    cObjGreLight();
    virtual void move();
};
class cObjGondola : public cObj {
public:
    virtual void move();
};
class cObjRobo : public cObj {
public:
    virtual void move();
};
class cObjMissile : public cObj {
public:
    virtual void move();
};
class cObjYagura : public cObj {
public:
    virtual void move();
};
class cObjEgg : public cObj {
public:
    cObjEgg();
    virtual void move();
};
class cObjTrolley : public cObj {
public:
    virtual void move();
};
class cObjBull : public cObj {
public:
    virtual void move();
};

void (*ObjInitFunc[0x40])(cObj*);

// Manager of the 0x3D8-byte cObj works (kind 2 of the unit managers).
cObjMgr::cObjMgr() : cManager<cObj>(sizeof(cObj), 2)
{
    setName("cObjMgr");
    Guid = 0;
}

// Manager warnings to the log.
void cObjMgr::log(const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    pLog->vwarn(6, 0, fmt, ap);
}

// Unit construction: placement-news the per-id class (0 cObj00 ... 0x3F) into the work.
#line 130 "D:/Bio4/Prog/obj.cpp"
int cObjMgr::construct(cObj* p, int id)
{
    switch (id) {
    case ID_NORMAL:
        p = new (p) cObj00();
        break;
    case ID_MAGAZINE:
        p = new (p) cObj01();
        break;
    case ID_SCROLL:
        p = new (p) cObjScr();
        break;
    case ID_03:
        p = new (p) cObj03();
        break;
    case ID_ESP:
        p = new (p) cObj04();
        break;
    case ID_KABOOM:
        p = new (p) cObj05();
        break;
    case ID_BOX:
        p = new (p) cObjBox();
        break;
    case ID_MISSILE:
        p = new (p) cObj08();
        break;
    case ID_ESP2:
        p = new (p) cObj09();
        break;
    case ID_WEP_ITEM:
        p = new (p) cWepItem();
        break;
    case ID_PL_WEAPON:
        p = new (p) cObjWep();
        break;
    case ID_EM12_WEAPON:
        p = new (p) cObj12();
        break;
    case ID_LADDER:
        p = new (p) cObjLadder();
        break;
    case ID_BELL:
        p = new (p) cObjBell();
        break;
    case ID_GATLING:
        p = new (p) cObjGatling();
        break;
    case ID_EM10_PARASITE:
        p = new (p) cObj16();
        break;
    case ID_EVENT:
        p = new (p) cObj18();
        break;
    case ID_ITEM:
        p = new (p) cItemObj();
        break;
    case ID_WEP_GRENADE:
        p = new (p) cObjGrenade();
        break;
    case ID_SPEAR:
        p = new (p) cObjSpear();
        break;
    case ID_FLOATISLAND:
        p = new (p) cObj1c();
        break;
    case ID_CHAIN:
        p = new (p) cObjChain();
        break;
    case ID_OBAMODEL:
        p = new (p) cObjObaModel();
        break;
    case ID_WEP_ROCKET:
        p = new (p) cObjRocket();
        break;
    case ID_WEP_LAUNCHER:
        p = new (p) cObjLauncher();
        break;
    case ID_EM2B_PARASITE:
        p = new (p) cObj26();
        break;
    case ID_WEP_GRE_FIRE:
        p = new (p) cObjGreFire();
        break;
    case ID_WEP_GRE_LIGHT:
        p = new (p) cObjGreLight();
        break;
    case ID_GONDOLA:
        p = new (p) cObjGondola();
        break;
    case ID_ROBO:
        p = new (p) cObjRobo();
        break;
    case ID_HELI_MISSILE:
        p = new (p) cObjMissile();
        break;
    case ID_YAGURA:
        p = new (p) cObjYagura();
        break;
    case ID_WEP_EGG:
        p = new (p) cObjEgg();
        break;
    case ID_TROLLEY:
        p = new (p) cObjTrolley();
        break;
    case ID_BULL:
        p = new (p) cObjBull();
        break;
    case ID_PILLAR:
        p = new (p) cObjPillar();
        break;
    default:
        if (id > 0x3F) {
#line 175 "D:/Bio4/Prog/obj.cpp"
            HALT();
        }
        ObjInitFunc[id](p);
        break;
    }
    p->serial = Guid;
    Guid++;
    p->id = id;
    return 1;
}

// cManager entry point: forwards to the int version.
int cObjMgr::construct(cObj* p, u32 id)
{
    return construct(p, (int) id);
}

// Per-frame: die check, then objMove on every alive object.
void cObjMgr::move()
{
    cObj* p;
    cObj* n;
    void (*func)(cObj*);

    dieCheck();
    func = objMove;
    p = pAlive;
    while (p) {
        n = p;
        p = (cObj*) p->pNext;
        func(n);
    }
}

// One object's frame: skips inactive objects (be_flag 0x20 clear) and, during an event
// (Status_flg[1] 0x10000000), objects without the no-suspend flag; runs move(), the shape
// animation, the position history; debug: obstacle / skeleton / bounding box displays.
void objMove(cObj* p)
{
    if (!(p->be_flag & 0x20)) {
        return;
    }
    if (StaFlagChk(pG, STA_SUSPEND) && !(p->be_flag & 0x800)) {
        return;
    }
    p->move();
    ShapeMove(p->pModelInfo);
    p->updateOldPos();
    if (DbgFlagChk(pG, DBG_OBA_VIEW)) {
        DrawOba(p);
    }
    if (DbgFlagChk(pG, DBG_OBJ_SKELETON)) {
        p->debugSkeletonDisp();
    }
    if (p->be_flag & 0x80000000) {
        p->drawAllBoundingBox(p->pModelInfo);
    }
}

// Destroys an object: releases its model/parts (push) when it was alive, then the manager slot.
void cObjMgr::destroy(cObj* p)
{
    if ((p->be_flag & 0x201) != 1) {
        return;
    }
    p->push();
    cManager<cObj>::destroy(p);
}

// New object: active + alive flags, kindid 1.
cObj::cObj()
{
    be_flag |= 0x21;
    kindid = 1;
}

cObjMgr ObjMgr;
