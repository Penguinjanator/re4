// game/em_set.cpp: enemy creation from the room enemy list (ESL) and the per-list death bits.

#include "atari.h"
#include "light.h"
#include "em_set.h"
#include "global.h"
#include "db_log.h"

extern cEm* pPL;   // game/em.cpp

cEm* errEm = 0;
static int emSetDummy = 0;

// Death bit of list entry `no` in the current enemy list (0 when no list is loaded).
// Death bit table of the current enemy list (pG->em_dead[pG->emlist_no]); the original computes
// it with byte arithmetic: the row offset is added to pG before the table offset.
#define EM_DEAD_TBL() ((u32*) (pG->emlist_no * 0x20 + (u32) pG + 0x501C))

static inline u32 EmSetDieCk(u32 no)
{
    u32 v;

    if (pG->emlist_no >= 0) {
        u32* tbl = EM_DEAD_TBL();

        v = tbl[no >> 5] & (0x80000000 >> (no & 31));
    } else {
        v = 0;
    }
    return v;
}

static inline void EmSetDieOn(u32 no)
{
    if (pG->emlist_no >= 0) {
        u32* tbl = EM_DEAD_TBL();

        tbl[no >> 5] |= 0x80000000 >> (no & 31);
    }
}

// Counter update through a reference: the store is a plain scalar access, so pG is reloaded after it.
static inline void CntInc(u32& c) { c++; }

// While flags_68 bit21 is set only the enemies 3 and 4 may be created.
#define EM_SET_ID_NG(id) ((pG->flags_68 & 0x00200000) && ((id) != 3 && (id) != 4))

static inline cEm* EmCreate(u8 id)
{
    if (id == 0xF || id == 0x25) {
        return EmMgr.createBack(id);
    }
    return EmMgr.create(id);
}

// Copy the list entry into the fresh enemy work.
static inline void EmSetWork(cEm* em, EmListData* d, u8 no)
{
    f32 kx = 1000.0f;
    f32 kr = 3.1415927f / 16384.0f;
    f32 kp = 10.0f;

    em->type = d->type;
    em->x38D = d->x3;
    em->flags_3C8 = d->flags4;
    em->x3D0 = d->xB;
    em->x3CC = (f32) d->x1A * kx;
    em->hpMax = em->hp = d->hp;
    em->rot.x = (f32) d->rot[0] * kr;
    em->rot.y = (f32) d->rot[1] * kr;
    em->rot.z = (f32) d->rot[2] * kr;
    em->pos.x = (f32) d->pos[0] * kp;
    em->pos.y = (f32) d->pos[1] * kp;
    em->pos.z = (f32) d->pos[2] * kp;
    em->oldPos = em->pos;
    em->emsetNo = no;
}

