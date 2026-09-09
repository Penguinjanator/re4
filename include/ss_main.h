#ifndef SS_MAIN_H
#define SS_MAIN_H

#include "types.h"
#include "sscrn.h"
#include "widget.h"
#include "id_sys.h"
#include "model.h"

// Sscrn module (the sub screen DLL, src/Sscrn/ss_*.cpp): declarations shared by its units.
// ss_main.cpp owns the task, the common id/model/light helpers and the exit/examine widgets.

class cLit;

// Scalar reference setters: a store through a reference keeps the following loads of other globals
// below it (global.h FSet/BitSet).
static inline void U8Set(u8& d, u8 v) { d = v; }
static inline void U16Set(u16& d, u16 v) { d = v; }
static inline void U32Set(u32& d, u32 v) { d = v; }
static inline void S32Set(s32& d, s32 v) { d = v; }
static inline void S16Set(s16& d, s16 v) { d = v; }
static inline void PSet(void*& d, void* v) { d = v; }

// game/sscrn.cpp id systems of the sub screen (sub screen ids / number digits).
extern IDSystem IdSub;
extern IDSystem IdNum;

// Every unit's linkonce block has Widget<SUB_SCREEN>::~Widget right after the cManager<cLight>
// copies, before the unit's own (synthesized) widget destructors and before Widget::quit/init/move:
// the destructor is instantiated by this header, before any derived class is declared (the other
// three at their first use in the unit).
static inline void ssWidgetDelete(Widget<SUB_SCREEN>* w)
{
    delete w;
}

// Item examine screen (ss_main.cpp; ss_cap/ss_file/ss_item chain into it).
class SsItemExamine : public Widget<SUB_SCREEN> {
public:
    u8 state;   // 0x10
    u8 pad_11[0x70 - 0x11];

    virtual void init(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

// The DLL's own model managers (ss_main.cpp; the DOL's PartsMgr/ModInfoMgr are swapped out while
// the sub screen is open). They are constructed by the DOL's cPartsMgr/cModInfoMgr constructors,
// but the module's view of the classes has no virtual destructor: the static destructor inlines
// cManager<T>::~cManager (stores the cManager vtable) instead of calling _._9cPartsMgr.
class cSsPartsMgr : public cManager<cParts> {
public:
    cSsPartsMgr() asm("__9cPartsMgr");
    virtual void* memAlloc(u32 size);
    virtual void memFree(void* p);
    virtual void memClear(cParts* p, u32 size);
    virtual void log(const char* fmt, ...);
    virtual int construct(cParts* p, u32 id);
};
class cSsModInfoMgr : public cManager<cModelInfo> {
public:
    cSsModInfoMgr() asm("__12cModInfoMgr");
    virtual void* memAlloc(u32 size);
    virtual void memFree(void* p);
    virtual void memClear(cModelInfo* p, u32 size);
    virtual void log(const char* fmt, ...);
    virtual int construct(cModelInfo* p, u32 id);
};
extern cSsPartsMgr ssPartsMgr;
extern cSsModInfoMgr ssModInfoMgr;

extern "C" {
// ss_main.cpp
void IdSubErase();
void IdNumErase();
void sscrnModelClear(SUB_SCREEN* wk);
void sscrnLightClear(SUB_SCREEN* wk);
void sscrnLightCreate(SUB_SCREEN* wk, cLit* lit);
void sscrnMainMenuInit(SUB_SCREEN* wk, int no);
// ss_debug.cpp
void SscrnDebugMenu(SUB_SCREEN* wk);
// ss_pzzl.cpp
void pieceModelInit(SUB_SCREEN* wk);
}

#endif