// COMPILER-DIFF: #13 (pool highs of an inlined body) -- in EmSetFromList2/EmSetEvent the original
// loads the three EmSetWork constants with `lis r9; lfs` / `lis r9; lfs` / `lis r11; lfs` pairs (the
// highs are not local qtys: they are live across every fpmem loadaddr birth, so the seven loadaddr
// copies take r10/r8..r3 and &pos/&oldPos fall to r30/r29) and its constant loads carry no
// anti-dependence on the following stores. A private cc1plus that keeps RTX_UNCHANGING_P on the
// inlined constant-pool MEMs (integrate.c copy_rtx_and_substitute) gives both functions from the plain
// EmSetWork inline (74 -> 1, 73 -> 3 words) but regresses 30 constant-store functions elsewhere
// (cModel/cParts ctors, weapon init), so nothing is installed. Tagged form: the named statics take the
// pool words' .rodata slots, the asm pairs load them with the highs pinned (r9 twice, r11), and the
// r11 high is kept live through the `hp` copy so the loadaddr copies avoid r9/r11.
#define EM_SET_WORK_K(em, d, no)                                                          \
    do {                                                                                  \
        static const f32 kx __attribute__((nosda)) = 1000.0f;                             \
        static const f32 kr __attribute__((nosda)) = 3.1415927f / 16384.0f;               \
        static const f32 kp __attribute__((nosda)) = 10.0f;                               \
        register u32 hx asm("r9");                                                        \
        register u32 hr asm("r11");                                                       \
        f32 vx, vr, vp;                                                                   \
        asm("lis %0,%1@ha" : "=r"(hx) : "i"(&kx));                                        \
        asm("lfs %0,%1@l(%2)" : "=f"(vx) : "i"(&kx), "r"(hx));                            \
        asm("lis %0,%1@ha" : "=r"(hr) : "i"(&kr));                                        \
        asm("lfs %0,%1@l(%2)" : "=f"(vr) : "i"(&kr), "r"(hr));                            \
        asm("lis %0,%1@ha" : "=r"(hx) : "i"(&kp));                                        \
        asm("lfs %0,%1@l(%2)" : "=f"(vp) : "i"(&kp), "r"(hx));                            \
        (em)->type = (d)->type;                                                           \
        (em)->x38D = (d)->x3;                                                             \
        (em)->flags_3C8 = (d)->flags4;                                                    \
        (em)->x3D0 = (d)->xB;                                                             \
        (em)->x3CC = (f32) (d)->x1A * vx;                                                 \
        {                                                                                 \
            register int hp_ asm("r9") = (d)->hp;                                         \
            asm("" : "+r"(hp_) : "r"(hr));                                                \
            (em)->hpMax = (em)->hp = hp_;                                                 \
        }                                                                                 \
        (em)->rot.x = (f32) (d)->rot[0] * vr;                                             \
        (em)->rot.y = (f32) (d)->rot[1] * vr;                                             \
        (em)->rot.z = (f32) (d)->rot[2] * vr;                                             \
        (em)->pos.x = (f32) (d)->pos[0] * vp;                                             \
        (em)->pos.y = (f32) (d)->pos[1] * vp;                                             \
        (em)->pos.z = (f32) (d)->pos[2] * vp;                                             \
        (em)->oldPos = (em)->pos;                                                         \
        (em)->emsetNo = (no);                                                             \
    } while (0)

static inline void EmSetDist(cEm* em)
{
    f32 dz = pPL->pos.z - em->pos.z;
    f32 dx = pPL->pos.x - em->pos.x;

    em->x374 = 1.0e16f;
    em->plDist2 = dx * dx + dz * dz;
}

// Work `no` with the range check read through a manager copy (map_obj.h getWork). A plain
// `for (i = 0; i < EmMgr.nArray; i++)` around it gives the original shape: gcse PRE turns the second
// nArray read into a copy of the first (`mr r10, r0`), the bottom test uses that copy and the back
// edge is threaded past the check.
static inline cEm* emSetWork(u32 no)
{
    cEmMgr* m = &EmMgr;
    if (no >= m->nArray) {
        return 0;
    }
    return (cEm*) ((u8*) m->pArray + m->size * no);
}

int checkListId(int no)
{
    u32 i;

    if (no == 0xFF) {
        return 1;
    }
    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* em = emSetWork(i);

        if ((em->be_flag & 0x201) == 1 && em->emsetNo == (u8) no) {
            return 0;
        }
    }
    return 1;
}

void EmSetFromList()
{
    u32 i;

    for (i = 0; i < 256; i++) {
        EmListData* d = EM_LIST(i);
        cEm* em;

        if (!(d->flags & 1)) {
            continue;
        }
        if (d->flags & 2) {
            continue;
        }
        if (EmSetDieCk(i)) {
            continue;
        }
        if (pG->stage_no != d->room >> 8) {
            continue;
        }
        if (pG->room_no != (d->room & 0xFF)) {
            continue;
        }
        if (checkListId(i) == 0) {
            continue;
        }
        // d->id read directly: the range fold of EM_SET_ID_NG keeps the QImode load and the int
        // uses share one PRE'd `clrlwi`; a `u8 id` local is promoted and never masked.
        if (d->id == 0) {
            continue;
        }
        if (EM_SET_ID_NG(d->id)) {
            continue;
        }
        em = EmCreate(d->id);
        if (em == 0) {
            pLog->err(0, 0, "EmSetFromList() Em set failed, Id = %x", d->id);
            continue;
        }
        EmSetWork(em, d, i);
        d->flags |= 2;
        if (d->flags & 4) {
            d->flags |= 8;
            d->flags &= ~4;
        } else if (!(d->flags & 8)) {
            d->flags |= 4;
        }
        EmSetDist(em);
        em->move();
    }
}

cEm* EmSetFromList2(int no, int chkDead)
{
    EmListData* d = EM_LIST(no);
    cEm* em;

    if (EM_SET_ID_NG(d->id)) {
        return errEm;
    }
    if (pG->stage_no != d->room >> 8) {
        return errEm;
    }
    if (pG->room_no != (d->room & 0xFF)) {
        return errEm;
    }
    if (d->flags & 2) {
        return errEm;
    }
    if (d->id == 0) {
        return errEm;
    }
    if (chkDead) {
        if (EmSetDieCk(no)) {
            return errEm;
        }
    }
    if (checkListId(no) == 0) {
        return errEm;
    }
    em = EmCreate(d->id);
    if (em == 0) {
        pLog->err(0, 0, "EmSetFromList2() Em set failed, Id = %x", d->id);
        return errEm;
    }
    EM_SET_WORK_K(em, d, no);
    d->flags |= 2;
    if (d->flags & 4) {
        d->flags |= 8;
        d->flags &= ~4;
    } else if (!(d->flags & 8)) {
        d->flags |= 4;
    }
    EmSetDist(em);
    em->move();
    return em;
}

// Event enemy from a list entry outside the room list (never called in the DOL).
cEm* EmSetEvent(EmListData* d)
{
    cEm* em;

    if (EM_SET_ID_NG(d->id)) {
        return errEm;
    }
    em = EmCreate(d->id);
    if (em == 0) {
        pLog->err(0, 0, "EmSetEvent() Em set failed, Id = %x", d->id);
        return errEm;
    }
    EM_SET_WORK_K(em, d, 0xFF);
    d->flags = 7;
    EmSetDist(em);
    em->move();
    return em;
}

cEm* GetEmPtrFromList(int no)
{
    u32 i;

    if (no == 0xFF) {
        return 0;
    }
    for (i = 0; i < EmMgr.nArray; i++) {
        cEm* em = emSetWork(i);

        if ((em->be_flag & 0x201) == 1 && em->emsetNo == (u8) no) {
            return em;
        }
    }
    return 0;
}

EmListData* GetListPtrFromEm(cEm* em)
{
    if (em->emsetNo == 0xFF) {
        return 0;
    }
    return EM_LIST(em->emsetNo);
}

u8 GetEmIdFromList(u32 no)
{
    EmListData* list;

    if (no >= 0xFF) {
        return 0xFF;
    }
    list = (EmListData*) pG->emlist;
    return list[no].id;
}

void EmListSetAlive(int no, int on)
{
    EmListData* d = EM_LIST(no);

    if (pG->stage_no != d->room >> 8) {
        return;
    }
    if (pG->room_no != (d->room & 0xFF)) {
        return;
    }
    if (on == 1) {
        d->flags |= 1;
    } else {
        d->flags &= ~1;
    }
}

void EmSetDie(cEm* em)
{
    if (pG->flags_68 & 0x04000000) {
        return;
    }
    if ((pG->room_id32 & 0xFFFF0000) == 0x00040000) {
        return;
    }
    if (pG->flags_6C & 0x00080000) {
        return;
    }
    if (EmSetDieCk(em->emsetNo)) {
        return;
    }
    EmSetDieOn(em->emsetNo);
}

void EmSetDieCnt()
{
    CntInc(pG->em_die_cnt);
    CntInc(pG->em_die_cnt2);
}

void EmSetRoomInit()
{
    int i;

    for (i = 0; i < 256; i++) {
        EmListData* d = EM_LIST(i);

        d->flags &= ~2;
    }
}

void EmListWaitDelete()
{
    int i;

    if (pG->flags_51E4 % 30 != 0) {
        return;
    }
    for (i = 0; i < 256; i++) {
    }
}
